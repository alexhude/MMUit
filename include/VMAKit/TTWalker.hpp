//
//  Copyright (c) 2017, Alexander Hude
//  All rights reserved.
//
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree.
//

#pragma once

#include "VMAPlatform.hpp"
#include "MMUConfig.hpp"
#include <functional>

enum class WalkOperation {
	Stop		= false,
	Continue	= true
};

enum class WalkResultType {
	Complete	= 0,
	Stopped		= 1,
	Failed		= 2,
	Undefined	= 3
};

struct WalkPosition {
	TTLevel		level;
	virt_addr_t	tableAddress;
	offset_t	entryOffset;
};

struct WalkResult {
	WalkResultType	type;
	TTLevel			level;
	ttentry_t		descriptor;
	phys_addr_t		outputAddress;
	
	WalkResultType	getType() { return type; }
	TTLevel			getLevel() { return level; }
	ttentry_t		getDescriptor() { return descriptor; }
	phys_addr_t		getOutputAddress() { return outputAddress; }
	
	WalkResult&		setType(WalkResultType type) {this->type = type; return *this; }
	WalkResult&		setLevel(TTLevel level) {this->level = level; return *this; }
	WalkResult&		setDescriptor(ttentry_t descriptor) {this->descriptor = descriptor; return *this; }
	WalkResult&		setOutputAddress(phys_addr_t address) {this->outputAddress = address; return *this; }
};

class TTGenericWalker
{
public:
	
	// WalkerCallback callback is called for every level walker is going through
	using WalkerCallback = std::function<WalkOperation(WalkPosition* position, TTGenericEntry* entry)>;
	static const WalkerCallback DefaultCallback;
	
public:
	
	virtual WalkResult	walkTo(virt_addr_t address, WalkerCallback callback) = 0;
	virtual bool		reverseWalkFrom(virt_addr_t address, WalkerCallback callback = DefaultCallback) = 0;
	virtual phys_addr_t findPhysicalAddress(virt_addr_t address) = 0;
	
};

const TTGenericWalker::WalkerCallback TTGenericWalker::DefaultCallback =
	[] (WalkPosition* position, TTGenericEntry* entry) -> WalkOperation
	{
		return WalkOperation::Continue;
	};

template <typename PRIMITIVES>
class TTWalker : public PRIMITIVES, public TTGenericWalker
{
public:

	TTWalker() = delete;
	
	TTWalker(MMUConfig mmuConfig, virt_addr_t tableBase)
		: m_mmuConfig(mmuConfig), m_tableBase(tableBase)
	{}

	WalkResult	walkTo(virt_addr_t address, WalkerCallback callback = DefaultCallback) override
	{
		switch (m_mmuConfig.granule) {
			case TTGranule::Granule4K: return performWalkTo<TTGranule::Granule4K>(address, callback);
			case TTGranule::Granule16K: return performWalkTo<TTGranule::Granule16K>(address, callback);
			case TTGranule::Granule64K: return performWalkTo<TTGranule::Granule64K>(address, callback);
				
			default: assert(0);
		}
		
		return WalkResult();
	}
	
	bool reverseWalkFrom(virt_addr_t address, WalkerCallback callback) override
	{
		switch (m_mmuConfig.granule) {
			case TTGranule::Granule4K: return performReverseWalkFrom<TTGranule::Granule4K>(address, callback);
			case TTGranule::Granule16K: return performReverseWalkFrom<TTGranule::Granule16K>(address, callback);
			case TTGranule::Granule64K: return performReverseWalkFrom<TTGranule::Granule64K>(address, callback);
				
			default: assert(0);
		}
		
		return false;
	}
	
	phys_addr_t findPhysicalAddress(virt_addr_t address) override
	{
		virt_addr_t pageMask = uint32_t(m_mmuConfig.granule) - 1;
		
		auto result = walkTo(address);
		if (result.getType() == WalkResultType::Complete)
			return result.getOutputAddress() | (address & pageMask);
		else
			return kInvalidAddress;
	}
	
private:
	
