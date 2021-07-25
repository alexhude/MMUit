//
//  Copyright (c) 2017, Alexander Hude
//  All rights reserved.
//
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree.
//

#pragma once

#include "TTWalker.hpp"
#include <algorithm>
#include <map>
#include <vector>

template <typename PRIMITIVES>
class PageRelocator : public PRIMITIVES
{
public:
	
	// Relocation callback is called when table translation entry is about to be updated
	using RelocatorCallback = std::function<ttentry_t(TTLevel level, TTGenericEntry* oldEntry, TTGenericEntry* newEntry)>;
	static const RelocatorCallback DefaultCallback;
	
public:
	
	PageRelocator() = delete;
	
	PageRelocator(MMUConfig mmuConfig, virt_addr_t tableBase)
		: 	m_mmuConfig(mmuConfig), m_tableBase(tableBase),
			kPageSize(uint32_t(mmuConfig.granule)), kPageMask(kPageSize - 1)
	{}

	bool isPageRelocatedFor(virt_addr_t address)
	{
		virt_addr_t targetPageAddress = address & ~kPageMask;
		
		if (std::find(m_relocatedPages.begin(), m_relocatedPages.end(), targetPageAddress) != m_relocatedPages.end())
			return true;
		else
			return false;
	}
	
	bool isRelocationPendingFor(virt_addr_t address)
	{
		// check if there is a pending relocation
		if (m_relocationPending == false)
			return false;

		virt_addr_t targetPageAddress = address & ~kPageMask;
		
		// check if target page matches one in pending relocation
		if (m_stagingInfo.targetPageVA != targetPageAddress)
			return false;
		else
			return true;
	}
	
	bool relocatePageFor(virt_addr_t address, RelocatorCallback callback = DefaultCallback)
	{
		virt_addr_t page = preparePageRelocationFor(address, callback);
		if (page == kInvalidAddress)
			return false;
		
		return completeRelocation();
	}
	
	virt_addr_t preparePageRelocationFor(virt_addr_t address, RelocatorCallback callback = DefaultCallback)
	{
		virt_addr_t targetPageAddress = address & ~kPageMask;
		
		if (std::find(m_relocatedPages.begin(), m_relocatedPages.end(), targetPageAddress) != m_relocatedPages.end())
			return false;

		// cancel pending relocations
		cancelRelocation();
		
		TTWalker<PRIMITIVES> walker(m_mmuConfig, m_tableBase);
		WalkResult result = walker.walkTo(address, [this, callback] (WalkPosition* position, TTGenericEntry* entry) {
			// safety checks
			if (position == nullptr || entry == nullptr)
				return WalkOperation::Stop;
			
			// get next level page
			virt_addr_t nextLevelPA = entry->getOutputAddress();
			
			// check if page is already relocated
			if (m_relocationMap.find(nextLevelPA) != m_relocationMap.end())
			{
				m_relocationMap[nextLevelPA].refCount++;
				return WalkOperation::Continue;
			}
			
			// allocate new page
			virt_addr_t newPageVA = this->allocInPhysicalMemory(kPageSize);
			assert((newPageVA & kPageMask) == 0);
			
			virt_addr_t nextLevelVA = this->physicalToVirtual(nextLevelPA);
			
			// clone page content
			this->copyInKernel(newPageVA, nextLevelVA, kPageSize);
			
			// get PA of allocated page
			phys_addr_t newPagePA = this->virtualToPhysical(newPageVA);
			
			// save original descriptor for current level
			ttentry_t oldEntryDescriptor = entry->getDescriptor();
			
			ttentry_t newEntryDescriptor;
			TTGenericEntry* oldEntry = entry->clone();
			
			// update entry PA
			entry->setOutputAddress(newPagePA);
			
			// apply external modifications
			newEntryDescriptor = callback(position->level, oldEntry, entry);
			delete oldEntry;

			Relocation relocation = {
				.originalEntry = oldEntryDescriptor,
				.allocatedPage = newPageVA,
				.refCount = 1
			};

			if (entry->isPageDescriptor() == false)
			{
				// write TT entry back
				this->writeAddress(position->tableAddress + position->entryOffset, newEntryDescriptor);
				
				// save relocated page
				m_relocationMap[newPagePA] = relocation;
			}
			else
			{
				// stage relocation
				m_stagingInfo.allocatedPagePA = newPagePA;
				m_stagingInfo.allocatedPageEntry = newEntryDescriptor;
				m_stagingInfo.entryPosition = *position;
				m_stagingInfo.relocation = relocation;
			}
			
			return WalkOperation::Continue;
		});
		
		if (result.getType() == WalkResultType::Complete)
		{
			m_stagingInfo.targetPageVA = targetPageAddress;
			m_relocationPending = true;
			
			return m_stagingInfo.relocation.allocatedPage;
		}
		else
		{
			restorePageFor(targetPageAddress);
			
			return kInvalidAddress;
		}
	}

