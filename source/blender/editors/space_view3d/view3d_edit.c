/**
 * $Id:
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
 * The Original Code is Copyright (C) 2008 Blender Foundation.
 * All rights reserved.
 *
 * 
 * Contributor(s): Blender Foundation
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

#include "DNA_action_types.h"
#include "DNA_camera_types.h"
#include "DNA_lamp_types.h"
#include "DNA_object_types.h"
#include "DNA_space_types.h"
#include "DNA_scene_types.h"
#include "DNA_screen_types.h"
#include "DNA_userdef_types.h"
#include "DNA_view3d_types.h"
#include "DNA_world_types.h"

#include "MEM_guardedalloc.h"

#include "BLI_blenlib.h"
#include "BLI_arithb.h"
#include "BLI_rand.h"

#include "BKE_action.h"
#include "BKE_object.h"
#include "BKE_global.h"
#include "BKE_scene.h"
#include "BKE_screen.h"
#include "BKE_utildefines.h"

#include "RE_pipeline.h"	// make_stars

#include "BIF_gl.h"
#include "BIF_retopo.h"

#include "WM_api.h"

#include "ED_screen.h"

#include "UI_interface.h"
#include "UI_resources.h"
#include "UI_view2d.h"

#include "ED_types.h"

#include "PIL_time.h" /* smoothview */

#include "view3d_intern.h"	// own include

/* ********************** view3d_edit: view manipulations ********************* */

#define TRACKBALLSIZE  (1.1)

/* the central math in this function was copied from trackball.cpp, sample code from the 
   Developers Toolbox series by SGI. */

/* trackball: better one than a full spherical solution */

void calctrackballvecfirst(rcti *area, short *mval, float *vec)
{
	float x, y, radius, d, z, t;
	
	radius= TRACKBALLSIZE;
	
	/* normalize x and y */
	x= (area->xmax + area->xmin)/2 -mval[0];
	x/= (float)((area->xmax - area->xmin)/2);
	y= (area->ymax + area->ymin)/2 -mval[1];
	y/= (float)((area->ymax - area->ymin)/2);
	
	d = sqrt(x*x + y*y);
	if (d < radius*M_SQRT1_2)  	/* Inside sphere */
		z = sqrt(radius*radius - d*d);
	else
	{ 			/* On hyperbola */
		t = radius / M_SQRT2;
		z = t*t / d;
	}

	vec[0]= x;
	vec[1]= y;
	vec[2]= -z;		/* yah yah! */

	if( fabs(vec[2])>fabs(vec[1]) && fabs(vec[2])>fabs(vec[0]) ) {
		vec[0]= 0.0;
		vec[1]= 0.0;
		if(vec[2]>0.0) vec[2]= 1.0; else vec[2]= -1.0;
	}
	else if( fabs(vec[1])>fabs(vec[0]) && fabs(vec[1])>fabs(vec[2]) ) {
		vec[0]= 0.0;
		vec[2]= 0.0;
		if(vec[1]>0.0) vec[1]= 1.0; else vec[1]= -1.0;
	}
	else  {
		vec[1]= 0.0;
		vec[2]= 0.0;
		if(vec[0]>0.0) vec[0]= 1.0; else vec[0]= -1.0;
	}
}

void calctrackballvec(rcti *area, short *mval, float *vec)
{
	float x, y, radius, d, z, t;
	
	radius= TRACKBALLSIZE;
	
	/* x en y normalizeren */
	x= (area->xmax + area->xmin)/2 -mval[0];
	x/= (float)((area->xmax - area->xmin)/4);
	y= (area->ymax + area->ymin)/2 -mval[1];
	y/= (float)((area->ymax - area->ymin)/2);
	
	d = sqrt(x*x + y*y);
	if (d < radius*M_SQRT1_2)  	/* Inside sphere */
		z = sqrt(radius*radius - d*d);
	else
	{ 			/* On hyperbola */
		t = radius / M_SQRT2;
		z = t*t / d;
	}

	vec[0]= x;
	vec[1]= y;
	vec[2]= -z;		/* yah yah! */

}


// ndof scaling will be moved to user setting.
// In the mean time this is just a place holder.

// Note: scaling in the plugin and ghostwinlay.c
// should be removed. With driver default setting,
// each axis returns approx. +-200 max deflection.

// The values I selected are based on the older
// polling i/f. With event i/f, the sensistivity
// can be increased for improved response from
// small deflections of the device input.


// lukep notes : i disagree on the range.
// the normal 3Dconnection driver give +/-400
// on defaut range in other applications
// and up to +/- 1000 if set to maximum
// because i remove the scaling by delta,
// which was a bad idea as it depend of the system
// speed and os, i changed the scaling values, but 
// those are still not ok


float ndof_axis_scale[6] = {
	+0.01,	// Tx
	+0.01,	// Tz
	+0.01,	// Ty
	+0.0015,	// Rx
	+0.0015,	// Rz
	+0.0015	// Ry
};

