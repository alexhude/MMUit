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

typedef struct {
	MMUConfig			mmu_config;
	virt_addr_t 		table_base;
	
	uintptr_t			cb_user_data;

	// primitives
	uintptr_t			(*read_address)(virt_addr_t address);
	virt_addr_t 		(*physical_to_virtual)(phys_addr_t address);
	
} ttwalker;

#ifndef __cplusplus
	
typedef enum {
	kWalkOperation_Stop		= 0,
	kWalkOperation_Continue	= 1
} WalkOperation;

typedef enum {
	kWalkResultType_Complete	= 0,
	kWalkResultType_Stopped		= 1,
	kWalkResultType_Failed		= 2,
	kWalkResultType_Undefined
} WalkResultType;

typedef struct {
	TTLevel		level;
	virt_addr_t	tableAddress;
	offset_t	entryOffset;
} WalkPosition;
	
typedef struct {
	WalkResultType	type;
	TTLevel			level;
	ttentry_t		descriptor;
	phys_addr_t		outputAddress;
} WalkResult;
	
#endif

typedef WalkOperation (*ttwalker_callback)(WalkPosition* position, TTEntryDetails* entry, uintptr_t user_data);

WalkResult	ttwalker_Walk(ttwalker* walker, virt_addr_t address, ttwalker_callback callback);
bool		ttwalker_ReverseWalk(ttwalker* walker, virt_addr_t address, ttwalker_callback callback);
phys_addr_t ttwalker_FindPhysicalAddress(ttwalker* walker, virt_addr_t address);

#ifdef __cplusplus
}
#endif
