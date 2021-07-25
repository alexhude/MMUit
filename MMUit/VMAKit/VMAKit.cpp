//
//  Copyright (c) 2017, Alexander Hude
//  All rights reserved.
//
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree.
//

#include "VMAKit.hpp"
#include "Primitives.hpp"


#include "TTWalker.h"
#include "PageRelocator.h"

class ttwalkerPrimitives : public Primitives
{
public:
	
	ttwalkerPrimitives()
	{
		if (s_walker)
		{
			m_funcReadAddress		= s_walker->read_address;
			m_funcPhysicalToVirtual = s_walker->physical_to_virtual;
		}
	}
	
	static void	init(ttwalker* walker)
	{
		if (walker == nullptr)
			return;
		
		s_walker = walker;
	}
	
	static void	close()
	{
		s_walker = nullptr;
	}
	
	uintptr_t	readAddress(uintptr_t address)
	{
		if (m_funcReadAddress == nullptr)
			return kInvalidAddress;
		
		return m_funcReadAddress(address);
	};
	
	virt_addr_t physicalToVirtual(phys_addr_t address)
	{
		if (m_funcPhysicalToVirtual == nullptr)
			return kInvalidAddress;
		
		return m_funcPhysicalToVirtual(address);
	}
	
private:

	static ttwalker* s_walker;
	
	uintptr_t	(*m_funcReadAddress)(virt_addr_t address) = nullptr;
	virt_addr_t (*m_funcPhysicalToVirtual)(phys_addr_t address) = nullptr;
	
};

ttwalker* ttwalkerPrimitives::s_walker = nullptr;

class pagerelocatorPrimitives : public Primitives
{
public:
	
	pagerelocatorPrimitives()
	{
		if (s_relocator)
		{
			m_funcReadAddress				= s_relocator->read_address;
			m_funcWriteAddress				= s_relocator->write_address;
			
			m_funcCopyInKernel				= s_relocator->copy_in_kernel;
			
			m_funcAllocInPhysicalMemory		= s_relocator->alloc_in_physical_memory;
			m_funcDeallocInPhysicalMemory	= s_relocator->dealloc_in_physical_memory;
			
			m_funcPhysicalToVirtual			= s_relocator->physical_to_virtual;
			m_funcVirtualToPhysical			= s_relocator->virtual_to_physical;
		}
	}

	static void	init(pagerelocator* relocator)
	{
		if (relocator == nullptr)
			return;
		
		s_relocator = relocator;
	}
	
	static void	close()
	{
		s_relocator = nullptr;
	}

	uintptr_t	readAddress(virt_addr_t address)
	{
		if (m_funcReadAddress == nullptr)
			return kInvalidAddress;
		
		return m_funcReadAddress(address);
	}
	void		writeAddress(virt_addr_t address, virt_addr_t data)
	{
		if (m_funcWriteAddress != nullptr)
			m_funcWriteAddress(address, data);
	}
	
	void		copyInKernel(virt_addr_t dst, virt_addr_t src, uint32_t size)
	{
		if (m_funcCopyInKernel != nullptr)
			m_funcCopyInKernel(dst, src, size);
	}
	
	virt_addr_t allocInPhysicalMemory(uint32_t size)
	{
		if (m_funcAllocInPhysicalMemory == nullptr)
			return kInvalidAddress;
		
		return m_funcAllocInPhysicalMemory(size);
	}
	bool		deallocInPhysicalMemory(virt_addr_t address, uint32_t size)
	{
		if (m_funcDeallocInPhysicalMemory == nullptr)
			return kInvalidAddress;
		
		return m_funcDeallocInPhysicalMemory(address, size);
	}
	
	virt_addr_t	physicalToVirtual(phys_addr_t address)
	{
		if (m_funcPhysicalToVirtual == nullptr)
			return kInvalidAddress;
		
		return m_funcPhysicalToVirtual(address);
	}
	virt_addr_t	virtualToPhysical(phys_addr_t address)
	{
		if (m_funcVirtualToPhysical == nullptr)
			return kInvalidAddress;
		
		return m_funcVirtualToPhysical(address);
	}
	
private:
	
	static pagerelocator* s_relocator;
	
