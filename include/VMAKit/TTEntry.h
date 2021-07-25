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

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	TTGranule	granule;
	TTLevel		level;
	ttentry_t	descriptor;
} TTEntryDetails;
	
// Generic Entry
static const ttentry_t kTTDescriptor_ValidBit = (1 << 0);
static const ttentry_t kTTDescriptor_TableBit = (1 << 1);
static const ttentry_t kTTDescriptor_PageBit = (1 << 1);
	
// Table attributes
static const ttentry_t kTTDescriptor_PXNTableBitShift	= (59);
static const ttentry_t kTTDescriptor_PXNTableBitMask	= ((ttentry_t)1 << kTTDescriptor_PXNTableBitShift);
static const ttentry_t kTTDescriptor_XNTableBitShift	= (60);
static const ttentry_t kTTDescriptor_XNTableBitMask		= ((ttentry_t)1 << kTTDescriptor_XNTableBitShift);
static const ttentry_t kTTDescriptor_APTableBitShift	= (61);
static const ttentry_t kTTDescriptor_APTableBitMask		= ((ttentry_t)0b11 << kTTDescriptor_APTableBitShift);
static const ttentry_t kTTDescriptor_NSTableBitShift	= (63);
static const ttentry_t kTTDescriptor_NSTableBitMask		= ((ttentry_t)1 << kTTDescriptor_NSTableBitShift);

typedef enum {
	kAPTable_NoEffect					= 0b00,
	kAPTable_NoEL0ReadAccess			= 0b01,
	kAPTable_NoAnyWriteAccess			= 0b10,
	kAPTable_NoEL0ReadAndAnyWriteAccess	= 0b11,
} TTDescriptorAPTable;
	
// Block and Page attributes

static const ttentry_t kTTDescriptor_AttrIndxBitShift	= (2);
static const ttentry_t kTTDescriptor_AttrIndxBitMask	= ((ttentry_t)0b111 << kTTDescriptor_AttrIndxBitShift);
static const ttentry_t kTTDescriptor_NSBitShift			= (5);
static const ttentry_t kTTDescriptor_NSBitMask			= ((ttentry_t)1 << kTTDescriptor_NSBitShift);
static const ttentry_t kTTDescriptor_APBitShift			= (6);
static const ttentry_t kTTDescriptor_APBitMask			= ((ttentry_t)0b11 << kTTDescriptor_APBitShift);
static const ttentry_t kTTDescriptor_SHBitShift			= (8);
static const ttentry_t kTTDescriptor_SHBitMask			= ((ttentry_t)0b11 << kTTDescriptor_SHBitShift);
static const ttentry_t kTTDescriptor_AFBitShift			= (10);
static const ttentry_t kTTDescriptor_AFBitMask			= ((ttentry_t)1 << kTTDescriptor_AFBitShift);
static const ttentry_t kTTDescriptor_nGBitShift			= (11);
static const ttentry_t kTTDescriptor_nGBitMask			= ((ttentry_t)1 << kTTDescriptor_nGBitShift);
static const ttentry_t kTTDescriptor_ContiguousBitShift	= (52);
static const ttentry_t kTTDescriptor_ContiguousBitMask	= ((ttentry_t)1 << kTTDescriptor_ContiguousBitShift);
static const ttentry_t kTTDescriptor_PXNBitShift		= (53);
static const ttentry_t kTTDescriptor_PXNBitMask			= ((ttentry_t)1 << kTTDescriptor_PXNBitShift);
static const ttentry_t kTTDescriptor_XNBitShift			= (54);
static const ttentry_t kTTDescriptor_XNBitMask			= ((ttentry_t)1 << kTTDescriptor_XNBitShift);
	
typedef enum {
	kAP_HigherELReadWriteEL0None		= 0b00,
	kAP_HigherELReadWriteEL0ReadWrite	= 0b01,
	kAP_HigherELReadOnlyEL0None			= 0b10,
	kAP_HigherELReadOnlyEL0ReadOnly		= 0b11,

} TTDescriptorAP;

typedef enum {
	kSH_NonShareable	= 0b00,
	kSH_Reserved		= 0b01,
	kSH_OuterShareable	= 0b10,
	kSH_InnerShareable	= 0b11,
} TTDescriptorSH;
	
// Generic Entry
	
bool		ttentry_IsValid(TTEntryDetails* entry);
bool		ttentry_IsBlockDescriptor(TTEntryDetails* entry);
bool		ttentry_IsTableDescriptor(TTEntryDetails* entry);
bool		ttentry_IsReserved(TTEntryDetails* entry);
bool		ttentry_IsPageDescriptor(TTEntryDetails* entry);

ttentry_t	ttentry_GetDescriptor(TTEntryDetails* entry);
void		ttentry_SetDescriptor(TTEntryDetails* entry, ttentry_t descriptor);

phys_addr_t	ttentry_GetOutputAddress(TTEntryDetails* entry);
void		ttentry_SetOutputAddress(TTEntryDetails* entry, phys_addr_t address);
	
// Table Entry
	
bool		ttentry_GetPXNTable(TTEntryDetails* entry);
bool		ttentry_GetXNTable(TTEntryDetails* entry);
TTDescriptorAPTable ttentry_GetAPTable(TTEntryDetails* entry);
bool		ttentry_GetNSTable(TTEntryDetails* entry);

void		ttentry_SetPXNTable(TTEntryDetails* entry, bool value);
void		ttentry_SetXNTable(TTEntryDetails* entry, bool value);
void		ttentry_SetAPTable(TTEntryDetails* entry, TTDescriptorAPTable value);
void		ttentry_SetNSTable(TTEntryDetails* entry, bool value);
	
// Block or Page Entry
	
uint8_t		ttentry_GetAttrIndx(TTEntryDetails* entry);
bool		ttentry_GetNS(TTEntryDetails* entry);
TTDescriptorAP	ttentry_GetAP(TTEntryDetails* entry);
TTDescriptorSH	ttentry_GetSH(TTEntryDetails* entry);
bool		ttentry_GetAF(TTEntryDetails* entry);
bool		ttentry_GetNG(TTEntryDetails* entry);
bool		ttentry_GetContiguous(TTEntryDetails* entry);
bool		ttentry_GetPXN(TTEntryDetails* entry);
bool		ttentry_GetXN(TTEntryDetails* entry);

void		ttentry_SetAttrIndx(TTEntryDetails* entry, uint8_t value);
void		ttentry_SetNS(TTEntryDetails* entry, bool value);
void		ttentry_SetAP(TTEntryDetails* entry, TTDescriptorAP value);
void		ttentry_SetSH(TTEntryDetails* entry, TTDescriptorSH value);
void		ttentry_SetAF(TTEntryDetails* entry, bool value);
void		ttentry_SetNG(TTEntryDetails* entry, bool value);
void		ttentry_SetContiguous(TTEntryDetails* entry, bool value);
void		ttentry_SetPXN(TTEntryDetails* entry, bool value);
void		ttentry_SetXN(TTEntryDetails* entry, bool value);

#ifdef __cplusplus
}
#endif
