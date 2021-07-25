//
//  Copyright (c) 2017, Alexander Hude
//  All rights reserved.
//
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree.
//

#pragma once

#include <stdint.h>

class Primitives
{
public:
	Primitives()
	{}
	
	~Primitives()
	{}
	
	// Read
	
	virtual uint8_t		read8(virt_addr_t address) { assert(0); }
	virtual uint16_t	read16(virt_addr_t address) { assert(0); }
	virtual uint32_t	read32(virt_addr_t address) { assert(0); }
	virtual uint64_t	read64(virt_addr_t address) { assert(0); }
	virtual uintptr_t	readAddress(virt_addr_t address) { assert(0); }

	// Write
	
	virtual void		write8(virt_addr_t address, uint8_t data) { assert(0); }
	virtual void		write16(virt_addr_t address, uint16_t data) { assert(0); }
	virtual void		write32(virt_addr_t address, uint32_t data) { assert(0); }
	virtual void		write64(virt_addr_t address, uint64_t data) { assert(0); }
	virtual void		writeAddress(virt_addr_t address, uintptr_t data) { assert(0); }

	// Function call
	
	virtual uintptr_t	callFunction(virt_addr_t address) { assert(0); }
	virtual uintptr_t	callFunction(virt_addr_t address, uintptr_t arg0) { assert(0); }
	virtual uintptr_t	callFunction(virt_addr_t address, uintptr_t arg0, uintptr_t arg1) { assert(0); }
	virtual uintptr_t	callFunction(virt_addr_t address, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2) { assert(0); }
	virtual uintptr_t	callFunction(virt_addr_t address, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3) { assert(0); }
	virtual uintptr_t	callFunction(virt_addr_t address, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4) { assert(0); }
	virtual uintptr_t	callFunction(virt_addr_t address, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t arg5) { assert(0); }

	// Memory allocation
	
	virtual virt_addr_t allocInPhysicalMemory(uint32_t size) { assert(0); }
	virtual bool deallocInPhysicalMemory(virt_addr_t address, uint32_t size) { assert(0); }

	// Memory copy
	
	virtual void copyInKernel(virt_addr_t dst, virt_addr_t src, uint32_t size) { assert(0); }
	
	// Virtual <-> Physical address conversion

	phys_addr_t virtualToPhysical (virt_addr_t address) { assert(0); }
	virt_addr_t physicalToVirtual (phys_addr_t address) { assert(0); }
	
};
