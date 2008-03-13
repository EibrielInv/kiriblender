/**
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
#ifndef __RAS_OPENGLRASTERIZER
#define __RAS_OPENGLRASTERIZER

#ifdef WIN32
#pragma warning (disable:4786)
#endif

#include "MT_CmMatrix4x4.h"
#include <vector>
using namespace std;

#include "RAS_IRasterizer.h"
#include "RAS_MaterialBucket.h"
#include "RAS_ICanvas.h"

#define RAS_MAX 3// match in BL_Material

struct	OglDebugLine
{
	MT_Vector3	m_from;
	MT_Vector3	m_to;
	MT_Vector3	m_color;
};

/**
 * 3D rendering device context.
 */
class RAS_OpenGLRasterizer : public RAS_IRasterizer
{

	RAS_ICanvas*	m_2DCanvas;
	
	// fogging vars
	bool			m_fogenabled;
	float			m_fogstart;
	float			m_fogdist;
	float			m_fogr;
	float			m_fogg;
	float			m_fogb;
	
	float			m_redback;
	float			m_greenback;
	float			m_blueback;
	float			m_alphaback;
	
	float			m_ambr;
	float			m_ambg;
	float			m_ambb;

	double			m_time;
	MT_CmMatrix4x4	m_viewmatrix;
	MT_Point3		m_campos;

	StereoMode		m_stereomode;
	StereoEye		m_curreye;
	float			m_eyeseparation;
	bool			m_seteyesep;
	float			m_focallength;
	bool			m_setfocallength;
	int				m_noOfScanlines;
	bool			InterlacedStereo() const;

	//motion blur
	int	m_motionblur;
	float	m_motionblurvalue;

protected:
	int				m_drawingmode;
	TexCoGen		m_texco[RAS_MAX];
	bool			m_useTang;

	/** Stores the caching information for the last material activated. */
	RAS_IPolyMaterial::TCachingInfo m_materialCachingInfo;

public:
	double GetTime();
	RAS_OpenGLRasterizer(RAS_ICanvas* canv);
	virtual ~RAS_OpenGLRasterizer();

	/*enum DrawType
	{
			KX_BOUNDINGBOX = 1,
			KX_WIREFRAME,
			KX_SOLID,
			KX_SHADED,
			KX_TEXTURED
	};

	enum DepthMask
	{
			KX_DEPTHMASK_ENABLED =1,
			KX_DEPTHMASK_DISABLED,
	};*/
	virtual void	SetDepthMask(DepthMask depthmask);

	virtual bool	SetMaterial(const RAS_IPolyMaterial& mat);
	virtual bool	Init();
	virtual void	Exit();
	virtual bool	BeginFrame(int drawingmode, double time);
	virtual void	ClearDepthBuffer();
	virtual void	ClearCachingInfo(void);
	virtual void	EndFrame();
	virtual void	SetRenderArea();

	virtual void	SetStereoMode(const StereoMode stereomode);
	virtual bool	Stereo();
	virtual void	SetEye(const StereoEye eye);
	virtual StereoEye	GetEye();
	virtual void	SetEyeSeparation(const float eyeseparation);
	virtual float	GetEyeSeparation();
	virtual void	SetFocalLength(const float focallength);
	virtual float	GetFocalLength();

	virtual void	SetAlphaTest(bool enable);

	virtual void	SwapBuffers();
	virtual void	IndexPrimitives(
						const vecVertexArray& vertexarrays,
						const vecIndexArrays & indexarrays,
						int mode,
						class RAS_IPolyMaterial* polymat,
						class RAS_IRenderTools* rendertools,
						bool useObjectColor,
						const MT_Vector4& rgbacolor,
						class KX_ListSlot** slot
					);

	virtual void	IndexPrimitives_Ex(
						const vecVertexArray& vertexarrays,
						const vecIndexArrays & indexarrays,
						int mode,
						class RAS_IPolyMaterial* polymat,
						class RAS_IRenderTools* rendertools,
						bool useObjectColor,
						const MT_Vector4& rgbacolor
					);

