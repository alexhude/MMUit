//
//  Copyright (c) 2017, Alexander Hude
//  All rights reserved.
//
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree.
//

#pragma once

enum class ExceptionLevel {
	EL0			= 0,
	EL1			= 1,
	EL2			= 2,
	EL3			= 3,
	Count,
	Undefined 	= -1
};

enum class TTLevel {
	Level0		= 0,
	Level1		= 1,
	Level2		= 2,
	Level3		= 3,
	Count,
	Undefined	= -1
};

TTLevel& operator++(TTLevel& level)
{
	switch(level)
	{
		case TTLevel::Level0: return level = TTLevel::Level1;
		case TTLevel::Level1: return level = TTLevel::Level2;
		case TTLevel::Level2: return level = TTLevel::Level3;
		case TTLevel::Level3: return level = TTLevel::Level3;
		case TTLevel::Count: { assert(0); }
		case TTLevel::Undefined: { assert(0); }
	}
};

TTLevel& operator++(TTLevel& level, int)
{
	switch(level)
	{
		case TTLevel::Level0: return level = TTLevel::Level1;
		case TTLevel::Level1: return level = TTLevel::Level2;
		case TTLevel::Level2: return level = TTLevel::Level3;
		case TTLevel::Level3: return level = TTLevel::Level3;
		case TTLevel::Count: { assert(0); }
		case TTLevel::Undefined: { assert(0); }
	}
};

TTLevel& operator--(TTLevel& level)
{
	switch(level)
	{
		case TTLevel::Level0: return level = TTLevel::Level0;
		case TTLevel::Level1: return level = TTLevel::Level0;
		case TTLevel::Level2: return level = TTLevel::Level1;
		case TTLevel::Level3: return level = TTLevel::Level2;
		case TTLevel::Count: { assert(0); }
		case TTLevel::Undefined: { assert(0); }
	}
};

TTLevel& operator--(TTLevel& level, int)
{
	switch(level)
	{
		case TTLevel::Level0: return level = TTLevel::Level0;
		case TTLevel::Level1: return level = TTLevel::Level0;
		case TTLevel::Level2: return level = TTLevel::Level1;
		case TTLevel::Level3: return level = TTLevel::Level2;
		case TTLevel::Count: { assert(0); }
		case TTLevel::Undefined: { assert(0); }
	}
};

enum class TTGranule {
	Granule4K		= 4 * 1024,
	Granule16K		= 16 * 1024,
	Granule64K		= 64 * 1024,
	Count			= 3,
	Undefined		= -1
};
