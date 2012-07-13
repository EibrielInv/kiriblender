/*
 * Copyright 2011, Blender Foundation.
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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Contributor: 
 *		Jeroen Bakker 
 *		Monique Dewanchand
 */

#ifndef _COM_MovieClipAttributeOperation_h
#define _COM_MovieClipAttributeOperation_h
#include "COM_NodeOperation.h"
#include "DNA_movieclip_types.h"

typedef enum MovieClipAttribute {
	MCA_SCALE,
	MCA_X,
	MCA_Y,
	MCA_ANGLE
} MovieClipAttribute;
/**
 * this program converts an input color to an output value.
 * it assumes we are in sRGB color space.
 */
class MovieClipAttributeOperation : public NodeOperation {
private:
	MovieClip *m_clip;
	float m_value;
	bool m_valueSet;
	int m_framenumber;
	MovieClipAttribute m_attribute;
public:
	/**
	 * Default constructor
	 */
	MovieClipAttributeOperation();
	
	/**
	 * the inner loop of this program
	 */
	void executePixel(float *color, float x, float y, PixelSampler sampler);
	void determineResolution(unsigned int resolution[], unsigned int preferredResolution[]);

	void setMovieClip(MovieClip *clip) { this->m_clip = clip; }
	void setFramenumber(int framenumber) { this->m_framenumber = framenumber; }
	void setAttribute(MovieClipAttribute attribute) { this->m_attribute = attribute; }
};
#endif
