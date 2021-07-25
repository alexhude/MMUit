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

enum class APTableAttribute : uint32_t	// Access permissions limit for subsequent levels of lookup
{
	NoEffect					= 0b00,	// No effect on permissions in subsequent levels of lookup.
	NoEL0ReadAccess				= 0b01,	// Access at EL0 not permitted, regardless of permissions in subsequent levels of lookup.
	NoAnyWriteAccess			= 0b10,	// Write access not permitted, at any Exception level, regardless of permissions in subsequent levels of lookup.
	NoEL0ReadAndAnyWriteAccess	= 0b11,	// Write access not permitted, at any Exception level / Read access not permitted at EL0.
};

enum class APAttribute : uint32_t	// Data Access Permissions bits
{
	HigherELReadWriteEL0None		= 0b00,
	HigherELReadWriteEL0ReadWrite	= 0b01,
	HigherELReadOnlyEL0None			= 0b10,
	HigherELReadOnlyEL0ReadOnly		= 0b11,
};

enum class SHAttribute : uint32_t	// Shareability
{
	NonShareable	= 0b00,	// Non-shareable
	Reserved		= 0b01,	// Reserved
	OuterShareable	= 0b10,	// Outer Shareable
	InnerShareable	= 0b11,	// Inner Shareable
};

constexpr ttentry_t MakeEntryAddress(phys_addr_t address, uint64_t bitPos, uint64_t bitLength)
{
	return ((address >> bitPos) & ((ttentry_t(1) << bitLength) - 1));
}

// D4.3 VMSAv8-64 translation table format descriptors (Figure D4-15)

struct TTInvalidDescriptor
{
	ttentry_t validBit	: 1;	// [0] should be 0
	ttentry_t ignored	: 63;	// [63:1]
	
	bool isValid() const	{ return validBit == 1; }
};

struct TTReservedDescriptor
{
	ttentry_t validBit	: 1;	// [0] should be 1
	ttentry_t pageBit	: 1;	// [1] should be 0
	ttentry_t RES0		: 62;	// [63:2]
	
	bool isReserved() const	{ return pageBit == 0; }
};

template<TTGranule GRANULE, TTLevel LEVEL> union TTDescriptorFormat {};

// MARK: - Table Translation Level 0 Descriptors 4K granule
// D4.3 VMSAv8-64 translation table format descriptors (Figure D4-15)

template<>
union TTDescriptorFormat<TTGranule::Granule4K, TTLevel::Level0>
{
	TTInvalidDescriptor invalid;
	
	struct TableDescriptor
	{
		ttentry_t validBit	: 1;	// [0] should be 1
		ttentry_t tableBit	: 1;	// [1] should be 1
		ttentry_t ignored_1	: 10;	// [11:2]
	//	ttentry_t RES0_1	: 0;
		ttentry_t address	: 36;	// [47:12]
		ttentry_t RES0_2	: 4;	// [51:48]
		ttentry_t ignored_2	: 7;	// [58:52]
		ttentry_t PNX		: 1;	// [59]
		ttentry_t XN		: 1;	// [60]
		ttentry_t AP		: 2;	// [62:61]
		ttentry_t NS		: 1;	// [63]
	} table;
	
	ttentry_t value;
	
	TTDescriptorFormat(ttentry_t descriptor) : value(descriptor) { assert(table.tableBit == 1); }
	
	bool isValid()				const	{ return invalid.isValid();	}
	bool isBlockDescriptor()	const	{ return false;				}
	bool isTableDescriptor()	const	{ return true && isValid(); } // entry can only be table (if valid)
	bool isReserved()			const	{ return false;				}
	bool isPageDescriptor()		const	{ return false;				}

	// Table attributes
	
	bool	getPXNTable()			const	{ assert(table.tableBit == 1); return table.PNX == 1;				}
	bool	getXNTable()			const	{ assert(table.tableBit == 1); return table.XN == 1;				}
	APTableAttribute getAPTable()	const	{ assert(table.tableBit == 1); return APTableAttribute(table.AP);	}
	bool	getNSTable()			const	{ assert(table.tableBit == 1); return table.NS == 1;				}

	void setPXNTable(bool value)			{ assert(table.tableBit == 1); table.PNX = (value)? 1 : 0;	}
	void setXNTable(bool value)				{ assert(table.tableBit == 1); table.XN = (value)? 1 : 0;	}
	void setAPTable(APTableAttribute value)	{ assert(table.tableBit == 1); table.AP = ttentry_t(value);	}
	void setNSTable(bool value)				{ assert(table.tableBit == 1); table.NS = (value)? 1 : 0;	}
	
	phys_addr_t getOutputAddress() const
	{
		return ttentry_t(table.address) << 12;
	}
	void		setOutputAddress(phys_addr_t address)
	{
		table.address = MakeEntryAddress(address, 12, 36);
	}
};

// MARK: - Table Translation Level 1 Descriptors 4K granule
// D4.3 VMSAv8-64 translation table format descriptors (Figure D4-15)

template<>
union TTDescriptorFormat<TTGranule::Granule4K, TTLevel::Level1>
{
	TTInvalidDescriptor invalid;
	
	struct BlockDescriptor
	{
		ttentry_t validBit	: 1;	// [0] should be 1
		ttentry_t tableBit	: 1;	// [1] should be 0
		ttentry_t attdIndx	: 3;	// [4:2]
		ttentry_t NS		: 1;	// [5]
		ttentry_t AP		: 2;	// [7:6]
		ttentry_t SH		: 2;	// [9:8]
		ttentry_t AF		: 1;	// [10]
		ttentry_t nG		: 1;	// [1]
		ttentry_t RES0_1	: 18;	// [29:12]
		ttentry_t address	: 18;	// [47:30]
		ttentry_t RES0_2	: 4;	// [51:48]
		ttentry_t contiguous: 1;	// [52]
		ttentry_t PXN		: 1;	// [53]
		ttentry_t XN		: 1;	// [54]
		ttentry_t IGNORED	: 9;	// [63:55]
	} block;

	using TableDescriptor = TTDescriptorFormat<TTGranule::Granule4K, TTLevel::Level0>::TableDescriptor;
	TableDescriptor table; // Identical to Level 0
	
	ttentry_t value;

	// Methods
	
	TTDescriptorFormat(ttentry_t descriptor) : value(descriptor) {}
	
	bool isValid()				const	{ return invalid.isValid();		}
	bool isBlockDescriptor()	const	{ return block.tableBit == 0;	}
	bool isTableDescriptor()	const	{ return table.tableBit == 1;	}
	bool isReserved()			const	{ return false;					}
	bool isPageDescriptor()		const	{ return false;					}
	
	// Table attributes
	