void filterNDOFvalues(float *sbval)
{
	int i=0;
	float max  = 0.0;
	
	for (i =0; i<6;i++)
		if (fabs(sbval[i]) > max)
			max = fabs(sbval[i]);
	for (i =0; i<6;i++)
		if (fabs(sbval[i]) != max )
			sbval[i]=0.0;
}

// statics for controlling v3d->dist corrections.
// viewmoveNDOF zeros and adjusts v3d->ofs.
// viewmove restores based on dz_flag state.

int dz_flag = 0;
float m_dist;

void viewmoveNDOFfly(ARegion *ar, View3D *v3d, int mode)
{
    int i;
    float phi;
    float dval[7];
	// static fval[6] for low pass filter; device input vector is dval[6]
	static float fval[6];
    float tvec[3],rvec[3];
    float q1[4];
	float mat[3][3];
	float upvec[3];


    /*----------------------------------------------------
	 * sometimes this routine is called from headerbuttons
     * viewmove needs to refresh the screen
     */
// XXX	areawinset(ar->win);


	// fetch the current state of the ndof device
// XXX	getndof(dval);

	if (v3d->ndoffilter)
		filterNDOFvalues(fval);

	// Scale input values

//	if(dval[6] == 0) return; // guard against divide by zero

	for(i=0;i<6;i++) {

		// user scaling
		dval[i] = dval[i] * ndof_axis_scale[i];
	}


	// low pass filter with zero crossing reset

	for(i=0;i<6;i++) {
		if((dval[i] * fval[i]) >= 0)
			dval[i] = (fval[i] * 15 + dval[i]) / 16;
		else
			fval[i] = 0;
	}


	// force perspective mode. This is a hack and is
	// incomplete. It doesn't actually effect the view
	// until the first draw and doesn't update the menu
	// to reflect persp mode.

	v3d->persp = V3D_PERSP;


	// Correct the distance jump if v3d->dist != 0

	// This is due to a side effect of the original
	// mouse view rotation code. The rotation point is
	// set a distance in front of the viewport to
	// make rotating with the mouse look better.
	// The distance effect is written at a low level
	// in the view management instead of the mouse
	// view function. This means that all other view
	// movement devices must subtract this from their
	// view transformations.

	if(v3d->dist != 0.0) {
		dz_flag = 1;
		m_dist = v3d->dist;
		upvec[0] = upvec[1] = 0;
		upvec[2] = v3d->dist;
		Mat3CpyMat4(mat, v3d->viewinv);
		Mat3MulVecfl(mat, upvec);
		VecSubf(v3d->ofs, v3d->ofs, upvec);
		v3d->dist = 0.0;
	}


	// Apply rotation
	// Rotations feel relatively faster than translations only in fly mode, so
	// we have no choice but to fix that here (not in the plugins)
	rvec[0] = -0.5 * dval[3];
	rvec[1] = -0.5 * dval[4];
	rvec[2] = -0.5 * dval[5];

	// rotate device x and y by view z

	Mat3CpyMat4(mat, v3d->viewinv);
	mat[2][2] = 0.0f;
	Mat3MulVecfl(mat, rvec);

	// rotate the view

	phi = Normalize(rvec);
	if(phi != 0) {
		VecRotToQuat(rvec,phi,q1);
		QuatMul(v3d->viewquat, v3d->viewquat, q1);
	}


	// Apply translation

	tvec[0] = dval[0];
	tvec[1] = dval[1];
	tvec[2] = -dval[2];

	// the next three lines rotate the x and y translation coordinates
	// by the current z axis angle

	Mat3CpyMat4(mat, v3d->viewinv);
	mat[2][2] = 0.0f;
	Mat3MulVecfl(mat, tvec);

	// translate the view

	VecSubf(v3d->ofs, v3d->ofs, tvec);


	/*----------------------------------------------------
     * refresh the screen XXX
      */

	// update render preview window

// XXX	BIF_view3d_previewrender_signal(ar, PR_DBASE|PR_DISPRECT);
}

