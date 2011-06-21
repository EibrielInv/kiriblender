/*
 * $Id$
 *
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * Copyright 2009-2011 Jörg Hermann Müller
 *
 * This file is part of AudaSpace.
 *
 * Audaspace is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * AudaSpace is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Audaspace; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file audaspace/intern/AUD_NULLDevice.cpp
 *  \ingroup audaspaceintern
 */


#include <limits>

#include "AUD_NULLDevice.h"
#include "AUD_IReader.h"
#include "AUD_IFactory.h"
#include "AUD_IHandle.h"

AUD_NULLDevice::AUD_NULLDevice()
{
}

AUD_DeviceSpecs AUD_NULLDevice::getSpecs() const
{
	AUD_DeviceSpecs specs;
	specs.channels = AUD_CHANNELS_INVALID;
	specs.format = AUD_FORMAT_INVALID;
	specs.rate = AUD_RATE_INVALID;
	return specs;
}

AUD_Reference<AUD_IHandle> AUD_NULLDevice::play(AUD_Reference<AUD_IReader> reader, bool keep)
{
	return 0;
}

AUD_Reference<AUD_IHandle> AUD_NULLDevice::play(AUD_Reference<AUD_IFactory> factory, bool keep)
{
	return 0;
}

void AUD_NULLDevice::lock()
{
}

void AUD_NULLDevice::unlock()
{
}

float AUD_NULLDevice::getVolume() const
{
	return 0;
}

void AUD_NULLDevice::setVolume(float volume)
{
}