	bool	getPXNTable()			const	{ assert(table.tableBit == 1); return table.PNX == 1;				}
	bool	getXNTable()			const	{ assert(table.tableBit == 1); return table.XN == 1;				}
	APTableAttribute getAPTable()	const	{ assert(table.tableBit == 1); return APTableAttribute(table.AP);	}
	bool	getNSTable()			const	{ assert(table.tableBit == 1); return table.NS == 1;				}
	
	void setPXNTable(bool value)			{ assert(table.tableBit == 1); table.PNX = (value)? 1 : 0;	}
	void setXNTable(bool value)				{ assert(table.tableBit == 1); table.XN = (value)? 1 : 0;	}
	void setAPTable(APTableAttribute value)	{ assert(table.tableBit == 1); table.AP = ttentry_t(value);	}
	void setNSTable(bool value)				{ assert(table.tableBit == 1); table.NS = (value)? 1 : 0;	}
	
	// Block attributes

	uint8_t		getAttrIndx()	const	{ assert(block.tableBit == 0); return block.attdIndx;			}
	bool		getNS()			const	{ assert(block.tableBit == 0); return block.NS == 1;			}
	APAttribute	getAP()			const	{ assert(block.tableBit == 0); return APAttribute(block.AP);	}
	SHAttribute	getSH()			const	{ assert(block.tableBit == 0); return SHAttribute(block.SH);	}
	bool		getAF()			const	{ assert(block.tableBit == 0); return block.AF == 1;			}
	bool		getNG()			const	{ assert(block.tableBit == 0); return block.nG == 1;			}
	bool		getContiguous()	const	{ assert(block.tableBit == 0); return block.contiguous == 1;	}
	bool		getPXN()		const	{ assert(block.tableBit == 0); return block.PXN == 1;			}
	bool		getXN()			const	{ assert(block.tableBit == 0); return block.XN == 1;			}

	void		setAttrIndx(uint8_t value)	{ assert(block.tableBit == 0); block.attdIndx = value;				}
	void		setNS(bool value)			{ assert(block.tableBit == 0); block.NS = (value)? 1 : 0;			}
	void		setAP(APAttribute value)	{ assert(block.tableBit == 0); block.AP = ttentry_t(value);			}
	void		setSH(SHAttribute value)	{ assert(block.tableBit == 0); block.SH = ttentry_t(value);			}
	void		setAF(bool value)			{ assert(block.tableBit == 0); block.AF = (value)? 1 : 0;			}
	void		setNG(bool value)			{ assert(block.tableBit == 0); block.nG = (value)? 1 : 0;			}
	void		setContiguous(bool value)	{ assert(block.tableBit == 0); block.contiguous = (value)? 1 : 0;	}
	void		setPXN(bool value)			{ assert(block.tableBit == 0); block.PXN = (value)? 1 : 0;			}
	void		setXN(bool value)			{ assert(block.tableBit == 0); block.XN = (value)? 1 : 0;			}
	
	phys_addr_t getOutputAddress() const
	{
		return (isTableDescriptor())? ttentry_t(table.address) << 12 : ttentry_t(block.address) << 30;
	}
	void		setOutputAddress(phys_addr_t address)
	{
		if (isTableDescriptor())
			table.address = MakeEntryAddress(address, 12, 36);
		else
			block.address = MakeEntryAddress(address, 30, 18);
	}
};

// MARK: - Table Translation Level 2 Descriptors 4K granule
// D4.3 VMSAv8-64 translation table format descriptors (Figure D4-15)

template<>
union TTDescriptorFormat<TTGranule::Granule4K, TTLevel::Level2>
{
	TTInvalidDescriptor invalid;
	
	struct BlockDescriptor
	{
		ttentry_t validBit	: 1;	// [0] should be 1
		ttentry_t tableBit	: 1;	// [1] should be 0
		ttentry_t attdIndx	: 3;	// [4:2]
		ttentry_t NS		: 1;	// [5]
		ttentry_t AP		: 2;	// [7:6]
		ttentry_t SH		: 2;	// [9:8]
		ttentry_t AF		: 1;	// [10]
		ttentry_t nG		: 1;	// [1]
		ttentry_t RES0_1	: 9;	// [20:12]
		ttentry_t address	: 27;	// [47:21]
		ttentry_t RES0_2	: 4;	// [51:48]
		ttentry_t contiguous: 1;	// [52]
		ttentry_t PXN		: 1;	// [53]
		ttentry_t XN		: 1;	// [54]
		ttentry_t IGNORED	: 9;	// [63:55]
	} block;
	
	using TableDescriptor = TTDescriptorFormat<TTGranule::Granule4K, TTLevel::Level0>::TableDescriptor;
	TableDescriptor table; // Identical to Level 0
	
	ttentry_t value;
	
	// Methods
	
	TTDescriptorFormat(ttentry_t descriptor) : value(descriptor) {}

	bool isValid()				const	{ return invalid.isValid();		}
	bool isBlockDescriptor()	const	{ return block.tableBit == 0;	}
	bool isTableDescriptor()	const	{ return table.tableBit == 1;	}
	bool isReserved()			const	{ return false;					}
	bool isPageDescriptor()		const	{ return false;					}
	
	// Table attributes
	
	bool	getPXNTable()			const	{ assert(table.tableBit == 1); return table.PNX == 1;				}
	bool	getXNTable()			const	{ assert(table.tableBit == 1); return table.XN == 1;				}
	APTableAttribute getAPTable()	const	{ assert(table.tableBit == 1); return APTableAttribute(table.AP);	}
	bool	getNSTable()			const	{ assert(table.tableBit == 1); return table.NS == 1;				}
	
	void setPXNTable(bool value)			{ assert(table.tableBit == 1); table.PNX = (value)? 1 : 0;	}
	void setXNTable(bool value)				{ assert(table.tableBit == 1); table.XN = (value)? 1 : 0;	}
	void setAPTable(APTableAttribute value)	{ assert(table.tableBit == 1); table.AP = ttentry_t(value);	}
	void setNSTable(bool value)				{ assert(table.tableBit == 1); table.NS = (value)? 1 : 0;	}

	// Block attributes
	
	uint8_t		getAttrIndx()	const	{ assert(block.tableBit == 0); return block.attdIndx;			}
	bool		getNS()			const	{ assert(block.tableBit == 0); return block.NS == 1;			}
	APAttribute	getAP()			const	{ assert(block.tableBit == 0); return APAttribute(block.AP);	}
	SHAttribute	getSH()			const	{ assert(block.tableBit == 0); return SHAttribute(block.SH);	}
	bool		getAF()			const	{ assert(block.tableBit == 0); return block.AF == 1;			}
	bool		getNG()			const	{ assert(block.tableBit == 0); return block.nG == 1;			}
	bool		getContiguous()	const	{ assert(block.tableBit == 0); return block.contiguous == 1;	}
	bool		getPXN()		const	{ assert(block.tableBit == 0); return block.PXN == 1;			}
	bool		getXN()			const	{ assert(block.tableBit == 0); return block.XN == 1;			}
	