	uintptr_t	(*m_funcReadAddress)(virt_addr_t address) = nullptr;
	void		(*m_funcWriteAddress)(virt_addr_t address, virt_addr_t data) = nullptr;
	
	void		(*m_funcCopyInKernel)(virt_addr_t dst, virt_addr_t src, uint32_t size) = nullptr;
	
	virt_addr_t (*m_funcAllocInPhysicalMemory)(uint32_t size) = nullptr;
	bool		(*m_funcDeallocInPhysicalMemory)(virt_addr_t address, uint32_t size) = nullptr;
	
	virt_addr_t	(*m_funcPhysicalToVirtual)(phys_addr_t address) = nullptr;
	virt_addr_t	(*m_funcVirtualToPhysical)(phys_addr_t address) = nullptr;

};

pagerelocator* pagerelocatorPrimitives::s_relocator = nullptr;

static TTLevel0Entry_4K gLO_4K(kTTDescriptor_TableBit);		// must be table
static TTLevel1Entry_4K gL1_4K;
static TTLevel2Entry_4K gL2_4K;
static TTLevel3Entry_4K gL3_4K(kTTDescriptor_PageBit);		// must be page;

static TTLevel0Entry_16K gLO_16K(kTTDescriptor_TableBit);	// must be table
static TTLevel1Entry_16K gL1_16K(kTTDescriptor_TableBit);	// must be table
static TTLevel2Entry_16K gL2_16K;
static TTLevel3Entry_16K gL3_16K(kTTDescriptor_PageBit);	// must be page;

static TTLevel0Entry_64K gLO_64K(kTTDescriptor_TableBit);	// must be table
static TTLevel1Entry_64K gL1_64K(kTTDescriptor_TableBit);	// must be table
static TTLevel2Entry_64K gL2_64K;
static TTLevel3Entry_64K gL3_64K(kTTDescriptor_PageBit);	// must be page;

static TTGenericEntry* gTTEntryObjects[uint32_t(TTGranule::Count)][uint32_t(TTLevel::Count)] = {
	{	&gLO_4K,	&gL1_4K,	&gL2_4K,	&gL3_4K		},
	{	&gLO_16K,	&gL1_16K,	&gL2_16K,	&gL3_16K	},
	{	&gLO_64K,	&gL1_64K,	&gL2_64K,	&gL3_64K	},
};

TTGenericEntry* GetTTEntryObject(TTEntryDetails* entry)
{
	if (! entry) return nullptr;
	
	uint32_t granuleInx;
	switch (entry->granule)
	{
		case TTGranule::Granule4K: granuleInx = 0; break;
		case TTGranule::Granule16K: granuleInx = 1; break;
		case TTGranule::Granule64K: granuleInx = 2; break;
		default: assert(0);
	}
	uint32_t levelIdx = uint32_t(entry->level);
	
	auto entryObj = gTTEntryObjects[granuleInx][levelIdx];
	entryObj->setDescriptor(entry->descriptor);
	
	return entryObj;
}

