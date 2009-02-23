/**
 * $Id$
 *
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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
 * ***** END GPL LICENSE BLOCK *****
 */

#ifndef __KX_BLENDERRENDERTOOLS
#define __KX_BLENDERRENDERTOOLS

#ifdef WIN32
// don't show stl-warnings
#pragma warning (disable:4786)
#endif

#include "RAS_IRenderTools.h"

struct KX_ClientObjectInfo;
class KX_RayCast;

/* BlenderRenderTools are a set of tools to apply 2D/3D graphics effects, which
 * are not part of the (polygon) Rasterizer. Effects like 2D text, 3D (polygon)
 * text, lighting.
 *
 * Most of this code is duplicated in GPC_RenderTools, so this should be
 * moved to some common location to avoid duplication. */

class KX_BlenderRenderTools  : public RAS_IRenderTools
{
	int		m_lastlightlayer;
	bool	m_lastlighting;
	static unsigned int m_numgllights;

public:
						KX_BlenderRenderTools();
	virtual				~KX_BlenderRenderTools();	

	void				EndFrame(RAS_IRasterizer* rasty);
	void				BeginFrame(RAS_IRasterizer* rasty);

	void				EnableOpenGLLights(RAS_IRasterizer *rasty);
	void				DisableOpenGLLights();
	void				ProcessLighting(RAS_IRasterizer *rasty, int layer, const MT_Transform& viewmat);

	void			    RenderText2D(RAS_TEXT_RENDER_MODE mode,
									 const char* text,
									 int xco,
									 int yco,
									 int width,
									 int height);
	void				RenderText(int mode,
								   class RAS_IPolyMaterial* polymat,
								   float v1[3],
								   float v2[3],
								   float v3[3],
								   float v4[3],
								   int glattrib);

	void				applyTransform(RAS_IRasterizer* rasty, double* oglmatrix, int objectdrawmode);
	int					applyLights(int objectlayer, const MT_Transform& viewmat);

	void				PushMatrix();
	void				PopMatrix();

	bool RayHit(KX_ClientObjectInfo* client, class KX_RayCast* result, void * const data);
	bool NeedRayCast(KX_ClientObjectInfo*) { return true; }

	virtual void MotionBlur(RAS_IRasterizer* rasterizer);

	virtual void Update2DFilter(vector<STR_String>& propNames, void* gameObj, RAS_2DFilterManager::RAS_2DFILTER_MODE filtermode, int pass, STR_String& text);

	virtual	void Render2DFilters(RAS_ICanvas* canvas);

	virtual void SetClientObject(RAS_IRasterizer *rasty, void* obj);
};

#endif //__KX_BLENDERRENDERTOOLS