	void		setAttrIndx(uint8_t value)	{ assert(block.tableBit == 0); block.attdIndx = value;				}
	void		setNS(bool value)			{ assert(block.tableBit == 0); block.NS = (value)? 1 : 0;			}
	void		setAP(APAttribute value)	{ assert(block.tableBit == 0); block.AP = ttentry_t(value);			}
	void		setSH(SHAttribute value)	{ assert(block.tableBit == 0); block.SH = ttentry_t(value);			}
	void		setAF(bool value)			{ assert(block.tableBit == 0); block.AF = (value)? 1 : 0;			}
	void		setNG(bool value)			{ assert(block.tableBit == 0); block.nG = (value)? 1 : 0;			}
	void		setContiguous(bool value)	{ assert(block.tableBit == 0); block.contiguous = (value)? 1 : 0;	}
	void		setPXN(bool value)			{ assert(block.tableBit == 0); block.PXN = (value)? 1 : 0;			}
	void		setXN(bool value)			{ assert(block.tableBit == 0); block.XN = (value)? 1 : 0;			}
	
	phys_addr_t getOutputAddress() const
	{
		return (isTableDescriptor())? ttentry_t(table.address) << 12 : ttentry_t(block.address) << 21;
	}
	void		setOutputAddress(phys_addr_t address)
	{
		if (isTableDescriptor())
			table.address = MakeEntryAddress(address, 12, 36);
		else
			block.address = MakeEntryAddress(address, 21, 27);
	}
};

// MARK: - Table Translation Level 3 Descriptors 4K granule
// D4.3 VMSAv8-64 translation table format descriptors (Figure D4-16)

template<>
union TTDescriptorFormat<TTGranule::Granule4K, TTLevel::Level3>
{
	TTInvalidDescriptor invalid;
	
	TTReservedDescriptor reserved;
	
	struct PageDescriptor
	{
		ttentry_t validBit	: 1;	// [0] should be 1
		ttentry_t pageBit	: 1;	// [1] should be 1
		ttentry_t attdIndx	: 3;	// [4:2]
		ttentry_t NS		: 1;	// [5]
		ttentry_t AP		: 2;	// [7:6]
		ttentry_t SH		: 2;	// [9:8]
		ttentry_t AF		: 1;	// [10]
		ttentry_t nG		: 1;	// [1]
	//	ttentry_t RES0_1	: 0;
		ttentry_t address	: 36;	// [47:12]
		ttentry_t RES0_2	: 4;	// [51:48]
		ttentry_t contiguous: 1;	// [52]
		ttentry_t PXN		: 1;	// [53]
		ttentry_t XN		: 1;	// [54]
		ttentry_t IGNORED	: 9;	// [63:55]
	} page;
	
	ttentry_t value;
	
	// Methods
	
	TTDescriptorFormat(ttentry_t descriptor) : value(descriptor) { assert(page.pageBit == 1); }
	
	bool isValid()				const	{ return invalid.isValid();		}
	bool isBlockDescriptor()	const	{ return false;					}
	bool isTableDescriptor()	const	{ return false;					}
	bool isReserved()			const	{ return reserved.isReserved(); }
	bool isPageDescriptor()		const	{ return true && isValid();		} // entry can only be page (if valid)
	
	// Page attributes
	
	uint8_t		getAttrIndx()	const	{ assert(page.pageBit == 1); return page.attdIndx;			}
	bool		getNS()			const	{ assert(page.pageBit == 1); return page.NS == 1;			}
	APAttribute	getAP()			const	{ assert(page.pageBit == 1); return APAttribute(page.AP);	}
	SHAttribute	getSH()			const	{ assert(page.pageBit == 1); return SHAttribute(page.SH);	}
	bool		getAF()			const	{ assert(page.pageBit == 1); return page.AF == 1;			}
	bool		getNG()			const	{ assert(page.pageBit == 1); return page.nG == 1;			}
	bool		getContiguous()	const	{ assert(page.pageBit == 1); return page.contiguous == 1;	}
	bool		getPXN()		const	{ assert(page.pageBit == 1); return page.PXN == 1;			}
	bool		getXN()			const	{ assert(page.pageBit == 1); return page.XN == 1;			}
	
	void		setAttrIndx(uint8_t value)	{ assert(page.pageBit == 1); page.attdIndx = value;				}
	void		setNS(bool value)			{ assert(page.pageBit == 1); page.NS = (value)? 1 : 0;			}
	void		setAP(APAttribute value)	{ assert(page.pageBit == 1); page.AP = ttentry_t(value);		}
	void		setSH(SHAttribute value)	{ assert(page.pageBit == 1); page.SH = ttentry_t(value);		}
	void		setAF(bool value)			{ assert(page.pageBit == 1); page.AF = (value)? 1 : 0;			}
	void		setNG(bool value)			{ assert(page.pageBit == 1); page.nG = (value)? 1 : 0;			}
	void		setContiguous(bool value)	{ assert(page.pageBit == 1); page.contiguous = (value)? 1 : 0;	}
	void		setPXN(bool value)			{ assert(page.pageBit == 1); page.PXN = (value)? 1 : 0;			}
	void		setXN(bool value)			{ assert(page.pageBit == 1); page.XN = (value)? 1 : 0;			}
	
	phys_addr_t getOutputAddress() const
	{
		return page.address << 12;
	}
	void		setOutputAddress(phys_addr_t address)
	{
		page.address = MakeEntryAddress(address, 12, 36);
	}
};

// MARK: - Table Translation Level 0 Descriptors 16K granule
// D4.3 VMSAv8-64 translation table format descriptors (Figure D4-15)

template<>
union TTDescriptorFormat<TTGranule::Granule16K, TTLevel::Level0>
{
	TTInvalidDescriptor invalid;
	
	struct TableDescriptor
	{
		ttentry_t validBit	: 1;	// [0] should be 1
		ttentry_t tableBit	: 1;	// [1] should be 1
		ttentry_t ignored_1	: 10;	// [11:2]
		ttentry_t RES0_1	: 2;	// [13:12]
		ttentry_t address	: 34;	// [47:14]
		ttentry_t RES0_2	: 4;	// [51:48]
		ttentry_t ignored_2	: 7;	// [58:52]
		ttentry_t PNX		: 1;	// [59]
		ttentry_t XN		: 1;	// [60]
		ttentry_t AP		: 2;	// [62:61]
		ttentry_t NS		: 1;	// [63]
	} table;
	
