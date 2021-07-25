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
#include "TCR.h"

#ifdef __cplusplus
extern "C" {
#endif
	
#ifndef __cplusplus
	
	typedef struct
	{
		TTGranule	granule;
		TTLevel		initial_level;
		uint32_t	region_size_offset;
	} MMUConfig;
	
#endif
	
	typedef struct {
		MMUConfig			mmu_config;
		virt_addr_t 		table_base;
		
		uintptr_t			cb_user_data;
		
		// primitives
		uintptr_t			(*read_address)(virt_addr_t address);
		virt_addr_t 		(*physical_to_virtual)(phys_addr_t address);
		
		// cpp object
		void*				object;
	} mmuconfigparser;
	
	void				mmuconfig_Init(mmuconfigparser* configparser);
	void				mmuconfig_SetTCR_EL1(mmuconfigparser* configparser, tcr_el1_t tcr_value);
	MMUConfig			mmuconfig_GetConfigFor(mmuconfigparser* configparser, ExceptionLevel el);
	void				mmuconfig_Clear(mmuconfigparser* configparser);
	void				mmuconfig_Close(mmuconfigparser* configparcer);
	
#ifdef __cplusplus
}
#endif

