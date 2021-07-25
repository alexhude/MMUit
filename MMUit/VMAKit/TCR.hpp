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

enum class TCR_EPD0 : uint32_t	// Translation table walk disable for translations using TTBR0_EL1.
{
	PerformWalk			= 0b0, 	// Perform translation table walks using TTBR0_EL1.
	FaultOnMiss			= 0b1, 	// A TLB miss on an address that is translated using TTBR0_EL1 generates a Translation fault. No translation table walk is performed.
};

enum class TCR_IRGN0 : uint32_t	// Inner cacheability attribute for memory associated with translation table walks using TTBR0_EL1.
{
	InnerNC				= 0b00, // Normal memory, Inner Non-cacheable
	InnerWB_RA_WA_C		= 0b01, // Normal memory, Inner Write-Back Read-Allocate Write-Allocate Cacheable
	InnerWT_RA_NWA_C 	= 0b10, // Normal memory, Inner Write-Through Read-Allocate No Write-Allocate Cacheable
	InnerWB_RA_NWA_C 	= 0b11, // Normal memory, Inner Write-Back Read-Allocate No Write-Allocate Cacheable
};

enum class TCR_ORGN0 : uint32_t	// Outer cacheability attribute for memory associated with translation table walks using TTBR0_EL1.

{
	OuterNC				= 0b00, // Normal memory, Outer Non-cacheable
	OuterWB_RA_WA_C		= 0b01, // Normal memory, Outer Write-Back Read-Allocate Write-Allocate Cacheable
	OuterWT_RA_NWA_C	= 0b10, // Normal memory, Outer Write-Through Read-Allocate No Write-Allocate Cacheable
	OuterWB_RA_NWA_C	= 0b11, // Normal memory, Outer Write-Back Read-Allocate No Write-Allocate Cacheable
};

enum class TCR_SH0 : uint32_t	// Shareability attribute for memory associated with translation table walks using TTBR0_EL1.
{
	NonShareable 		= 0b00, // Non-shareable
	OuterShareable		= 0b10, // Outer Shareable
	InnerShareable		= 0b11, // Inner Shareable
};

enum class TCR_TG0 : uint32_t	// Granule size for the TTBR0_EL1.
{
	Granule4K			= 0b00, // 4KB
	Granule64K			= 0b01, // 64KB
	Granule16K			= 0b10, // 16KB
};

enum class TCR_A1 : uint32_t	// Selects whether TTBR0_EL1 or TTBR1_EL1 defines the ASID.
{
	ASID_TTBR0			= 0b0,	// TTBR0_EL1.ASID defines the ASID.
	ASID_TTBR1			= 0b1, 	// TTBR1_EL1.ASID defines the ASID.
};

enum class TCR_EPD1 : uint32_t	// Translation table walk disable for translations using TTBR1_EL1.
{
	PerformWalk			= 0b0, 	// Perform translation table walks using TTBR1_EL1.
	FaultOnMiss			= 0b1, 	// A TLB miss on an address that is translated using TTBR1_EL1 generates a Translation fault. No translation table walk is performed.
};

enum class TCR_IRGN1 : uint32_t	// Inner cacheability attribute for memory associated with translation table walks using TTBR1_EL1.
{
	InnerNC				= 0b00, // Normal memory, Inner Non-cacheable
	InnerWB_RA_WA_C		= 0b01, // Normal memory, Inner Write-Back Read-Allocate Write-Allocate Cacheable
	InnerWT_RA_NWA_C 	= 0b10, // Normal memory, Inner Write-Through Read-Allocate No Write-Allocate Cacheable
	InnerWB_RA_NWA_C 	= 0b11, // Normal memory, Inner Write-Back Read-Allocate No Write-Allocate Cacheable
};

enum class TCR_ORGN1 : uint32_t	// Outer cacheability attribute for memory associated with translation table walks using TTBR1_EL1.
{
	OuterNC				= 0b00, // Normal memory, Outer Non-cacheable
	OuterWB_RA_WA_C		= 0b01, // Normal memory, Outer Write-Back Read-Allocate Write-Allocate Cacheable
	OuterWT_RA_NWA_C	= 0b10, // Normal memory, Outer Write-Through Read-Allocate No Write-Allocate Cacheable
	OuterWB_RA_NWA_C	= 0b11, // Normal memory, Outer Write-Back Read-Allocate No Write-Allocate Cacheable
};

enum class TCR_SH1 : uint32_t	// Shareability attribute for memory associated with translation table walks using TTBR1_EL1.
{
	NonShareable 		= 0b00, // Non-shareable
	OuterShareable		= 0b10, // Outer Shareable
	InnerShareable		= 0b11, // Inner Shareable
};

enum class TCR_TG1 : uint32_t	// Granule size for the TTBR1_EL1.
{
	Granule16K			= 0b01, // 16KB
	Granule4K			= 0b10, // 4KB
	Granule64K			= 0b11, // 64KB
};