/* Zooms in on a border drawn by the user */
static int view_autodist(Scene *scene, ARegion *ar, View3D *v3d, short *mval, float mouse_worldloc[3] ) //, float *autodist )
{
	rcti rect;
	/* ZBuffer depth vars */
	bglMats mats;
	float depth, depth_close= MAXFLOAT;
	int had_depth = 0;
	double cent[2],  p[3];
	int xs, ys;
	
// XXX		getmouseco_areawin(mval);
	
// XXX	persp(PERSP_VIEW);
	
	rect.xmax = mval[0] + 4;
	rect.ymax = mval[1] + 4;
	
	rect.xmin = mval[0] - 4;
	rect.ymin = mval[1] - 4;
	
	/* Get Z Depths, needed for perspective, nice for ortho */
	bgl_get_mats(&mats);
	draw_depth(scene, ar, v3d, NULL);
	
	/* force updating */
	if (v3d->depths) {
		had_depth = 1;
		v3d->depths->damaged = 1;
	}
	
	view3d_update_depths(ar, v3d);
	
	/* Constrain rect to depth bounds */
	if (rect.xmin < 0) rect.xmin = 0;
	if (rect.ymin < 0) rect.ymin = 0;
	if (rect.xmax >= v3d->depths->w) rect.xmax = v3d->depths->w-1;
	if (rect.ymax >= v3d->depths->h) rect.ymax = v3d->depths->h-1;		
	
	/* Find the closest Z pixel */
	for (xs=rect.xmin; xs < rect.xmax; xs++) {
		for (ys=rect.ymin; ys < rect.ymax; ys++) {
			depth= v3d->depths->depths[ys*v3d->depths->w+xs];
			if(depth < v3d->depths->depth_range[1] && depth > v3d->depths->depth_range[0]) {
				if (depth_close > depth) {
					depth_close = depth;
				}
			}
		}
	}
	
	if (depth_close==MAXFLOAT)
		return 0;
		
	if (had_depth==0) {
		MEM_freeN(v3d->depths->depths);
		v3d->depths->depths = NULL;
	}
	v3d->depths->damaged = 1;
	
	cent[0] = (double)mval[0];
	cent[1] = (double)mval[1];
	
	if (!gluUnProject(cent[0], cent[1], depth_close, mats.modelview, mats.projection, (GLint *)mats.viewport, &p[0], &p[1], &p[2]))
		return 0;

	mouse_worldloc[0] = (float)p[0];
	mouse_worldloc[1] = (float)p[1];
	mouse_worldloc[2] = (float)p[2];
	return 1;
}

static void view_zoom_mouseloc(ARegion *ar, View3D *v3d, float dfac, short *mouseloc)
{
	if(U.uiflag & USER_ZOOM_TO_MOUSEPOS) {
		short vb[2];
		float dvec[3];
		float tvec[3];
		float tpos[3];
		float new_dist;
		
		/* find the current window width and height */
		vb[0] = ar->winx;
		vb[1] = ar->winy;
		
		tpos[0] = -v3d->ofs[0];
		tpos[1] = -v3d->ofs[1];
		tpos[2] = -v3d->ofs[2];
		
		/* Project cursor position into 3D space */
		initgrabz(v3d, tpos[0], tpos[1], tpos[2]);
		window_to_3d(ar, v3d, dvec, mouseloc[0]-vb[0]/2, mouseloc[1]-vb[1]/2);
		
		/* Calculate view target position for dolly */
		tvec[0] = -(tpos[0] + dvec[0]);
		tvec[1] = -(tpos[1] + dvec[1]);
		tvec[2] = -(tpos[2] + dvec[2]);
		
		/* Offset to target position and dolly */
		new_dist = v3d->dist * dfac;
		
		VECCOPY(v3d->ofs, tvec);
		v3d->dist = new_dist;
		
		/* Calculate final offset */
		dvec[0] = tvec[0] + dvec[0] * dfac;
		dvec[1] = tvec[1] + dvec[1] * dfac;
		dvec[2] = tvec[2] + dvec[2] * dfac;
		
		VECCOPY(v3d->ofs, dvec);
	} else {
		v3d->dist *= dfac;
	}
}


#define COS45 0.70710678118654746
#define SIN45 COS45

