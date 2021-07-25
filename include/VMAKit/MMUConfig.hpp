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
#include "TCR.hpp"

struct MMUConfig
{
	TTGranule	granule;
	TTLevel		initialLevel;
	uint32_t	regionSizeOffset;
};

class MMUConfigParser
{
public:
			MMUConfigParser()
			{
				clear();
			};
			~MMUConfigParser()
			{};
	
	void	setTCR_EL1(TCR_EL1 tcr_el1)
	{
		// Parse TCR for TTBR0
		MMUConfig* config = &m_configs[uint32_t(ExceptionLevel::EL0)];
		
        config->regionSizeOffset = tcr_el1.getT0SZ();
        switch (tcr_el1.getTG0()) {
            case TCR_TG0::Granule4K: config->granule = TTGranule::Granule4K; break;
            case TCR_TG0::Granule16K: config->granule = TTGranule::Granule16K; break;
            case TCR_TG0::Granule64K: config->granule = TTGranule::Granule64K; break;
            default: assert(0);
        }
            
        if (config->regionSizeOffset) {
            config->initialLevel = getInitialLevel(config->granule, config->regionSizeOffset);
        }

		// Parse TCR for TTBR1
		config = &m_configs[uint32_t(ExceptionLevel::EL1)];
		
        config->regionSizeOffset = tcr_el1.getT1SZ();
        switch (tcr_el1.getTG1()) {
            case TCR_TG1::Granule4K: config->granule = TTGranule::Granule4K; break;
            case TCR_TG1::Granule16K: config->granule = TTGranule::Granule16K; break;
            case TCR_TG1::Granule64K: config->granule = TTGranule::Granule64K; break;
            default: assert(0);
        }
            
        if (config->regionSizeOffset) {
            config->initialLevel = getInitialLevel(config->granule, config->regionSizeOffset);
        }
	}
	
	void    setTCR_EL1(tcr_el1_t tcr_value)
	{
		setTCR_EL1(TCR_EL1(tcr_value));
	}
	
	void	setTCR_EL2(TCR_EL2 tcr_el2)
	{
        // Parse TCR for TTBR0
		MMUConfig* config = &m_configs[uint32_t(ExceptionLevel::EL2)];
		
        config->regionSizeOffset = tcr_el2.getT0SZ();
        switch (tcr_el2.getTG0()) {
            case TCR_TG0::Granule4K: config->granule = TTGranule::Granule4K; break;
            case TCR_TG0::Granule16K: config->granule = TTGranule::Granule16K; break;
            case TCR_TG0::Granule64K: config->granule = TTGranule::Granule64K; break;
            default: assert(0);
        }
            
        if (config->regionSizeOffset) {
            config->initialLevel = getInitialLevel(config->granule, config->regionSizeOffset);
        }
	}
	
	void    setTCR_EL2(tcr_el2_t tcr_value)
	{
		setTCR_EL2(TCR_EL2(tcr_value));
	}
	
	void	setTCR_EL3(TCR_EL3 tcr_el3)
	{
        // Parse TCR for TTBR0
		MMUConfig* config = &m_configs[uint32_t(ExceptionLevel::EL3)];
		
        config->regionSizeOffset = tcr_el3.getT0SZ();
        switch (tcr_el3.getTG0()) {
            case TCR_TG0::Granule4K: config->granule = TTGranule::Granule4K; break;
            case TCR_TG0::Granule16K: config->granule = TTGranule::Granule16K; break;
            case TCR_TG0::Granule64K: config->granule = TTGranule::Granule64K; break;
            default: assert(0);
        }
            
        if (config->regionSizeOffset) {
            config->initialLevel = getInitialLevel(config->granule, config->regionSizeOffset);
        }
	}
	
	void setTCR_EL3(tcr_el3_t tcr_value)
	{
		setTCR_EL3(TCR_EL3(tcr_value));
	}
	
	MMUConfig	getConfigFor(ExceptionLevel el)
	{
		return m_configs[uint32_t(el)];
	}
	
	void		clear()
	{
		for (uint32_t i = 0; i < uint32_t(ExceptionLevel::Count); i++)
			m_configs[i] = { .granule = TTGranule::Undefined, .initialLevel = TTLevel::Undefined, .regionSizeOffset = 0 };
	}
	
private:
	
	TTLevel		getInitialLevel(TTGranule granule, uint32_t regionOffsetSize)
	{
		assert(regionOffsetSize >= 16 && regionOffsetSize <= 39);
		
		switch (granule) {
			case TTGranule::Granule4K:
			{
				// Table D4-11 TCR.TnSZ values and IA ranges, 4K granule with no concatenation of tables
                switch (regionOffsetSize) {
                    case 16 ... 24: return TTLevel::Level0;
                    case 25 ... 33: return TTLevel::Level1;
                    case 34 ... 39: return TTLevel::Level2;
                    default: break;
                };
				break;
			}
			case TTGranule::Granule16K:
			{
				// Table D4-14 TCR.TnSZ values and IA ranges, 16K granule with no concatenation of tables
                switch (regionOffsetSize) {
                    case 16:        return TTLevel::Level0;
                    case 17 ... 27: return TTLevel::Level1;
                    case 28 ... 38: return TTLevel::Level2;
                    case 39:        return TTLevel::Level3;
                    default: break;
                };
				break;
			}
			case TTGranule::Granule64K:
			{
				// Table D4-17 TCR.TnSZ values and IA ranges, 64K granule with no concatenation of tables
                switch (regionOffsetSize) {
                    case 16 ... 21: return TTLevel::Level1;
                    case 22 ... 34: return TTLevel::Level2;
                    case 35 ... 39: return TTLevel::Level3;
                    default: break;
                }
				break;
			}
				
			default: assert(0);
		}
		
		return TTLevel::Undefined;
	}
	
private:
	
	MMUConfig	m_configs[uint32_t(ExceptionLevel::Count)];
};