	ttentry_t value;
	
	// Methods
	
	TTDescriptorFormat(ttentry_t descriptor) : value(descriptor) { assert(table.tableBit == 1); }
	
	bool isValid()				const	{ return invalid.isValid();	}
	bool isBlockDescriptor()	const	{ return false;				}
	bool isTableDescriptor()	const	{ return true && isValid(); } // entry can only be table (if valid)
	bool isReserved()			const	{ return false;				}
	bool isPageDescriptor()		const	{ return false;				}
	
	// Table attributes
	
	bool	getPXNTable()			const	{ assert(table.tableBit == 1); return table.PNX == 1;				}
	bool	getXNTable()			const	{ assert(table.tableBit == 1); return table.XN == 1;				}
	APTableAttribute getAPTable()	const	{ assert(table.tableBit == 1); return APTableAttribute(table.AP);	}
	bool	getNSTable()			const	{ assert(table.tableBit == 1); return table.NS == 1;				}
	
	void setPXNTable(bool value)			{ assert(table.tableBit == 1); table.PNX = (value)? 1 : 0;	}
	void setXNTable(bool value)				{ assert(table.tableBit == 1); table.XN = (value)? 1 : 0;	}
	void setAPTable(APTableAttribute value)	{ assert(table.tableBit == 1); table.AP = ttentry_t(value);	}
	void setNSTable(bool value)				{ assert(table.tableBit == 1); table.NS = (value)? 1 : 0;	}
	
	phys_addr_t getOutputAddress() const
	{
		return ttentry_t(table.address) << 14;
	}
	void		setOutputAddress(phys_addr_t address)
	{
		table.address = MakeEntryAddress(address, 14, 34);
	}
};

// MARK: - Table Translation Level 1 Descriptors 16K granule
// D4.3 VMSAv8-64 translation table format descriptors (Figure D4-15)

template<>
union TTDescriptorFormat<TTGranule::Granule16K, TTLevel::Level1>
{
	TTInvalidDescriptor invalid;
	
	using TableDescriptor = TTDescriptorFormat<TTGranule::Granule16K, TTLevel::Level0>::TableDescriptor;
	TableDescriptor table; // Identical to Level 0
	
	ttentry_t value;
	
	// Methods
	
	TTDescriptorFormat(ttentry_t descriptor) : value(descriptor) { assert(table.tableBit == 1); }
	
	bool isValid()				const	{ return invalid.isValid();		}
	bool isBlockDescriptor()	const	{ return table.tableBit == 0;	}
	bool isTableDescriptor()	const	{ return table.tableBit == 1;	}
	bool isReserved()			const	{ return false;					}
	bool isPageDescriptor()		const	{ return false;					}
	
	// Table attributes
	
	bool	getPXNTable()			const	{ assert(table.tableBit == 1); return table.PNX == 1;				}
	bool	getXNTable()			const	{ assert(table.tableBit == 1); return table.XN == 1;				}
	APTableAttribute getAPTable()	const	{ assert(table.tableBit == 1); return APTableAttribute(table.AP);	}
	bool	getNSTable()			const	{ assert(table.tableBit == 1); return table.NS == 1;				}
	
	void setPXNTable(bool value)			{ assert(table.tableBit == 1); table.PNX = (value)? 1 : 0;	}
	void setXNTable(bool value)				{ assert(table.tableBit == 1); table.XN = (value)? 1 : 0;	}
	void setAPTable(APTableAttribute value)	{ assert(table.tableBit == 1); table.AP = ttentry_t(value);	}
	void setNSTable(bool value)				{ assert(table.tableBit == 1); table.NS = (value)? 1 : 0;	}
	
	phys_addr_t getOutputAddress() const
	{
		return ttentry_t(table.address) << 14;
	}
	void		setOutputAddress(phys_addr_t address)
	{
		table.address = MakeEntryAddress(address, 14, 34);
	}
};

// MARK: - Table Translation Level 2 Descriptors 16K granule
// D4.3 VMSAv8-64 translation table format descriptors (Figure D4-15)

template<>
union TTDescriptorFormat<TTGranule::Granule16K, TTLevel::Level2>
{
	TTInvalidDescriptor invalid;
	
	struct BlockDescriptor
	{
		ttentry_t validBit	: 1;	// [0] should be 1
		ttentry_t tableBit	: 1;	// [1] should be 0
		ttentry_t attdIndx	: 3;	// [4:2]
		ttentry_t NS		: 1;	// [5]
		ttentry_t AP		: 2;	// [7:6]
		ttentry_t SH		: 2;	// [9:8]
		ttentry_t AF		: 1;	// [10]
		ttentry_t nG		: 1;	// [1]
		ttentry_t RES0_1	: 13;	// [24:12]
		ttentry_t address	: 23;	// [47:25]
		ttentry_t RES0_2	: 4;	// [51:48]
		ttentry_t contiguous: 1;	// [52]
		ttentry_t PXN		: 1;	// [53]
		ttentry_t XN		: 1;	// [54]
		ttentry_t IGNORED	: 9;	// [63:55]
	} block;
	
	using TableDescriptor = TTDescriptorFormat<TTGranule::Granule16K, TTLevel::Level0>::TableDescriptor;
	TableDescriptor table; // Identical to Level 0
	
	ttentry_t value;
	
	// Methods
	
	TTDescriptorFormat(ttentry_t descriptor) : value(descriptor) {}
	
	bool isValid()				const	{ return invalid.isValid();		}
	bool isBlockDescriptor()	const	{ return block.tableBit == 0;	}
	bool isTableDescriptor()	const	{ return table.tableBit == 1;	}
	bool isReserved()			const	{ return false;					}
	bool isPageDescriptor()		const	{ return false;					}
	
	// Table attributes
	
	bool	getPXNTable()			const	{ assert(table.tableBit == 1); return table.PNX == 1;				}
	bool	getXNTable()			const	{ assert(table.tableBit == 1); return table.XN == 1;				}
	APTableAttribute getAPTable()	const	{ assert(table.tableBit == 1); return APTableAttribute(table.AP);	}
	bool	getNSTable()			const	{ assert(table.tableBit == 1); return table.NS == 1;				}
	
	void setPXNTable(bool value)			{ assert(table.tableBit == 1); table.PNX = (value)? 1 : 0;	}
	void setXNTable(bool value)				{ assert(table.tableBit == 1); table.XN = (value)? 1 : 0;	}
	void setAPTable(APTableAttribute value)	{ assert(table.tableBit == 1); table.AP = ttentry_t(value);	}
	void setNSTable(bool value)				{ assert(table.tableBit == 1); table.NS = (value)? 1 : 0;	}
	