	virtual void	IndexPrimitives_3DText(
						const vecVertexArray& vertexarrays,
						const vecIndexArrays & indexarrays,
						int mode,
						class RAS_IPolyMaterial* polymat,
						class RAS_IRenderTools* rendertools,
						bool useObjectColor,
						const MT_Vector4& rgbacolor
					);

	virtual void IndexPrimitivesMulti( 
						const vecVertexArray& vertexarrays,
						const vecIndexArrays & indexarrays,
						int mode,
						class RAS_IPolyMaterial* polymat,
						class RAS_IRenderTools* rendertools,
						bool useObjectColor,
						const MT_Vector4& rgbacolor,
						class KX_ListSlot** slot);

	virtual void IndexPrimitivesMulti_Ex( 
						const vecVertexArray& vertexarrays,
						const vecIndexArrays & indexarrays,
						int mode,
						class RAS_IPolyMaterial* polymat,
						class RAS_IRenderTools* rendertools,
						bool useObjectColor,
						const MT_Vector4& rgbacolor);


	virtual void	SetProjectionMatrix(MT_CmMatrix4x4 & mat);
	virtual void	SetProjectionMatrix(const MT_Matrix4x4 & mat);
	virtual void	SetViewMatrix(
						const MT_Matrix4x4 & mat,
						const MT_Vector3& campos,
						const MT_Point3 &camLoc,
						const MT_Quaternion &camOrientQuat
					);

	virtual const	MT_Point3& GetCameraPosition();
	virtual void	LoadViewMatrix();
	
	virtual void	SetFog(
						float start,
						float dist,
						float r,
						float g,
						float b
					);

	virtual void	SetFogColor(
						float r,
						float g,
						float b
					);

	virtual void	SetFogStart(float fogstart);
	virtual void	SetFogEnd(float fogend);

	void			DisableFog();
	virtual void	DisplayFog();

	virtual void	SetBackColor(
						float red,
						float green,
						float blue,
						float alpha
					);
	
	virtual void	SetDrawingMode(int drawingmode);
	virtual int		GetDrawingMode();

	virtual void	EnableTextures(bool enable);
	virtual void	SetCullFace(bool enable);
	virtual void	SetLines(bool enable);

	virtual MT_Matrix4x4 GetFrustumMatrix(
							float left,
							float right,
							float bottom,
							float top,
							float frustnear,
							float frustfar,
							bool perspective
						);

	virtual void	SetSpecularity(
						float specX,
						float specY,
						float specZ,
						float specval
					);

	virtual void	SetShinyness(float shiny);
	virtual void	SetDiffuse(
						float difX,
						float difY,
						float difZ,
						float diffuse
					);
	virtual void	SetEmissive(float eX,
								float eY,
								float eZ,
								float e
							   );

	virtual void	SetAmbientColor(float red, float green, float blue);
	virtual void	SetAmbient(float factor);

	virtual void	SetPolygonOffset(float mult, float add);

	virtual	void	DrawDebugLine(const MT_Vector3& from,const MT_Vector3& to,const MT_Vector3& color)
	{
		OglDebugLine line;
		line.m_from = from;
		line.m_to = to;
		line.m_color = color;
		m_debugLines.push_back(line);
	}

	std::vector <OglDebugLine>	m_debugLines;

	virtual void	SetTexCoords(TexCoGen coords,int enabled);
	virtual void	SetAttrib(int type);
	void			TexCoord(const RAS_TexVert &tv, int unit);
	virtual void	GetViewMatrix(MT_Matrix4x4 &mat) const;

	void	Tangent(const RAS_TexVert& v1,
					const RAS_TexVert& v2,
					const RAS_TexVert& v3,
					const MT_Vector3 &no);
	
	virtual void	EnableMotionBlur(float motionblurvalue);
	virtual void	DisableMotionBlur();
	virtual float	GetMotionBlurValue(){return m_motionblurvalue;};
	virtual int	GetMotionBlurState(){return m_motionblur;};
	virtual void SetMotionBlurState(int newstate)
	{
		if(newstate<0) 
			m_motionblur = 0;
		else if(newstate>2)
			m_motionblur = 2;
		else 
			m_motionblur = newstate;
	};
};

#endif //__RAS_OPENGLRASTERIZER


