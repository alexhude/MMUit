//
//  Copyright (c) 2017, Alexander Hude
//  All rights reserved.
//
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree.
//

#pragma once

#include <stdint.h>

#ifndef __cplusplus
	typedef uint64_t phys_addr_t;
	typedef uint64_t virt_addr_t;
	typedef uint64_t offset_t;
	typedef uint64_t ttentry_t;

	static const uint32_t kPlatformAddressSize = sizeof(virt_addr_t);
	static const uint32_t kPlatformAddressBits = kPlatformAddressSize * 8;
	static const offset_t kInvalidAddress = -1;
	static const offset_t kInvalidAddressOffset = -1;
#endif