	// Block attributes
	
	uint8_t		getAttrIndx()	const	{ assert(block.tableBit == 0); return block.attdIndx;			}
	bool		getNS()			const	{ assert(block.tableBit == 0); return block.NS == 1;			}
	APAttribute	getAP()			const	{ assert(block.tableBit == 0); return APAttribute(block.AP);	}
	SHAttribute	getSH()			const	{ assert(block.tableBit == 0); return SHAttribute(block.SH);	}
	bool		getAF()			const	{ assert(block.tableBit == 0); return block.AF == 1;			}
	bool		getNG()			const	{ assert(block.tableBit == 0); return block.nG == 1;			}
	bool		getContiguous()	const	{ assert(block.tableBit == 0); return block.contiguous == 1;	}
	bool		getPXN()		const	{ assert(block.tableBit == 0); return block.PXN == 1;			}
	bool		getXN()			const	{ assert(block.tableBit == 0); return block.XN == 1;			}
	
	void		setAttrIndx(uint8_t value)	{ assert(block.tableBit == 0); block.attdIndx = value;				}
	void		setNS(bool value)			{ assert(block.tableBit == 0); block.NS = (value)? 1 : 0;			}
	void		setAP(APAttribute value)	{ assert(block.tableBit == 0); block.AP = ttentry_t(value);			}
	void		setSH(SHAttribute value)	{ assert(block.tableBit == 0); block.SH = ttentry_t(value);			}
	void		setAF(bool value)			{ assert(block.tableBit == 0); block.AF = (value)? 1 : 0;			}
	void		setNG(bool value)			{ assert(block.tableBit == 0); block.nG = (value)? 1 : 0;			}
	void		setContiguous(bool value)	{ assert(block.tableBit == 0); block.contiguous = (value)? 1 : 0;	}
	void		setPXN(bool value)			{ assert(block.tableBit == 0); block.PXN = (value)? 1 : 0;			}
	void		setXN(bool value)			{ assert(block.tableBit == 0); block.XN = (value)? 1 : 0;			}
	
	phys_addr_t getOutputAddress() const
	{
		return (isTableDescriptor())? ttentry_t(table.address) << 14 : ttentry_t(block.address) << 25;
	}
	void		setOutputAddress(phys_addr_t address)
	{
		if (isTableDescriptor())
			table.address = MakeEntryAddress(address, 14, 34);
		else
			block.address = MakeEntryAddress(address, 25, 23);
	}
};

// MARK: - Table Translation Level 3 Descriptors 16K granule
// D4.3 VMSAv8-64 translation table format descriptors (Figure D4-16)

template<>
union TTDescriptorFormat<TTGranule::Granule16K, TTLevel::Level3>
{
	TTInvalidDescriptor invalid;
	
	TTReservedDescriptor reserved;
	
	struct PageDescriptor
	{
		ttentry_t validBit	: 1;	// [0] should be 1
		ttentry_t pageBit	: 1;	// [1] should be 0
		ttentry_t attdIndx	: 3;	// [4:2]
		ttentry_t NS		: 1;	// [5]
		ttentry_t AP		: 2;	// [7:6]
		ttentry_t SH		: 2;	// [9:8]
		ttentry_t AF		: 1;	// [10]
		ttentry_t nG		: 1;	// [1]
		ttentry_t RES0_1	: 2;	// [13:12]
		ttentry_t address	: 34;	// [47:14]
		ttentry_t RES0_2	: 4;	// [51:48]
		ttentry_t contiguous: 1;	// [52]
		ttentry_t PXN		: 1;	// [53]
		ttentry_t XN		: 1;	// [54]
		ttentry_t IGNORED	: 9;	// [63:55]
	} page;
	
	ttentry_t value;
	
	// Methods
	
	TTDescriptorFormat(ttentry_t descriptor) : value(descriptor) { assert(page.pageBit == 1); }
	
	bool isValid()				const	{ return invalid.isValid();		}
	bool isBlockDescriptor()	const	{ return false;					}
	bool isTableDescriptor()	const	{ return false;					}
	bool isReserved()			const	{ return reserved.isReserved(); }
	bool isPageDescriptor()		const	{ return true && isValid();		} // entry can only be page (if valid)

	// Page attributes
	
	uint8_t		getAttrIndx()	const	{ assert(page.pageBit == 1); return page.attdIndx;			}
	bool		getNS()			const	{ assert(page.pageBit == 1); return page.NS == 1;			}
	APAttribute	getAP()			const	{ assert(page.pageBit == 1); return APAttribute(page.AP);	}
	SHAttribute	getSH()			const	{ assert(page.pageBit == 1); return SHAttribute(page.SH);	}
	bool		getAF()			const	{ assert(page.pageBit == 1); return page.AF == 1;			}
	bool		getNG()			const	{ assert(page.pageBit == 1); return page.nG == 1;			}
	bool		getContiguous()	const	{ assert(page.pageBit == 1); return page.contiguous == 1;	}
	bool		getPXN()		const	{ assert(page.pageBit == 1); return page.PXN == 1;			}
	bool		getXN()			const	{ assert(page.pageBit == 1); return page.XN == 1;			}
	
	void		setAttrIndx(uint8_t value)	{ assert(page.pageBit == 1); page.attdIndx = value;				}
	void		setNS(bool value)			{ assert(page.pageBit == 1); page.NS = (value)? 1 : 0;			}
	void		setAP(APAttribute value)	{ assert(page.pageBit == 1); page.AP = ttentry_t(value);		}
	void		setSH(SHAttribute value)	{ assert(page.pageBit == 1); page.SH = ttentry_t(value);		}
	void		setAF(bool value)			{ assert(page.pageBit == 1); page.AF = (value)? 1 : 0;			}
	void		setNG(bool value)			{ assert(page.pageBit == 1); page.nG = (value)? 1 : 0;			}
	void		setContiguous(bool value)	{ assert(page.pageBit == 1); page.contiguous = (value)? 1 : 0;	}
	void		setPXN(bool value)			{ assert(page.pageBit == 1); page.PXN = (value)? 1 : 0;			}
	void		setXN(bool value)			{ assert(page.pageBit == 1); page.XN = (value)? 1 : 0;			}
	
	phys_addr_t getOutputAddress() const
	{
		return page.address << 14;
	}
	void		setOutputAddress(phys_addr_t address)
	{
		page.address = MakeEntryAddress(address, 14, 34);
	}
};

// MARK: - Table Translation Level 0 Descriptors 64K granule
// D4.3 VMSAv8-64 translation table format descriptors (Figure D4-15)

template<>
union TTDescriptorFormat<TTGranule::Granule64K, TTLevel::Level0>
{
	TTInvalidDescriptor invalid;
	
