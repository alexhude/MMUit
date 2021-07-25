//
//  Copyright (c) 2017, Alexander Hude
//  All rights reserved.
//
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree.
//

#include "MMUit.hpp"

#include <iostream>

// MARK: - MMU emulation

// LEVEL 0 -> LEVEL 1 -> LEVEL 2.1 -> LEVEL 3.0 -> PAGE: 0xAAAAAAAA11111111
//                   |            |
//                   |             -> LEVEL 3.1 -> PAGE: 0xBBBBBBBB11111111
//                   |
//                    -> LEVEL 2.3 -> LEVEL 3.2 -> PAGE: 0xCCCCCCCC11111111
//                                |
//                                 -> LEVEL 3.3 -> PAGE: 0xDDDDDDDD22222222

// LEVEL 0 -> LEVEL 1 -> LEVEL 2.0 -> LEVEL 3.0

// Use Granule4K (12 bit address offset)
enum { E0, E1, E2, E3 };
enum { L0, L1, L2, L3 };
const uint32_t kAddressBitOffset = 12;
const uint32_t kTableIndexShift = 4;

constexpr ttentry_t MakeEntry(uint32_t arrayIdx, bool tableOrPage)
{
	// convert platform address to index and set valid and table bit if necessary
	return (((arrayIdx << kTableIndexShift) << kAddressBitOffset) * kPlatformAddressSize) | ((tableOrPage)? 0x2 : 0x0) | (0x1);
}

constexpr virt_addr_t MakeEntryIndex(uint64_t index, uint64_t level)
{
	return index << (kAddressBitOffset + ((3 - level) * 9));
}

constexpr virt_addr_t MakeVA(uint64_t e0Idx, uint64_t e1Idx, uint64_t e2Idx, uint64_t e3Idx, uint64_t offset)
{
	return	MakeEntryIndex(e0Idx, L0) |
			MakeEntryIndex(e1Idx, L1) |
			MakeEntryIndex(e2Idx, L2) |
			MakeEntryIndex(e3Idx, L3) |
			(offset * kPlatformAddressSize); // convert index to platform address
}

uintptr_t GetLevelIndex(uintptr_t address)
{
	address /= kPlatformAddressSize; // convert platform address to index
	return (address >> kTableIndexShift) >> kAddressBitOffset;
}

uintptr_t GetEntryIndex(uintptr_t address)
{
	address /= kPlatformAddressSize; // convert platform address to index
	return (address & 0xF);
}

ttentry_t TestTables[19][4] = {
	// Level 0 tables
	{	MakeEntry(1, true),		0,						0,						0						}, // [0] LEVEL 0
	// Level 1 tables
	{	0,						MakeEntry(3, true),		0,						MakeEntry(5, true)		}, // [1] LEVEL 1
	// Level 2 tables
	{	0,						0,						0,						0						}, // [2] LEVEL 2.0
	{	0,						0,						MakeEntry(6, true),		MakeEntry(7, true)		}, // [3] LEVEL 2.1
	{	0,						0,						0,						0						}, // [4] LEVEL 2.2
	{	MakeEntry(8, true),		MakeEntry(9, true),		0,						0						}, // [5] LEVEL 2.3
	// Level 3 tables
	{	0,						MakeEntry(10, true),	0,						0						}, // [6] LEVEL 3.0
	{	0,						0,						0,						MakeEntry(11, true)		}, // [7] LEVEL 3.1
	{	MakeEntry(12, true),	0,						0,						0						}, // [8] LEVEL 3.2
	{	0,						0,						MakeEntry(13, true),	0						}, // [9] LEVEL 3.3
	// Pages
	{	0xAAAAAAAA11111111,		0xAAAAAAAA22222222,		0xAAAAAAAA33333333,		0xAAAAAAAA44444444		}, // [10] PAGE 1
	{	0xBBBBBBBB11111111,		0xBBBBBBBB22222222,		0xBBBBBBBB33333333,		0xBBBBBBBB44444444		}, // [11] PAGE 2
	{	0xCCCCCCCC11111111,		0xCCCCCCCC22222222,		0xCCCCCCCC33333333,		0xCCCCCCCC44444444		}, // [12] PAGE 3
	{	0xDDDDDDDD11111111,		0xDDDDDDDD22222222,		0xDDDDDDDD33333333,		0xDDDDDDDD44444444		}, // [13] PAGE 4
	// Allocations
	{	0,						0,						0,						0						}, // [14] ALLOC 1 (LEVEL 2.1 COPY)
	{	0,						0,						0,						0						}, // [15] ALLOC 2 (LEVEL 3.0 COPY)
	{	0,						0,						0,						0						}, // [16] ALLOC 3 (PAGE 1 COPY)
	{	0,						0,						0,						0						}, // [17] ALLOC 4
	{	0,						0,						0,						0						}, // [18] ALLOC 5
};