enum class TCR_PS : uint32_t	// Intermediate Physical Address Size.
{
	Size4GB				= 0b000, // 32 bits, 4GB.
	Size64GB			= 0b001, // 36 bits, 64GB.
	Size1TB				= 0b010, // 40 bits, 1TB.
	Size4TB				= 0b011, // 42 bits, 4TB.
	Size16TB			= 0b100, // 44 bits, 16TB.
	Size256TB			= 0b101, // 48 bits, 256TB.
};

using TCR_IPS = TCR_PS;

enum class TCR_AS : uint32_t	// ASID Size
{
	ASID_8bit			= 0b0,	// 8 bit
	ASID_16bit			= 0b1,	// 16 bit
};

enum class TCR_TBI : uint32_t	// Top Byte ignored, indicates whether the top byte of an address is used for address match for the TTBR0_ELx region
{
	TopByteUsed			= 0b0,	// Top Byte used in the address calculation.
	TopByteIgnored		= 0b1,	// Top Byte ignored in the address calculation.
};

template<ExceptionLevel LEVEL> union TCRFormat {};

// MARK: - TCR_EL1
// D7.2.84. TCR_EL1, Translation Control Register (EL1)

using tcr_el1_t	= uint64_t;

template<>
union TCRFormat<ExceptionLevel::EL1>
{
	using tcr_value_t = tcr_el1_t;
	
	struct
	{
		tcr_value_t T0SZ	: 6;	// [5:0]
		tcr_value_t RES0_1	: 1;	// [6]
		TCR_EPD0	EPD0	: 1;	// [7]
		TCR_IRGN0 	IRGN0	: 2;	// [9:8]
		TCR_ORGN0 	ORGN0	: 2;	// [11:10]
		TCR_SH0		SH0		: 2;	// [13:12]
		TCR_TG0 	TG0		: 2;	// [15:14]
		tcr_value_t T1SZ	: 6;	// [21:16]
		TCR_A1 		A1		: 1;	// [22]
		TCR_EPD1 	EPD1	: 1;	// [23]
		TCR_IRGN1 	IRGN1	: 2;	// [25:24]
		TCR_ORGN1 	ORGN1	: 2;	// [27:26]
		TCR_SH1 	SH1		: 2;	// [29:28]
		TCR_TG1		TG1		: 2;	// [31:30]
		TCR_IPS 	IPS		: 3;	// [34:32]
		tcr_value_t RES0_2	: 1;	// [35]
		TCR_AS 		AS		: 1;	// [36]
		TCR_TBI 	TBI0	: 1;	// [37]
		TCR_TBI 	TBI1	: 1;	// [38]
		tcr_value_t RES0_3	: 25;	// [63:39]
	} details;
	
	tcr_value_t value;
	
	TCRFormat(tcr_value_t value) : value(value) { }
	
	uint32_t	getT0SZ() 	{ return uint32_t(details.T0SZ);	}
	TCR_EPD0 	getEPD0() 	{ return details.EPD0; 				}
	TCR_IRGN0 	getIRGN0() 	{ return details.IRGN0; 			}
	TCR_ORGN0	getORGN0()	{ return details.ORGN0; 			}
	TCR_SH0		getSH0() 	{ return details.SH0;	 			}
	TCR_TG0		getTG0() 	{ return details.TG0; 				}
	uint32_t 	getT1SZ()	{ return uint32_t(details.T1SZ); 	}
	TCR_A1		getA1()		{ return details.A1; 				}
	TCR_EPD1	getEPD1()	{ return details.EPD1; 				}
	TCR_IRGN1	getIRGN1()	{ return details.IRGN1;				}
	TCR_ORGN1	getORGN1()	{ return details.ORGN1;				}
	TCR_SH1		getSH1() 	{ return details.SH1; 				}
	TCR_TG1		getTG1() 	{ return details.TG1;				}
	TCR_IPS		getIPS() 	{ return details.IPS;				}
	TCR_AS		getAS()		{ return details.AS;				}
	TCR_TBI		getTBI0()	{ return details.TBI0;				}
	TCR_TBI		getTBI1()	{ return details.TBI1;				}
};

// MARK: - TCR_EL2
// D7.2.85. TCR_EL2, Translation Control Register (EL2)

using tcr_el2_t	= uint32_t;

template<>
union TCRFormat<ExceptionLevel::EL2>
{
	using tcr_value_t = tcr_el2_t;
	
	struct
	{
		tcr_value_t T0SZ	: 6;	// [5:0]
		tcr_value_t RES0_1	: 2;	// [7:6]
		TCR_IRGN0 	IRGN0	: 2;	// [9:8]
		TCR_ORGN0 	ORGN0	: 2;	// [11:10]
		TCR_SH0		SH0		: 2;	// [13:12]
		TCR_TG0 	TG0		: 2;	// [15:14]
		TCR_PS 		PS		: 3;	// [18:16]
		tcr_value_t RES0_2	: 1;	// [19]
		TCR_TBI 	TBI		: 1;	// [20]
		tcr_value_t RES0_3	: 2;	// [22:21]
		tcr_value_t RES1_1	: 1;	// [23]
		tcr_value_t RES0_4	: 7;	// [30:24]
		tcr_value_t RES1_2	: 1;	// [31]
	} details;
	
