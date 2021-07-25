//
//  Copyright (c) 2017, Alexander Hude
//  All rights reserved.
//
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree.
//

#pragma once

#include <stdint.h>
#include <assert.h>

using phys_addr_t = uint64_t;
using virt_addr_t = uint64_t;
using offset_t = uint64_t;
using ttentry_t = uint64_t;

static const uint32_t kPlatformAddressSize = sizeof(virt_addr_t);
static const uint32_t kPlatformAddressBits = kPlatformAddressSize * 8;
static const offset_t kInvalidAddress = -1;
static const offset_t kInvalidAddressOffset = -1;

// Input address (IA) using the 4K translation granule
// D4.2 The VMSAv8-64 address translation system (Figure D4-3)
using vm_addr_4k = union {
	struct {
		virt_addr_t outputAddress : 12;	// IA[11:0]
		virt_addr_t tableIndexL3 : 9;	// IA[20:12]
		virt_addr_t tableIndexL2 : 9;	// IA[29:21]
		virt_addr_t tableIndexL1 : 9;	// IA[38:30]
		virt_addr_t tableIndexL0 : 9;	// IA[47:39]
		virt_addr_t ttbrRange : 16;
	} details;
	
	virt_addr_t value;
};

// Input address (IA) using the 16K translation granule
// D4.2 The VMSAv8-64 address translation system (Figure D4-4)
using vm_addr_16k = union {
	struct {
		virt_addr_t outputAddress : 14;	// IA[13:0]
		virt_addr_t tableIndexL3 : 11;	// IA[24:14]
		virt_addr_t tableIndexL2 : 11;	// IA[35:25]
		virt_addr_t tableIndexL1 : 11;	// IA[46:36]
		virt_addr_t tableIndexL0 : 1;	// IA[47]
		virt_addr_t ttbrRange : 16;
	} details;
	
	virt_addr_t value;
};

// Input address (IA) using the 16K translation granule
// D4.2 The VMSAv8-64 address translation system (Figure D4-5)
using vm_addr_64k = union {
	struct {
		virt_addr_t outputAddress : 16;	// IA[15:0]
		virt_addr_t tableIndexL3 : 13;	// IA[28:16]
		virt_addr_t tableIndexL2 : 13;	// IA[41:29]
		virt_addr_t tableIndexL1 : 6;	// IA[47:42]
		virt_addr_t ttbrRange : 16;
	} details;
	
	virt_addr_t value;
};