	struct TableDescriptor
	{
		ttentry_t validBit	: 1;	// [0] should be 1
		ttentry_t tableBit	: 1;	// [1] should be 1
		ttentry_t ignored_1	: 10;	// [11:2]
		ttentry_t RES0_1	: 4;	// [15:12]
		ttentry_t address	: 32;	// [47:16]
		ttentry_t RES0_2	: 4;	// [51:48]
		ttentry_t ignored_2	: 7;	// [58:52]
		ttentry_t PNX		: 1;	// [59]
		ttentry_t XN		: 1;	// [60]
		ttentry_t AP		: 2;	// [62:61]
		ttentry_t NS		: 1;	// [63]
	} table;
	
	ttentry_t value;
	
	// Methods
	
	TTDescriptorFormat(ttentry_t descriptor) : value(descriptor) { assert(table.tableBit == 1); }
	
	bool isValid()				const	{ return invalid.isValid(); }
	bool isBlockDescriptor()	const	{ return false;				}
	bool isTableDescriptor()	const	{ return true && isValid(); } // entry can only be table (if valid)
	bool isReserved()			const	{ return false;				}
	bool isPageDescriptor()		const	{ return false;				}

	// Table attributes
	
	bool	getPXNTable()			const	{ assert(table.tableBit == 1); return table.PNX == 1;				}
	bool	getXNTable()			const	{ assert(table.tableBit == 1); return table.XN == 1;				}
	APTableAttribute getAPTable()	const	{ assert(table.tableBit == 1); return APTableAttribute(table.AP);	}
	bool	getNSTable()			const	{ assert(table.tableBit == 1); return table.NS == 1;				}
	
	void setPXNTable(bool value)			{ assert(table.tableBit == 1); table.PNX = (value)? 1 : 0;	}
	void setXNTable(bool value)				{ assert(table.tableBit == 1); table.XN = (value)? 1 : 0;	}
	void setAPTable(APTableAttribute value)	{ assert(table.tableBit == 1); table.AP = ttentry_t(value);	}
	void setNSTable(bool value)				{ assert(table.tableBit == 1); table.NS = (value)? 1 : 0;	}
	
	phys_addr_t getOutputAddress() const
	{
		return ttentry_t(table.address) << 16;
	}
	void		setOutputAddress(phys_addr_t address)
	{
		table.address = MakeEntryAddress(address, 16, 32);
	}
};

// MARK: - Table Translation Level 1 Descriptors 64K granule
// D4.3 VMSAv8-64 translation table format descriptors (Figure D4-15)

template<>
union TTDescriptorFormat<TTGranule::Granule64K, TTLevel::Level1>
{
	TTInvalidDescriptor invalid;
	
	using TableDescriptor = TTDescriptorFormat<TTGranule::Granule64K, TTLevel::Level0>::TableDescriptor;
	TableDescriptor table; // Identical to Level 0
	
	ttentry_t value;
	
	// Methods
	
	TTDescriptorFormat(ttentry_t descriptor) : value(descriptor) { assert(table.tableBit == 1); }
	
	bool isValid()				const	{ return invalid.isValid();		}
	bool isBlockDescriptor()	const	{ return table.tableBit == 0;	}
	bool isTableDescriptor()	const	{ return table.tableBit == 1;	}
	bool isReserved()			const	{ return false;					}
	bool isPageDescriptor()		const	{ return false;					}
	
	// Table attributes
	
	bool	getPXNTable()			const	{ assert(table.tableBit == 1); return table.PNX == 1;				}
	bool	getXNTable()			const	{ assert(table.tableBit == 1); return table.XN == 1;				}
	APTableAttribute getAPTable()	const	{ assert(table.tableBit == 1); return APTableAttribute(table.AP);	}
	bool	getNSTable()			const	{ assert(table.tableBit == 1); return table.NS == 1;				}
	
	void setPXNTable(bool value)			{ assert(table.tableBit == 1); table.PNX = (value)? 1 : 0;	}
	void setXNTable(bool value)				{ assert(table.tableBit == 1); table.XN = (value)? 1 : 0;	}
	void setAPTable(APTableAttribute value)	{ assert(table.tableBit == 1); table.AP = ttentry_t(value);	}
	void setNSTable(bool value)				{ assert(table.tableBit == 1); table.NS = (value)? 1 : 0;	}
	
	phys_addr_t getOutputAddress() const
	{
		return ttentry_t(table.address) << 16;
	}
	void		setOutputAddress(phys_addr_t address)
	{
		table.address = MakeEntryAddress(address, 16, 32);
	}
};

// MARK: - Table Translation Level 2 Descriptors 64K granule
// D4.3 VMSAv8-64 translation table format descriptors (Figure D4-15)

template<>
union TTDescriptorFormat<TTGranule::Granule64K, TTLevel::Level2>
{
	TTInvalidDescriptor invalid;
	
	struct BlockDescriptor
	{
		ttentry_t validBit	: 1;	// [0] should be 1
		ttentry_t tableBit	: 1;	// [1] should be 0
		ttentry_t attdIndx	: 3;	// [4:2]
		ttentry_t NS		: 1;	// [5]
		ttentry_t AP		: 2;	// [7:6]
		ttentry_t SH		: 2;	// [9:8]
		ttentry_t AF		: 1;	// [10]
		ttentry_t nG		: 1;	// [1]
		ttentry_t RES0_1	: 17;	// [28:12]
		ttentry_t address	: 19;	// [47:29]
		ttentry_t RES0_2	: 4;	// [51:48]
		ttentry_t contiguous: 1;	// [52]
		ttentry_t PXN		: 1;	// [53]
		ttentry_t XN		: 1;	// [54]
		ttentry_t IGNORED	: 9;	// [63:55]
	} block;
	
	// Identical to Level 0
	using TableDescriptor = TTDescriptorFormat<TTGranule::Granule64K, TTLevel::Level0>::TableDescriptor;
	TableDescriptor table;
	
	ttentry_t value;
	
	// Methods
	
	TTDescriptorFormat(ttentry_t descriptor) : value(descriptor) {}
	
	bool isValid()				const	{ return invalid.isValid();		}
	bool isBlockDescriptor()	const	{ return block.tableBit == 0;	}
	bool isTableDescriptor()	const	{ return table.tableBit == 1;	}
	bool isReserved()			const	{ return false;					}
	bool isPageDescriptor()		const	{ return false;					}
	
	// Table attributes
	
	bool	getPXNTable()			const	{ assert(table.tableBit == 1); return table.PNX == 1;				}
	bool	getXNTable()			const	{ assert(table.tableBit == 1); return table.XN == 1;				}
	APTableAttribute getAPTable()	const	{ assert(table.tableBit == 1); return APTableAttribute(table.AP);	}
	bool	getNSTable()			const	{ assert(table.tableBit == 1); return table.NS == 1;				}
	