extern "C"
{
	// MARK: - ttentry functions (Generic Entry)
	
	bool ttentry_IsValid(TTEntryDetails* entry)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		return entryObj->isValid();
	}

	bool ttentry_IsBlockDescriptor(TTEntryDetails* entry)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);

		return entryObj->isBlockDescriptor();
	}

	bool ttentry_IsTableDescriptor(TTEntryDetails* entry)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);

		return entryObj->isTableDescriptor();
	}

	bool ttentry_IsReserved(TTEntryDetails* entry)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);

		return entryObj->isReserved();
	}

	bool ttentry_IsPageDescriptor(TTEntryDetails* entry)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);

		return entryObj->isPageDescriptor();
	}
	
	ttentry_t	ttentry_GetDescriptor(TTEntryDetails* entry)
	{
		if (entry == nullptr)
			assert(0);
		
		return entry->descriptor;
	}
	
	void		ttentry_SetDescriptor(TTEntryDetails* entry, ttentry_t descriptor)
	{
		if (entry == nullptr)
			assert(0);
		
		entry->descriptor = descriptor;
	}

	phys_addr_t ttentry_GetOutputAddress(TTEntryDetails* entry)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		return entryObj->getOutputAddress();
	}
	
	void ttentry_SetOutputAddress(TTEntryDetails* entry, phys_addr_t address)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		entryObj->setOutputAddress(address);
		entry->descriptor = entryObj->getDescriptor();
	}
	
	// MARK: - ttentry functions (Table Entry)
	
	bool		ttentry_GetPXNTable(TTEntryDetails* entry)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		if (entryObj->isTableDescriptor() == false)
			assert(0);
		
		return (entryObj->getDescriptor() & kTTDescriptor_PXNTableBitMask) != 0;
	}

	bool		ttentry_GetXNTable(TTEntryDetails* entry)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		if (entryObj->isTableDescriptor() == false)
			assert(0);
		
		return (entryObj->getDescriptor() & kTTDescriptor_XNTableBitMask) != 0;
	}

	TTDescriptorAPTable		ttentry_GetAPTable(TTEntryDetails* entry)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		if (entryObj->isTableDescriptor() == false)
			assert(0);
		
		return TTDescriptorAPTable((entryObj->getDescriptor() & kTTDescriptor_APTableBitMask) >> kTTDescriptor_APTableBitShift);
	}

	bool		ttentry_GetNSTable(TTEntryDetails* entry)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		if (entryObj->isTableDescriptor() == false)
			assert(0);
		
		return (entryObj->getDescriptor() & kTTDescriptor_NSTableBitMask) != 0;
	}
	
	void		ttentry_SetPXNTable(TTEntryDetails* entry, bool value)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		if (entryObj->isTableDescriptor() == false)
			assert(0);
		
		if (value)
			entry->descriptor |= kTTDescriptor_PXNTableBitMask;
		else
			entry->descriptor &= ~kTTDescriptor_PXNTableBitMask;
	}
	
	void		ttentry_SetXNTable(TTEntryDetails* entry, bool value)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		if (entryObj->isTableDescriptor() == false)
			assert(0);
		
		if (value)
			entry->descriptor |= kTTDescriptor_XNTableBitMask;
		else
			entry->descriptor &= ~kTTDescriptor_XNTableBitMask;
	}
	
	void		ttentry_SetAPTable(TTEntryDetails* entry, TTDescriptorAPTable value)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		if (entryObj->isTableDescriptor() == false)
			assert(0);
		
		entry->descriptor &= ~kTTDescriptor_PXNTableBitMask;
		entry->descriptor |= (ttentry_t(value) << kTTDescriptor_PXNTableBitShift);
	}
	
	void		ttentry_SetNSTable(TTEntryDetails* entry, bool value)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		if (entryObj->isTableDescriptor() == false)
			assert(0);
		
		if (value)
			entry->descriptor |= kTTDescriptor_NSTableBitMask;
		else
			entry->descriptor &= ~kTTDescriptor_NSTableBitMask;
	}
	
	// MARK: - ttentry functions (Block or Page Entry)
	
	uint8_t		ttentry_GetAttrIndx(TTEntryDetails* entry)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		if (entryObj->isBlockDescriptor() == false && entryObj->isPageDescriptor() == false)
			assert(0);
		
		return uint8_t((entryObj->getDescriptor() & kTTDescriptor_AttrIndxBitMask) >> kTTDescriptor_AttrIndxBitShift);
	}
	
	bool		ttentry_GetNS(TTEntryDetails* entry)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		if (entryObj->isBlockDescriptor() == false && entryObj->isPageDescriptor() == false)
			assert(0);
		
		return (entryObj->getDescriptor() & kTTDescriptor_NSBitMask) != 0;
	}
	
	TTDescriptorAP	ttentry_GetAP(TTEntryDetails* entry)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		if (entryObj->isBlockDescriptor() == false && entryObj->isPageDescriptor() == false)
			assert(0);
		
		return TTDescriptorAP((entryObj->getDescriptor() & kTTDescriptor_APBitMask) >> kTTDescriptor_APBitShift);
	}
	
	TTDescriptorSH	ttentry_GetSH(TTEntryDetails* entry)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		if (entryObj->isBlockDescriptor() == false && entryObj->isPageDescriptor() == false)
			assert(0);
		
		return TTDescriptorSH((entryObj->getDescriptor() & kTTDescriptor_SHBitMask) >> kTTDescriptor_SHBitShift);
	}
	
	bool		ttentry_GetAF(TTEntryDetails* entry)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		if (entryObj->isBlockDescriptor() == false && entryObj->isPageDescriptor() == false)
			assert(0);
		
		return (entryObj->getDescriptor() & kTTDescriptor_AFBitMask) != 0;
	}
	
	bool		ttentry_GetNG(TTEntryDetails* entry)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		if (entryObj->isBlockDescriptor() == false && entryObj->isPageDescriptor() == false)
			assert(0);
		
		return (entryObj->getDescriptor() & kTTDescriptor_nGBitMask) != 0;
	}
	
	bool		ttentry_GetContiguous(TTEntryDetails* entry)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		if (entryObj->isBlockDescriptor() == false && entryObj->isPageDescriptor() == false)
			assert(0);
		
		return (entryObj->getDescriptor() & kTTDescriptor_ContiguousBitMask) != 0;
	}
	
	bool		ttentry_GetPXN(TTEntryDetails* entry)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		if (entryObj->isBlockDescriptor() == false && entryObj->isPageDescriptor() == false)
			assert(0);
		
		return (entryObj->getDescriptor() & kTTDescriptor_PXNBitMask) != 0;
	}
	
	bool		ttentry_GetXN(TTEntryDetails* entry)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		if (entryObj->isBlockDescriptor() == false && entryObj->isPageDescriptor() == false)
			assert(0);
		
		return (entryObj->getDescriptor() & kTTDescriptor_XNBitMask) != 0;
	}

	
	void		ttentry_SetAttrIndx(TTEntryDetails* entry, uint8_t value)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		if (entryObj->isBlockDescriptor() == false && entryObj->isPageDescriptor() == false)
			assert(0);
		
		entry->descriptor &= ~kTTDescriptor_AttrIndxBitMask;
		entry->descriptor |= (ttentry_t(value) << kTTDescriptor_AttrIndxBitShift);
	}
	
	void		ttentry_SetNS(TTEntryDetails* entry, bool value)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		if (entryObj->isBlockDescriptor() == false && entryObj->isPageDescriptor() == false)
			assert(0);
		
		if (value)
			entry->descriptor |= kTTDescriptor_NSBitMask;
		else
			entry->descriptor &= ~kTTDescriptor_NSBitMask;
	}

	void		ttentry_SetAP(TTEntryDetails* entry, TTDescriptorAP value)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		if (entryObj->isBlockDescriptor() == false && entryObj->isPageDescriptor() == false)
			assert(0);
		
		entry->descriptor &= ~kTTDescriptor_APBitMask;
		entry->descriptor |= (ttentry_t(value) << kTTDescriptor_APBitShift);
	}

	void		ttentry_SetSH(TTEntryDetails* entry, TTDescriptorSH value)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		if (entryObj->isBlockDescriptor() == false && entryObj->isPageDescriptor() == false)
			assert(0);
		
		entry->descriptor &= ~kTTDescriptor_SHBitMask;
		entry->descriptor |= (ttentry_t(value) << kTTDescriptor_SHBitShift);
	}

	void		ttentry_SetAF(TTEntryDetails* entry, bool value)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		if (entryObj->isBlockDescriptor() == false && entryObj->isPageDescriptor() == false)
			assert(0);
		
		if (value)
			entry->descriptor |= kTTDescriptor_AFBitMask;
		else
			entry->descriptor &= ~kTTDescriptor_AFBitMask;
	}

	void		ttentry_SetNG(TTEntryDetails* entry, bool value)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		if (entryObj->isBlockDescriptor() == false && entryObj->isPageDescriptor() == false)
			assert(0);
		
		if (value)
			entry->descriptor |= kTTDescriptor_nGBitMask;
		else
			entry->descriptor &= ~kTTDescriptor_nGBitMask;
	}

	void		ttentry_SetContiguous(TTEntryDetails* entry, bool value)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		if (entryObj->isBlockDescriptor() == false && entryObj->isPageDescriptor() == false)
			assert(0);
		
		if (value)
			entry->descriptor |= kTTDescriptor_ContiguousBitMask;
		else
			entry->descriptor &= ~kTTDescriptor_ContiguousBitMask;
	}

	void		ttentry_SetPXN(TTEntryDetails* entry, bool value)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		if (entryObj->isBlockDescriptor() == false && entryObj->isPageDescriptor() == false)
			assert(0);
		
		if (value)
			entry->descriptor |= kTTDescriptor_PXNBitMask;
		else
			entry->descriptor &= ~kTTDescriptor_PXNBitMask;
	}

	void		ttentry_SetXN(TTEntryDetails* entry, bool value)
	{
		auto entryObj = GetTTEntryObject(entry);
		if (entryObj == nullptr)
			assert(0);
		
		if (entryObj->isBlockDescriptor() == false && entryObj->isPageDescriptor() == false)
			assert(0);
		
		if (value)
			entry->descriptor |= kTTDescriptor_XNBitMask;
		else
			entry->descriptor &= ~kTTDescriptor_XNBitMask;
	}
	
	// MARK: - MMU Config
	
	void		mmuconfig_Init(mmuconfigparser* configparser)
	{
		if (configparser == nullptr)
			return;
		
		auto configParserObj = new MMUConfigParser();
		configparser->object = (void*)configParserObj;
	}
	
	void		mmuconfig_SetTCR_EL1(mmuconfigparser* configparser, tcr_el1_t tcr_value)
	{
		if (configparser == nullptr)
			return;
		
		auto configParserObj = (MMUConfigParser*)configparser->object;
		configParserObj->setTCR_EL1(tcr_value);
	}
	
	MMUConfig	mmuconfig_GetConfigFor(mmuconfigparser* configparser, ExceptionLevel el)
	{
		if (configparser == nullptr)
			return MMUConfig();
		
		auto configParserObj = (MMUConfigParser*)configparser->object;
		return configParserObj->getConfigFor(el);
	}
	
	void		mmuconfig_Clear(mmuconfigparser* configparser)
	{
		if (configparser == nullptr)
			return;
		
		auto configParserObj = (MMUConfigParser*)configparser->object;
		configParserObj->clear();
	}
	
	void		mmuconfig_Close(mmuconfigparser* configparser)
	{
		if (configparser == nullptr)
			return;
		
		auto configParserObj = (MMUConfigParser*)configparser->object;
		delete configParserObj;
		configparser->object = nullptr;
	}
	
	// MARK: - ttwalker functions
	
	WalkResult ttwalker_Walk(ttwalker* walker, virt_addr_t address, ttwalker_callback callback)
	{
		WalkResult result = {.type = WalkResultType::Undefined, .level = TTLevel::Level0, .descriptor = 0, .outputAddress = 0};
		
		if (walker == nullptr)
			return result.setType(WalkResultType::Failed);
		
		ttwalkerPrimitives::init(walker);
		
		TTWalker<ttwalkerPrimitives> walkerObj(walker->mmu_config, walker->table_base);
		result = walkerObj.walkTo(address, [callback, walker] (WalkPosition* position, TTGenericEntry* entry) -> WalkOperation {
			assert(position != nullptr && entry != nullptr);
			TTEntryDetails details = {
				.granule	= walker->mmu_config.granule,
				.level		= position->level,
				.descriptor	= entry->getDescriptor()
			};
			if (callback != nullptr)
				return callback(position, &details, walker->cb_user_data);
			else
				return WalkOperation::Continue;
		});
		
		ttwalkerPrimitives::close();
		
		return result;
	}
	
	bool ttwalker_ReverseWalk(ttwalker* walker, virt_addr_t address, ttwalker_callback callback)
	{
		bool result = false;
		
		if (walker == nullptr)
			return false;
		
		ttwalkerPrimitives::init(walker);
		
		TTWalker<ttwalkerPrimitives> walkerObj(walker->mmu_config, walker->table_base);
		result = walkerObj.reverseWalkFrom(address, [callback, walker] (WalkPosition* position, TTGenericEntry* entry) -> WalkOperation {
			assert(position != nullptr && entry != nullptr);
			TTEntryDetails details = {
				.granule	= walker->mmu_config.granule,
				.level		= position->level,
				.descriptor	= entry->getDescriptor()
			};
			if (callback != nullptr)
				return callback(position, &details, walker->cb_user_data);
			else
				return WalkOperation::Continue;
		});
		
		ttwalkerPrimitives::close();
		
		return result;
	}
	
	phys_addr_t ttwalker_FindPhysicalAddress(ttwalker* walker, virt_addr_t address)
	{
		if (walker == nullptr)
			return kInvalidAddress;
		
		ttwalkerPrimitives::init(walker);
		
		TTWalker<ttwalkerPrimitives> walkerObj(walker->mmu_config, walker->table_base);
		
		phys_addr_t result = walkerObj.findPhysicalAddress(address);
		
		ttwalkerPrimitives::close();
		
		return result;
	}
 
	// MARK: - pagerelocator functions
	
	void pagerelocator_Init(pagerelocator* relocator)
	{
		if (relocator == nullptr)
			return;
		
		pagerelocatorPrimitives::init(relocator);
		
		auto relocatorObj = new PageRelocator<pagerelocatorPrimitives>(relocator->mmu_config, relocator->table_base);
		relocator->object = (void*)relocatorObj;
	}
	
	bool pagerelocator_RelocatePage(pagerelocator* relocator, virt_addr_t address, pagerelocator_callback callback)
	{
		if (relocator == nullptr)
			return false;

		if (relocator->object == nullptr)
			return false;
		
		virt_addr_t page = pagerelocator_PreparePageRelocation(relocator, address, callback);
		if (page == kInvalidAddress)
			return false;
		
		return pagerelocator_CompleteRelocation(relocator);
	}
	
	virt_addr_t	pagerelocator_PreparePageRelocation(pagerelocator* relocator, virt_addr_t address, pagerelocator_callback callback)
	{
		if (relocator == nullptr)
			return false;
		
		if (relocator->object == nullptr)
			return false;

		auto relocatorObj = (PageRelocator<pagerelocatorPrimitives>*)relocator->object;
		return relocatorObj->preparePageRelocationFor(address, [callback, relocator] (TTLevel level,
																					  TTGenericEntry* oldEntry,
																					  TTGenericEntry* newEntry) -> ttentry_t {
			assert(oldEntry != nullptr && newEntry != nullptr);
			TTEntryDetails oldDetails = {
				.granule	= relocator->mmu_config.granule,
				.level		= level,
				.descriptor	= oldEntry->getDescriptor()
			};
			TTEntryDetails newDetails = {
				.granule	= relocator->mmu_config.granule,
				.level		= level,
				.descriptor	= newEntry->getDescriptor()
			};
			if (callback != nullptr)
				return callback(level, &oldDetails, &newDetails, relocator->cb_user_data);
			else
				return newDetails.descriptor;
		});
	}
	
	bool		pagerelocator_CompleteRelocation(pagerelocator* relocator)
	{
		if (relocator == nullptr)
			return false;
		
		if (relocator->object == nullptr)
			return false;
		
		auto relocatorObj = (PageRelocator<pagerelocatorPrimitives>*)relocator->object;
		return relocatorObj->completeRelocation();
	}
	
	bool		pagerelocator_CancelRelocation(pagerelocator* relocator)
	{
		if (relocator == nullptr)
			return false;
		
		if (relocator->object == nullptr)
			return false;
	
		auto relocatorObj = (PageRelocator<pagerelocatorPrimitives>*)relocator->object;
		return relocatorObj->cancelRelocation();
	}
	
	bool pagerelocator_RestorePage(pagerelocator* relocator, virt_addr_t address)
	{
		if (relocator == nullptr)
			return false;
		
		if (relocator->object == nullptr)
			return false;
		
		auto relocatorObj = (PageRelocator<pagerelocatorPrimitives>*)relocator->object;
		return relocatorObj->restorePageFor(address);
	}
	
	void pagerelocator_Close(pagerelocator* relocator)
	{
		if (relocator == nullptr)
			return;
		
		auto relocatorObj = (PageRelocator<pagerelocatorPrimitives>*)relocator->object;
		delete relocatorObj;
		relocator->object = nullptr;
		
		pagerelocatorPrimitives::close();
	}
}