// MARK: - MyPrimitives class

class MyPrimitives : public Primitives
{
public:
	uintptr_t	readAddress(virt_addr_t address)
	{
		// address here is actually physical to simplify emulation
		return TestTables[GetLevelIndex(address)][GetEntryIndex(address)];
	};
	
	void		writeAddress(virt_addr_t address, virt_addr_t data)
	{
		// address here is actually physical to simplify emulation
		TestTables[GetLevelIndex(address)][GetEntryIndex(address)] = data;
	}
	
	void		copyInKernel(virt_addr_t dst, virt_addr_t src, uint32_t size)
	{
		// addresses here are actually physical to simplify emulation
		TestTables[GetLevelIndex(dst)][GetEntryIndex(dst) + 0] = TestTables[GetLevelIndex(src)][GetEntryIndex(src) + 0];
		TestTables[GetLevelIndex(dst)][GetEntryIndex(dst) + 1] = TestTables[GetLevelIndex(src)][GetEntryIndex(src) + 1];
		TestTables[GetLevelIndex(dst)][GetEntryIndex(dst) + 2] = TestTables[GetLevelIndex(src)][GetEntryIndex(src) + 2];
		TestTables[GetLevelIndex(dst)][GetEntryIndex(dst) + 3] = TestTables[GetLevelIndex(src)][GetEntryIndex(src) + 3];
	}
	
	virt_addr_t allocInPhysicalMemory(uint32_t size)
	{
		static uint32_t freePage = 14;
		
		uint32_t l1 = 0, l2 = 0, l3 = 0;
		uint32_t e1 = 0, e2 = 0, e3 = 0;
		
		// find free level 1 entry
		for (uint32_t e = 0; e < 4; e++)
		{
			if (TestTables[1][e] == 0)
			{
				l1 = 1; e1 = e;
				break;
			}
		}

		// find free level 2 entry
		for (uint32_t l = 2; l < 6; l++)
		{
			bool found = false;
			for (uint32_t e = 0; e < 4; e++)
			{
				if (TestTables[l][e] == 0)
				{
					l2 = l; e2 = e;
					found = true;
					break;
				}
			}
			if (found)
				break;
		}
		
		// find free level 3 entry
		for (uint32_t l = 6; l < 10; l++)
		{
			bool found = false;
			for (uint32_t e = 0; e < 4; e++)
			{
				if (TestTables[l][e] == 0)
				{
					l3 = l; e3 = e;
					found = true;
					break;
				}
			}
			if (found)
				break;
		}
		
		// update translation table
		TestTables[l1][e1] = MakeEntry(l2, true);
		TestTables[l2][e2] = MakeEntry(l3, true);
		TestTables[l3][e3] = MakeEntry(freePage, true);

		// create virtual address
		// return MakeVA(E0, e1, e2, e3, 0);

		// to simplify emulation return physical address here
		virt_addr_t address = ((freePage << kTableIndexShift) << kAddressBitOffset) * kPlatformAddressSize;
		freePage++;
		
		return address;
	}
	
	bool deallocInPhysicalMemory(virt_addr_t address, uint32_t size)
	{
		return true;
	}
	
	virt_addr_t physicalToVirtual (phys_addr_t address)
	{
		// treat physical address as virtual to simplify emulation
		return address;
	}

	phys_addr_t virtualToPhysical (virt_addr_t address)
	{
		// treat virtual address as physical to simplify emulation
		return address;
	}

};

// MARK: - main