void viewmove(Scene *scene, ARegion *ar, View3D *v3d, int mode)
{
	static float lastofs[3] = {0,0,0};
	Object *ob = OBACT;
	float firstvec[3], newvec[3], dvec[3];
	float reverse, oldquat[4], q1[4], si, phi, dist0;
	float ofs[3], obofs[3]= {0.0f, 0.0f, 0.0f};
	int firsttime=1;
	short mvalball[2], mval[2], mvalo[2], mval_area[2], mvali[2];
	short use_sel = 0;
	short preview3d_event= 1;
	
	// locals for dist correction
	float mat[3][3];
	float upvec[3];

		/* 3D window may not be defined */
	if( !v3d ) {
		fprintf( stderr, "v3d == NULL in viewmove()\n" );
		return;
	}
	
	// dist correction from other movement devices	
	if((dz_flag)||v3d->dist==0) {
		dz_flag = 0;
		v3d->dist = m_dist;
		upvec[0] = upvec[1] = 0;
		upvec[2] = v3d->dist;
		Mat3CpyMat4(mat, v3d->viewinv);
		Mat3MulVecfl(mat, upvec);
		VecAddf(v3d->ofs, v3d->ofs, upvec);
	}
		
	/* sometimes this routine is called from headerbuttons */

// XXX	areawinset(ar->win);
	
	QUATCOPY(oldquat, v3d->viewquat);
	
// XXX	getmouseco_areawin(mval_area);	/* for zoom to mouse loc */
// XXX	getmouseco_sc(mvali);		/* work with screen coordinates because of trackball function */
	mvalball[0]= mvalo[0] = mvali[0];			/* needed for turntable to work */
	mvalball[1]= mvalo[1] = mvali[1];
	dist0= v3d->dist;
	
	calctrackballvec(&ar->winrct, mvalo, firstvec);

	/* cumultime(0); */

	if(!G.obedit && (G.f & G_SCULPTMODE) && ob && v3d->pivot_last) {
		use_sel= 1;
		VecCopyf(ofs, v3d->ofs);

// XXX		VecCopyf(obofs, sculpt_data()->pivot);
		Mat4MulVecfl(ob->obmat, obofs);
		obofs[0]= -obofs[0];
		obofs[1]= -obofs[1];
		obofs[2]= -obofs[2];
	}
	else if (U.uiflag & USER_ORBIT_SELECTION) {
		use_sel = 1;
		
		VECCOPY(ofs, v3d->ofs);
		
		/* If there's no selection, lastofs is unmodified and last value since static */
// XXX		calculateTransformCenter(V3D_CENTROID, lastofs);
		
		VECCOPY(obofs, lastofs);
		VecMulf(obofs, -1.0f);
	}
	else if (U.uiflag & USER_ORBIT_ZBUF) {
		if ((use_sel= view_autodist(scene, ar, v3d, mval, obofs))) {
			if (v3d->persp==V3D_PERSP) {
				float my_origin[3]; /* original v3d->ofs */
				float my_pivot[3]; /* view */
				
				VECCOPY(my_origin, v3d->ofs);
				VecMulf(my_origin, -1.0f);				/* ofs is flipped */
				
				/* Set the dist value to be the distance from this 3d point */
				/* this means youll always be able to zoom into it and panning wont go bad when dist was zero */
				
				/* remove dist value */			
				upvec[0] = upvec[1] = 0;
				upvec[2] = v3d->dist;
				Mat3CpyMat4(mat, v3d->viewinv);
				Mat3MulVecfl(mat, upvec);
				VecSubf(my_pivot, v3d->ofs, upvec);
				VecMulf(my_pivot, -1.0f);				/* ofs is flipped */
				
				/* find a new ofs value that is allong the view axis (rather then the mouse location) */
				lambda_cp_line_ex(obofs, my_pivot, my_origin, dvec);
				dist0 = v3d->dist = VecLenf(my_pivot, dvec);
				
				VecMulf(dvec, -1.0f);
				VECCOPY(v3d->ofs, dvec);
			}
			VecMulf(obofs, -1.0f);
			VECCOPY(ofs, v3d->ofs);
		} else {
			ofs[0] = ofs[1] = ofs[2] = 0.0f;
		}
	}
	else
		ofs[0] = ofs[1] = ofs[2] = 0.0f;
	
	initgrabz(v3d, -v3d->ofs[0], -v3d->ofs[1], -v3d->ofs[2]);
	
	reverse= 1.0f;
	if (v3d->persmat[2][1] < 0.0f)
		reverse= -1.0f;

	while(TRUE) {
// XXX			getmouseco_sc(mval);
		
		if(	(mode==2 && U.viewzoom==USER_ZOOM_CONT) || /* continues zoom always update */
			 mval[0]!=mvalo[0] || mval[1]!=mvalo[1]) { /* mouse moved, so update */
			
			if(firsttime) {
				
				firsttime= 0;
				/* are we translating, rotating or zooming? */
				if(mode==0) {
// XXX					if(v3d->view!=0) scrarea_queue_headredraw(ar);	/*for button */
				}
				if(v3d->persp==V3D_CAMOB && mode!=1 && v3d->camera) {
					v3d->persp= V3D_PERSP;
// XXX					scrarea_do_windraw(ar);
// XXX					scrarea_queue_headredraw(ar);
				}
			}

			if(mode==0) {	/* view rotate */
				v3d->view= 0; /* need to reset everytime because of view snapping */
		
				if (U.uiflag & USER_AUTOPERSP) v3d->persp= V3D_PERSP;

				if (U.flag & USER_TRACKBALL) mvalball[0]= mval[0];
				mvalball[1]= mval[1];
				
				calctrackballvec(&ar->winrct, mvalball, newvec);
				
				VecSubf(dvec, newvec, firstvec);
				
				si= sqrt(dvec[0]*dvec[0]+ dvec[1]*dvec[1]+ dvec[2]*dvec[2]);
				si/= (2.0*TRACKBALLSIZE);
			
				if (U.flag & USER_TRACKBALL) {
					Crossf(q1+1, firstvec, newvec);
	
					Normalize(q1+1);
	
					/* Allow for rotation beyond the interval
					 * [-pi, pi] */
					while (si > 1.0)
						si -= 2.0;
		
					/* This relation is used instead of
					 * phi = asin(si) so that the angle
					 * of rotation is linearly proportional
					 * to the distance that the mouse is
					 * dragged. */
					phi = si * M_PI / 2.0;
		
					si= sin(phi);
					q1[0]= cos(phi);
					q1[1]*= si;
					q1[2]*= si;
					q1[3]*= si;	
					QuatMul(v3d->viewquat, q1, oldquat);

					if (use_sel) {
						/* compute the post multiplication quat, to rotate the offset correctly */
						QUATCOPY(q1, oldquat);
						QuatConj(q1);
						QuatMul(q1, q1, v3d->viewquat);

						QuatConj(q1); /* conj == inv for unit quat */
						VECCOPY(v3d->ofs, ofs);
						VecSubf(v3d->ofs, v3d->ofs, obofs);
						QuatMulVecf(q1, v3d->ofs);
						VecAddf(v3d->ofs, v3d->ofs, obofs);
					}
				} else {
					/* New turntable view code by John Aughey */

					float m[3][3];
					float m_inv[3][3];
					float xvec[3] = {1,0,0};
					/* Sensitivity will control how fast the viewport rotates.  0.0035 was
					   obtained experimentally by looking at viewport rotation sensitivities
					   on other modeling programs. */
					/* Perhaps this should be a configurable user parameter. */
					const float sensitivity = 0.0035;

					/* Get the 3x3 matrix and its inverse from the quaternion */
					QuatToMat3(v3d->viewquat, m);
					Mat3Inv(m_inv,m);

					/* Determine the direction of the x vector (for rotating up and down) */
					/* This can likely be compuated directly from the quaternion. */
					Mat3MulVecfl(m_inv,xvec);

					/* Perform the up/down rotation */
					phi = sensitivity * -(mval[1] - mvalo[1]);
					si = sin(phi);
					q1[0] = cos(phi);
					q1[1] = si * xvec[0];
					q1[2] = si * xvec[1];
					q1[3] = si * xvec[2];
					QuatMul(v3d->viewquat, v3d->viewquat, q1);

					if (use_sel) {
						QuatConj(q1); /* conj == inv for unit quat */
						VecSubf(v3d->ofs, v3d->ofs, obofs);
						QuatMulVecf(q1, v3d->ofs);
						VecAddf(v3d->ofs, v3d->ofs, obofs);
					}

					/* Perform the orbital rotation */
					phi = sensitivity * reverse * (mval[0] - mvalo[0]);
					q1[0] = cos(phi);
					q1[1] = q1[2] = 0.0;
					q1[3] = sin(phi);
					QuatMul(v3d->viewquat, v3d->viewquat, q1);

					if (use_sel) {
						QuatConj(q1);
						VecSubf(v3d->ofs, v3d->ofs, obofs);
						QuatMulVecf(q1, v3d->ofs);
						VecAddf(v3d->ofs, v3d->ofs, obofs);
					}
				}
				
				/* check for view snap */
				if (G.qual==LR_CTRLKEY){
					int i;
					float viewmat[3][3];

					static const float thres = 0.93f; //cos(20 deg);
					
					static float snapquats[39][6] = {
						/*{q0, q1, q3, q4, view, oposite_direction}*/
						{COS45, -SIN45, 0.0, 0.0, 1, 0},  //front
						{0.0, 0.0, -SIN45, -SIN45, 1, 1}, //back
						{1.0, 0.0, 0.0, 0.0, 7, 0},       //top
						{0.0, -1.0, 0.0, 0.0, 7, 1},      //bottom
						{0.5, -0.5, -0.5, -0.5, 3, 0},    //left
						{0.5, -0.5, 0.5, 0.5, 3, 1},      //right
						
						/* some more 45 deg snaps */
						{0.65328145027160645, -0.65328145027160645, 0.27059805393218994, 0.27059805393218994, 0, 0},
						{0.92387950420379639, 0.0, 0.0, 0.38268342614173889, 0, 0},
						{0.0, -0.92387950420379639, 0.38268342614173889, 0.0, 0, 0},
						{0.35355335474014282, -0.85355335474014282, 0.35355338454246521, 0.14644660055637360, 0, 0},
						{0.85355335474014282, -0.35355335474014282, 0.14644660055637360, 0.35355338454246521, 0, 0},
						{0.49999994039535522, -0.49999994039535522, 0.49999997019767761, 0.49999997019767761, 0, 0},
						{0.27059802412986755, -0.65328145027160645, 0.65328145027160645, 0.27059802412986755, 0, 0},
						{0.65328145027160645, -0.27059802412986755, 0.27059802412986755, 0.65328145027160645, 0, 0},
						{0.27059799432754517, -0.27059799432754517, 0.65328139066696167, 0.65328139066696167, 0, 0},
						{0.38268336653709412, 0.0, 0.0, 0.92387944459915161, 0, 0},
						{0.0, -0.38268336653709412, 0.92387944459915161, 0.0, 0, 0},
						{0.14644658565521240, -0.35355335474014282, 0.85355335474014282, 0.35355335474014282, 0, 0},
						{0.35355335474014282, -0.14644658565521240, 0.35355335474014282, 0.85355335474014282, 0, 0},
						{0.0, 0.0, 0.92387944459915161, 0.38268336653709412, 0, 0},
						{-0.0, 0.0, 0.38268336653709412, 0.92387944459915161, 0, 0},
						{-0.27059802412986755, 0.27059802412986755, 0.65328133106231689, 0.65328133106231689, 0, 0},
						{-0.38268339633941650, 0.0, 0.0, 0.92387938499450684, 0, 0},
						{0.0, 0.38268339633941650, 0.92387938499450684, 0.0, 0, 0},
						{-0.14644658565521240, 0.35355338454246521, 0.85355329513549805, 0.35355332493782043, 0, 0},
						{-0.35355338454246521, 0.14644658565521240, 0.35355332493782043, 0.85355329513549805, 0, 0},
						{-0.49999991059303284, 0.49999991059303284, 0.49999985098838806, 0.49999985098838806, 0, 0},
						{-0.27059799432754517, 0.65328145027160645, 0.65328139066696167, 0.27059799432754517, 0, 0},
						{-0.65328145027160645, 0.27059799432754517, 0.27059799432754517, 0.65328139066696167, 0, 0},
						{-0.65328133106231689, 0.65328133106231689, 0.27059793472290039, 0.27059793472290039, 0, 0},
						{-0.92387932538986206, 0.0, 0.0, 0.38268333673477173, 0, 0},
						{0.0, 0.92387932538986206, 0.38268333673477173, 0.0, 0, 0},
						{-0.35355329513549805, 0.85355329513549805, 0.35355329513549805, 0.14644657075405121, 0, 0},
						{-0.85355329513549805, 0.35355329513549805, 0.14644657075405121, 0.35355329513549805, 0, 0},
						{-0.38268330693244934, 0.92387938499450684, 0.0, 0.0, 0, 0},
						{-0.92387938499450684, 0.38268330693244934, 0.0, 0.0, 0, 0},
						{-COS45, 0.0, 0.0, SIN45, 0, 0},
						{COS45, 0.0, 0.0, SIN45, 0, 0},
						{0.0, 0.0, 0.0, 1.0, 0, 0}
					};

					QuatToMat3(v3d->viewquat, viewmat);

					for (i = 0 ; i < 39; i++){
						float snapmat[3][3];
						float view = (int)snapquats[i][4];
						float oposite_dir = (int)snapquats[i][5];
						
						QuatToMat3(snapquats[i], snapmat);
						
						if ((Inpf(snapmat[0], viewmat[0]) > thres) &&
							(Inpf(snapmat[1], viewmat[1]) > thres) &&
							(Inpf(snapmat[2], viewmat[2]) > thres)){
							
							QUATCOPY(v3d->viewquat, snapquats[i]);
							
							v3d->view = view;
							if (view){
								if (oposite_dir){
									v3d->flag2 |= V3D_OPP_DIRECTION_NAME;
								}else{
									v3d->flag2 &= ~V3D_OPP_DIRECTION_NAME;
								}
							}
							
							break;
						}
					}
				}
			}
			else if(mode==1) {	/* translate */
				if(v3d->persp==V3D_CAMOB) {
					float max= (float)MAX2(ar->winx, ar->winy);

					v3d->camdx += (mvalo[0]-mval[0])/(max);
					v3d->camdy += (mvalo[1]-mval[1])/(max);
					CLAMP(v3d->camdx, -1.0f, 1.0f);
					CLAMP(v3d->camdy, -1.0f, 1.0f);
					preview3d_event= 0;
				}
				else {
					window_to_3d(ar, v3d, dvec, mval[0]-mvalo[0], mval[1]-mvalo[1]);
					VecAddf(v3d->ofs, v3d->ofs, dvec);
				}
			}
			else if(mode==2) {
				float zfac=1.0;

				/* use initial value (do not use mvalo (that is used to detect mouse moviments)) */
				mvalo[0] = mvali[0];
				mvalo[1] = mvali[1];
				
				if(U.viewzoom==USER_ZOOM_CONT) {
					// oldstyle zoom
					zfac = 1.0+(float)(mvalo[0]-mval[0]+mvalo[1]-mval[1])/1000.0;
				}
				else if(U.viewzoom==USER_ZOOM_SCALE) {
					int ctr[2], len1, len2;
					// method which zooms based on how far you move the mouse
					
					ctr[0] = (ar->winrct.xmax + ar->winrct.xmin)/2;
					ctr[1] = (ar->winrct.ymax + ar->winrct.ymin)/2;
					
					len1 = (int)sqrt((ctr[0] - mval[0])*(ctr[0] - mval[0]) + (ctr[1] - mval[1])*(ctr[1] - mval[1])) + 5;
					len2 = (int)sqrt((ctr[0] - mvalo[0])*(ctr[0] - mvalo[0]) + (ctr[1] - mvalo[1])*(ctr[1] - mvalo[1])) + 5;
					
					zfac = dist0 * ((float)len2/len1) / v3d->dist;
				}
				else {	/* USER_ZOOM_DOLLY */
					float len1 = (ar->winrct.ymax - mval[1]) + 5;
					float len2 = (ar->winrct.ymax - mvalo[1]) + 5;
					zfac = dist0 * (2.0*((len2/len1)-1.0) + 1.0) / v3d->dist;
				}

				if(zfac != 1.0 && zfac*v3d->dist > 0.001*v3d->grid && 
					zfac*v3d->dist < 10.0*v3d->far)
					view_zoom_mouseloc(ar, v3d, zfac, mval_area);
				
				
				if ((U.uiflag & USER_ORBIT_ZBUF) && (U.viewzoom==USER_ZOOM_CONT) && (v3d->persp==V3D_PERSP)) {
					/* Secret apricot feature, translate the view when in continues mode */
					upvec[0] = upvec[1] = 0;
					upvec[2] = (dist0 - v3d->dist) * v3d->grid;
					v3d->dist = dist0;
					Mat3CpyMat4(mat, v3d->viewinv);
					Mat3MulVecfl(mat, upvec);
					VecAddf(v3d->ofs, v3d->ofs, upvec);
				} else {
					/* these limits are in toets.c too */
					if(v3d->dist<0.001*v3d->grid) v3d->dist= 0.001*v3d->grid;
					if(v3d->dist>10.0*v3d->far) v3d->dist=10.0*v3d->far;
				}
				
				if(v3d->persp==V3D_ORTHO || v3d->persp==V3D_CAMOB) preview3d_event= 0;
			}
			
			
			
			mvalo[0]= mval[0];
			mvalo[1]= mval[1];

// XXX			if(G.f & G_PLAYANIM) inner_play_anim_loop(0, 0);

			/* If in retopo paint mode, update lines */
			if(retopo_mesh_paint_check() && v3d->retopo_view_data) {
				v3d->retopo_view_data->queue_matrix_update= 1;
				retopo_paint_view_update(v3d);
			}

//	XXX		scrarea_do_windraw(ar);
// XXX			screen_swapbuffers();
		}
		else {
//			short val;
//			unsigned short event;
			/* we need to empty the queue... when you do this very long it overflows */
// XX			while(qtest()) event= extern_qread(&val);
			
// XXX			BIF_wait_for_statechange();
		}
		
		/* this in the end, otherwise get_mbut does not work on a PC... */
// XXX		if( !(get_mbut() & (L_MOUSE|M_MOUSE))) break;
	}

	if(v3d->depths) v3d->depths->damaged= 1;
// XXX	retopo_queue_updates(v3d);
// XXX	allqueue(REDRAWVIEW3D, 0);

// XXX	if(preview3d_event) 
//		BIF_view3d_previewrender_signal(ar, PR_DBASE|PR_DISPRECT);
//	else
//		BIF_view3d_previewrender_signal(ar, PR_PROJECTED);

}
 

