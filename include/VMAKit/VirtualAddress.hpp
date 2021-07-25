//
//  Copyright (c) 2017, Alexander Hude
//  All rights reserved.
//
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree.
//

#pragma once

#include "VMAPlatform.hpp"
#include "VMATypes.hpp"

template <TTGranule GRANULE> struct VirtualAddressType {};

template <> struct VirtualAddressType<TTGranule::Granule4K>
{
	using Type = vm_addr_4k;
};

template <> struct VirtualAddressType<TTGranule::Granule16K>
{
	using Type = vm_addr_16k;
};

template <> struct VirtualAddressType<TTGranule::Granule64K>
{
	using Type = vm_addr_64k;
};

class GenericVirtualAddress
{
	virtual offset_t getOffsetForLevel(TTLevel level) = 0;
	virtual offset_t getOffsetForLevel(uint32_t level) = 0;
};

template <TTGranule GRANULE>
class VirtualAddress : public GenericVirtualAddress
{
public:
	
	using VirtualAddressType = typename VirtualAddressType<GRANULE>::Type;
	
	VirtualAddress() = delete;
	
	VirtualAddress(VirtualAddressType address, uint32_t regionSizeOffset = 0)
		: m_virtAddress(address), m_regionSizeOffset(regionSizeOffset)
	{ assert(regionSizeOffset < kPlatformAddressBits); }
	
	VirtualAddress(virt_addr_t address, uint32_t regionSizeOffset = 0)
		: m_virtAddress({.value = address}), m_regionSizeOffset(regionSizeOffset)
	{ assert(regionSizeOffset < kPlatformAddressBits); }
	
    virt_addr_t rawValue()
    {
        return m_virtAddress.value;
    }

    VirtualAddressType virtualAddress()
    {
        return m_virtAddress;
    }

    uint32_t regionSizeOffset()
    {
        return m_regionSizeOffset;
    }

	offset_t getOffsetForLevel(TTLevel level) override
	{
		VirtualAddressType va({.value = m_virtAddress.value & ((virt_addr_t(1) << (kPlatformAddressBits - m_regionSizeOffset)) - 1)});
		
		switch (level)
		{
			case TTLevel::Level0: return va.details.tableIndexL0 * kPlatformAddressSize;
			case TTLevel::Level1: return va.details.tableIndexL1 * kPlatformAddressSize;
			case TTLevel::Level2: return va.details.tableIndexL2 * kPlatformAddressSize;
			case TTLevel::Level3: return va.details.tableIndexL3 * kPlatformAddressSize;
			default: assert(0);
		}
	}
	
	offset_t getOffsetForLevel(uint32_t level) override
	{
		if (level < (uint32_t)TTLevel::Count)
			return getOffsetForLevel((TTLevel)level);
		else
			return kInvalidAddressOffset;
	}
	
private:

	VirtualAddressType	m_virtAddress;
	uint32_t			m_regionSizeOffset;
};

template <>
offset_t VirtualAddress<TTGranule::Granule64K>::getOffsetForLevel(TTLevel level)
{
	assert(level != TTLevel::Level0);
	
	VirtualAddressType va({.value = m_virtAddress.value & ((virt_addr_t(1) << (kPlatformAddressBits - m_regionSizeOffset)) - 1) });
	
	switch (level)
	{
		case TTLevel::Level1: return va.details.tableIndexL1 * kPlatformAddressSize;
		case TTLevel::Level2: return va.details.tableIndexL2 * kPlatformAddressSize;
		case TTLevel::Level3: return va.details.tableIndexL3 * kPlatformAddressSize;
		default: assert(0);
	}
}

using VA4K = VirtualAddress<TTGranule::Granule4K>;
using VA16K = VirtualAddress<TTGranule::Granule16K>;
using VA64K = VirtualAddress<TTGranule::Granule64K>;