int main(int argc, const char * argv[])
{
	uintptr_t value;
	virt_addr_t vaddr;
	phys_addr_t paddr;
	
	TCR_EL1 tcr_el1(0x2A51C251C);
	
	printf("\n*** TEST MMU Config\n");
	
	MMUConfig mmuConfig;
	MMUConfigParser mmuConfigParser;
	mmuConfigParser.setTCR_EL1(tcr_el1);
	
	mmuConfig = mmuConfigParser.getConfigFor(ExceptionLevel::EL1);
	printf("TCR: 0x%.16llX\n", tcr_el1.getValue());
	printf("     Granule:          %u\n", uint32_t(mmuConfig.granule));
	printf("     InitialLevel:     %u\n", uint32_t(mmuConfig.initialLevel));
	printf("     RegionSizeOffset: %u\n", uint32_t(mmuConfig.regionSizeOffset));
	assert(mmuConfig.granule == TTGranule::Granule4K && mmuConfig.initialLevel == TTLevel::Level1 && mmuConfig.regionSizeOffset == 28);

	virt_addr_t ttbr = ((uint32_t(mmuConfig.initialLevel) << kTableIndexShift) << kAddressBitOffset) * kPlatformAddressSize;
	TTWalker<MyPrimitives> walker(mmuConfig, ttbr);
	PageRelocator<MyPrimitives> relocator(mmuConfig, ttbr);
	
	printf("\n*** TEST findPhysicalAddress()\n");
	
	vaddr = MakeVA(E0, E1, E2, E1, 0);
	paddr = walker.findPhysicalAddress(vaddr);
	value = walker.readAddress(walker.physicalToVirtual(paddr));
	printf("[0] 0x%.16llX -> 0x%.16llX : 0x%.16lX\n", vaddr, paddr, value);
	assert(value == 0xAAAAAAAA11111111);

	vaddr = MakeVA(E0, E1, E3, E3, 1);
	paddr = walker.findPhysicalAddress(vaddr);
	value = walker.readAddress(walker.physicalToVirtual(paddr));
	printf("[1] 0x%.16llX -> 0x%.16llX : 0x%.16lX\n", vaddr, paddr, value);
	assert(value == 0xBBBBBBBB22222222);

	vaddr = MakeVA(E0, E3, E0, E0, 2);
	paddr = walker.findPhysicalAddress(vaddr);
	value = walker.readAddress(walker.physicalToVirtual(paddr));
	printf("[2] 0x%.16llX -> 0x%.16llX : 0x%.16lX\n", vaddr, paddr, value);
	assert(value == 0xCCCCCCCC33333333);
	
	vaddr = MakeVA(E0, E3, E1, E2, 3);
	paddr = walker.findPhysicalAddress(vaddr);
	value = walker.readAddress(walker.physicalToVirtual(paddr));
	printf("[3] 0x%.16llX -> 0x%.16llX : 0x%.16lX\n", vaddr, paddr, value);
	assert(value == 0xDDDDDDDD44444444);
	
	TTGenericWalker* genericWalker = &walker;
	
	printf("\n*** TEST walkTo()\n");

	WalkResult walkResult;
	vaddr = MakeVA(E0, E1, E3, E3, 0);
	printf("Address: 0x%.16llX\n", vaddr);
	walkResult = genericWalker->walkTo(vaddr, [] (WalkPosition* position, TTGenericEntry* entry) -> WalkOperation {
		printf(" Level%d: %c%c%c [%.2lu][%.2lu]\n", position->level,
			   (entry->isValid())? 'v' : '-',
			   (entry->isTableDescriptor())? 't' : '-',
			   (entry->isPageDescriptor())? 'p' : '-',
			   GetLevelIndex(position->tableAddress), GetEntryIndex(position->entryOffset));
		return WalkOperation::Continue;
	});
	assert(walkResult.getType() == WalkResultType::Complete);
	
	paddr = walkResult.getOutputAddress();
	printf("   page:     [%.2lu][%.2lu] = 0x%.16lX\n",
		   GetLevelIndex(paddr),
		   GetEntryIndex(paddr),
		   walker.readAddress(walker.physicalToVirtual(paddr)));

	printf("\n*** TEST reverseWalkFrom()\n");
	
	bool reverseResult;
	vaddr = MakeVA(E0, E1, E3, E3, 0);
	printf("Address: 0x%.16llX\n", vaddr);

	// With defaul callback: genericWalker->reverseWalkFrom(vaddr, TTGenericWalker::DefaultCallback);
	reverseResult = genericWalker->reverseWalkFrom(vaddr, [] (WalkPosition* position, TTGenericEntry* entry) -> WalkOperation {
		printf(" Level%d: %c%c%c [%.2lu][%.2lu]\n", position->level,
			   (entry->isValid())? 'v' : '-',
			   (entry->isTableDescriptor())? 't' : '-',
			   (entry->isPageDescriptor())? 'p' : '-',
			   GetLevelIndex(position->tableAddress), GetEntryIndex(position->entryOffset));
		return WalkOperation::Continue;
	});
	assert(reverseResult == true);

	printf("\n*** TEST relocatePageFor()\n");
	
	vaddr = MakeVA(E0, E1, E3, E3, 0);
	
	// check before state
	paddr = walker.findPhysicalAddress(vaddr);
	value = walker.readAddress(walker.physicalToVirtual(paddr));
	printf("ORIGINAL: 0x%.16llX -> 0x%.16llX : 0x%.16lX\n", vaddr, paddr, value);
	assert(value == 0xBBBBBBBB11111111);

	// With default callback: relocator.relocatePageFor(vaddr, PageRelocator<TTGranule::Granule4K, MyPrimitives>::DefaultCallback);
	bool relocateResult;
	relocateResult = relocator.relocatePageFor(vaddr, [&relocator] (TTLevel level, TTGenericEntry* oldEntry, TTGenericEntry* newEntry) -> ttentry_t {
		printf("    MOVE: 0x%.16llX -> 0x%.16llX | [%.2lu][] -> [%.2lu][]\n",
			   oldEntry->getOutputAddress(),
			   newEntry->getOutputAddress(),
			   GetLevelIndex(oldEntry->getOutputAddress()), GetLevelIndex(newEntry->getOutputAddress()));
		// patch data in new entry
        if (newEntry->isPageDescriptor()) {
            dynamic_cast<TTLevel3Entry_4K*>(newEntry)->setXN(false);
            dynamic_cast<TTLevel3Entry_4K*>(newEntry)->setPXN(false);
			relocator.writeAddress(relocator.physicalToVirtual(newEntry->getOutputAddress()), 0xDEADBEEFDEADBEEF);
        }
		return newEntry->getDescriptor();
	});
	assert(relocateResult == true);

	// check after state
	paddr = walker.findPhysicalAddress(vaddr);
	value = walker.readAddress(walker.physicalToVirtual(paddr));
	printf("RELOCATE: 0x%.16llX -> 0x%.16llX : 0x%.16lX\n", vaddr, paddr, value);
	assert(value == 0xDEADBEEFDEADBEEF);
	
	printf("\n*** TEST preparePageRelocationFor()\n");
	
    bool cancel = false;
	vaddr = MakeVA(E0, E1, E2, E1, 0);
	
	// check before state
	paddr = walker.findPhysicalAddress(vaddr);
	value = walker.readAddress(walker.physicalToVirtual(paddr));
	printf("ORIGINAL: 0x%.16llX -> 0x%.16llX : 0x%.16lX\n", vaddr, paddr, value);
	assert(value == 0xAAAAAAAA11111111);
	
	virt_addr_t newPageVA;
	newPageVA = relocator.preparePageRelocationFor(vaddr, [&relocator] (TTLevel level, TTGenericEntry* oldEntry, TTGenericEntry* newEntry) -> ttentry_t {
		printf("    MOVE: 0x%.16llX -> 0x%.16llX | [%.2lu][] -> [%.2lu][]\n",
			   oldEntry->getOutputAddress(),
			   newEntry->getOutputAddress(),
			   GetLevelIndex(oldEntry->getOutputAddress()), GetLevelIndex(newEntry->getOutputAddress()));
		return newEntry->getDescriptor();
	});
	assert(newPageVA != kInvalidAddress);

	// patch data in new entry
	relocator.writeAddress(newPageVA, 0xDEADBEEFDEADBEEF);

	printf("\n*** TEST cancelRelocation()\n");
	
    if (cancel) {
        // cancel relocation
        relocateResult = relocator.cancelRelocation();
    } else {
        // complete relocation
        relocateResult = relocator.completeRelocation();
    }
	
	// check after state
	paddr = walker.findPhysicalAddress(vaddr);
	value = walker.readAddress(walker.physicalToVirtual(paddr));
	printf("CANCELED: 0x%.16llX -> 0x%.16llX : 0x%.16lX\n", vaddr, paddr, value);
	assert(value == 0xAAAAAAAA11111111);
	
	printf("\n*** TEST restorePageFor()\n");

	vaddr = MakeVA(E0, E1, E3, E3, 0);

	// restore original page
	bool restoreResult;
	restoreResult = relocator.restorePageFor(vaddr);
	assert(restoreResult == true);
	
	// check after restore
	paddr = walker.findPhysicalAddress(vaddr);
	value = walker.readAddress(walker.physicalToVirtual(paddr));
	printf(" RESTORE: 0x%.16llX -> 0x%.16llX : 0x%.16lX\n", vaddr, paddr, value);
	assert(value == 0xBBBBBBBB11111111);

	printf("\n*** TEST COMPLETE\n");
    return 0;
}