	void setPXNTable(bool value)			{ assert(table.tableBit == 1); table.PNX = (value)? 1 : 0;	}
	void setXNTable(bool value)				{ assert(table.tableBit == 1); table.XN = (value)? 1 : 0;	}
	void setAPTable(APTableAttribute value)	{ assert(table.tableBit == 1); table.AP = ttentry_t(value);	}
	void setNSTable(bool value)				{ assert(table.tableBit == 1); table.NS = (value)? 1 : 0;	}
	
	// Block attributes
	
	uint8_t		getAttrIndx()	const	{ assert(block.tableBit == 0); return block.attdIndx;			}
	bool		getNS()			const	{ assert(block.tableBit == 0); return block.NS == 1;			}
	APAttribute	getAP()			const	{ assert(block.tableBit == 0); return APAttribute(block.AP);	}
	SHAttribute	getSH()			const	{ assert(block.tableBit == 0); return SHAttribute(block.SH);	}
	bool		getAF()			const	{ assert(block.tableBit == 0); return block.AF == 1;			}
	bool		getNG()			const	{ assert(block.tableBit == 0); return block.nG == 1;			}
	bool		getContiguous()	const	{ assert(block.tableBit == 0); return block.contiguous == 1;	}
	bool		getPXN()		const	{ assert(block.tableBit == 0); return block.PXN == 1;			}
	bool		getXN()			const	{ assert(block.tableBit == 0); return block.XN == 1;			}
	
	void		setAttrIndx(uint8_t value)	{ assert(block.tableBit == 0); block.attdIndx = value;				}
	void		setNS(bool value)			{ assert(block.tableBit == 0); block.NS = (value)? 1 : 0;			}
	void		setAP(APAttribute value)	{ assert(block.tableBit == 0); block.AP = ttentry_t(value);			}
	void		setSH(SHAttribute value)	{ assert(block.tableBit == 0); block.SH = ttentry_t(value);			}
	void		setAF(bool value)			{ assert(block.tableBit == 0); block.AF = (value)? 1 : 0;			}
	void		setNG(bool value)			{ assert(block.tableBit == 0); block.nG = (value)? 1 : 0;			}
	void		setContiguous(bool value)	{ assert(block.tableBit == 0); block.contiguous = (value)? 1 : 0;	}
	void		setPXN(bool value)			{ assert(block.tableBit == 0); block.PXN = (value)? 1 : 0;			}
	void		setXN(bool value)			{ assert(block.tableBit == 0); block.XN = (value)? 1 : 0;			}
	
	phys_addr_t getOutputAddress() const
	{
		return (isTableDescriptor())? ttentry_t(table.address) << 16 : ttentry_t(block.address) << 29;
	}
	void		setOutputAddress(phys_addr_t address)
	{
		if (isTableDescriptor())
			table.address = MakeEntryAddress(address, 16, 32);
		else
			block.address = MakeEntryAddress(address, 29, 19);
	}
};

// MARK: - Table Translation Level 3 Descriptors 16K granule
// D4.3 VMSAv8-64 translation table format descriptors (Figure D4-16)

template<>
union TTDescriptorFormat<TTGranule::Granule64K, TTLevel::Level3>
{
	TTInvalidDescriptor invalid;
	
	TTReservedDescriptor reserved;
	
	struct PageDescriptor
	{
		ttentry_t validBit	: 1;	// [0] should be 1
		ttentry_t pageBit	: 1;	// [1] should be 1
		ttentry_t attdIndx	: 3;	// [4:2]
		ttentry_t NS		: 1;	// [5]
		ttentry_t AP		: 2;	// [7:6]
		ttentry_t SH		: 2;	// [9:8]
		ttentry_t AF		: 1;	// [10]
		ttentry_t nG		: 1;	// [1]
		ttentry_t RES0_1	: 4;	// [15:12]
		ttentry_t address	: 32;	// [47:16]
		ttentry_t RES0_2	: 4;	// [51:48]
		ttentry_t contiguous: 1;	// [52]
		ttentry_t PXN		: 1;	// [53]
		ttentry_t XN		: 1;	// [54]
		ttentry_t IGNORED	: 9;	// [63:55]
	} page;
	
	ttentry_t value;
	
	// Methods
	
	TTDescriptorFormat(ttentry_t descriptor) : value(descriptor) { assert(page.pageBit == 1); }
	
	bool isValid()				const	{ return invalid.isValid();		}
	bool isBlockDescriptor()	const	{ return false;					}
	bool isTableDescriptor()	const	{ return false;					}
	bool isReserved()			const	{ return reserved.isReserved(); }
	bool isPageDescriptor()		const	{ return true && isValid();		} // entry can only be page (if valid)
	
	// Page attributes
	
	uint8_t		getAttrIndx()	const	{ assert(page.pageBit == 1); return page.attdIndx;			}
	bool		getNS()			const	{ assert(page.pageBit == 1); return page.NS == 1;			}
	APAttribute	getAP()			const	{ assert(page.pageBit == 1); return APAttribute(page.AP);	}
	SHAttribute	getSH()			const	{ assert(page.pageBit == 1); return SHAttribute(page.SH);	}
	bool		getAF()			const	{ assert(page.pageBit == 1); return page.AF == 1;			}
	bool		getNG()			const	{ assert(page.pageBit == 1); return page.nG == 1;			}
	bool		getContiguous()	const	{ assert(page.pageBit == 1); return page.contiguous == 1;	}
	bool		getPXN()		const	{ assert(page.pageBit == 1); return page.PXN == 1;			}
	bool		getXN()			const	{ assert(page.pageBit == 1); return page.XN == 1;			}
	
	void		setAttrIndx(uint8_t value)	{ assert(page.pageBit == 1); page.attdIndx = value;				}
	void		setNS(bool value)			{ assert(page.pageBit == 1); page.NS = (value)? 1 : 0;			}
	void		setAP(APAttribute value)	{ assert(page.pageBit == 1); page.AP = ttentry_t(value);		}
	void		setSH(SHAttribute value)	{ assert(page.pageBit == 1); page.SH = ttentry_t(value);		}
	void		setAF(bool value)			{ assert(page.pageBit == 1); page.AF = (value)? 1 : 0;			}
	void		setNG(bool value)			{ assert(page.pageBit == 1); page.nG = (value)? 1 : 0;			}
	void		setContiguous(bool value)	{ assert(page.pageBit == 1); page.contiguous = (value)? 1 : 0;	}
	void		setPXN(bool value)			{ assert(page.pageBit == 1); page.PXN = (value)? 1 : 0;			}
	void		setXN(bool value)			{ assert(page.pageBit == 1); page.XN = (value)? 1 : 0;			}
	
