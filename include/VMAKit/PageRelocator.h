//
//  Copyright (c) 2017, Alexander Hude
//  All rights reserved.
//
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree.
//

#pragma once

#include "VMAPlatform.h"
#include "VMATypes.h"
#include "TTEntry.h"
#include "MMUConfig.h"

#ifdef __cplusplus
extern "C" {
#endif
	
typedef struct
{
	MMUConfig			mmu_config;
	virt_addr_t 		table_base;
	
	uintptr_t			cb_user_data;
	
	// primitives
	uintptr_t			(*read_address)(virt_addr_t address);
	void				(*write_address)(virt_addr_t address, virt_addr_t data);
	
	void				(*copy_in_kernel)(virt_addr_t dst, virt_addr_t src, uint32_t size);
	
	virt_addr_t 		(*alloc_in_physical_memory)(uint32_t size);
	bool				(*dealloc_in_physical_memory)(virt_addr_t address, uint32_t size);
	
	virt_addr_t			(*physical_to_virtual)(phys_addr_t address);
	virt_addr_t			(*virtual_to_physical)(phys_addr_t address);
	
	// cpp object
	void*				object;
} pagerelocator;
	
typedef ttentry_t (*pagerelocator_callback)(TTLevel level, TTEntryDetails* oldEntry, TTEntryDetails* newEntry, uintptr_t user_data);
	
void		pagerelocator_Init(pagerelocator* relocator);
bool		pagerelocator_RelocatePage(pagerelocator* relocator, virt_addr_t address, pagerelocator_callback callback);
virt_addr_t	pagerelocator_PreparePageRelocation(pagerelocator* relocator, virt_addr_t address, pagerelocator_callback callback);
bool		pagerelocator_CompleteRelocation(pagerelocator* relocator);
bool		pagerelocator_CancelRelocation(pagerelocator* relocator);
bool		pagerelocator_RestorePage(pagerelocator* relocator, virt_addr_t address);
void		pagerelocator_Close(pagerelocator* relocator);
	
#ifdef __cplusplus
}
#endif