	bool completeRelocation()
	{
		if (m_relocationPending == false)
			return false;
		
		// write TT entry back
		this->writeAddress(m_stagingInfo.entryPosition.tableAddress + m_stagingInfo.entryPosition.entryOffset, m_stagingInfo.allocatedPageEntry);
		
		// save relocated page
		m_relocationMap[m_stagingInfo.allocatedPagePA] = m_stagingInfo.relocation;

		// add page to relocated pages
		m_relocatedPages.push_back(m_stagingInfo.targetPageVA);
		
		m_relocationPending = false;
		
		return true;
	}
	
	bool cancelRelocation()
	{
		if (m_relocationPending == false)
			return false;
		
		// deallocate page
		this->deallocInPhysicalMemory(m_stagingInfo.relocation.allocatedPage, kPageSize);
		
		// restore TT entries
		bool result = restorePageFor(m_stagingInfo.targetPageVA);
		
		m_relocationPending = false;
		
		return result;
	}
	
	bool restorePageFor(virt_addr_t address)
	{
		virt_addr_t targetPageAddress = address & ~kPageMask;
		
		if (std::find(m_relocatedPages.begin(), m_relocatedPages.end(), targetPageAddress) == m_relocatedPages.end())
		{
			// unable to find page, check if there is a pending relocation
			if (m_relocationPending == false)
				return false;
			
			// check if target page matches one in pending relocation
			if (m_stagingInfo.targetPageVA != targetPageAddress)
				return false;
		}
		
		TTWalker<PRIMITIVES> walker(m_mmuConfig, m_tableBase);
		
		bool result = walker.reverseWalkFrom(address, [this] (WalkPosition* position, TTGenericEntry* entry) {
			// safety checks
			if (position == nullptr || entry == nullptr)
				return WalkOperation::Stop;
			
			// get level page
			virt_addr_t levelPA = entry->getOutputAddress();
			if (m_relocationMap.find(levelPA) == m_relocationMap.end())
				return WalkOperation::Continue;
			
			// get original descriptor
			Relocation& relocation = m_relocationMap[levelPA];

			if (relocation.refCount == 1)
			{
				// restore TT entry
				this->writeAddress(position->tableAddress + position->entryOffset, relocation.originalEntry);

				// deallocate page
				this->deallocInPhysicalMemory(relocation.allocatedPage, kPageSize);

				// remove page from relocation map
				m_relocationMap.erase(levelPA);
			}
			else
			{
				relocation.refCount--;
			}
			
			return WalkOperation::Continue;
		});

		if (m_relocationPending == false)
		{
			// remove page from relocated pages
			m_relocatedPages.erase(std::remove(m_relocatedPages.begin(), m_relocatedPages.end(), targetPageAddress), m_relocatedPages.end());
		}
		
		return result;
	}
	
private:
	
	const uint32_t 		kPageSize;
	const virt_addr_t 	kPageMask;
	
	MMUConfig	m_mmuConfig;
	virt_addr_t m_tableBase = kInvalidAddress;
	
	struct Relocation
	{
		ttentry_t	originalEntry;
		virt_addr_t	allocatedPage;
		uint32_t	refCount;
	};
	
	struct StagingInfo
	{
		virt_addr_t		targetPageVA;
		phys_addr_t		allocatedPagePA;
		ttentry_t		allocatedPageEntry;
		WalkPosition	entryPosition;
		Relocation		relocation;
	};
	
	bool			m_relocationPending = false;
	StagingInfo		m_stagingInfo;
	
	std::vector<virt_addr_t>			m_relocatedPages;
	std::map<virt_addr_t, Relocation>	m_relocationMap;
};

template <typename PRIMITIVES>
const typename PageRelocator<PRIMITIVES>::RelocatorCallback PageRelocator<PRIMITIVES>::DefaultCallback =
[] (TTLevel level, TTGenericEntry* entry) -> ttentry_t
{
	assert(entry != nullptr);
	return entry->getDescriptor();
};