void viewmoveNDOF(Scene *scene, View3D *v3d, int mode)
{
    float fval[7];
    float dvec[3];
    float sbadjust = 1.0f;
    float len;
	short use_sel = 0;
	Object *ob = OBACT;
    float m[3][3];
    float m_inv[3][3];
    float xvec[3] = {1,0,0};
    float yvec[3] = {0,-1,0};
    float zvec[3] = {0,0,1};
	float phi, si;
    float q1[4];
    float obofs[3];
    float reverse;
    //float diff[4];
    float d, curareaX, curareaY;
    float mat[3][3];
    float upvec[3];

    /* Sensitivity will control how fast the view rotates.  The value was
     * obtained experimentally by tweaking until the author didn't get dizzy watching.
     * Perhaps this should be a configurable user parameter. 
     */
    float psens = 0.005f * (float) U.ndof_pan;   /* pan sensitivity */
    float rsens = 0.005f * (float) U.ndof_rotate;  /* rotate sensitivity */
    float zsens = 0.3f;   /* zoom sensitivity */

    const float minZoom = -30.0f;
    const float maxZoom = 300.0f;

	//reset view type
	v3d->view = 0;
//printf("passing here \n");
//
	if (G.obedit==NULL && ob && !(ob->flag & OB_POSEMODE)) {
		use_sel = 1;
	}

    if((dz_flag)||v3d->dist==0) {
		dz_flag = 0;
		v3d->dist = m_dist;
		upvec[0] = upvec[1] = 0;
		upvec[2] = v3d->dist;
		Mat3CpyMat4(mat, v3d->viewinv);
		Mat3MulVecfl(mat, upvec);
		VecAddf(v3d->ofs, v3d->ofs, upvec);
	}

    /*----------------------------------------------------
	 * sometimes this routine is called from headerbuttons
     * viewmove needs to refresh the screen
     */
// XXX	areawinset(curarea->win);

    /*----------------------------------------------------
     * record how much time has passed. clamp at 10 Hz
     * pretend the previous frame occured at the clamped time 
     */
//    now = PIL_check_seconds_timer();
 //   frametime = (now - prevTime);
 //   if (frametime > 0.1f){        /* if more than 1/10s */
 //       frametime = 1.0f/60.0;      /* clamp at 1/60s so no jumps when starting to move */
//    }
//    prevTime = now;
 //   sbadjust *= 60 * frametime;             /* normalize ndof device adjustments to 100Hz for framerate independence */

    /* fetch the current state of the ndof device & enforce dominant mode if selected */
// XXX    getndof(fval);
	if (v3d->ndoffilter)
		filterNDOFvalues(fval);
	
	
    // put scaling back here, was previously in ghostwinlay
    fval[0] = fval[0] * (1.0f/600.0f);
    fval[1] = fval[1] * (1.0f/600.0f);
    fval[2] = fval[2] * (1.0f/1100.0f);
    fval[3] = fval[3] * 0.00005f;
    fval[4] =-fval[4] * 0.00005f;
    fval[5] = fval[5] * 0.00005f;
    fval[6] = fval[6] / 1000000.0f;
			
    // scale more if not in perspective mode
    if (v3d->persp == V3D_ORTHO) {
        fval[0] = fval[0] * 0.05f;
        fval[1] = fval[1] * 0.05f;
        fval[2] = fval[2] * 0.05f;
        fval[3] = fval[3] * 0.9f;
        fval[4] = fval[4] * 0.9f;
        fval[5] = fval[5] * 0.9f;
        zsens *= 8;
    }
			
	
    /* set object offset */
	if (ob) {
		obofs[0] = -ob->obmat[3][0];
		obofs[1] = -ob->obmat[3][1];
		obofs[2] = -ob->obmat[3][2];
	}
	else {
		VECCOPY(obofs, v3d->ofs);
	}

    /* calc an adjustment based on distance from camera
       disabled per patch 14402 */
     d = 1.0f;

/*    if (ob) {
        VecSubf(diff, obofs, v3d->ofs);
        d = VecLength(diff);
    }
*/

    reverse = (v3d->persmat[2][1] < 0.0f) ? -1.0f : 1.0f;

    /*----------------------------------------------------
     * ndof device pan 
     */
    psens *= 1.0f + d;
    curareaX = sbadjust * psens * fval[0];
    curareaY = sbadjust * psens * fval[1];
    dvec[0] = curareaX * v3d->persinv[0][0] + curareaY * v3d->persinv[1][0];
    dvec[1] = curareaX * v3d->persinv[0][1] + curareaY * v3d->persinv[1][1];
    dvec[2] = curareaX * v3d->persinv[0][2] + curareaY * v3d->persinv[1][2];
    VecAddf(v3d->ofs, v3d->ofs, dvec);

    /*----------------------------------------------------
     * ndof device dolly 
     */
    len = zsens * sbadjust * fval[2];

    if (v3d->persp==V3D_CAMOB) {
        if(v3d->persp==V3D_CAMOB) { /* This is stupid, please fix - TODO */
            v3d->camzoom+= 10.0f * -len;
        }
        if (v3d->camzoom < minZoom) v3d->camzoom = minZoom;
        else if (v3d->camzoom > maxZoom) v3d->camzoom = maxZoom;
    }
    else if ((v3d->dist> 0.001*v3d->grid) && (v3d->dist<10.0*v3d->far)) {
        v3d->dist*=(1.0 + len);
    }


    /*----------------------------------------------------
     * ndof device turntable
     * derived from the turntable code in viewmove
     */

    /* Get the 3x3 matrix and its inverse from the quaternion */
    QuatToMat3(v3d->viewquat, m);
    Mat3Inv(m_inv,m);

    /* Determine the direction of the x vector (for rotating up and down) */
    /* This can likely be compuated directly from the quaternion. */
    Mat3MulVecfl(m_inv,xvec);
    Mat3MulVecfl(m_inv,yvec);
    Mat3MulVecfl(m_inv,zvec);

    /* Perform the up/down rotation */
    phi = sbadjust * rsens * /*0.5f * */ fval[3]; /* spin vertically half as fast as horizontally */
    si = sin(phi);
    q1[0] = cos(phi);
    q1[1] = si * xvec[0];
    q1[2] = si * xvec[1];
    q1[3] = si * xvec[2];
    QuatMul(v3d->viewquat, v3d->viewquat, q1);

    if (use_sel) {
        QuatConj(q1); /* conj == inv for unit quat */
        VecSubf(v3d->ofs, v3d->ofs, obofs);
        QuatMulVecf(q1, v3d->ofs);
        VecAddf(v3d->ofs, v3d->ofs, obofs);
    }

    /* Perform the orbital rotation */
    /* Perform the orbital rotation 
       If the seen Up axis is parallel to the zoom axis, rotation should be
       achieved with a pure Roll motion (no Spin) on the device. When you start 
       to tilt, moving from Top to Side view, Spinning will increasingly become 
       more relevant while the Roll component will decrease. When a full 
       Side view is reached, rotations around the world's Up axis are achieved
       with a pure Spin-only motion.  In other words the control of the spinning
       around the world's Up axis should move from the device's Spin axis to the
       device's Roll axis depending on the orientation of the world's Up axis 
       relative to the screen. */
    //phi = sbadjust * rsens * reverse * fval[4];  /* spin the knob, y axis */
    phi = sbadjust * rsens * (yvec[2] * fval[4] + zvec[2] * fval[5]);
    q1[0] = cos(phi);
    q1[1] = q1[2] = 0.0;
    q1[3] = sin(phi);
    QuatMul(v3d->viewquat, v3d->viewquat, q1);

    if (use_sel) {
        QuatConj(q1);
        VecSubf(v3d->ofs, v3d->ofs, obofs);
        QuatMulVecf(q1, v3d->ofs);
        VecAddf(v3d->ofs, v3d->ofs, obofs);
    }

    /*----------------------------------------------------
     * refresh the screen
     */
// XXX    scrarea_do_windraw(curarea);
}