	phys_addr_t getOutputAddress() const
	{
		return page.address << 16;
	}
	void		setOutputAddress(phys_addr_t address)
	{
		page.address = MakeEntryAddress(address, 16, 32);
	}
};

// MARK: - Table Translation Entry

class TTGenericEntry
{
public:
	TTGenericEntry(TTGranule granule, TTLevel level)
		: m_granule(granule), m_level(level)
	{}
	
	TTGranule			getGranule() { return m_granule; }
	TTLevel				getLevel() { return m_level; }
	
public:
	
	virtual ~TTGenericEntry() {};
	
	virtual TTGenericEntry* clone() = 0;
	
	virtual bool		isValid()			const	= 0;
	virtual bool		isBlockDescriptor()	const	= 0;
	virtual bool		isTableDescriptor()	const	= 0;
	virtual bool		isReserved()		const	= 0;
	virtual bool		isPageDescriptor()	const	= 0;
	
	virtual ttentry_t	getDescriptor()		const	= 0;
	virtual void		setDescriptor(ttentry_t descriptor)		= 0;
	
	virtual phys_addr_t	getOutputAddress()	const	= 0;
	virtual void		setOutputAddress(phys_addr_t address)	= 0;

private:
	const TTGranule		m_granule;
	const TTLevel		m_level;
};

template <TTGranule GRANULE, TTLevel LEVEL>
class TTEntry : public TTGenericEntry
{
public:

	TTEntry()
		: TTGenericEntry(GRANULE, LEVEL), m_descriptor(0)
	{}

	TTEntry(ttentry_t descriptor)
		: TTGenericEntry(GRANULE, LEVEL), m_descriptor(descriptor)
	{}
	
	~TTEntry() override
	{};
	
public: // TTGenericEntry
	
	TTGenericEntry* clone() override
	{
		return new TTEntry<GRANULE, LEVEL>(m_descriptor.value);
	}
	
	bool isValid()				const override { return m_descriptor.isValid();				}
	bool isBlockDescriptor()	const override { return m_descriptor.isBlockDescriptor();	}
	bool isTableDescriptor()	const override { return m_descriptor.isTableDescriptor();	}
	bool isReserved()			const override { return m_descriptor.isReserved();			}
	bool isPageDescriptor()		const override { return m_descriptor.isPageDescriptor();	}
	
	ttentry_t getDescriptor() const	override
	{
		return m_descriptor.value;
	}
	void	  setDescriptor(ttentry_t descriptor) override
	{
		m_descriptor.value = descriptor;
	}
	
	phys_addr_t getOutputAddress() const override
	{
		if (! m_descriptor.isValid())
			return kInvalidAddress;
		
		return m_descriptor.getOutputAddress();
	}
	void setOutputAddress(phys_addr_t address) override
	{
		m_descriptor.setOutputAddress(address);
	}
	
public: // Format specific
	
	// Table attributes
	
	bool				getPXNTable()	const	{ return m_descriptor.getPXNTable();	}
	bool				getXNTable()	const	{ return m_descriptor.getXNTable();		}
	APTableAttribute	getAPTable()	const	{ return m_descriptor.getAPTable();		}
	bool				getNSTable()	const	{ return m_descriptor.getNSTable();		}
	
	void setPXNTable(bool value)				{ m_descriptor.setPXNTable(value);	}
	void setXNTable(bool value)					{ m_descriptor.setXNTable(value);	}
	void setAPTable(APTableAttribute value)		{ m_descriptor.setAPTable(value);	}
	void setNSTable(bool value)					{ m_descriptor.setNSTable(value);	}

	// Block and Page attributes
	
	uint8_t		getAttrIndx()	const	{ return m_descriptor.getAttrIndx();	}
	bool		getNS()			const	{ return m_descriptor.getNS();			}
	APAttribute	getAP()			const	{ return m_descriptor.getAP();			}
	SHAttribute	getSH()			const	{ return m_descriptor.getSH();			}
	bool		getAF()			const	{ return m_descriptor.getAF();			}
	bool		getNG()			const	{ return m_descriptor.getNG();			}
	bool		getContiguous()	const	{ return m_descriptor.getContinuous();	}
	bool		getPXN()		const	{ return m_descriptor.getPXN();			}
	bool		getXN()			const	{ return m_descriptor.getXN();			}
	
	void		setAttrIndx(uint8_t value)	{ m_descriptor.setAttrIndx(value);		}
	void		setNS(bool value)			{ m_descriptor.setNS(value);			}
	void		setAP(APAttribute value)	{ m_descriptor.setAP(value);			}
	void		setSH(SHAttribute value)	{ m_descriptor.setSH(value);			}
	void		setAF(bool value)			{ m_descriptor.setAF(value);			}
	void		setNG(bool value)			{ m_descriptor.setNG(value);			}
	void		setContiguous(bool value)	{ m_descriptor.setContinuous(value);	}
	void		setPXN(bool value)			{ m_descriptor.setPXN(value);			}
	void		setXN(bool value)			{ m_descriptor.setXN(value);			}
	
private:
	
	using DescriptorFormat = TTDescriptorFormat<GRANULE, LEVEL>;
	
	DescriptorFormat	m_descriptor;
};

// MARK: - Table Translation Entry types

using TTLevel0Entry_4K	= TTEntry<TTGranule::Granule4K, TTLevel::Level0>;
using TTLevel0Entry_16K	= TTEntry<TTGranule::Granule16K, TTLevel::Level0>;
using TTLevel0Entry_64K	= TTEntry<TTGranule::Granule64K, TTLevel::Level0>;

using TTLevel1Entry_4K	= TTEntry<TTGranule::Granule4K, TTLevel::Level1>;
using TTLevel1Entry_16K	= TTEntry<TTGranule::Granule16K, TTLevel::Level1>;
using TTLevel1Entry_64K	= TTEntry<TTGranule::Granule64K, TTLevel::Level1>;

using TTLevel2Entry_4K	= TTEntry<TTGranule::Granule4K, TTLevel::Level2>;
using TTLevel2Entry_16K	= TTEntry<TTGranule::Granule16K, TTLevel::Level2>;
using TTLevel2Entry_64K	= TTEntry<TTGranule::Granule64K, TTLevel::Level2>;

using TTLevel3Entry_4K	= TTEntry<TTGranule::Granule4K, TTLevel::Level3>;
using TTLevel3Entry_16K	= TTEntry<TTGranule::Granule16K, TTLevel::Level3>;
using TTLevel3Entry_64K	= TTEntry<TTGranule::Granule64K, TTLevel::Level3>;