	template <TTGranule GRANULE>
	WalkResult	performWalkTo(virt_addr_t address, WalkerCallback callback = DefaultCallback)
	{
		WalkResult result;
		WalkPosition pos = {
			.level = m_mmuConfig.initialLevel,
			.tableAddress = m_tableBase,
			.entryOffset = 0
		};
		
		VirtualAddress<GRANULE> va(address, m_mmuConfig.regionSizeOffset);
		
		while (1)
		{
			result.level = pos.level;
			result.descriptor = 0;
			
			pos.entryOffset = va.getOffsetForLevel(pos.level);
			
			// get current table translation entry
			switch (pos.level)
			{
				case TTLevel::Level0:
				{
					auto entry = TTEntry<GRANULE, TTLevel::Level0>(this->readAddress(pos.tableAddress + pos.entryOffset));
					result.descriptor = entry.getDescriptor();
					
					// check is entry is valid
					if (entry.isValid() == false)
						return result.setType(WalkResultType::Failed).setOutputAddress(kInvalidAddress);
					
					// invalid if not table descriptor
					if (entry.isTableDescriptor() == false)
						return result.setType(WalkResultType::Failed).setOutputAddress(kInvalidAddress);
					
					// execute callback and interrupt walk if needed
					if (callback(&pos, &entry) == WalkOperation::Stop)
						return result.setType(WalkResultType::Stopped).setOutputAddress(entry.getOutputAddress());
					
					// get next table address
					pos.tableAddress = this->physicalToVirtual(entry.getOutputAddress());
					
					break;
				}
				case TTLevel::Level1:
				{
					auto entry = TTEntry<GRANULE, TTLevel::Level1>(this->readAddress(pos.tableAddress + pos.entryOffset));
					result.descriptor = entry.getDescriptor();
					
					// check is entry is valid
					if (entry.isValid() == false)
						return result.setType(WalkResultType::Failed).setOutputAddress(kInvalidAddress);
					
					// execute callback and interrupt walk if needed
					if (callback(&pos, &entry) == WalkOperation::Stop)
						return result.setType(WalkResultType::Stopped).setOutputAddress(entry.getOutputAddress());
					
					// return block address if not table descriptor
					if (entry.isTableDescriptor() == false)
						return result.setType(WalkResultType::Complete).setOutputAddress(entry.getOutputAddress());
					
					// get next table address
					pos.tableAddress = this->physicalToVirtual(entry.getOutputAddress());
					
					break;
				}
				case TTLevel::Level2:
				{
					auto entry = TTEntry<GRANULE, TTLevel::Level2>(this->readAddress(pos.tableAddress + pos.entryOffset));
					result.descriptor = entry.getDescriptor();
					
					// check is entry is valid
					if (entry.isValid() == false)
						return result.setType(WalkResultType::Failed).setOutputAddress(kInvalidAddress);
					
					// execute callback and interrupt walk if needed
					if (callback(&pos, &entry) == WalkOperation::Stop)
						return result.setType(WalkResultType::Stopped).setOutputAddress(entry.getOutputAddress());
					
					// return block address if not table descriptor
					if (entry.isTableDescriptor() == false)
						return result.setType(WalkResultType::Complete).setOutputAddress(entry.getOutputAddress());
					
					// get next table address
					pos.tableAddress = this->physicalToVirtual(entry.getOutputAddress());
					
					break;
				}
				case TTLevel::Level3:
				{
					auto entry = TTEntry<GRANULE, TTLevel::Level3>(this->readAddress(pos.tableAddress + pos.entryOffset));
					result.descriptor = entry.getDescriptor();
					
					// check is entry is valid
					if (entry.isValid() == false)
						return result.setType(WalkResultType::Failed).setOutputAddress(kInvalidAddress);
					
					// invalid if not page descriptor
					if (entry.isPageDescriptor() == false)
						return result.setType(WalkResultType::Failed).setOutputAddress(kInvalidAddress);
					
					// execute callback and interrupt walk if needed
					if (callback(&pos, &entry) == WalkOperation::Stop)
						return result.setType(WalkResultType::Stopped).setOutputAddress(entry.getOutputAddress());
					
					// return page address
					return result.setType(WalkResultType::Complete).setOutputAddress(entry.getOutputAddress());
				}
				default: assert(0);
			}
			
			if (pos.tableAddress == kInvalidAddress)
				return result.setType(WalkResultType::Failed).setOutputAddress(kInvalidAddress);
			
			// switch to the next level
			pos.level++;
		}
		
		return result.setType(WalkResultType::Failed).setOutputAddress(kInvalidAddress);
	}
	
	template <TTGranule GRANULE>
	bool performReverseWalkFrom(virt_addr_t address, WalkerCallback callback)
	{
		struct ReverseWalk
		{
			uint32_t		levels;
			WalkPosition	position[uint32_t(TTLevel::Count)];
			ttentry_t		entry[uint32_t(TTLevel::Count)];
		} walk;
		
		walk.levels = 0;
		
		// walk forward and save translation lookups
		TTWalker<PRIMITIVES> walker(m_mmuConfig, m_tableBase);
		WalkResult result = walker.walkTo(address, [&walk] (WalkPosition* position, TTGenericEntry* entry) {
			// safety checks
			if (position == nullptr || entry == nullptr)
				return WalkOperation::Stop;
			
			walk.position[walk.levels] = *position;
			walk.entry[walk.levels] = entry->getDescriptor();
			walk.levels++;
			
			return WalkOperation::Continue;
		});
		
		if (result.getType() == WalkResultType::Failed)
			return false;
		
		// walk backwards
		while (walk.levels != 0)
		{
			uint32_t currentLevel = walk.levels - 1;
			
			switch (walk.position[currentLevel].level)
			{
				case TTLevel::Level0:
				{
					auto entry = TTEntry<GRANULE, TTLevel::Level0>(walk.entry[currentLevel]);
					if (callback(&(walk.position[currentLevel]), &entry) == WalkOperation::Stop)
						return false;
					break;
				}
				case TTLevel::Level1:
				{
					auto entry = TTEntry<GRANULE, TTLevel::Level1>(walk.entry[currentLevel]);
					if (callback(&(walk.position[currentLevel]), &entry) == WalkOperation::Stop)
						return false;
					break;
				}
				case TTLevel::Level2:
				{
					auto entry = TTEntry<GRANULE, TTLevel::Level2>(walk.entry[currentLevel]);
					if (callback(&(walk.position[currentLevel]), &entry) == WalkOperation::Stop)
						return false;
					break;
				}
				case TTLevel::Level3:
				{
					auto entry = TTEntry<GRANULE, TTLevel::Level3>(walk.entry[currentLevel]);
					if (callback(&(walk.position[currentLevel]), &entry) == WalkOperation::Stop)
						return false;
					break;
				}
				default: assert(0);
			}
			
			walk.levels--;
		}
		
		return true;
	}
	
private:
	
	MMUConfig 	m_mmuConfig;
	virt_addr_t m_tableBase = kInvalidAddress;
};
