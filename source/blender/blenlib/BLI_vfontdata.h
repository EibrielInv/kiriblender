/**
 * @file BLI_vfontdata.h
 * 
 * A structure to represent vector fonts, 
 * and to load them from PostScript fonts.
 * 
 * $Id$
 *
 * ***** BEGIN GPL/BL DUAL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version. The Blender
 * Foundation also sells licenses for use in proprietary software under
 * the Blender License.  See http://www.blender.org/BL/ for information
 * about this.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL/BL DUAL LICENSE BLOCK *****
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
 
#ifndef BLI_VFONTDATA_H
#define BLI_VFONTDATA_H

#include "DNA_listBase.h"

struct PackedFile;

#define MAX_VF_CHARS 256

typedef struct VFontData {
	ListBase nurbsbase[MAX_VF_CHARS];
	float	    resol[MAX_VF_CHARS];
	float	    width[MAX_VF_CHARS];
	float	    *points[MAX_VF_CHARS];
} VFontData;

/**
 * Construct a new VFontData structure from 
 * PostScript font data in a PackedFile.
 * 
 * @param pf The font data.
 * @retval A new VFontData structure, or NULL
 * if unable to load.
 */
	VFontData*
BLI_vfontdata_from_psfont(
	struct PackedFile *pf);

#endif