	tcr_value_t value;
	
	TCRFormat(tcr_value_t value) : value(value) { }
	
	uint32_t	getT0SZ() 	{ return uint32_t(details.T0SZ);	}
	TCR_IRGN0 	getIRGN0() 	{ return details.IRGN0; 			}
	TCR_ORGN0	getORGN0()	{ return details.ORGN0; 			}
	TCR_SH0		getSH0() 	{ return details.SH0;	 			}
	TCR_TG0		getTG0() 	{ return details.TG0; 				}
	TCR_PS		getPS()		{ return details.PS;				}
	TCR_TBI		getTBI()	{ return details.TBI;				}
};

// MARK: - TCR_EL3
// D7.2.85. TCR_EL3, Translation Control Register (EL3)

using tcr_el3_t	= uint32_t;

template<>
union TCRFormat<ExceptionLevel::EL3>
{
	using tcr_value_t = tcr_el3_t;
	
	struct
	{
		tcr_value_t T0SZ	: 6;	// [5:0]
		tcr_value_t RES0_1	: 2;	// [7:6]
		TCR_IRGN0 	IRGN0	: 2;	// [9:8]
		TCR_ORGN0 	ORGN0	: 2;	// [11:10]
		TCR_SH0		SH0		: 2;	// [13:12]
		TCR_TG0 	TG0		: 2;	// [15:14]
		TCR_PS 		PS		: 3;	// [18:16]
		tcr_value_t RES0_2	: 1;	// [19]
		TCR_TBI 	TBI		: 1;	// [20]
		tcr_value_t RES0_3	: 2;	// [22:21]
		tcr_value_t RES1_1	: 1;	// [23]
		tcr_value_t RES0_4	: 7;	// [30:24]
		tcr_value_t RES1_2	: 1;	// [31]
	} details;
	
	tcr_value_t value;
	
	TCRFormat(tcr_value_t value) : value(value) { }
	
	uint32_t	getT0SZ() 	{ return uint32_t(details.T0SZ);	}
	TCR_IRGN0 	getIRGN0() 	{ return details.IRGN0; 			}
	TCR_ORGN0	getORGN0()	{ return details.ORGN0; 			}
	TCR_SH0		getSH0() 	{ return details.SH0;	 			}
	TCR_TG0		getTG0() 	{ return details.TG0; 				}
	TCR_PS		getPS()		{ return details.PS;				}
	TCR_TBI		getTBI()	{ return details.TBI;				}
};

// MARK: - TCR

template <ExceptionLevel LEVEL>
class TCR
{
public:
	
	using tcr_value_t = typename TCRFormat<LEVEL>::tcr_value_t;
	
	TCR() : m_tcr(0)
	{}
	
	TCR(tcr_value_t value) : m_tcr(value)
	{}
	
	~TCR()
	{};
	
	tcr_value_t getValue()	{ return m_tcr.value;		}
	
	uint32_t	getT0SZ() 	{ return m_tcr.getT0SZ();	}
	TCR_EPD0 	getEPD0() 	{ return m_tcr.getEPD0();	}
	TCR_IRGN0 	getIRGN0() 	{ return m_tcr.getIRGN0();	}
	TCR_ORGN0	getORGN0()	{ return m_tcr.getORGN0();	}
	TCR_SH0		getSH0() 	{ return m_tcr.getSH0(); 	}
	TCR_TG0		getTG0() 	{ return m_tcr.getTG0();	}
	uint32_t 	getT1SZ()	{ return m_tcr.getT1SZ(); 	}
	TCR_A1		getA1()		{ return m_tcr.getA1();		}
	TCR_EPD1	getEPD1()	{ return m_tcr.getEPD1();	}
	TCR_IRGN1	getIRGN1()	{ return m_tcr.getIRGN1();	}
	TCR_ORGN1	getORGN1()	{ return m_tcr.getORGN1();	}
	TCR_SH1		getSH1() 	{ return m_tcr.getSH1();	}
	TCR_TG1		getTG1() 	{ return m_tcr.getTG1();	}
	TCR_IPS		getIPS() 	{ return m_tcr.getIPS();	}
	TCR_AS		getAS()		{ return m_tcr.getAS();		}
	TCR_TBI		getTBI0()	{ return m_tcr.getTBI0();	}
	TCR_TBI		getTBI1()	{ return m_tcr.getTBI1();	}
	
	TCR_PS		getPS()		{ return m_tcr.getPS();		}
	TCR_TBI		getTBI()	{ return m_tcr.getTBI();	}
	
private:
	
	using TCRFormatType = TCRFormat<LEVEL>;
	
	TCRFormatType	m_tcr;
};

// MARK: - TCR types

using TCR_EL1	= TCR<ExceptionLevel::EL1>;
using TCR_EL2	= TCR<ExceptionLevel::EL2>;
using TCR_EL3	= TCR<ExceptionLevel::EL3>;

