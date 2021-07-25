//
//  Copyright (c) 2017, Alexander Hude
//  All rights reserved.
//
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree.
//

#pragma once

#ifndef __cplusplus

typedef enum {
	kExceptionLevel0			= 0,
	kExceptionLevel1			= 1,
	kExceptionLevel2			= 2,
	kExceptionLevel3			= 3,
	kExceptionLevelCount,
	kExceptionLevelUndefined	= -1,
} ExceptionLevel;

typedef enum {
	kTTLevel0			= 0,
	kTTLevel1			= 1,
	kTTLevel2			= 2,
	kTTLevel3			= 3,
	kTTLevelCount,
	kTTLevelUndefined	= -1,
} TTLevel;

typedef enum {
	kTTGranule4K		= 4 * 1024,
	kTTGranule16K		= 16 * 1024,
	kTTGranule64K		= 64 * 1024,
	kTTGranuleCount 	= 3,
	kTTGranuleUndefined	= -1,
} TTGranule;

#endif
