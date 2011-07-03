/**
***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * Contributor(s): Miika Hämäläinen
 *
 * ***** END GPL LICENSE BLOCK *****
 */


#include "MEM_guardedalloc.h"

#include <math.h>
#include <stdio.h>

#include "BLI_blenlib.h"
#include "BLI_math.h"
#include "BLI_kdtree.h"
#include "BLI_utildefines.h"

/* Platform independend time	*/
#include "PIL_time.h"

#include "BKE_animsys.h"
#include "BKE_bvhutils.h"	/* bvh tree	*/
#include "BKE_blender.h"
#include "BKE_cdderivedmesh.h"
#include "BKE_context.h"
#include "BKE_customdata.h"
#include "BKE_colortools.h"
#include "BKE_deform.h"
#include "BKE_depsgraph.h"
#include "BKE_DerivedMesh.h"
#include "BKE_dynamicpaint.h"
#include "BKE_effect.h"
#include "BKE_global.h"
#include "BKE_main.h"
#include "BKE_material.h"
#include "BKE_modifier.h"
#include "BKE_object.h"
#include "BKE_particle.h"
#include "BKE_pointcache.h"
#include "BKE_report.h"
#include "BKE_scene.h"
#include "BKE_texture.h"

#include "DNA_anim_types.h"
#include "DNA_dynamicpaint_types.h"
#include "DNA_group_types.h" /*GroupObject*/
#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_modifier_types.h"
#include "DNA_object_types.h"
#include "DNA_scene_types.h"
#include "DNA_userdef_types.h"	/* to get temp file path	*/

/* for bake operator	*/
#include "ED_screen.h"
#include "WM_types.h"
#include "WM_api.h"

/* for image output	*/
#include "IMB_imbuf_types.h"
#include "IMB_imbuf.h"
#include "BKE_image.h"
#include "intern/IMB_filetype.h"
#ifdef WITH_OPENEXR
#include "intern/openexr/openexr_api.h"
#endif

/* uv validate	*/
#include "intern/MOD_util.h"

/* to read object material color	*/
#include "DNA_texture_types.h"
#include "../render/intern/include/render_types.h"
#include "../render/intern/include/voxeldata.h"
#include "DNA_material_types.h"
#include "RE_render_ext.h"


#define DPOUTPUT_JPEG 0
#define DPOUTPUT_PNG 1
#define DPOUTPUT_OPENEXR 2

struct Object;
struct Scene;
struct DerivedMesh;

/*
*	Init predefined antialias jitter data
*/
float jitterDistances[5] = {0.0f,
							0.447213595f,
							0.447213595f,
							0.447213595f,
							0.5f};

/* precalculated gaussian factors for 5x super sampling	*/
float gaussianFactors[5] = {	0.996849f,
								0.596145f,
								0.596145f,
								0.596145f,
								0.524141f};
float gaussianTotal = 3.309425f;

/*
*	UV Image neighbouring pixel table x and y list
*/
int neighX[8] = {1,1,0,-1,-1,-1, 0, 1};
int neighY[8] = {0,1,1, 1, 0,-1,-1,-1};

static int dynamicPaint_doStep(Scene *scene, Object *ob, DynamicPaintSurface *surface, float timescale, float subframe);
static int dynamicPaint_calculateFrame(DynamicPaintSurface *surface, Scene *scene, Object *cObject, int frame);

/***************************** Internal Structs ***************************/

typedef struct FaceAdv {
	float no[3];
	float no_q[3];
} FaceAdv;

typedef struct BB2d {
	float min[2], max[2];
} BB2d;

typedef struct Vec3f {
	float v[3];
} Vec3f;

/* Surface data used while processing a frame	*/
typedef struct PaintBakePoint {
	float invNorm[3];  /* current pixel world-space inverted normal. depends on face shading mode */
	float normal_scale; /* normal directional scale for displace mapping */
} PaintBakePoint;

typedef struct PaintBakeData {
	PaintBakePoint *bPoint;
	float *realCoord;  /* current pixel center world-space coordinates * numOfSamples
					   *  ordered as (total_points*sample+index)*3*/
} PaintBakeData;

/* UV Image sequence format point	*/
typedef struct PaintUVPoint {
	/* Pixel / mesh data */
	unsigned int face_index, pixel_index;		/* face index on domain derived mesh */
	unsigned int v1, v2, v3;		/* vertex indexes */

	unsigned int neighbour_pixel;	/* If this pixel isn't uv mapped to any face,
							but it's neighbouring pixel is */
	short quad;
} PaintUVPoint;

typedef struct ImgSeqFormatData {
	PaintUVPoint *uv_p;
	Vec3f *barycentricWeights;	/* b-weights for all pixel samples */
} ImgSeqFormatData;

typedef struct EffVelPoint {
	float previous_pos[3];
	float previous_vel[3];
} EffVelPoint;

typedef struct BakeNeighPoint {
	float dir[3];	/* vector pointing towards this neighbour */
	float dist;		/* distance to */
} BakeNeighPoint;


/* adjacency data flags */
#define ADJ_ON_MESH_EDGE (1<<0)

typedef struct PaintAdjData {
	unsigned int *n_index;		/* total_points sized index array */
	unsigned int *numOf_n;		/* num of neighs for each point */
	unsigned int *n_target;		/* array of neighbouring point indexes
							   for single sample use n_index+neigh_num */
	unsigned int *flags;		/* for each point */
	unsigned int total_targets; /* size of n_target */
} PaintAdjData;

/***************************** General Utils ******************************/

/*
*	Output error message to both ui and console
*/
static int printError(DynamicPaintCanvasSettings *canvas, char *string)
{
	if (strlen(string)>64) string[63] = '\0';

	/* Add error to canvas ui info label */
	sprintf(canvas->error, string);

	/* Print console output */
	printf("DynamicPaint bake failed: %s\n", canvas->error);

	return 0;
}

/* Get number of surface points for cached types */
static int dynamicPaint_surfaceNumOfPoints(DynamicPaintSurface *surface)
{
	if (surface->format == MOD_DPAINT_SURFACE_F_PTEX) {
		return 0; /* not supported atm */
	}
	else if (surface->format == MOD_DPAINT_SURFACE_F_VERTEX) {
		if (!surface->canvas->dm) return 0; /* invalid derived mesh */
		return surface->canvas->dm->getNumVerts(surface->canvas->dm);
	}
	else
		return 0;
}

/* checks whether surface's format/type has realtime preview */
int dynamicPaint_surfaceHasColorPreview(DynamicPaintSurface *surface) {
	if (surface->format == MOD_DPAINT_SURFACE_F_IMAGESEQ) return 0;
	else if (surface->format == MOD_DPAINT_SURFACE_F_VERTEX) {
		if (surface->type == MOD_DPAINT_SURFACE_T_DISPLACE ||
			surface->type == MOD_DPAINT_SURFACE_T_WAVE) return 0;
		else return 1;
	}
	else return 1;
}

/* get currently active surface (in user interface) */
static DynamicPaintSurface *get_activeSurface(DynamicPaintCanvasSettings *canvas)
{
	DynamicPaintSurface *surface = canvas->surfaces.first;
	int i;

	for(i=0; surface; surface=surface->next) {
		if(i == canvas->active_sur)
			return surface;
		i++;
	}
	return NULL;
}

/* set preview to first previewable surface */
static void dynamicPaint_resetPreview(DynamicPaintCanvasSettings *canvas)
{
	DynamicPaintSurface *surface = canvas->surfaces.first;
	int done=0;

	for(; surface; surface=surface->next) {
		if (!done && dynamicPaint_surfaceHasColorPreview(surface)) {
			surface->flags |= MOD_DPAINT_PREVIEW;
			done=1;
		}
		else
			surface->flags &= ~MOD_DPAINT_PREVIEW;
	}
}

/* set preview to defined surface */
static void dynamicPaint_setPreview(DynamicPaintSurface *t_surface)
{
	DynamicPaintSurface *surface = t_surface->canvas->surfaces.first;
	for(; surface; surface=surface->next) {
		if (surface == t_surface)
			surface->flags |= MOD_DPAINT_PREVIEW;
		else
			surface->flags &= ~MOD_DPAINT_PREVIEW;
	}
}

/* change surface data to defaults on new type */
void dynamicPaintSurface_updateType(struct DynamicPaintSurface *surface) {
	if (surface->format == MOD_DPAINT_SURFACE_F_IMAGESEQ) {
		surface->output_name[0]='\0';
		surface->output_name2[0]='\0';
	}
	else {
		sprintf(surface->output_name, "dp_");
		strcpy(surface->output_name2,surface->output_name);
	}

	if (surface->type == MOD_DPAINT_SURFACE_T_PAINT) {
		strcat(surface->output_name,"paintmap");
		strcat(surface->output_name2,"wetmap");
	}
	else if (surface->type == MOD_DPAINT_SURFACE_T_DISPLACE) {
		strcat(surface->output_name,"displace");
	}
	else if (surface->type == MOD_DPAINT_SURFACE_T_WEIGHT) {
		strcat(surface->output_name,"weight");
	}
	else if (surface->type == MOD_DPAINT_SURFACE_T_WAVE) {
		strcat(surface->output_name,"wave");
		strcat(surface->output_name2,"foam");
	}

	/* update preview */
	if (dynamicPaint_surfaceHasColorPreview(surface))
		dynamicPaint_setPreview(surface);
	else
		dynamicPaint_resetPreview(surface->canvas);
}

static int surfaceDublicateNameExists(void *arg, const char *name)
{
	DynamicPaintSurface *t_surface = (DynamicPaintSurface*)arg;
	DynamicPaintSurface *surface = t_surface->canvas->surfaces.first;

	for(; surface; surface=surface->next) {
		if (surface!=t_surface && !strcmp(name, surface->name)) return 1;
	}
	return 0;
}

void dynamicPaintSurface_setUniqueName(DynamicPaintSurface *surface, char *basename) {
	char name[64];
	strncpy(name, basename, 62); /* in case basename is surface->name use a copy */
	BLI_uniquename_cb(surfaceDublicateNameExists, surface, name, '.', surface->name, sizeof(surface->name));
}

/* assumes source alpha > 0.0f */
static void mixColors(float *t_color, float t_alpha, float *s_color, float s_alpha)
{
	float invFact = (s_alpha<t_alpha) ? 1.0f : t_alpha/s_alpha;
	float factor = 1.0f - invFact;
	/* set initial color depending on existing alpha */
	t_color[0] = t_color[0]*invFact + s_color[0]*factor;
	t_color[1] = t_color[1]*invFact + s_color[1]*factor;
	t_color[2] = t_color[2]*invFact + s_color[2]*factor;

	/* mix final color */
	factor = s_alpha;
	invFact = 1.0f - factor;
	t_color[0] = t_color[0]*invFact + s_color[0]*factor;
	t_color[1] = t_color[1]*invFact + s_color[1]*factor;
	t_color[2] = t_color[2]*invFact + s_color[2]*factor;
}

/***************************** Freeing data ******************************/

/* Free brush data */
static void dynamicPaint_freeBrush(struct DynamicPaintModifierData *pmd)
{
	if(pmd->brush) {
		if(pmd->brush->dm)
			pmd->brush->dm->release(pmd->brush->dm);
		pmd->brush->dm = NULL;

		if(pmd->brush->paint_ramp)
			 MEM_freeN(pmd->brush->paint_ramp);
		pmd->brush->paint_ramp = NULL;

		MEM_freeN(pmd->brush);
		pmd->brush = NULL;
	}
}

static void dynamicPaint_freeEffectData(PaintSurfaceData *data)
{
	if (data->adj_data) {
		if (data->adj_data->n_index) MEM_freeN(data->adj_data->n_index);
		if (data->adj_data->numOf_n) MEM_freeN(data->adj_data->numOf_n);
		if (data->adj_data->n_target) MEM_freeN(data->adj_data->n_target);
		if (data->adj_data->flags) MEM_freeN(data->adj_data->flags);
		MEM_freeN(data->adj_data);
	}
}

static void dynamicPaint_freeSurfaceData(DynamicPaintSurface *surface)
{
	PaintSurfaceData *data = surface->data;
	if (!data) return;
	if (data->format_data) {
		/* format specific free */
		if (surface->format == MOD_DPAINT_SURFACE_F_IMAGESEQ) {
			ImgSeqFormatData *format_data = (ImgSeqFormatData*)data->format_data;
			if (format_data->uv_p)
				MEM_freeN(format_data->uv_p);
			if (format_data->barycentricWeights)
				MEM_freeN(format_data->barycentricWeights);
		}
		MEM_freeN(data->format_data);
	}
	if (data->type_data) MEM_freeN(data->type_data);
	dynamicPaint_freeEffectData(data);
	MEM_freeN(surface->data);
	surface->data = NULL;
}

static void dynamicPaint_freeSurface(DynamicPaintSurface *surface)
{
	if (!surface) return;

	/* point cache */
	BKE_ptcache_free_list(&(surface->ptcaches));
	surface->pointcache = NULL;

	if(surface->effector_weights)
		MEM_freeN(surface->effector_weights);
	surface->effector_weights = NULL;

	BLI_remlink(&(surface->canvas->surfaces), surface);
	dynamicPaint_freeSurfaceData(surface);
	MEM_freeN(surface);
}

/* Free canvas data */
static void dynamicPaint_freeCanvas(DynamicPaintModifierData *pmd)
{
	if(pmd->canvas) {
		/* Free surface data */
		DynamicPaintSurface *surface = pmd->canvas->surfaces.first;
		DynamicPaintSurface *next_surface = NULL;

		while (surface) {
			next_surface = surface->next;
			dynamicPaint_freeSurface(surface);
			surface = next_surface;
		}

		/* free dm copy */
		if (pmd->canvas->dm)
			pmd->canvas->dm->release(pmd->canvas->dm);
		pmd->canvas->dm = NULL;

		MEM_freeN(pmd->canvas);
		pmd->canvas = NULL;
	}
}

/* Free whole dp modifier */
void dynamicPaint_Modifier_free(struct DynamicPaintModifierData *pmd)
{
	if(pmd) {
		dynamicPaint_freeCanvas(pmd);
		dynamicPaint_freeBrush(pmd);
	}
}


/***************************** Initialize and reset ******************************/

/*
*	Creates a new surface and adds it to the list
*	A pointer to this surface is returned
*/
static DynamicPaintSurface *dynamicPaint_createNewSurface(DynamicPaintCanvasSettings *canvas)
{
	DynamicPaintSurface *surface= MEM_callocN(sizeof(DynamicPaintSurface), "DynamicPaintSurface");
	if (!surface) return NULL;

	surface->canvas = canvas;
	surface->format = MOD_DPAINT_SURFACE_F_VERTEX;
	surface->type = MOD_DPAINT_SURFACE_T_PAINT;

	/* cache */
	surface->pointcache = BKE_ptcache_add(&(surface->ptcaches));
	surface->pointcache->flag |= PTCACHE_DISK_CACHE;
	surface->pointcache->step = 1;

	/* Set initial values */
	surface->flags = MOD_DPAINT_ANTIALIAS | MOD_DPAINT_MULALPHA | MOD_DPAINT_DRY_LOG | MOD_DPAINT_DISSOLVE_LOG |
					 MOD_DPAINT_ACTIVE | MOD_DPAINT_PREVIEW | MOD_DPAINT_OUT1;
	surface->effect = 0;
	surface->effect_ui = 1;

	surface->diss_speed = 300;
	surface->dry_speed = 300;
	surface->disp_depth = 1.0f;
	surface->disp_type = MOD_DPAINT_DISP_DISPLACE;
	surface->image_fileformat = MOD_DPAINT_IMGFORMAT_PNG;

	surface->image_resolution = 256;
	surface->start_frame = 1;
	surface->end_frame = 250;
	surface->substeps = 0;

	surface->spread_speed = 1.0f;
	surface->shrink_speed = 1.0f;

	surface->wave_damping = 0.05f;
	surface->wave_speed = 0.8f;
	surface->wave_timescale = 1.0f;
	surface->wave_spring = 0.20;

	sprintf(surface->image_output_path, "%sdynamicpaint/", "/tmp/");
	dynamicPaintSurface_setUniqueName(surface, "Surface");

	surface->effector_weights = BKE_add_effector_weights(NULL);

	dynamicPaintSurface_updateType(surface);

	BLI_addtail(&canvas->surfaces, surface);

	return surface;
}

/*
*	Initialize modifier data
*/
void dynamicPaint_Modifier_createType(struct DynamicPaintModifierData *pmd)
{
	if(pmd)
	{
		if(pmd->type & MOD_DYNAMICPAINT_TYPE_CANVAS)
		{
			if(pmd->canvas)
				dynamicPaint_freeCanvas(pmd);

			pmd->canvas = MEM_callocN(sizeof(DynamicPaintCanvasSettings), "DynamicPaint Canvas");
			pmd->canvas->pmd = pmd;
			pmd->canvas->dm = NULL;

			/* Create one surface */
			dynamicPaint_createNewSurface(pmd->canvas);

			pmd->canvas->ui_info[0] = '\0';

		}
		else if(pmd->type & MOD_DYNAMICPAINT_TYPE_BRUSH)
		{
			if(pmd->brush)
				dynamicPaint_freeBrush(pmd);

			pmd->brush = MEM_callocN(sizeof(DynamicPaintBrushSettings), "DynamicPaint Paint");
			pmd->brush->pmd = pmd;

			pmd->brush->psys = NULL;

			pmd->brush->flags = MOD_DPAINT_ABS_ALPHA;
			pmd->brush->collision = MOD_DPAINT_COL_VOLUME;
			
			pmd->brush->mat = NULL;
			pmd->brush->r = 1.0f;
			pmd->brush->g = 1.0f;
			pmd->brush->b = 1.0f;
			pmd->brush->alpha = 1.0f;
			pmd->brush->wetness = 1.0f;

			pmd->brush->paint_distance = 0.1f;
			pmd->brush->proximity_falloff = MOD_DPAINT_PRFALL_SMOOTH;

			pmd->brush->displace_distance = 0.5f;
			pmd->brush->prox_displace_strength = 0.5f;

			pmd->brush->particle_radius = 0.2f;
			pmd->brush->particle_smooth = 0.05f;

			pmd->brush->wave_factor = 1.0f;

			pmd->brush->dm = NULL;

			/*
			*	Paint proximity falloff colorramp.
			*/
			{
				CBData *ramp;

				pmd->brush->paint_ramp = add_colorband(0);
				ramp = pmd->brush->paint_ramp->data;
				/* Add default smooth-falloff ramp.	*/
				ramp[0].r = ramp[0].g = ramp[0].b = ramp[0].a = 1.0f;
				ramp[0].pos = 0.0f;
				ramp[1].r = ramp[1].g = ramp[1].b = ramp[1].pos = 1.0f;
				ramp[1].a = 0.0f;
				pmd->brush->paint_ramp->tot = 2;
			}
		}
	}
}

void dynamicPaint_Modifier_copy(struct DynamicPaintModifierData *pmd, struct DynamicPaintModifierData *tpmd)
{
	/* Init modifier	*/
	tpmd->type = pmd->type;
	dynamicPaint_Modifier_createType(tpmd);

	/* Copy data	*/
	if (tpmd->canvas) {
		pmd->canvas->pmd = tpmd;

		tpmd->canvas->ui_info[0] = '\0';

		/*tpmd->canvas->flags = pmd->canvas->flags;
		tpmd->canvas->output = pmd->canvas->output;
		tpmd->canvas->disp_type = pmd->canvas->disp_type;
		tpmd->canvas->disp_format = pmd->canvas->disp_format;
		tpmd->canvas->effect = pmd->canvas->effect;
		tpmd->canvas->effect_ui = 1;

		tpmd->canvas->resolution = pmd->canvas->resolution;
		tpmd->canvas->start_frame = pmd->canvas->start_frame;
		tpmd->canvas->end_frame = pmd->canvas->end_frame;
		tpmd->canvas->substeps = pmd->canvas->substeps;

		tpmd->canvas->dry_speed = pmd->canvas->dry_speed;
		tpmd->canvas->diss_speed = pmd->canvas->diss_speed;
		tpmd->canvas->disp_depth = pmd->canvas->disp_depth;
		tpmd->canvas->dflat_speed = pmd->canvas->dflat_speed;

		strncpy(tpmd->canvas->paint_output_path, pmd->canvas->paint_output_path, 240);
		strncpy(tpmd->canvas->wet_output_path, pmd->canvas->wet_output_path, 240);
		strncpy(tpmd->canvas->displace_output_path, pmd->canvas->displace_output_path, 240);

		tpmd->canvas->spread_speed = pmd->canvas->spread_speed;
		tpmd->canvas->drip_speed = pmd->canvas->drip_speed;
		tpmd->canvas->shrink_speed = pmd->canvas->shrink_speed;

		strncpy(tpmd->canvas->uvlayer_name, tpmd->canvas->uvlayer_name, 32);*/

	} else if (tpmd->brush) {
		pmd->brush->pmd = tpmd;

		tpmd->brush->flags = pmd->brush->flags;
		tpmd->brush->collision = pmd->brush->collision;

		tpmd->brush->r = pmd->brush->r;
		tpmd->brush->g = pmd->brush->g;
		tpmd->brush->b = pmd->brush->b;
		tpmd->brush->alpha = pmd->brush->alpha;
		tpmd->brush->wetness = pmd->brush->wetness;

		tpmd->brush->particle_radius = pmd->brush->particle_radius;
		tpmd->brush->particle_smooth = pmd->brush->particle_smooth;
		tpmd->brush->paint_distance = pmd->brush->paint_distance;
		tpmd->brush->psys = pmd->brush->psys;
		tpmd->brush->displace_distance = pmd->brush->displace_distance;
		tpmd->brush->prox_displace_strength = pmd->brush->prox_displace_strength;

		tpmd->brush->paint_ramp = pmd->brush->paint_ramp;

		tpmd->brush->proximity_falloff = pmd->brush->proximity_falloff;
	}
}

/* allocates surface data depending on surface type */
static void dynamicPaint_allocateSurfaceType(DynamicPaintSurface *surface)
{
	PaintSurfaceData *sData = surface->data;

	if (surface->type == MOD_DPAINT_SURFACE_T_PAINT) {
		sData->type_data = MEM_callocN(sizeof(PaintPoint)*sData->total_points, "DynamicPaintSurface Data");
	}
	else if (surface->type == MOD_DPAINT_SURFACE_T_DISPLACE) {
		sData->type_data = MEM_callocN(sizeof(float)*sData->total_points, "DynamicPaintSurface DepthData");
	}
	else if (surface->type == MOD_DPAINT_SURFACE_T_WEIGHT) {
		sData->type_data = MEM_callocN(sizeof(float)*sData->total_points, "DynamicPaintSurface WeightData");
	}
	else if (surface->type == MOD_DPAINT_SURFACE_T_WAVE) {
		sData->type_data = MEM_callocN(sizeof(PaintWavePoint)*sData->total_points, "DynamicPaintSurface WaveData");
	}
	else return;

	if (sData->type_data == NULL) printError(surface->canvas, "Not enough free memory!");
}

static int surface_usesNeighData(DynamicPaintSurface *surface) {
	return (surface->type == MOD_DPAINT_SURFACE_T_PAINT ||
		    surface->type == MOD_DPAINT_SURFACE_T_WAVE);
}

/* initialize surface adjacency data */
static void dynamicPaint_initAdjacencyData(DynamicPaintSurface *surface) {
	PaintSurfaceData *sData = surface->data;
	PaintAdjData *ed;
	int *temp_data;
	int neigh_points = 0;

	if (!surface->effect && !surface_usesNeighData(surface)) return;

	if (surface->format == MOD_DPAINT_SURFACE_F_VERTEX) {
		/* For vertex format, neighbours are connected by edges */
		neigh_points = 2*surface->canvas->dm->getNumEdges(surface->canvas->dm);
	}
	else if (surface->format == MOD_DPAINT_SURFACE_F_IMAGESEQ)
		neigh_points = sData->total_points*8;

	if (!neigh_points) return;

	/* allocate memory */
	ed = sData->adj_data = MEM_callocN(sizeof(PaintAdjData), "Surface Adj Data");
	if (!ed) return;
	ed->n_index = MEM_callocN(sizeof(int)*sData->total_points, "Surface Adj Index");
	ed->numOf_n = MEM_callocN(sizeof(int)*sData->total_points, "Surface Adj Counts");
	temp_data = MEM_callocN(sizeof(int)*sData->total_points, "Temp Adj Data");
	ed->n_target = MEM_callocN(sizeof(int)*neigh_points, "Surface Adj Targets");
	ed->flags = MEM_callocN(sizeof(int)*sData->total_points, "Surface Adj Flags");
	ed->total_targets = neigh_points;

	/* in case of error, free allocated memory */
	if (!ed->n_index || !ed->numOf_n || !ed->n_target || !temp_data) {
		dynamicPaint_freeEffectData(sData);
		MEM_freeN(temp_data);
		printError(surface->canvas, "Not enough free memory.");
		return;
	}

	if (surface->format == MOD_DPAINT_SURFACE_F_VERTEX) {
		int i;
		int n_pos;

		/* For vertex format, count every vertex that is connected by an edge */
		int numOfEdges = surface->canvas->dm->getNumEdges(surface->canvas->dm);
		int numOfFaces = surface->canvas->dm->getNumFaces(surface->canvas->dm);
		struct MEdge *edge =  surface->canvas->dm->getEdgeArray(surface->canvas->dm);
		struct MFace *face =  surface->canvas->dm->getFaceArray(surface->canvas->dm);

		/* count number of edges per vertex */
		for (i=0; i<numOfEdges; i++) {
			ed->numOf_n[edge[i].v1]++;
			ed->numOf_n[edge[i].v2]++;

			temp_data[edge[i].v1]++;
			temp_data[edge[i].v2]++;
		}

		/* to locate points on "mesh edge" */
		for (i=0; i<numOfFaces; i++) {
			temp_data[face[i].v1]++;
			temp_data[face[i].v2]++;
			temp_data[face[i].v3]++;
			if (face[i].v4)
				temp_data[face[i].v4]++;
		}

		/* now check if total number of edges+faces for
		*  each vertex is even, if not -> vertex is on mesh edge */
		for (i=0; i<sData->total_points; i++) {
			if ((temp_data[i]%2) ||
				temp_data[i] < 4)
				ed->flags[i] |= ADJ_ON_MESH_EDGE;
				
			/* reset temp data */ 
			temp_data[i] = 0;
		}

		/* order n_index array */
		n_pos = 0;
		for (i=0; i<sData->total_points; i++) {
			ed->n_index[i] = n_pos;
			n_pos += ed->numOf_n[i];
		}

		/* and now add neighbour data using that info */
		for (i=0; i<numOfEdges; i++) {
			/* first vertex */
			int index = edge[i].v1;
			n_pos = ed->n_index[index]+temp_data[index];
			ed->n_target[n_pos] = edge[i].v2;
			temp_data[index]++;

			/* second vertex */
			index = edge[i].v2;
			n_pos = ed->n_index[index]+temp_data[index];
			ed->n_target[n_pos] = edge[i].v1;
			temp_data[index]++;
		}
	}
	else if (surface->format == MOD_DPAINT_SURFACE_F_IMAGESEQ) {
		/* for image sequences, only allocate memory.
		*  bake initialization takes care of rest */
	}

	MEM_freeN(temp_data);
}

/* clears surface data back to zero */
void dynamicPaint_clearSurface(DynamicPaintSurface *surface) {
	PaintSurfaceData *sData = surface->data;
	if (sData && sData->type_data) {
		unsigned int data_size;

		if (surface->type == MOD_DPAINT_SURFACE_T_PAINT)
			data_size = sizeof(PaintPoint);
		else if (surface->type == MOD_DPAINT_SURFACE_T_WAVE)
			data_size = sizeof(PaintWavePoint);
		else
			data_size = sizeof(float);

		memset(sData->type_data, 0, data_size * sData->total_points);
	}
}

/* completely (re)initializes surface (only for point cache types)*/
int dynamicPaint_resetSurface(DynamicPaintSurface *surface)
{
	int numOfPoints = dynamicPaint_surfaceNumOfPoints(surface);
	/* dont touch image sequence types. they get handled only on bake */
	if (surface->format == MOD_DPAINT_SURFACE_F_IMAGESEQ) return 1;

	if (surface->data) dynamicPaint_freeSurfaceData(surface);
	if (numOfPoints < 1) return 0;

	/* allocate memory */
	surface->data = MEM_callocN(sizeof(PaintSurfaceData), "PaintSurfaceData");
	if (!surface->data) return 0;

	/* allocate data depending on surface type and format */
	surface->data->total_points = numOfPoints;
	surface->data->samples = 1;
	dynamicPaint_allocateSurfaceType(surface);
	dynamicPaint_initAdjacencyData(surface);

	return 1;
}

/* make sure allocated surface size matches current requirements */
static void dynamicPaint_checkSurfaceData(DynamicPaintSurface *surface)
{
	if (!surface->data || ((dynamicPaint_surfaceNumOfPoints(surface) != surface->data->total_points))) {
		dynamicPaint_resetSurface(surface);
	}
}


/***************************** Modifier processing ******************************/


/* update cache frame range */
void dynamicPaint_cacheUpdateFrames(DynamicPaintSurface *surface) {
	if (surface->pointcache) {
		surface->pointcache->startframe = surface->start_frame;
		surface->pointcache->endframe = surface->end_frame;
	}
}

/*
*	Updates derived mesh copy and processes dynamic paint step / caches.
*/
static void dynamicPaint_canvasUpdate(DynamicPaintModifierData *pmd, Scene *scene, Object *ob, DerivedMesh *dm)
{
	if((pmd->type & MOD_DYNAMICPAINT_TYPE_CANVAS) && pmd->canvas) {
		DynamicPaintCanvasSettings *canvas = pmd->canvas;
		DynamicPaintSurface *surface = canvas->surfaces.first;

		/* update derived mesh copy */
		if (canvas->dm) canvas->dm->release(canvas->dm);
			canvas->dm = CDDM_copy(dm);

		/* in case image sequence baking, stop here */
		if (canvas->flags & MOD_DPAINT_BAKING) return;

		/* loop through surfaces */
		for (; surface; surface=surface->next) {
			int current_frame = (int)scene->r.cfra;

			/* image sequences are handled by bake operator */
			if (surface->format == MOD_DPAINT_SURFACE_F_IMAGESEQ) continue;
			if (!(surface->flags & MOD_DPAINT_ACTIVE)) continue;

			/* make sure surface is valid */
			dynamicPaint_checkSurfaceData(surface);

			/* limit frame range */
			CLAMP(current_frame, surface->start_frame, surface->end_frame);

			if (current_frame != surface->current_frame || (int)scene->r.cfra == surface->start_frame) {
				PointCache *cache = surface->pointcache;
				PTCacheID pid;
				surface->current_frame = current_frame;

				/* read point cache */
				BKE_ptcache_id_from_dynamicpaint(&pid, ob, surface);
				pid.cache->startframe = surface->start_frame;
				pid.cache->endframe = surface->end_frame;
				BKE_ptcache_id_time(&pid, scene, scene->r.cfra, NULL, NULL, NULL);

				/* reset non-baked cache at first frame */
				if((int)scene->r.cfra == surface->start_frame && !(cache->flag & PTCACHE_BAKED))
				{
					cache->flag |= PTCACHE_REDO_NEEDED;
					BKE_ptcache_id_reset(scene, &pid, PTCACHE_RESET_OUTDATED);
					cache->flag &= ~PTCACHE_REDO_NEEDED;
				}

				/* try to read from cache */
				if(BKE_ptcache_read(&pid, (float)scene->r.cfra)) {
					BKE_ptcache_validate(cache, (int)scene->r.cfra);
				}
				/* if read failed and we're on surface range do recalculate */
				else if ((int)scene->r.cfra == current_frame) {
					/* calculate surface frame */
					dynamicPaint_calculateFrame(surface, scene, ob, current_frame);

					BKE_ptcache_validate(cache, surface->current_frame);
					BKE_ptcache_write(&pid, surface->current_frame);
				}
			}
		}
	}
	else if((pmd->type & MOD_DYNAMICPAINT_TYPE_BRUSH) && pmd->brush) {

		if (pmd->brush->dm) pmd->brush->dm->release(pmd->brush->dm);
		pmd->brush->dm = CDDM_copy(dm);
	}
}

/*
*	Apply canvas data to the object derived mesh
*/
struct DerivedMesh *dynamicPaint_Modifier_apply(DynamicPaintModifierData *pmd, Scene *scene, Object *ob, DerivedMesh *dm)
{	
	DerivedMesh *result = CDDM_copy(dm);

	if((pmd->type & MOD_DYNAMICPAINT_TYPE_CANVAS) && pmd->canvas &&
		!(pmd->canvas->flags & MOD_DPAINT_BAKING)) {

		DynamicPaintSurface *surface = pmd->canvas->surfaces.first;
		pmd->canvas->flags &= ~MOD_DPAINT_PREVIEW_READY;

		/* loop through surfaces */
		for (; surface; surface=surface->next) {

			if (surface && surface->format != MOD_DPAINT_SURFACE_F_IMAGESEQ && surface->data) {
				if (!(surface->flags & (MOD_DPAINT_ACTIVE))) continue;

				/* process vertex surface previews */
				if (surface->format == MOD_DPAINT_SURFACE_F_VERTEX) {

					/* vertex color paint */
					if (surface->type == MOD_DPAINT_SURFACE_T_PAINT) {

						MFace *mface = result->getFaceArray(result);
						int numOfFaces = result->getNumFaces(result);
						int i;
						PaintPoint* pPoint = (PaintPoint*)surface->data->type_data;
						MCol *col;

						/* paint is stored on dry and wet layers, so mix final color first */
						float *fcolor = MEM_callocN(sizeof(float)*surface->data->total_points*4, "Temp paint color");

						#pragma omp parallel for schedule(static)
						for (i=0; i<surface->data->total_points; i++) {
							int j=i*4;

							fcolor[j]   = pPoint[i].color[0];
							fcolor[j+1] = pPoint[i].color[1];
							fcolor[j+2] = pPoint[i].color[2];
							/* mix colors */
							if (pPoint[i].e_alpha) mixColors(&fcolor[j], pPoint[i].alpha, pPoint[i].e_color, pPoint[i].e_alpha);

							/* Use highest alpha	*/
							fcolor[j+3] = (pPoint[i].e_alpha > pPoint[i].alpha) ? pPoint[i].e_alpha : pPoint[i].alpha;
						}

						/* viewport preview */
						if (surface->flags & MOD_DPAINT_PREVIEW) {
							/* Save preview results to weight layer, to be
							*   able to share same drawing methods */
							col = result->getFaceDataArray(result, CD_WEIGHT_MCOL);
							if (!col) col = CustomData_add_layer(&result->faceData, CD_WEIGHT_MCOL, CD_CALLOC, NULL, numOfFaces);

							if (col) {
								#pragma omp parallel for schedule(static)
								for (i=0; i<numOfFaces; i++) {
									int j=0;
									float invAlpha;
									Material *material = give_current_material(ob, mface[i].mat_nr+1);

									for (; j<((mface[i].v4)?4:3); j++) {
										int index = (j==0)?mface[i].v1: (j==1)?mface[i].v2: (j==2)?mface[i].v3: mface[i].v4;
										index *= 4;
										invAlpha = 1.0f - fcolor[index+3];

										/* Apply material color as base vertex color for preview */
										col[i*4+j].a = 255;
										if (material) {
											col[i*4+j].r = (unsigned char)(material->b*255);
											col[i*4+j].g = (unsigned char)(material->g*255);
											col[i*4+j].b = (unsigned char)(material->r*255);
										}
										else {
											col[i*4+j].r = 165;
											col[i*4+j].g = 165;
											col[i*4+j].b = 165;
										}

										/* mix surface color */
										col[i*4+j].r = (char)(((float)col[i*4+j].r)*invAlpha + (fcolor[index+2]*255*fcolor[index+3]));
										col[i*4+j].g = (char)(((float)col[i*4+j].g)*invAlpha + (fcolor[index+1]*255*fcolor[index+3]));
										col[i*4+j].b = (char)(((float)col[i*4+j].b)*invAlpha + (fcolor[index]*255*fcolor[index+3]));
									}
								}
								pmd->canvas->flags |= MOD_DPAINT_PREVIEW_READY;
							}
						}


						/* save layer data to output layer */

						/* paint layer */
						col = CustomData_get_layer_named(&dm->faceData, CD_MCOL, surface->output_name);
						if (col) {
							#pragma omp parallel for schedule(static)
							for (i=0; i<numOfFaces; i++) {
								int j=0;
								for (; j<((mface[i].v4)?4:3); j++) {
									int index = (j==0)?mface[i].v1: (j==1)?mface[i].v2: (j==2)?mface[i].v3: mface[i].v4;
									index *= 4;

									col[i*4+j].a = (char)(fcolor[index+3]*255);
									col[i*4+j].r = (char)(fcolor[index+2]*255);
									col[i*4+j].g = (char)(fcolor[index+1]*255);
									col[i*4+j].b = (char)(fcolor[index]*255);
								}
							}
						}
						MEM_freeN(fcolor);

						/* wet layer */
						col = CustomData_get_layer_named(&dm->faceData, CD_MCOL, surface->output_name2);
						if (col) {
							#pragma omp parallel for schedule(static)
							for (i=0; i<numOfFaces; i++) {
								int j=0;

								for (; j<((mface[i].v4)?4:3); j++) {
									int index = (j==0)?mface[i].v1: (j==1)?mface[i].v2: (j==2)?mface[i].v3: mface[i].v4;

									col[i*4+j].a = 255;
									col[i*4+j].r = (char)(pPoint[index].wetness*255);
									col[i*4+j].g = (char)(pPoint[index].wetness*255);
									col[i*4+j].b = (char)(pPoint[index].wetness*255);
								}
							}
						}
					}
					/* displace paint */
					else if (surface->type == MOD_DPAINT_SURFACE_T_DISPLACE) {
						MVert *mvert = result->getVertArray(result);
						int i;
						float normal[3];
						float* value = (float*)surface->data->type_data;

						for (i=0; i<surface->data->total_points; i++) {
							normal_short_to_float_v3(normal, mvert[i].no);
							normalize_v3(normal);

							mvert[i].co[0] -= normal[0]*value[i];
							mvert[i].co[1] -= normal[1]*value[i];
							mvert[i].co[2] -= normal[2]*value[i];
						}

						CDDM_calc_normals(result);
					}

					/* vertex group paint */
					else if (surface->type == MOD_DPAINT_SURFACE_T_WEIGHT) {
						int defgrp_index = defgroup_name_index(ob, surface->output_name);
						MDeformVert *dvert = result->getVertDataArray(result, CD_MDEFORMVERT);
						float *weight = (float*)surface->data->type_data;
						/* viewport preview */
						if (surface->flags & MOD_DPAINT_PREVIEW) {
							/* Save preview results to weight layer, to be
							*   able to share same drawing methods */
							MFace *mface = result->getFaceArray(result);
							int numOfFaces = result->getNumFaces(result);
							int i;
							MCol *col = result->getFaceDataArray(result, CD_WEIGHT_MCOL);
							if (!col) col = CustomData_add_layer(&result->faceData, CD_WEIGHT_MCOL, CD_CALLOC, NULL, numOfFaces);

							if (col) {
								#pragma omp parallel for schedule(static)
								for (i=0; i<numOfFaces; i++) {
									float temp_color[3];
									int j=0;
									for (; j<((mface[i].v4)?4:3); j++) {
										int index = (j==0)?mface[i].v1: (j==1)?mface[i].v2: (j==2)?mface[i].v3: mface[i].v4;

										col[i*4+j].a = 255;

										weight_to_rgb(weight[index], temp_color, temp_color+1, temp_color+2);
										col[i*4+j].r = (char)(temp_color[2]*255);
										col[i*4+j].g = (char)(temp_color[1]*255);
										col[i*4+j].b = (char)(temp_color[0]*255);
									}
								}
								pmd->canvas->flags |= MOD_DPAINT_PREVIEW_READY;
							}
						}

						/* apply weights into a vertex group, if doesnt exists add a new layer */
						if (defgrp_index >= 0 && !dvert && strlen(surface->output_name)>0)
							dvert = CustomData_add_layer_named(&result->vertData, CD_MDEFORMVERT, CD_CALLOC,
																NULL, surface->data->total_points, surface->output_name);
						if (defgrp_index >= 0 && dvert) {
							int i;
							for(i=0; i<surface->data->total_points; i++) {
								int j;
								MDeformVert *dv= &dvert[i];
								MDeformWeight *def_weight = NULL;

								/* check if this vertex has a weight */
								for (j=0; j<dv->totweight; j++) {
									if (dv->dw[j].def_nr == defgrp_index) {
										def_weight = &dv->dw[j];
										break;
									}
								}

								/* if not found, add a weight for it */
								if (!def_weight) {
									MDeformWeight *newdw = MEM_callocN(sizeof(MDeformWeight)*(dv->totweight+1), 
														 "deformWeight");
									if(dv->dw){
										memcpy(newdw, dv->dw, sizeof(MDeformWeight)*dv->totweight);
										MEM_freeN(dv->dw);
									}
									dv->dw=newdw;
									dv->dw[dv->totweight].def_nr=defgrp_index;
									def_weight = &dv->dw[dv->totweight];
									dv->totweight++;
								}

								/* set weight value */
								def_weight->weight = weight[i];
							}
						}
					}
					/* wave simulation */
					else if (surface->type == MOD_DPAINT_SURFACE_T_WAVE) {
						MVert *mvert = result->getVertArray(result);
						int i;
						float normal[3];
						PaintWavePoint* wPoint = (PaintWavePoint*)surface->data->type_data;

						for (i=0; i<surface->data->total_points; i++) {
							normal_short_to_float_v3(normal, mvert[i].no);
							normalize_v3(normal);

							mvert[i].co[0] += normal[0]*wPoint[i].height;
							mvert[i].co[1] += normal[1]*wPoint[i].height;
							mvert[i].co[2] += normal[2]*wPoint[i].height;
						}

						CDDM_calc_normals(result);
					}
				}
			}
		}
	}

	return result;
}

/* Modifier call. Processes dynamic paint modifier step. */
struct DerivedMesh *dynamicPaint_Modifier_do(DynamicPaintModifierData *pmd, Scene *scene, Object *ob, DerivedMesh *dm)
{	
	/* Update derived mesh data to modifier if baking	*/
	dynamicPaint_canvasUpdate(pmd, scene, ob, dm);

	/* Return output mesh */
	return dynamicPaint_Modifier_apply(pmd, scene, ob, dm);
}


/***************************** Image Sequence / UV Image Canvas Calls ******************************/

/*
*	Tries to find the neighbouring pixel in given (uv space) direction.
*	Result is used by effect system to move paint on the surface.
*
*   px,py : origin pixel x and y
*	n_index : lookup direction index (use neighX,neighY to get final index)
*/
static int dynamicPaint_findNeighbourPixel(PaintUVPoint *tempPoints, DerivedMesh *dm, char *uvname, int w, int h, int px, int py, int n_index)
{
	/* Note: Current method only uses polygon edges to detect neighbouring pixels.
	*  -> It doesn't always lead to the optimum pixel but is accurate enough
	*  and faster/simplier than including possible face tip point links)
	*/

	int x,y;
	PaintUVPoint *tPoint = NULL;
	PaintUVPoint *cPoint = NULL;

	/* shift position by given n_index */
	x = px + neighX[n_index];
	y = py + neighY[n_index];

	if (x<0 || x>=w) return -1;
	if (y<0 || y>=h) return -1;

	tPoint = &tempPoints[x+w*y];		/* UV neighbour */
	cPoint = &tempPoints[px+w*py];		/* Origin point */

	/*
	*	Check if shifted point is on same face -> it's a correct neighbour
	*   (and if it isn't marked as an "edge pixel")
	*/
	if ((tPoint->face_index == cPoint->face_index) && (tPoint->neighbour_pixel == -1))
		return (x+w*y);

	/*
	*	Even if shifted point is on another face
	*	-> use this point.
	*	
	*	!! Replace with "is uv faces linked" check !!
	*	This should work fine as long as uv island
	*	margin is > 1 pixel.
	*/
	if ((tPoint->face_index != -1) && (tPoint->neighbour_pixel == -1)) {
		return (x+w*y);
	}

	/*
	*	If we get here, the actual neighbouring pixel
	*	is located on a non-linked uv face, and we have to find
	*	it's "real" position.
	*
	*	Simple neighbouring face finding algorithm:
	*	- find closest uv edge to shifted pixel and get
	*	  the another face that shares that edge
	*	- find corresponding position of that new face edge
	*	  in uv space
	*
	*	TODO: Implement something more accurate / optimized?
	*/
	{
		int numOfFaces = dm->getNumFaces(dm);
		MFace *mface = dm->getFaceArray(dm);
		MTFace *tface =  CustomData_get_layer_named(&dm->faceData, CD_MTFACE, uvname);

		/* Get closest edge to that subpixel on UV map	*/
		{
			float pixel[2], dist, t_dist;
			int i, uindex[2], edge1_index, edge2_index,
				e1_index, e2_index, target_face;
			float closest_point[2], lambda, dir_vec[2];
			int target_uv1, target_uv2, final_pixel[2], final_index;

			float *s_uv1, *s_uv2, *t_uv1, *t_uv2;

			pixel[0] = ((float)(px + neighX[n_index]) + 0.5f) / (float)w;
			pixel[1] = ((float)(py + neighY[n_index]) + 0.5f) / (float)h;

			/* Get uv indexes for current face part	*/
			if (cPoint->quad) {
				uindex[0] = 0; uindex[1] = 2; uindex[2] = 3;
			}
			else {
				uindex[0] = 0; uindex[1] = 1; uindex[2] = 2;
			}

			/*
			*	Find closest edge to that pixel
			*/
			/* Dist to first edge	*/
			e1_index = cPoint->v1; e2_index = cPoint->v2; edge1_index = uindex[0]; edge2_index = uindex[1];
			dist = dist_to_line_segment_v2(pixel, tface[cPoint->face_index].uv[edge1_index], tface[cPoint->face_index].uv[edge2_index]);

			/* Dist to second edge	*/
			t_dist = dist_to_line_segment_v2(pixel, tface[cPoint->face_index].uv[uindex[1]], tface[cPoint->face_index].uv[uindex[2]]);
			if (t_dist < dist) {e1_index = cPoint->v2; e2_index = cPoint->v3; edge1_index = uindex[1]; edge2_index = uindex[2]; dist = t_dist;}

			/* Dist to third edge	*/
			t_dist = dist_to_line_segment_v2(pixel, tface[cPoint->face_index].uv[uindex[2]], tface[cPoint->face_index].uv[uindex[0]]);
			if (t_dist < dist) {e1_index = cPoint->v3; e2_index = cPoint->v1;  edge1_index = uindex[2]; edge2_index = uindex[0]; dist = t_dist;}


			/*
			*	Now find another face that is linked to that edge
			*/
			target_face = -1;

			for (i=0; i<numOfFaces; i++) {
				/*
				*	Check if both edge vertices share this face
				*/
				int v4 = (mface[i].v4) ? mface[i].v4 : -1;

				if ((e1_index == mface[i].v1 || e1_index == mface[i].v2 || e1_index == mface[i].v3 || e1_index == v4) &&
					(e2_index == mface[i].v1 || e2_index == mface[i].v2 || e2_index == mface[i].v3 || e2_index == v4)) {
					if (i == cPoint->face_index) continue;

					target_face = i;

					/*
					*	Get edge UV index
					*/
					if (e1_index == mface[i].v1) target_uv1 = 0;
					else if (e1_index == mface[i].v2) target_uv1 = 1;
					else if (e1_index == mface[i].v3) target_uv1 = 2;
					else target_uv1 = 3;

					if (e2_index == mface[i].v1) target_uv2 = 0;
					else if (e2_index == mface[i].v2) target_uv2 = 1;
					else if (e2_index == mface[i].v3) target_uv2 = 2;
					else target_uv2 = 3;

					break;
				}
			}

			/* If none found return -1	*/
			if (target_face == -1) return -1;

			/*
			*	If target face is connected in UV space as well, just use original index
			*/
			s_uv1 = (float *)tface[cPoint->face_index].uv[edge1_index];
			s_uv2 = (float *)tface[cPoint->face_index].uv[edge2_index];
			t_uv1 = (float *)tface[target_face].uv[target_uv1];
			t_uv2 = (float *)tface[target_face].uv[target_uv2];

			//printf("connected UV : %f,%f & %f,%f - %f,%f & %f,%f\n", s_uv1[0], s_uv1[1], s_uv2[0], s_uv2[1], t_uv1[0], t_uv1[1], t_uv2[0], t_uv2[1]);

			if (((s_uv1[0] == t_uv1[0] && s_uv1[1] == t_uv1[1]) &&
				 (s_uv2[0] == t_uv2[0] && s_uv2[1] == t_uv2[1]) ) ||
				((s_uv2[0] == t_uv1[0] && s_uv2[1] == t_uv1[1]) &&
				 (s_uv1[0] == t_uv2[0] && s_uv1[1] == t_uv2[1]) )) return ((px+neighX[n_index]) + w*(py+neighY[n_index]));

			/*
			*	Find a point that is relatively at same edge position
			*	on this other face UV
			*/
			lambda = closest_to_line_v2(closest_point, pixel, tface[cPoint->face_index].uv[edge1_index], tface[cPoint->face_index].uv[edge2_index]);
			if (lambda < 0.0f) lambda = 0.0f;
			if (lambda > 1.0f) lambda = 1.0f;

			sub_v2_v2v2(dir_vec, tface[target_face].uv[target_uv2], tface[target_face].uv[target_uv1]);

			mul_v2_fl(dir_vec, lambda);

			copy_v2_v2(pixel, tface[target_face].uv[target_uv1]);
			add_v2_v2(pixel, dir_vec);
			pixel[0] = (pixel[0] * (float)w) - 0.5f;
			pixel[1] = (pixel[1] * (float)h) - 0.5f;

			final_pixel[0] = (int)floor(pixel[0]);
			final_pixel[1] = (int)floor(pixel[1]);

			/* If current pixel uv is outside of texture	*/
			if (final_pixel[0] < 0 || final_pixel[0] >= w) return -1;
			if (final_pixel[1] < 0 || final_pixel[1] >= h) return -1;

			final_index = final_pixel[0] + w * final_pixel[1];

			/* If we ended up to our origin point ( mesh has smaller than pixel sized faces)	*/
			if (final_index == (px+w*py)) return -1;
			/* If found pixel still lies on wrong face ( mesh has smaller than pixel sized faces)	*/
			if (tempPoints[final_index].face_index != target_face) return -1;

			/*
			*	If final point is an "edge pixel", use it's "real" neighbour instead
			*/
			if (tempPoints[final_index].neighbour_pixel != -1) final_index = cPoint->neighbour_pixel;

			return final_index;
		}
	}
}

/*
*	Create a surface for image sequence format
*/
static int dynamicPaint_createUVSurface(DynamicPaintSurface *surface)
{
	/* Antialias jitter point relative coords	*/
	float jitter5sample[10] =  {0.0f, 0.0f,
							-0.2f, -0.4f,
							0.2f, 0.4f,
							0.4f, -0.2f,
							-0.4f, 0.3f};
	int yy;
	int w,h;
	int numOfFaces;
	char uvname[32];
	int active_points = 0;
	int error = 0;

	PaintSurfaceData *sData;
	DynamicPaintCanvasSettings *canvas = surface->canvas;
	DerivedMesh *dm = canvas->dm;

	PaintUVPoint *tempPoints = NULL;
	Vec3f *tempWeights = NULL;
	MVert *mvert = NULL;
	MFace *mface = NULL;
	MTFace *tface = NULL;
	BB2d *faceBB = NULL;
	int *final_index;

	if (!dm) return printError(canvas, "Canvas mesh not updated.");
	if (surface->format != MOD_DPAINT_SURFACE_F_IMAGESEQ) return printError(canvas, "Can't bake non-\"image sequence\" formats.");

	numOfFaces = dm->getNumFaces(dm);
	mvert = dm->getVertArray(dm);
	mface = dm->getFaceArray(dm);

	/* get uv layer */
	validate_layer_name(&dm->faceData, CD_MTFACE, surface->uvlayer_name, uvname);
	tface = CustomData_get_layer_named(&dm->faceData, CD_MTFACE, uvname);

	/* Check for validity	*/
	if (!tface) return printError(canvas, "No UV data on canvas.");
	if (surface->image_resolution < 16 || surface->image_resolution > 8096) return printError(canvas, "Invalid resolution.");

	w = h = surface->image_resolution;

	/*
	*	Start generating the surface
	*/
	printf("DynamicPaint: Preparing UV surface of %ix%i pixels and %i faces.\n", w, h, numOfFaces);

	/* Init data struct */
	if (surface->data) dynamicPaint_freeSurfaceData(surface);
	sData = surface->data = MEM_callocN(sizeof(PaintSurfaceData), "PaintSurfaceData");
	if (!surface->data) return printError(canvas, "Not enough free memory.");

	sData->samples = (surface->flags & MOD_DPAINT_ANTIALIAS) ? 5 : 1;
	tempPoints = (struct PaintUVPoint *) MEM_callocN(w*h*sizeof(struct PaintUVPoint), "Temp PaintUVPoint");
	if (!tempPoints) error=1;

	final_index = (int *) MEM_callocN(w*h*sizeof(int), "Temp UV Final Indexes");
	if (!final_index) error=1;

	if (!error) {
		tempWeights = (struct Vec3f *) MEM_mallocN(w*h*sData->samples*sizeof(struct Vec3f), "Temp bWeights");
		if (!tempWeights) error=1;
	}

	/*
	*	Generate a temporary bounding box array for UV faces to optimize
	*	the pixel-inside-a-face search.
	*/
	if (!error) {
		faceBB = (struct BB2d *) MEM_mallocN(numOfFaces*sizeof(struct BB2d), "MPCanvasFaceBB");
		if (!faceBB) error=1;
	}

	if (!error)
	for (yy=0; yy<numOfFaces; yy++) {
		int numOfVert = (mface[yy].v4) ? 4 : 3;
		int i;

		VECCOPY2D(faceBB[yy].min, tface[yy].uv[0]);
		VECCOPY2D(faceBB[yy].max, tface[yy].uv[0]);

		for (i = 1; i<numOfVert; i++) {
			if (tface[yy].uv[i][0] < faceBB[yy].min[0]) faceBB[yy].min[0] = tface[yy].uv[i][0];
			if (tface[yy].uv[i][1] < faceBB[yy].min[1]) faceBB[yy].min[1] = tface[yy].uv[i][1];
			if (tface[yy].uv[i][0] > faceBB[yy].max[0]) faceBB[yy].max[0] = tface[yy].uv[i][0];
			if (tface[yy].uv[i][1] > faceBB[yy].max[1]) faceBB[yy].max[1] = tface[yy].uv[i][1];

		}
	}

	/*
	*	Loop through every pixel and check
	*	if pixel is uv-mapped on a canvas face.
	*/
	if (!error) {
		#pragma omp parallel for schedule(static)
		for (yy = 0; yy < h; yy++)
		{
			int xx;
			for (xx = 0; xx < w; xx++)
			{
				int i, sample;
				int index = xx+w*yy;
				PaintUVPoint *tPoint = (&tempPoints[index]);

				short isInside = 0;	/* if point is inside a uv face */

				float d1[2], d2[2], d3[2], point[5][2];
				float dot00,dot01,dot02,dot11,dot12, invDenom, u,v;

				/* Init per pixel settings */
				tPoint->face_index = -1;
				tPoint->neighbour_pixel = -1;
				tPoint->pixel_index = index;

				/* Actual pixel center, used when collision is found	*/
				point[0][0] = ((float)xx + 0.5f) / w;
				point[0][1] = ((float)yy + 0.5f) / h;

				/*
				* A pixel middle sample isn't enough to find very narrow polygons
				* So using 4 samples of each corner too
				*/
				point[1][0] = ((float)xx) / w;
				point[1][1] = ((float)yy) / h;

				point[2][0] = ((float)xx+1) / w;
				point[2][1] = ((float)yy) / h;

				point[3][0] = ((float)xx) / w;
				point[3][1] = ((float)yy+1) / h;

				point[4][0] = ((float)xx+1) / w;
				point[4][1] = ((float)yy+1) / h;


				/* Loop through samples, starting from middle point	*/
				for (sample=0; sample<5; sample++) {
					
					/* Loop through every face in the mesh	*/
					for (i=0; i<numOfFaces; i++) {

						/* Check uv bb	*/
						if (faceBB[i].min[0] > (point[sample][0])) continue;
						if (faceBB[i].min[1] > (point[sample][1])) continue;
						if (faceBB[i].max[0] < (point[sample][0])) continue;
						if (faceBB[i].max[1] < (point[sample][1])) continue;

						/*  Calculate point inside a triangle check
						*	for uv0,1,2 */
						VECSUB2D(d1,  tface[i].uv[2], tface[i].uv[0]);	// uv2 - uv0
						VECSUB2D(d2,  tface[i].uv[1], tface[i].uv[0]);	// uv1 - uv0
						VECSUB2D(d3,  point[sample], tface[i].uv[0]);	// point - uv0

						dot00 = d1[0]*d1[0] + d1[1]*d1[1];
						dot01 = d1[0]*d2[0] + d1[1]*d2[1];
						dot02 = d1[0]*d3[0] + d1[1]*d3[1];
						dot11 = d2[0]*d2[0] + d2[1]*d2[1];
						dot12 = d2[0]*d3[0] + d2[1]*d3[1];

						invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
						u = (dot11 * dot02 - dot01 * dot12) * invDenom;
						v = (dot00 * dot12 - dot01 * dot02) * invDenom;

						if ((u > 0) && (v > 0) && (u + v < 1)) {isInside=1;} /* is inside a triangle */

						/*  If collision wasn't found but the face is a quad
						*	do another check for the second half */
						if ((!isInside) && mface[i].v4)
						{

							/* change d2 to test the other half	*/
							VECSUB2D(d2,  tface[i].uv[3], tface[i].uv[0]);	// uv3 - uv0

							/* test again	*/
							dot00 = d1[0]*d1[0] + d1[1]*d1[1];
							dot01 = d1[0]*d2[0] + d1[1]*d2[1];
							dot02 = d1[0]*d3[0] + d1[1]*d3[1];
							dot11 = d2[0]*d2[0] + d2[1]*d2[1];
							dot12 = d2[0]*d3[0] + d2[1]*d3[1];

							invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
							u = (dot11 * dot02 - dot01 * dot12) * invDenom;
							v = (dot00 * dot12 - dot01 * dot02) * invDenom;

							if ((u > 0) && (v > 0) && (u + v < 1)) {isInside=2;} /* is inside the second half of the quad */

						}

						/*
						*	If point was inside the face
						*/
						if (isInside != 0) {

							float uv1co[2], uv2co[2], uv3co[2], uv[2];
							int j;

							/* Get triagnle uvs	*/
							if (isInside==1) {
								VECCOPY2D(uv1co, tface[i].uv[0]);
								VECCOPY2D(uv2co, tface[i].uv[1]);
								VECCOPY2D(uv3co, tface[i].uv[2]);
							}
							else {
								VECCOPY2D(uv1co, tface[i].uv[0]);
								VECCOPY2D(uv2co, tface[i].uv[2]);
								VECCOPY2D(uv3co, tface[i].uv[3]);
							}

							/* Add b-weights per anti-aliasing sample	*/
							for (j=0; j<sData->samples; j++) {
								uv[0] = point[0][0] + jitter5sample[j*2] / w;
								uv[1] = point[0][1] + jitter5sample[j*2+1] / h;

								barycentric_weights_v2(uv1co, uv2co, uv3co, uv, tempWeights[index*sData->samples+j].v);
							}

							/* Set surface point face values	*/
							tPoint->face_index = i;							/* face index */
							tPoint->quad = (isInside == 2) ? 1 : 0;		/* quad or tri part*/

							/* save vertex indexes	*/
							tPoint->v1 = (isInside == 2) ? mface[i].v1 : mface[i].v1;
							tPoint->v2 = (isInside == 2) ? mface[i].v3 : mface[i].v2;
							tPoint->v3 = (isInside == 2) ? mface[i].v4 : mface[i].v3;
							
							sample = 5;	/* make sure we exit sample loop as well */
							break;
						}
					}
				} /* sample loop */
			}
		}

		/*
		*	Now loop through every pixel that was left without index
		*	and find if they have neighbouring pixels that have an index.
		*	If so use that polygon as pixel surface.
		*	(To avoid seams on uv island edges)
		*/
		#pragma omp parallel for schedule(static)
		for (yy = 0; yy < h; yy++)
		{
			int xx;
			for (xx = 0; xx < w; xx++)
			{
				int index = xx+w*yy;
				PaintUVPoint *tPoint = (&tempPoints[index]);

				/* If point isnt't on canvas mesh	*/
				if (tPoint->face_index == -1) {
					int u_min, u_max, v_min, v_max;
					int u,v, ind;
					float point[2];

					/* get loop area	*/
					u_min = (xx > 0) ? -1 : 0;
					u_max = (xx < (w-1)) ? 1 : 0;
					v_min = (yy > 0) ? -1 : 0;
					v_max = (yy < (h-1)) ? 1 : 0;

					point[0] = ((float)xx + 0.5f) / w;
					point[1] = ((float)yy + 0.5f) / h;

					/* search through defined area for neighbour	*/
					for (u=u_min; u<=u_max; u++)
						for (v=v_min; v<=v_max; v++) {
							/* if not this pixel itself	*/
							if (u!=0 || v!=0) {
								ind = (xx+u)+w*(yy+v);

								/* if neighbour has index	*/
								if (tempPoints[ind].face_index != -1) {

									float uv1co[2], uv2co[2], uv3co[2], uv[2];
									int i = tempPoints[ind].face_index, j;

									/* Now calculate pixel data for this pixel as it was on polygon surface */
									if (!tempPoints[ind].quad) {
										VECCOPY2D(uv1co, tface[i].uv[0]);
										VECCOPY2D(uv2co, tface[i].uv[1]);
										VECCOPY2D(uv3co, tface[i].uv[2]);
									}
									else {
										VECCOPY2D(uv1co, tface[i].uv[0]);
										VECCOPY2D(uv2co, tface[i].uv[2]);
										VECCOPY2D(uv3co, tface[i].uv[3]);
									}

									/* Add b-weights per anti-aliasing sample	*/
									for (j=0; j<sData->samples; j++) {

										uv[0] = point[0] + jitter5sample[j*2] / w;
										uv[1] = point[1] + jitter5sample[j*2+1] / h;
										barycentric_weights_v2(uv1co, uv2co, uv3co, uv, tempWeights[index*sData->samples+j].v);
									}

									/* Set values	*/
									tPoint->neighbour_pixel = ind;				// face index
									tPoint->quad = tempPoints[ind].quad;		// quad or tri

									/* save vertex indexes	*/
									tPoint->v1 = (tPoint->quad) ? mface[i].v1 : mface[i].v1;
									tPoint->v2 = (tPoint->quad) ? mface[i].v3 : mface[i].v2;
									tPoint->v3 = (tPoint->quad) ? mface[i].v4 : mface[i].v3;

									u = u_max + 1;	/* make sure we exit outer loop as well */
									break;
								}
						}
					}
				}
			}
		}

		/*
		*	When base loop is over convert found neighbour indexes to real ones
		*	Also count the final number of active surface points
		*/
		for (yy = 0; yy < h; yy++)
		{
			int xx;
			for (xx = 0; xx < w; xx++)
			{
				int index = xx+w*yy;
				PaintUVPoint *tPoint = (&tempPoints[index]);

				if (tPoint->face_index == -1 && tPoint->neighbour_pixel != -1) tPoint->face_index = tempPoints[tPoint->neighbour_pixel].face_index;
				if (tPoint->face_index != -1) active_points++;
			}
		}


		/*	If any effect enabled, create surface effect / wet layer
		*	neighbour lists. Processes possibly moving data. */
		if (surface_usesNeighData(surface)) {

			int i, cursor=0;

			/* Create a temporary array of final indexes (before unassigned
			*  pixels have been dropped) */
			for (i=0; i<w*h; i++) {
				if (tempPoints[i].face_index != -1) {
					final_index[i] = cursor;
					cursor++;
				}
			}

			/* allocate memory */
			sData->total_points = w*h;
			dynamicPaint_initAdjacencyData(surface);

			if (sData->adj_data) {
				PaintAdjData *ed = sData->adj_data;
				unsigned int n_pos = 0;
				//#pragma omp parallel for schedule(static)
				for (yy = 0; yy < h; yy++)
				{
					int xx;
					for (xx = 0; xx < w; xx++)
					{
						int i, index = xx+w*yy;

						if (tempPoints[index].face_index != -1) {
							ed->n_index[final_index[index]] = n_pos;
							ed->numOf_n[final_index[index]] = 0;

							for (i=0; i<8; i++) {

								/* Try to find a neighbouring pixel in defined direction
								*  If not found, -1 is returned */
								int n_target = dynamicPaint_findNeighbourPixel(tempPoints, dm, uvname, w, h, xx, yy, i);

								if (n_target != -1) {
									ed->n_target[n_pos] = final_index[n_target];
									ed->numOf_n[final_index[index]]++;
									n_pos++;
								}
							}
						}
					}
				}
			}
		}

		/* Create final surface data without inactive points */
		{
			ImgSeqFormatData *f_data = MEM_callocN(sizeof(struct ImgSeqFormatData), "ImgSeqFormatData");
			if (f_data) {
				f_data->uv_p = MEM_callocN(active_points*sizeof(struct PaintUVPoint), "PaintUVPoint");
				f_data->barycentricWeights = MEM_callocN(active_points*sData->samples*sizeof(struct Vec3f), "PaintUVPoint");

				if (!f_data->uv_p || !f_data->barycentricWeights) error=1;
			}
			else error=1;

			sData->total_points = active_points;
			
			/* in case of allocation error, free everything */
			if (error) {
				if (f_data) {
					if (f_data->uv_p) MEM_freeN(f_data->uv_p);
					if (f_data->barycentricWeights) MEM_freeN(f_data->barycentricWeights);
					MEM_freeN(f_data);
				}
			}
			else {
				int index, cursor = 0;
				sData->total_points = active_points;
				sData->format_data = f_data;

				for(index = 0; index < (w*h); index++) {
					if (tempPoints[index].face_index != -1) {
						memcpy(&f_data->uv_p[cursor], &tempPoints[index], sizeof(PaintUVPoint));
						memcpy(&f_data->barycentricWeights[cursor*sData->samples], &tempWeights[index*sData->samples], sizeof(Vec3f)*sData->samples);
						cursor++;
					}
				}
			}
		}
	}
	if (error==1) printError(canvas, "Not enough free memory.");

	if (faceBB) MEM_freeN(faceBB);
	if (tempPoints) MEM_freeN(tempPoints);
	if (tempWeights) MEM_freeN(tempWeights);
	if (final_index) MEM_freeN(final_index);

	/* Init surface type data */
	if (!error) {
		dynamicPaint_allocateSurfaceType(surface);

#if 0
		/*  -----------------------------------------------------------------
		*	For debug, output pixel statuses to the color map
		*	-----------------------------------------------------------------*/
		#pragma omp parallel for schedule(static)
		for (index = 0; index < sData->total_points; index++)
		{
			ImgSeqFormatData *f_data = (ImgSeqFormatData*)sData->format_data;
			PaintUVPoint *uvPoint = &((PaintUVPoint*)f_data->uv_p)[index];
			PaintPoint *pPoint = &((PaintPoint*)sData->type_data)[index];
			pPoint->alpha=1.0f;

			/* Every pixel that is assigned as "edge pixel" gets blue color	*/
			if (uvPoint->neighbour_pixel != -1) pPoint->color[2] = 1.0f;
			/* and every pixel that finally got an polygon gets red color	*/
			if (uvPoint->face_index != -1) pPoint->color[0] = 1.0f;
			/* green color shows pixel face index hash	*/
			if (uvPoint->face_index != -1) pPoint->color[1] = (float)(uvPoint->face_index % 255)/256.0f;
		}

#endif
	}

	return (error == 0);
}

#define DPOUTPUT_PAINT 0
#define DPOUTPUT_WET 1
#define DPOUTPUT_DISPLACE 2
#define DPOUTPUT_WAVES 3

/*
*	Outputs an image file from uv surface data.
*/
void dynamicPaint_outputImage(DynamicPaintSurface *surface, char* filename, short format, short type)
{
	int index;
	ImBuf* mhImgB = NULL;
	PaintSurfaceData *sData = surface->data;
	ImgSeqFormatData *f_data = (ImgSeqFormatData*)sData->format_data;
	char output_file[250];

	if (sData == NULL || sData->type_data == NULL) {printError(surface->canvas, "Image save failed: Invalid surface.");return;}

	if (format == DPOUTPUT_JPEG) sprintf(output_file,"%s.jpg",filename);
	else if (format == DPOUTPUT_OPENEXR) sprintf(output_file,"%s.exr",filename);
	else sprintf(output_file,"%s.png",filename);

	/* Validate output file path	*/
	BLI_path_abs(output_file, G.main->name);
	BLI_make_existing_file(output_file);

	/* Init image buffer	*/
	mhImgB = IMB_allocImBuf(surface->image_resolution, surface->image_resolution, 32, IB_rectfloat);
	if (mhImgB == NULL) {printError(surface->canvas, "Image save failed: Not enough free memory.");return;}

	#pragma omp parallel for schedule(static)
	for (index = 0; index < sData->total_points; index++)
	{
		int pos=f_data->uv_p[index].pixel_index*4;	/* image buffer position */
		

		/* Set values of preferred type */
		if (type == DPOUTPUT_WET) {
			PaintPoint *point = &((PaintPoint*)sData->type_data)[index];
			float value = (point->wetness > 1.0f) ? 1.0f : point->wetness;

			mhImgB->rect_float[pos]=value;
			mhImgB->rect_float[pos+1]=value;
			mhImgB->rect_float[pos+2]=value;
			mhImgB->rect_float[pos+3]=1.0f;
		}
		else if (type == DPOUTPUT_PAINT) {
			PaintPoint *point = &((PaintPoint*)sData->type_data)[index];

			mhImgB->rect_float[pos]   = point->color[0];
			mhImgB->rect_float[pos+1] = point->color[1];
			mhImgB->rect_float[pos+2] = point->color[2];
			/* mix wet layer */
			if (point->e_alpha) mixColors(&mhImgB->rect_float[pos], point->alpha, point->e_color, point->e_alpha);

			/* use highest alpha	*/
			mhImgB->rect_float[pos+3] = (point->e_alpha > point->alpha) ? point->e_alpha : point->alpha;

			/* Multiply color by alpha if enabled	*/
			if (surface->flags & MOD_DPAINT_MULALPHA) {
				mhImgB->rect_float[pos]   *= mhImgB->rect_float[pos+3];
				mhImgB->rect_float[pos+1] *= mhImgB->rect_float[pos+3];
				mhImgB->rect_float[pos+2] *= mhImgB->rect_float[pos+3];
			}
		}
		else if (type == DPOUTPUT_DISPLACE) {
			float depth = ((float*)sData->type_data)[index];

			if (surface->disp_type == MOD_DPAINT_DISP_DISPLACE) {
				depth = (0.5f - depth);
				if (depth < 0.0f) depth = 0.0f;
				if (depth > 1.0f) depth = 1.0f;
			}

			mhImgB->rect_float[pos]=depth;
			mhImgB->rect_float[pos+1]=depth;
			mhImgB->rect_float[pos+2]=depth;
			mhImgB->rect_float[pos+3]=1.0f;
		}
		else if (type == DPOUTPUT_WAVES) {
			PaintWavePoint *wPoint = &((PaintWavePoint*)sData->type_data)[index];
			float depth = wPoint->height/2.0f+0.5f;

			mhImgB->rect_float[pos]=depth;
			mhImgB->rect_float[pos+1]=depth;
			mhImgB->rect_float[pos+2]=depth;
			mhImgB->rect_float[pos+3]=1.0f;
		}
	}

	/* Save image buffer	*/
	if (format == DPOUTPUT_JPEG) {	/* JPEG */
		mhImgB->ftype= JPG|95;
		IMB_rect_from_float(mhImgB);
		imb_savejpeg(mhImgB, output_file, IB_rectfloat);
	}
#ifdef WITH_OPENEXR
	else if (format == DPOUTPUT_OPENEXR) {	/* OpenEXR 32-bit float */
		mhImgB->ftype = OPENEXR | OPENEXR_COMPRESS;
		IMB_rect_from_float(mhImgB);
		imb_save_openexr(mhImgB, output_file, IB_rectfloat);
	}
#endif
	else {	/* DPOUTPUT_PNG */
		mhImgB->ftype= PNG|95;
		IMB_rect_from_float(mhImgB);
		imb_savepng(mhImgB, output_file, IB_rectfloat);
	}

	IMB_freeImBuf(mhImgB);
}


/***************************** Material / Texture Sampling ******************************/

/*
*	Update animated textures and calculate inverse matrices
*	for material related objects in case texture is mapped to an object.
*	(obj->imat isn't auto-updated)
*/
static void dynamicPaint_updateMaterial(Material *mat, Scene *scene)
{
	MTex *mtex = NULL;
	Tex *tex = NULL;
	int tex_nr;
	if (mat == NULL) return;

	/* update material anims */
	BKE_animsys_evaluate_animdata(&mat->id, mat->adt, BKE_curframe(scene), ADT_RECALC_ANIM);

	/*
	*	Loop through every material texture and check
	*	if they are mapped by other object
	*/
	for(tex_nr=0; tex_nr<MAX_MTEX; tex_nr++) {
		/* separate tex switching */
		if(mat->septex & (1<<tex_nr)) continue;
	
		if(mat->mtex[tex_nr]) {
			mtex= mat->mtex[tex_nr];
			tex= mtex->tex;

			if(tex==0) continue;
			
			/* which coords */
			if(mtex->texco==TEXCO_OBJECT) { 
				Object *ob= mtex->object;
				if(ob) {						
					invert_m4_m4(ob->imat, ob->obmat);
				}
			}

			/* update texture anims */
			BKE_animsys_evaluate_animdata(&tex->id, tex->adt, BKE_curframe(scene), ADT_RECALC_ANIM);

			/* update cache if voxel data */
			if(tex->id.us && tex->type==TEX_VOXELDATA) {
				cache_voxeldata(tex, (int)scene->r.cfra);
			}
			/* update image sequences and movies */
			if(tex->ima && ELEM(tex->ima->source, IMA_SRC_MOVIE, IMA_SRC_SEQUENCE)) {
				if(tex->iuser.flag & IMA_ANIM_ALWAYS)
					BKE_image_user_calc_frame(&tex->iuser, (int)scene->r.cfra, 0);
			}
		}
	}
}

/* Initialize materials for object:
*   Calculates inverce matrices for linked objects, updates
*   volume caches etc. */
static void dynamicPaint_updateObjectMaterials(Object *brushOb, Material *ui_mat, Scene *scene)
{
	/* Calculate inverse transformation matrix
	*  for this object */
	invert_m4_m4(brushOb->imat, brushOb->obmat);

	/* Now process every material linked to this brush object */
	if ((ui_mat == NULL) && brushOb->mat && brushOb->totcol) {
		int i, tot=(*give_totcolp(brushOb))+1;
		for (i=1; i<tot; i++) {
			dynamicPaint_updateMaterial(give_current_material(brushOb,i), scene);
		}
	}
	else {
		dynamicPaint_updateMaterial(ui_mat, scene);
	}
}

/* A modified part of shadeinput.c -> shade_input_set_uv() / shade_input_set_shade_texco()
*  Used for sampling UV mapped texture color */
static void textured_face_generate_uv(float *uv, float *normal, float *hit, float *v1, float *v2, float *v3)
{

	float detsh, t00, t10, t01, t11, xn, yn, zn;
	int axis1, axis2;

	/* find most stable axis to project */
	xn= fabs(normal[0]);
	yn= fabs(normal[1]);
	zn= fabs(normal[2]);

	if(zn>=xn && zn>=yn) { axis1= 0; axis2= 1; }
	else if(yn>=xn && yn>=zn) { axis1= 0; axis2= 2; }
	else { axis1= 1; axis2= 2; }

	/* compute u,v and derivatives */
	t00= v3[axis1]-v1[axis1]; t01= v3[axis2]-v1[axis2];
	t10= v3[axis1]-v2[axis1]; t11= v3[axis2]-v2[axis2];

	detsh= 1.0f/(t00*t11-t10*t01);
	t00*= detsh; t01*=detsh; 
	t10*=detsh; t11*=detsh;

	uv[0] = (hit[axis1]-v3[axis1])*t11-(hit[axis2]-v3[axis2])*t10;
	uv[1] = (hit[axis2]-v3[axis2])*t00-(hit[axis1]-v3[axis1])*t01;

	/* u and v are in range -1 to 0, we allow a little bit extra but not too much, screws up speedvectors */
	CLAMP(uv[0], -2.0f, 1.0f);
	CLAMP(uv[1], -2.0f, 1.0f);
}

/* a modified part of shadeinput.c -> shade_input_set_uv() / shade_input_set_shade_texco()
*  Used for sampling UV mapped texture color */
static void textured_face_get_uv(float *uv_co, float *normal, float *uv, int faceIndex, short quad, MTFace *tface)
{
	float *uv1, *uv2, *uv3;
	float l;

	l= 1.0f+uv[0]+uv[1];
		
	uv1= tface[faceIndex].uv[0];
	uv2= (quad) ? tface[faceIndex].uv[2] : tface[faceIndex].uv[1];
	uv3= (quad) ? tface[faceIndex].uv[3] : tface[faceIndex].uv[2];
				
	uv_co[0]= -1.0f + 2.0f*(l*uv3[0]-uv[0]*uv1[0]-uv[1]*uv2[0]);
	uv_co[1]= -1.0f + 2.0f*(l*uv3[1]-uv[0]*uv1[1]-uv[1]*uv2[1]);
	uv_co[2]= 0.0f;	/* texture.c assumes there are 3 coords */
}

/*
*	Edited version of do_material_tex()
*
*	Samples color and alpha from a "Surface" type material
*	on a given point, without need for ShadeInput.
*
*	Keep up-to-date with new mapping settings
*
*	also see shade_input_set_shade_texco() for ORCO settings
*	and shade_input_set_uv() for face uv calculation
*/
void dynamicPaint_sampleSolidMaterial(float color[3], float *alpha, Material *mat, Object *brushOb, float xyz[3], int faceIndex, short isQuad, DerivedMesh *orcoDm)
{
	MTex *mtex = NULL;
	Tex *tex = NULL;
	TexResult texres= {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, NULL};
	float co[3], xyz_local[3];
	float fact, stencilTin=1.0;
	float texvec[3];
	int tex_nr, rgbnor= 0;
	float uv[3], normal[3];
	MFace *mface;
	int v1, v2, v3;
	MVert *mvert;
	
	/* Get face data	*/
	mvert = orcoDm->getVertArray(orcoDm);
	mface = orcoDm->getFaceArray(orcoDm);
	v1=mface[faceIndex].v1, v2=mface[faceIndex].v2, v3=mface[faceIndex].v3;
	if (isQuad) {v2=mface[faceIndex].v3; v3=mface[faceIndex].v4;}
	normal_tri_v3( normal, mvert[v1].co, mvert[v2].co, mvert[v3].co);

	/* Assign material base values	*/
	color[0] = mat->r;
	color[1] = mat->g;
	color[2] = mat->b;
	*alpha = mat->alpha;

	VECCOPY(xyz_local, xyz);
	mul_m4_v3(brushOb->imat, xyz_local);

	for(tex_nr=0; tex_nr<MAX_MTEX; tex_nr++) {
		
		/* separate tex switching */
		if(mat->septex & (1<<tex_nr)) continue;
		
		if(mat->mtex[tex_nr]) {
			mtex= mat->mtex[tex_nr];
			tex= mtex->tex;
			
			tex= mtex->tex;
			if(tex==0) continue;

			/* which coords */
			if(mtex->texco==TEXCO_ORCO) {
				float l;
				/* Get generated UV */
				textured_face_generate_uv(uv, normal, xyz_local, mvert[v1].co, mvert[v2].co, mvert[v3].co);
				l= 1.0f+uv[0]+uv[1];

				/* calculate generated coordinate
				*  ** Keep up-to-date with shadeinput.c -> shade_input_set_shade_texco() **/
				co[0]= l*mvert[v3].co[0]-uv[0]*mvert[v1].co[0]-uv[1]*mvert[v2].co[0];
				co[1]= l*mvert[v3].co[1]-uv[0]*mvert[v1].co[1]-uv[1]*mvert[v2].co[1];
				co[2]= l*mvert[v3].co[2]-uv[0]*mvert[v1].co[2]-uv[1]*mvert[v2].co[2];
			}
			else if(mtex->texco==TEXCO_OBJECT) {
				Object *ob= mtex->object;

				VECCOPY(co, xyz);
				/* convert from world space to paint space */
				mul_m4_v3(brushOb->imat, co);
				if(ob) {
					mul_m4_v3(ob->imat, co);
				}
			}
			else if(mtex->texco==TEXCO_GLOB) {
				VECCOPY(co, xyz);
			}
			else if(mtex->texco==TEXCO_UV) {
				MTFace *tface;

				/* Get UV layer */
				if(mtex->uvname[0] != 0)
					tface = CustomData_get_layer_named(&orcoDm->faceData, CD_MTFACE, mtex->uvname);
				else
					tface = DM_get_face_data_layer(orcoDm, CD_MTFACE);
				/* Get generated coordinates to calculate UV from */
				textured_face_generate_uv(uv, normal, xyz_local, mvert[v1].co, mvert[v2].co, mvert[v3].co);
				/* Get UV mapping coordinate */
				textured_face_get_uv(co, normal, uv, faceIndex, isQuad, tface);
			}
			else continue;	/* non-supported types get just skipped:
							TEXCO_REFL, TEXCO_NORM, TEXCO_TANGENT
							TEXCO_WINDOW, TEXCO_STRAND, TEXCO_STRESS etc.
							*/

			/* get texture mapping */
			texco_mapping_ext(normal, tex, mtex, co, 0, 0, texvec);

			if(tex->use_nodes && tex->nodetree) {
				/* No support for nodes (yet). */
				continue;
			}
			else {
				rgbnor = multitex_ext(mtex->tex, co, 0, 0, 0, &texres);
			}

			/* texture output */
			if( (rgbnor & TEX_RGB) && (mtex->texflag & MTEX_RGBTOINT)) {
				texres.tin= (0.35*texres.tr+0.45*texres.tg+0.2*texres.tb);
				rgbnor-= TEX_RGB;
			}

			/* Negate and stencil masks */
			if(mtex->texflag & MTEX_NEGATIVE) {
				if(rgbnor & TEX_RGB) {
					texres.tr= 1.0-texres.tr;
					texres.tg= 1.0-texres.tg;
					texres.tb= 1.0-texres.tb;
				}
				texres.tin= 1.0-texres.tin;
			}
			if(mtex->texflag & MTEX_STENCIL) {
				if(rgbnor & TEX_RGB) {
					fact= texres.ta;
					texres.ta*= stencilTin;
					stencilTin*= fact;
				}
				else {
					fact= texres.tin;
					texres.tin*= stencilTin;
					stencilTin*= fact;
				}
			}

			/* mapping */
			if(mtex->mapto & (MAP_COL)) {
				float tcol[3];
				/* stencil maps on the texture control slider, not texture intensity value */
				tcol[0]=texres.tr; tcol[1]=texres.tg; tcol[2]=texres.tb;
				if((rgbnor & TEX_RGB)==0) {
					tcol[0]= mtex->r;
					tcol[1]= mtex->g;
					tcol[2]= mtex->b;
				}
				else if(mtex->mapto & MAP_ALPHA) {
					texres.tin= stencilTin;
				}
				else texres.tin= texres.ta;
				if(mtex->mapto & MAP_COL) {
					float colfac= mtex->colfac*stencilTin;
					texture_rgb_blend(color, tcol, color, texres.tin, colfac, mtex->blendtype);
				}
			}

			if(mtex->mapto & MAP_VARS) {
				/* stencil maps on the texture control slider, not texture intensity value */
				if(rgbnor & TEX_RGB) {
					if(texres.talpha) texres.tin= texres.ta;
					else texres.tin= (0.35*texres.tr+0.45*texres.tg+0.2*texres.tb);
				}

				if(mtex->mapto & MAP_ALPHA) {
					float alphafac= mtex->alphafac*stencilTin;
					*alpha= texture_value_blend(mtex->def_var, *alpha, texres.tin, alphafac, mtex->blendtype);
					if(*alpha<0.0) *alpha= 0.0;
					else if(*alpha>1.0) *alpha= 1.0;
				}
			}
		}
	}
}


/*
*	Edited version of texture.c -> do_volume_tex()
*
*	Samples color and density from a volume type texture
*	without need for ShadeInput.
*
*	Keep up-to-date with new mapping settings
*/
void dynamicPaint_sampleVolumeMaterial(float color[3], float *alpha, Material *mat, Object *brushOb, float xyz[3])
{
	int mapto_flag  = MAP_DENSITY | MAP_REFLECTION_COL | MAP_TRANSMISSION_COL;
	float *col = color;

	MTex *mtex = NULL;
	Tex *tex = NULL;
	TexResult texres= {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, NULL};
	int tex_nr, rgbnor= 0;
	float co[3], texvec[3];
	float fact, stencilTin=1.0;

	/* set base color */
	color[0] = mat->vol.reflection_col[0];
	color[1] = mat->vol.reflection_col[1];
	color[2] = mat->vol.reflection_col[2];
	*alpha = mat->vol.density;
	
	for(tex_nr=0; tex_nr<MAX_MTEX; tex_nr++) {

		/* separate tex switching */
		if(mat->septex & (1<<tex_nr)) continue;
		
		if(mat->mtex[tex_nr]) {
			mtex= mat->mtex[tex_nr];
			tex= mtex->tex;
			if(tex==0) continue;

			/* only process if this texture is mapped 
				* to one that we're interested in */
			if (!(mtex->mapto & mapto_flag)) continue;
			texres.nor= NULL;
			
			/* which coords */
			if(mtex->texco==TEXCO_OBJECT) { 
				Object *ob= mtex->object;
				ob= mtex->object;
				if(ob) {						
					VECCOPY(co, xyz);
					mul_m4_v3(ob->imat, co);
				}
			}
			else if(mtex->texco==TEXCO_ORCO) {
				{
					Object *ob= brushOb;
					VECCOPY(co, xyz);
					mul_m4_v3(ob->imat, co);
				}
			}
			else if(mtex->texco==TEXCO_GLOB) {							
				VECCOPY(co, xyz);
			}
			else continue;	/* Skip unsupported types */

			if(tex->type==TEX_IMAGE) {
				continue;	/* not supported yet */				
			}
			else {
				/* placement */
				if(mtex->projx) texvec[0]= mtex->size[0]*(co[mtex->projx-1]+mtex->ofs[0]);
				else texvec[0]= mtex->size[0]*(mtex->ofs[0]);

				if(mtex->projy) texvec[1]= mtex->size[1]*(co[mtex->projy-1]+mtex->ofs[1]);
				else texvec[1]= mtex->size[1]*(mtex->ofs[1]);

				if(mtex->projz) texvec[2]= mtex->size[2]*(co[mtex->projz-1]+mtex->ofs[2]);
				else texvec[2]= mtex->size[2]*(mtex->ofs[2]);
			}
			rgbnor= multitex_ext(tex, texvec, NULL, NULL, 0, &texres);
			
			/* texture output */
			if( (rgbnor & TEX_RGB) && (mtex->texflag & MTEX_RGBTOINT)) {
				texres.tin= (0.35*texres.tr+0.45*texres.tg+0.2*texres.tb);
				rgbnor-= TEX_RGB;
			}
			/* Negate and stencil */
			if(mtex->texflag & MTEX_NEGATIVE) {
				if(rgbnor & TEX_RGB) {
					texres.tr= 1.0-texres.tr;
					texres.tg= 1.0-texres.tg;
					texres.tb= 1.0-texres.tb;
				}
				texres.tin= 1.0-texres.tin;
			}
			if(mtex->texflag & MTEX_STENCIL) {
				if(rgbnor & TEX_RGB) {
					fact= texres.ta;
					texres.ta*= stencilTin;
					stencilTin*= fact;
				}
				else {
					fact= texres.tin;
					texres.tin*= stencilTin;
					stencilTin*= fact;
				}
			}
			
			/* Map values */
			if((mapto_flag & (MAP_EMISSION_COL+MAP_TRANSMISSION_COL+MAP_REFLECTION_COL)) && (mtex->mapto & (MAP_EMISSION_COL+MAP_TRANSMISSION_COL+MAP_REFLECTION_COL))) {
				float tcol[3];
				/* stencil maps on the texture control slider, not texture intensity value */
				if((rgbnor & TEX_RGB)==0) {
					tcol[0]= mtex->r;
					tcol[1]= mtex->g;
					tcol[2]= mtex->b;
				} else {
					tcol[0]=texres.tr;
					tcol[1]=texres.tg;
					tcol[2]=texres.tb;
					if(texres.talpha)
						texres.tin= texres.ta;
				}
				
				/* used for emit */
				if((mapto_flag & MAP_EMISSION_COL) && (mtex->mapto & MAP_EMISSION_COL)) {
					float colemitfac= mtex->colemitfac*stencilTin;
					texture_rgb_blend(col, tcol, col, texres.tin, colemitfac, mtex->blendtype);
				}
				if((mapto_flag & MAP_REFLECTION_COL) && (mtex->mapto & MAP_REFLECTION_COL)) {
					float colreflfac= mtex->colreflfac*stencilTin;
					texture_rgb_blend(col, tcol, col, texres.tin, colreflfac, mtex->blendtype);
				}
				if((mapto_flag & MAP_TRANSMISSION_COL) && (mtex->mapto & MAP_TRANSMISSION_COL)) {
					float coltransfac= mtex->coltransfac*stencilTin;
					texture_rgb_blend(col, tcol, col, texres.tin, coltransfac, mtex->blendtype);
				}
			}
			
			if((mapto_flag & MAP_VARS) && (mtex->mapto & MAP_VARS)) {
				/* stencil maps on the texture control slider, not texture intensity value */
				
				/* convert RGB to intensity if intensity info isn't provided */
				if (!(rgbnor & TEX_INT)) {
					if (rgbnor & TEX_RGB) {
						if(texres.talpha) texres.tin= texres.ta;
						else texres.tin= (0.35*texres.tr+0.45*texres.tg+0.2*texres.tb);
					}
				}
				if((mapto_flag & MAP_DENSITY) && (mtex->mapto & MAP_DENSITY)) {
					float densfac= mtex->densfac*stencilTin;

					*alpha = texture_value_blend(mtex->def_var, *alpha, texres.tin, densfac, mtex->blendtype);
					CLAMP(*alpha, 0.0, 1.0);
				}
			}
		}
	}
}

/*
*	Get material diffuse color and alpha (including linked textures) in given coordinates
*	
*	color,paint : input/output color values
*	pixelCoord : canvas pixel coordinates in global space. used if material is volumetric
*	paintHit : ray hit point on paint object surface in global space. used by "surface" type materials
*	faceIndex : ray hit face index
*	orcoDm : orco state derived mesh of paint object
*	ui_mat : force material. if NULL, material linked to mesh face is used.
*
*	*"brush object" = object to sample material color from
*/
void dynamicPaint_getMaterialColor(float *color, float *alpha, Object *brushOb, float pixelCoord[3], float paintHit[3], int faceIndex, short isQuad, DerivedMesh *orcoDm, Material *ui_mat)
{
	Material *material = ui_mat;

	/* Get face material */
	if (material == NULL) {
		MFace *mface = NULL;
		mface = orcoDm->getFaceArray(orcoDm);
		material = give_current_material(brushOb, mface[faceIndex].mat_nr+1);

		if (material == NULL) return;	/* No material assigned */
	}

	/* Sample textured material color in given position depending on material type */
	if (material->material_type == MA_TYPE_SURFACE) {
		/* Solid material */
		dynamicPaint_sampleSolidMaterial(color, alpha, material, brushOb, paintHit, faceIndex, isQuad, orcoDm);
	}
	else if (material->material_type == MA_TYPE_VOLUME) {
		/* Volumetric material */
		dynamicPaint_sampleVolumeMaterial(color, alpha, material, brushOb, pixelCoord);
	}
	else if (material->material_type == MA_TYPE_HALO) {
		/* Halo type not supported */
	}
}


/***************************** Ray / Nearest Point Utils ******************************/


/*  A modified callback to bvh tree raycast. The tree must bust have been built using bvhtree_from_mesh_faces.
*   userdata must be a BVHMeshCallbackUserdata built from the same mesh as the tree.
*  
*	To optimize brush detection speed this doesn't calculate hit coordinates or normal.
*	If ray hit the second half of a quad, no[0] is set to 1.0f.
*/
static void mesh_faces_spherecast_dp(void *userdata, int index, const BVHTreeRay *ray, BVHTreeRayHit *hit)
{
	const BVHTreeFromMesh *data = (BVHTreeFromMesh*) userdata;
	MVert *vert	= data->vert;
	MFace *face = data->face + index;
	short quad = 0;

	float *t0, *t1, *t2, *t3;
	t0 = vert[ face->v1 ].co;
	t1 = vert[ face->v2 ].co;
	t2 = vert[ face->v3 ].co;
	t3 = face->v4 ? vert[ face->v4].co : NULL;

	do
	{	
		float dist;
		dist = ray_tri_intersection(ray, hit->dist, t0, t1, t2);

		if(dist >= 0 && dist < hit->dist)
		{
			hit->index = index;
			hit->dist = dist;
			hit->no[0] = (quad) ? 1.0f : 0.0f;
		}

		t1 = t2;
		t2 = t3;
		t3 = NULL;
		quad = 1;

	} while(t2);
}

/* A modified callback to bvh tree nearest point. The tree must bust have been built using bvhtree_from_mesh_faces.
*  userdata must be a BVHMeshCallbackUserdata built from the same mesh as the tree.
*  
*	To optimize brush detection speed this doesn't calculate hit normal.
*	If ray hit the second half of a quad, no[0] is set to 1.0f, else 0.0f
*/
static void mesh_faces_nearest_point_dp(void *userdata, int index, const float *co, BVHTreeNearest *nearest)
{
	const BVHTreeFromMesh *data = (BVHTreeFromMesh*) userdata;
	MVert *vert	= data->vert;
	MFace *face = data->face + index;
	short quad = 0;

	float *t0, *t1, *t2, *t3;
	t0 = vert[ face->v1 ].co;
	t1 = vert[ face->v2 ].co;
	t2 = vert[ face->v3 ].co;
	t3 = face->v4 ? vert[ face->v4].co : NULL;

	do
	{	
		float nearest_tmp[3], dist;
		int vertex, edge;
		
		dist = nearest_point_in_tri_surface(t0, t1, t2, co, &vertex, &edge, nearest_tmp);
		if(dist < nearest->dist)
		{
			nearest->index = index;
			nearest->dist = dist;
			VECCOPY(nearest->co, nearest_tmp);
			nearest->no[0] = (quad) ? 1.0f : 0.0f;
		}

		t1 = t2;
		t2 = t3;
		t3 = NULL;
		quad = 1;

	} while(t2);
}


/***************************** Painting Calls ******************************/

/*
*	Mix color values to canvas point.
*
*	surface : canvas surface
*	index : surface point index
*	paintFlags : paint object flags
*   paintColor,Alpha,Wetness : to be mixed paint values
*	timescale : value used to adjust time dependand
*			    operations when using substeps
*/
static void dynamicPaint_mixPaintColors(DynamicPaintSurface *surface, int index, int paintFlags, float *paintColor, float *paintAlpha, float *paintWetness, float *timescale)
{
	PaintPoint *pPoint = &((PaintPoint*)surface->data->type_data)[index];

	/* Add paint	*/
	if (!(paintFlags & MOD_DPAINT_ERASE)) {
		float wetness;
		float temp_alpha = (*paintAlpha) * ((paintFlags & MOD_DPAINT_ABS_ALPHA) ? 1.0f : (*timescale));

		/* mix brush color with wet layer color */
		if (temp_alpha) mixColors(pPoint->e_color, pPoint->e_alpha, paintColor, temp_alpha);

		/* alpha */
		if (paintFlags & MOD_DPAINT_ABS_ALPHA) {
			if (pPoint->e_alpha < (*paintAlpha)) pPoint->e_alpha = (*paintAlpha);
		}
		else {
			pPoint->e_alpha += temp_alpha;
			if (pPoint->e_alpha > 1.0f) pPoint->e_alpha = 1.0f;
		}

		/* only increase wetness if it's below paint level	*/
		wetness = (*paintWetness) * pPoint->e_alpha;
		if (pPoint->wetness < wetness) pPoint->wetness = wetness;
	}
	/* Erase paint	*/
	else {
		float a_ratio, a_highest;
		float wetness;
		float invFact = 1.0f - (*paintAlpha);

		/*
		*	Make highest alpha to match erased value
		*	but maintain alpha ratio
		*/
		if (paintFlags & MOD_DPAINT_ABS_ALPHA) {
			a_highest = (pPoint->e_alpha > pPoint->alpha) ? pPoint->e_alpha : pPoint->alpha;
			if (a_highest > invFact) {
				a_ratio = invFact / a_highest;

				pPoint->e_alpha *= a_ratio;
				pPoint->alpha *= a_ratio;
			}
		}
		else {
			pPoint->e_alpha -= (*paintAlpha) * (*timescale);
			if (pPoint->e_alpha < 0.0f) pPoint->e_alpha = 0.0f;
			pPoint->alpha -= (*paintAlpha) * (*timescale);
			if (pPoint->alpha < 0.0f) pPoint->alpha = 0.0f;
		}

		wetness = (1.0f - (*paintWetness)) * pPoint->e_alpha;
		if (pPoint->wetness > wetness) pPoint->wetness = wetness;
	}
}

static void dynamicPaint_mixWaveHeight(PaintWavePoint *wPoint, DynamicPaintBrushSettings *brush, float isect_height)
{
	int hit = 0;
	isect_height *= brush->wave_factor;

	/* determine hit depending on wave_factor */
	if (brush->wave_factor > 0.0f && wPoint->height > isect_height)
		hit = 1;
	else if (brush->wave_factor < 0.0f && wPoint->height < isect_height)
		hit = 1;

	if (hit) {
		if (brush->wave_type == MOD_DPAINT_WAVEB_DEPTH) {
			wPoint->height = isect_height;
			wPoint->state = 1;
			wPoint->velocity = 0.0f;
		}
		else if (brush->wave_type == MOD_DPAINT_WAVEB_FORCE)
			wPoint->velocity = isect_height;
		else if (brush->wave_type == MOD_DPAINT_WAVEB_REFLECT)
			wPoint->state = 2;
	}
}

/*
*	Paint a brush object mesh to the surface
*/
static int dynamicPaint_paintMesh(DynamicPaintSurface *surface, PaintBakeData *bData, DynamicPaintBrushSettings *brush, Object *canvasOb, Object *brushOb, float timescale)
{
	DerivedMesh *dm = NULL;
	MVert *mvert = NULL;
	MFace *mface = NULL;
	PaintSurfaceData *sData = surface->data;

	if (!brush->dm) return 0;

	{
		BVHTreeFromMesh treeData = {0};
		int index;

		int numOfVerts;
		int ii;

		/*
		*	Transform collider vertices to world space
		*	(Faster than transforming per pixel
		*   coordinates and normals to object space)
		*/
		dm = CDDM_copy(brush->dm);
		mvert = dm->getVertArray(dm);
		mface = dm->getFaceArray(dm);
		numOfVerts = dm->getNumVerts(dm);

		for (ii=0; ii<numOfVerts; ii++) {
			mul_m4_v3(brushOb->obmat, mvert[ii].co);
		}

		/* Build a bvh tree from transformed vertices	*/
		bvhtree_from_mesh_faces(&treeData, dm, 0.0f, 4, 6);

		if(treeData.tree) {
			#pragma omp parallel for schedule(static)
			for (index = 0; index < sData->total_points; index++)
			{
				{
					int ss;
					float brushFactor = 0.0f;	/* brush influence factor */
					float depth = 0.0f;		/* displace depth */

					float paintColor[3] = {0.0f, 0.0f, 0.0f};
					int numOfHits = 0;
					float paintAlpha = 0.0f;

					/* Supersampling	*/
					for (ss=0; ss<sData->samples; ss++)
					{

						float ray_start[3], ray_dir[3];
						float gaus_factor;
						BVHTreeRayHit hit;
						BVHTreeNearest nearest;
						short hit_found = 0;

						/* If it's a proximity hit, store distance rate */
						float distRate = -1.0f;

						/* hit data	*/
						float hitCoord[3];		/* mid-sample hit coordinate */
						int hitFace = -1;		/* mid-sample hit face */
						short hitQuad;			/* mid-sample hit quad status */

						/* Supersampling factor	*/
						if (sData->samples > 1) {
							gaus_factor = gaussianFactors[ss];
						}
						else {
							gaus_factor = 1.0f;
						}

						/* Get current sample position in world coordinates	*/
						VECCOPY(ray_start, &bData->realCoord[(sData->total_points*ss+index)*3]);
						VECCOPY(ray_dir, bData->bPoint[index].invNorm);

						hit.index = -1;
						hit.dist = 9999;
						nearest.index = -1;
						nearest.dist = brush->paint_distance * brush->paint_distance; /* find_nearest search uses squared distance */

						/* Check volume collision	*/
						if (brush->collision == MOD_DPAINT_COL_VOLUME || brush->collision == MOD_DPAINT_COL_VOLDIST)
						if(BLI_bvhtree_ray_cast(treeData.tree, ray_start, ray_dir, 0.0f, &hit, mesh_faces_spherecast_dp, &treeData) != -1)
						{
							/* We hit a triangle, now check if collision point normal is facing the point	*/


							/*	For optimization sake, hit point normal isn't calculated in ray cast loop	*/
							int v1=mface[hit.index].v1, v2=mface[hit.index].v2, v3=mface[hit.index].v3, quad=(hit.no[0] == 1.0f);
							float dot;

							if (quad) {v2=mface[hit.index].v3; v3=mface[hit.index].v4;}
						
							/* Get hit normal	*/
							normal_tri_v3( hit.no, mvert[v1].co, mvert[v2].co, mvert[v3].co);
							dot = ray_dir[0]*hit.no[0] + ray_dir[1]*hit.no[1] + ray_dir[2]*hit.no[2];

							/*
							*	If ray and hit normal are facing same direction
							*	hit point is inside a closed mesh.
							*/
							if (dot>=0)
							{
								/* Add factor on supersample filter	*/
								brushFactor += gaus_factor;
								depth += hit.dist;
								hit_found = 1;

								/*
								*	Mark hit info
								*/
								if (hitFace == -1) {
									VECADDFAC(hitCoord, ray_start, ray_dir, hit.dist);	/* Calculate final hit coordinates */
									hitQuad = quad;
									hitFace = hit.index;
								}
							}
						}	// end of raycast
					
						/* Check proximity collision	*/
						if ((brush->collision == MOD_DPAINT_COL_DIST || brush->collision == MOD_DPAINT_COL_VOLDIST))
						{
							float proxDist = -1.0f;
							float hitCo[3];
							short hQuad;
							int face;

							/*
							*	If pure distance proximity, find the nearest point on the mesh
							*/
							if (!(brush->flags & MOD_DPAINT_PROX_FACEALIGNED)) {
								if (BLI_bvhtree_find_nearest(treeData.tree, ray_start, &nearest, mesh_faces_nearest_point_dp, &treeData) != -1) {
									proxDist = sqrt(nearest.dist);	/* find_nearest returns a squared distance, so gotta change it back to real distance */
									copy_v3_v3(hitCo, nearest.co);
									hQuad = (nearest.no[0] == 1.0f);
									face = nearest.index;
								}
							}
							else { /*  else cast a ray in surface normal direction	*/
								negate_v3(ray_dir);
								hit.index = -1;
								hit.dist = brush->paint_distance;

								/* Do a face normal directional raycast, and use that distance	*/
								if(BLI_bvhtree_ray_cast(treeData.tree, ray_start, ray_dir, 0.0f, &hit, mesh_faces_spherecast_dp, &treeData) != -1)
								{
									proxDist = hit.dist;
									VECADDFAC(hitCo, ray_start, ray_dir, hit.dist);	/* Calculate final hit coordinates */
									hQuad = (hit.no[0] == 1.0f);
									face = hit.index;
								}
							}

							/* If a hit was found, calculate required values	*/
							if (proxDist >= 0.0f) {
								float dist_rate = proxDist / brush->paint_distance;
								float prox_influence = 0.0f;

								/* Smooth range or color ramp	*/
								if (brush->proximity_falloff == MOD_DPAINT_PRFALL_SMOOTH ||
									brush->proximity_falloff == MOD_DPAINT_PRFALL_RAMP) {

									/* Limit distance to 0.0 - 1.0 */
									if (dist_rate > 1.0f) dist_rate = 1.0f;
									if (dist_rate < 0.0f) dist_rate = 0.0f;

									/* if using smooth falloff, multiply gaussian factor */
									if (brush->proximity_falloff == MOD_DPAINT_PRFALL_SMOOTH) {
										prox_influence = (1.0f - dist_rate) * gaus_factor;
									}
									else prox_influence = gaus_factor;

									if (hitFace == -1) {
										distRate = dist_rate;
									}
								}
								else prox_influence = gaus_factor;

								hit_found = 1;
								if (brush->flags & MOD_DPAINT_INVERSE_PROX) {
									brushFactor -= prox_influence;
									distRate = -distRate;
								}
								else 
									brushFactor += prox_influence;

								if (hitFace == -1) {
									copy_v3_v3(hitCoord, hitCo);
									hitQuad = hQuad;
									hitFace = face;
								}
							}
						}

						/*
						*	Process color and alpha
						*/
						if (hit_found)
						{
							float sampleColor[3];
							float sampleAlpha = 1.0f;
							float bandres[4];

							sampleColor[0] = brush->r;
							sampleColor[1] = brush->g;
							sampleColor[2] = brush->b;
						
							/* Get material+textures color on hit point if required	*/
							if (brush->flags & MOD_DPAINT_USE_MATERIAL) dynamicPaint_getMaterialColor(sampleColor, &sampleAlpha, brushOb, &bData->realCoord[(sData->total_points*ss+index)*3], hitCoord, hitFace, hitQuad, brush->dm, brush->mat);

							/* Sample colorband if required	*/
							if ((distRate >= 0.0f) && (brush->proximity_falloff == MOD_DPAINT_PRFALL_RAMP) && do_colorband(brush->paint_ramp, distRate, bandres)) {
								if (!(brush->flags & MOD_DPAINT_RAMP_ALPHA)) {
									sampleColor[0] = bandres[0];
									sampleColor[1] = bandres[1];
									sampleColor[2] = bandres[2];
								}
								sampleAlpha *= bandres[3];
							}

							/* Add AA sample */
							paintColor[0] += sampleColor[0];
							paintColor[1] += sampleColor[1];
							paintColor[2] += sampleColor[2];

							paintAlpha += sampleAlpha;
							numOfHits++;
						}
					} // end supersampling


					/* if any sample was inside paint range	*/
					if (brushFactor > 0.01f) {

						/* apply supersampling results	*/
						if (sData->samples > 1) {
							brushFactor /= gaussianTotal;
						}
						CLAMP(brushFactor, 0.0f, 1.0f);
						brushFactor *= brush->alpha;

						//cPoint->state = 2;

						if (surface->type == MOD_DPAINT_SURFACE_T_PAINT) {

							float paintWetness = brush->wetness * brushFactor;

							/* Get final pixel color and alpha	*/
							paintColor[0] /= numOfHits;
							paintColor[1] /= numOfHits;
							paintColor[2] /= numOfHits;
							paintAlpha /= numOfHits;

							/* Multiply alpha value by the ui multiplier	*/
							paintAlpha = paintAlpha * brushFactor;
							if (paintAlpha > 1.0f) paintAlpha = 1.0f;

							/*
							*	Mix paint to the surface
							*/
							dynamicPaint_mixPaintColors(surface, index, brush->flags, paintColor, &paintAlpha, &paintWetness, &timescale);
						}
						/* displace */
						else if (surface->type == MOD_DPAINT_SURFACE_T_DISPLACE) {
							float *value = (float*)sData->type_data;

							if (brush->flags & MOD_DPAINT_ERASE) {
								value[index] *= (1.0f - brushFactor);
								if (value[index] < 0.0f) value[index] = 0.0f;
							}
							else {
								depth /= bData->bPoint[index].normal_scale;
								/* do displace	*/
								if (value[index] < depth) value[index] = depth;
							}
						}
						/* vertex weight */
						else if (surface->type == MOD_DPAINT_SURFACE_T_WEIGHT) {
							float *value = (float*)sData->type_data;

							if (brush->flags & MOD_DPAINT_ERASE) {
								value[index] *= (1.0f - brushFactor);
								if (value[index] < 0.0f) value[index] = 0.0f;
							}
							else {
								if (brush->flags & MOD_DPAINT_ABS_ALPHA) {
									if (value[index] < brushFactor) value[index] = brushFactor;
								}
								else {
									value[index] += brushFactor;
									if (value[index] > 1.0f) value[index] = 1.0f;
								}
							}
						}
						/* wave */
						else if (surface->type == MOD_DPAINT_SURFACE_T_WAVE) {
							depth *= -1.0f/bData->bPoint[index].normal_scale;

							dynamicPaint_mixWaveHeight(&((PaintWavePoint*)surface->data->type_data)[index],
									brush, depth);
						}
					}
				}
			}
		}
		/* free bhv tree */
		free_bvhtree_from_mesh(&treeData);
		dm->release(dm);

	}

	return 1;
}

/*
*	Paint a particle system to the surface
*/
static int dynamicPaint_paintParticles(DynamicPaintSurface *surface, PaintBakeData *bData, ParticleSystem *psys, DynamicPaintBrushSettings *brush, Object *canvasOb, float timescale)
{
	int index;
	ParticleSettings *part=psys->part;
	ParticleData *pa = NULL;
	PaintSurfaceData *sData = surface->data;

	KDTree *tree;
	int particlesAdded = 0;
	int invalidParticles = 0;
	int p = 0;

	if (psys->totpart < 1) return 1;

	/*
	*	Build a kd-tree to optimize distance search
	*/
	tree= BLI_kdtree_new(psys->totpart);

	/* loop through particles and insert valid ones	to the tree	*/
	for(p=0, pa=psys->particles; p<psys->totpart; p++, pa++)	{

		/* Proceed only if particle is active	*/
		if(pa->alive == PARS_UNBORN && (part->flag & PART_UNBORN)==0) continue;									
		else if(pa->alive == PARS_DEAD && (part->flag & PART_DIED)==0) continue;									
		else if(pa->flag & PARS_NO_DISP || pa->flag & PARS_UNEXIST) continue;

		/*	for debug purposes check if any NAN particle proceeds
		*	For some reason they get past activity check, this should rule most of them out	*/
		if (isnan(pa->state.co[0]) || isnan(pa->state.co[1]) || isnan(pa->state.co[2])) {invalidParticles++;continue;}

		BLI_kdtree_insert(tree, p, pa->state.co, NULL);
		particlesAdded++;
	}
	if (invalidParticles)
		printf("Warning: Invalid particle(s) found!\n");

	/* If no suitable particles were found, exit	*/
	if (particlesAdded < 1) {
		BLI_kdtree_free(tree);
		return 1;
	}
	/* balance tree	*/
	BLI_kdtree_balance(tree);

	/*
	*	Loop through every surface point
	*/
	#pragma omp parallel for schedule(static)
	for (index = 0; index < sData->total_points; index++)
	{
		float disp_intersect = 0;
		float radius;
		float solidradius = brush->particle_radius;
		float smooth = brush->particle_smooth;
		float strength = 0.0f;

		/* If using per particle radius	*/
		if (brush->flags & MOD_DPAINT_PART_RAD) {
			/*
			*	If we use per particle radius, we have to sample all particles
			*	within max radius range
			*/
			KDTreeNearest *nearest = NULL;
			int n, particles = 0;
			float range = psys->part->size + smooth;

			particles = BLI_kdtree_range_search(tree, range, &bData->realCoord[index*3], NULL, &nearest);
			for(n=0; n<particles; n++) {

				/*
				*	Find particle that produces highest influence
				*/
				ParticleData *pa = psys->particles + nearest[n].index;
				float rad = pa->size + smooth;
				float str,smooth_range;

				if (nearest[n].dist > rad) continue; /* if outside range, continue to next one */

				if ((rad-nearest[n].dist) > disp_intersect) {
					disp_intersect = rad-nearest[n].dist;
					radius = rad;
				}

				/* Continue with paint check	*/
				if (nearest[n].dist < pa->size) {
					/*  If particle is inside the solid range, no need to continue further
					*	since no other particle can have higher influence */
					strength = 1.0f;
					break;
				}
				smooth_range = (nearest[n].dist - pa->size);

				/* do smoothness if enabled	*/
				if (smooth) smooth_range/=smooth;
				str = 1.0f - smooth_range;
				/* if influence is greater, use this one	*/
				if (str > strength) strength = str;

			}
			if (nearest) MEM_freeN(nearest);

		}
		else {
			/*
			*	With predefined radius, there is no variation between particles.
			*	It's enough to just find the nearest one.
			*/
			KDTreeNearest nearest;
			float smooth_range;
			radius = solidradius + smooth;

			/* Find nearest particle and get distance to it	*/
			BLI_kdtree_find_nearest(tree, &bData->realCoord[index*3], NULL, &nearest);
			if (nearest.dist > radius) continue;

			/* distances inside solid radius have maximum influence -> dist = 0	*/
			smooth_range = (nearest.dist - solidradius);
			if (smooth_range<0) smooth_range=0.0f;
			/* do smoothness if enabled	*/
			if (smooth) smooth_range/=smooth;

			strength = 1.0f - smooth_range;
			disp_intersect = radius - nearest.dist;
		}

		if (strength <= 0.0f) continue;

		//cPoint->state = 2;

		if (surface->type == MOD_DPAINT_SURFACE_T_PAINT) {

			float paintAlpha = brush->alpha * strength;
			float paintWetness = brush->wetness * strength;
			float paintColor[3];
			paintColor[0] = brush->r;
			paintColor[1] = brush->g;
			paintColor[2] = brush->b;

			if (paintAlpha > 1.0f) paintAlpha = 1.0f;

			dynamicPaint_mixPaintColors(surface, index, brush->flags, paintColor, &paintAlpha, &paintWetness, &timescale);

		}
		else if (surface->type == MOD_DPAINT_SURFACE_T_DISPLACE) {
			float sdepth;
			float *value = (float*)sData->type_data;

			/* change falloff type to inverse square to match real displace depth	*/
			disp_intersect = (1.0f - sqrt(disp_intersect / radius)) * radius;
			/* get displace depth	*/
			sdepth = (radius - disp_intersect) / bData->bPoint[index].normal_scale;

			if (sdepth<0.0f) sdepth = 0.0f;
			if (value[index] < sdepth) value[index] = sdepth;
		}
		else if (surface->type == MOD_DPAINT_SURFACE_T_WEIGHT) {
			float *value = (float*)sData->type_data;

			if (brush->flags & MOD_DPAINT_ERASE) {
				value[index] *= (1.0f - strength);
				if (value[index] < 0.0f) value[index] = 0.0f;
			}
			else {
				if (value[index] < strength) value[index] = strength;
			}
		}
		/* wave */
		else if (surface->type == MOD_DPAINT_SURFACE_T_WAVE) {
			float sdepth;

			/* change falloff type to inverse square to match real displace depth	*/
			disp_intersect = (1.0f - sqrt(disp_intersect / radius)) * radius;
			/* get displace depth	*/
			sdepth = -1.0f * (radius - disp_intersect) / bData->bPoint[index].normal_scale;

			dynamicPaint_mixWaveHeight(&((PaintWavePoint*)surface->data->type_data)[index],
					brush, sdepth);
		}
	}
	BLI_kdtree_free(tree);

	return 1;
}


/***************************** Dynamic Paint Step / Baking ******************************/

/*
*	Calculate current frame neighbouring point distances
*	and direction vectors
*/
static void dynamicPaint_prepareNeighbourData(DynamicPaintSurface *surface, BakeNeighPoint *bNeighs, float *realCoords)
{
	PaintSurfaceData *sData = surface->data;
	int index;

	#pragma omp parallel for schedule(static)
	for (index = 0; index < sData->total_points; index++)
	{
		int i;
		int numOfNeighs = sData->adj_data->numOf_n[index];

		for (i=0; i<numOfNeighs; i++) {
			int n_index = sData->adj_data->n_index[index]+i;
			int t_index = sData->adj_data->n_target[n_index];

			/* dir vec */
			sub_v3_v3v3(bNeighs[n_index].dir, &realCoords[t_index*3], &realCoords[index*3]);
			/* dist */
			bNeighs[n_index].dist = len_v3(bNeighs[n_index].dir);
			/* normalize dir */
			if (bNeighs[n_index].dist) mul_v3_fl(bNeighs[n_index].dir, 1.0f/bNeighs[n_index].dist);
		}
	}
}

/* paint effect default movement per frame in global units */
#define EFF_MOVEMENT_PER_FRAME 0.05f

/*
*	Prepare data required by effects for current frame.
*	Returns number of steps required
*/
static int dynamicPaint_prepareEffectStep(DynamicPaintSurface *surface, Scene *scene, Object *ob, BakeNeighPoint *bNeighs, float **force, float *realCoords, float timescale)
{
	double average_dist = 0.0f;
	double average_force = 0.0f;
	float shrink_speed=0.0f, spread_speed=0.0f;
	float fastest_effect;
	int steps;
	PaintSurfaceData *sData = surface->data;
	int index;

	/* Init force data if required */
	if (surface->effect & MOD_DPAINT_EFFECT_DO_DRIP) {
		float vel[3] = {0};
		ListBase *effectors = pdInitEffectors(scene, ob, NULL, surface->effector_weights);

		/* allocate memory for force data (dir vector + strength) */
		*force = MEM_mallocN(sData->total_points*4*sizeof(float), "PaintEffectForces");

		if (*force) {
			#pragma omp parallel for schedule(static)
			for (index = 0; index < sData->total_points; index++)
			{
				float forc[3] = {0};

				/* apply force fields */
				if (effectors) {
					EffectedPoint epoint;
					pd_point_from_loc(scene, &realCoords[index*3], vel, index, &epoint);
					epoint.vel_to_sec = 1.0f;
					pdDoEffectors(effectors, NULL, surface->effector_weights, &epoint, forc, NULL);
				}

				/* if global gravity is enabled, add it too */
				if (scene->physics_settings.flag & PHYS_GLOBAL_GRAVITY)
					/* also divide by 10 to about match default grav
					*  with default force strength (1.0) */
					madd_v3_v3fl(forc, scene->physics_settings.gravity, 
								surface->effector_weights->global_gravity*surface->effector_weights->weight[0] / 10.f);

				/* force strength */
				(*force)[index*4+3] = len_v3(forc);
				/* normalize and copy */
				if ((*force)[index*4+3]) mul_v3_fl(forc, 1.0f/(*force)[index*4+3]);
				VECCOPY(&((*force)[index*4]), forc);
			}
		}
		pdEndEffectors(&effectors);
	}

	/* calculate average values (single thread) */
	for (index = 0; index < sData->total_points; index++)
	{
		int i;
		int numOfNeighs = sData->adj_data->numOf_n[index];

		if (*force)
			average_force += (*force)[index*4+3];

		for (i=0; i<numOfNeighs; i++) {
			average_dist += bNeighs[sData->adj_data->n_index[index]+i].dist;
		}
	}
	average_force /= sData->total_points;
	average_dist  /= sData->adj_data->total_targets;

	/* Get number of required steps using averate point distance
	*  so that just a few ultra close pixels wont up substeps to max */

	/* adjust number of required substep by fastest active effect */
	if (surface->effect & MOD_DPAINT_EFFECT_DO_SPREAD)
		spread_speed = surface->spread_speed;
	if (surface->effect & MOD_DPAINT_EFFECT_DO_SHRINK)
		shrink_speed = surface->shrink_speed;

	fastest_effect = MAX3(spread_speed, shrink_speed, average_force);

	steps = (int)ceil(EFF_MOVEMENT_PER_FRAME*fastest_effect/average_dist*timescale);
	CLAMP(steps, 1, 8);

	//printf("Average distance is %f, avg force %f, num of steps %i\n", average_dist, average_force, steps);

	return steps;
}

/*
*	Processes active effect step.
*/
static void dynamicPaint_doEffectStep(DynamicPaintSurface *surface, BakeNeighPoint *bNeighs, float *force, PaintPoint *prevPoint, float timescale)
{
	PaintSurfaceData *sData = surface->data;
	int index;

	if (!sData->adj_data) return;
	/*
	*	Spread Effect
	*/
	if (surface->effect & MOD_DPAINT_EFFECT_DO_SPREAD)  {
		float eff_scale = EFF_MOVEMENT_PER_FRAME*surface->spread_speed*timescale;

		/* Copy current surface to the previous points array to read unmodified values	*/
		memcpy(prevPoint, sData->type_data, sData->total_points*sizeof(struct PaintPoint));

		#pragma omp parallel for schedule(static)
		for (index = 0; index < sData->total_points; index++)
		{
			int i;
			int numOfNeighs = sData->adj_data->numOf_n[index];
			float totalAlpha = 0.0f;
			PaintPoint *pPoint = &((PaintPoint*)sData->type_data)[index];

			/*
			*	Only reads values from the surface copy (prevPoint[]),
			*	so this one is thread safe
			*/

			/*	Loop through neighbouring points	*/
			for (i=0; i<numOfNeighs; i++) {
				int n_index = sData->adj_data->n_index[index]+i;
				float w_factor, alphaAdd = 0.0f;
				PaintPoint *ePoint = &prevPoint[sData->adj_data->n_target[n_index]];
				float speed_scale = (bNeighs[n_index].dist<eff_scale) ? 1.0f : eff_scale/bNeighs[n_index].dist;

				totalAlpha += ePoint->e_alpha;

				/* Check if neighbouring point has higher wetness,
				*  if so, add it's wetness to this point as well*/
				if (ePoint->wetness <= pPoint->wetness) continue;
				w_factor = ePoint->wetness/numOfNeighs * (ePoint->wetness - pPoint->wetness) * speed_scale;
				if (w_factor <= 0.0f) continue;

				if (ePoint->e_alpha > pPoint->e_alpha) {
					alphaAdd = ePoint->e_alpha/numOfNeighs * (ePoint->wetness*ePoint->e_alpha - pPoint->wetness*pPoint->e_alpha) * speed_scale;
				}

				/* mix new color */
				mixColors(pPoint->e_color, pPoint->e_alpha, ePoint->e_color, w_factor);

				pPoint->e_alpha += alphaAdd;
				pPoint->wetness += w_factor;

				if (pPoint->e_alpha > 1.0f) pPoint->e_alpha = 1.0f;
			}

			/* For antialiasing sake, don't let alpha go much higher than average alpha of neighbours	*/
			if (pPoint->e_alpha > (totalAlpha/numOfNeighs+0.25f)) {
				pPoint->e_alpha = (totalAlpha/numOfNeighs+0.25f);
				if (pPoint->e_alpha>1.0f) pPoint->e_alpha = 1.0f;
			}
		}
	}

	/*
	*	Shrink Effect
	*/
	if (surface->effect & MOD_DPAINT_EFFECT_DO_SHRINK)  {
		float eff_scale = EFF_MOVEMENT_PER_FRAME*surface->shrink_speed*timescale;

		/* Copy current surface to the previous points array to read unmodified values	*/
		memcpy(prevPoint, sData->type_data, sData->total_points*sizeof(struct PaintPoint));

		#pragma omp parallel for schedule(static)
		for (index = 0; index < sData->total_points; index++)
		{
			int i;
			int numOfNeighs = sData->adj_data->numOf_n[index];
			float totalAlpha = 0.0f;
			PaintPoint *pPoint = &((PaintPoint*)sData->type_data)[index];

			for (i=0; i<numOfNeighs; i++) {
				int n_index = sData->adj_data->n_index[index]+i;
				float speed_scale = (bNeighs[n_index].dist<eff_scale) ? 1.0f : eff_scale/bNeighs[n_index].dist;
				PaintPoint *ePoint = &prevPoint[sData->adj_data->n_target[n_index]];
				float a_factor, ea_factor, w_factor;

				totalAlpha += ePoint->e_alpha;

				/* Check if neighbouring point has lower alpha,
				*  if so, decrease this point's alpha as well*/
				if (pPoint->alpha <= 0.0f && pPoint->e_alpha <= 0.0f && pPoint->wetness <= 0.0f) continue;

				/* decrease factor for dry paint alpha */
				a_factor = (1.0f - ePoint->alpha)/numOfNeighs * (pPoint->alpha - ePoint->alpha) * speed_scale;
				if (a_factor < 0.0f) a_factor = 0.0f;
				/* decrease factor for wet paint alpha */
				ea_factor = (1.0f - ePoint->e_alpha)/8 * (pPoint->e_alpha - ePoint->e_alpha) * speed_scale;
				if (ea_factor < 0.0f) ea_factor = 0.0f;
				/* decrease factor for paint wetness */
				w_factor = (1.0f - ePoint->wetness)/8 * (pPoint->wetness - ePoint->wetness) * speed_scale;
				if (w_factor < 0.0f) w_factor = 0.0f;

				if (a_factor) {
					pPoint->alpha -= a_factor;
					if (pPoint->alpha < 0.0f) pPoint->alpha = 0.0f;
					pPoint->wetness -= a_factor;

				}
				else {
					pPoint->e_alpha -= ea_factor;
					if (pPoint->e_alpha < 0.0f) pPoint->e_alpha = 0.0f;
					pPoint->wetness -= w_factor;
					if (pPoint->wetness < 0.0f) pPoint->wetness = 0.0f;
				}
			}
		}
	}

	/*
	*	Drip Effect
	*/
	if (surface->effect & MOD_DPAINT_EFFECT_DO_DRIP && force) 
	{
		float eff_scale = EFF_MOVEMENT_PER_FRAME*timescale/10.0f;
		for (index = 0; index < sData->total_points; index++) {
			int i;
			int numOfNeighs = sData->adj_data->numOf_n[index];
			PaintPoint *pPoint = &((PaintPoint*)sData->type_data)[index];

			/* to make dripping happen with as little spread as possible,
			*  only use two closest point dirs "around" the force dir */
			int closest_id[2] = {-1, -1};
			float closest_d[2] = {-1.0f, -1.0f};

			/* adjust drip speed depending on wetness */
			float w_factor = pPoint->wetness*0.4 - 0.05f;
			if (w_factor <= 0) continue;
			w_factor *= w_factor;

			/* find closest neigh dir */
			for (i=0; i<numOfNeighs; i++) {
				int n_index = sData->adj_data->n_index[index]+i;
				float dir_dot = dot_v3v3(bNeighs[n_index].dir, &force[index*4]);

				if (dir_dot>closest_d[0] && dir_dot>0.0f) {closest_d[0]=dir_dot; closest_id[0]=n_index;}
			}

			/* find other neigh */
			for (i=0; i<numOfNeighs; i++) {
				int n_index = sData->adj_data->n_index[index]+i;
				float dir_dot = dot_v3v3(bNeighs[n_index].dir, &force[index*4]);
				float closest_dot = dot_v3v3(bNeighs[n_index].dir, bNeighs[closest_id[0]].dir);

				if (n_index == closest_id[0]) continue;

				/* only accept neighbour at "other side" of the first one in relation to force dir
				*  so make sure angle between this and closest neigh is greater than first angle */
				if (dir_dot>closest_d[1] && closest_dot<closest_d[0] && dir_dot>0.0f) {closest_d[1]=dir_dot; closest_id[1]=n_index;}
			}

			/* if two valid neighs found, change closest_d
			*  values to match final paint factors */
			if (closest_id[1] != -1) {
				float neigh_diff = dot_v3v3(bNeighs[closest_id[0]].dir, bNeighs[closest_id[1]].dir);
				/* linearize both values */
				neigh_diff *= neigh_diff;
				closest_d[0] *= closest_d[0];

				/* get relation to angle between neighs */
				closest_d[0] -= neigh_diff;
				/* and normalize as relative angle within neigh range */
				closest_d[0] /= 1.0f - neigh_diff;

				/* simply set other neigh to cover missing half of this factor */
				closest_d[1] = 1.0f - closest_d[0];
			}
			else if (closest_id[0] != -1) {
				/* if only one neigh, still linearize to minimize spread */
				closest_d[0] *= closest_d[0];
			}


			/* Apply movement towards those two points */
			for (i=0; i<2; i++) {
				int n_index = closest_id[i];
				if (n_index != -1 && closest_d[i]>0.0f) {
					float dir_dot = closest_d[i], dir_factor;
					float speed_scale = eff_scale*force[index*4+3]/bNeighs[n_index].dist;
					PaintPoint *ePoint = &((PaintPoint*)sData->type_data)[sData->adj_data->n_target[n_index]];

					/* just skip if angle is too extreme */
					if (dir_dot <= 0.0f) continue;

					dir_factor = dir_dot * speed_scale * w_factor;
					if (dir_factor > 1.0f) dir_factor = 1.0f;

					/* mix new color */
					mixColors(ePoint->e_color, ePoint->e_alpha, pPoint->e_color, dir_factor);

					ePoint->e_alpha += dir_factor;
					ePoint->wetness += dir_factor;
					if (ePoint->e_alpha > 1.0f) ePoint->e_alpha = 1.0f;

					/* and decrease paint wetness on current point */
					pPoint->wetness -= dir_factor;
				}
			}
		}

		/* Keep values within acceptable range */
		#pragma omp parallel for schedule(static)
		for (index = 0; index < sData->total_points; index++)
		{
			PaintPoint *cPoint = &((PaintPoint*)sData->type_data)[index];

			if (cPoint->e_alpha > 1.0f) cPoint->e_alpha=1.0f;
			if (cPoint->wetness > 2.5f) cPoint->wetness=2.5f;

			if (cPoint->e_alpha < 0.0f) cPoint->e_alpha=0.0f;
			if (cPoint->wetness < 0.0f) cPoint->wetness=0.0f;
		}
	}
}

#define WAVE_TIME_FAC 0.1

void dynamicPaint_doWaveStep(DynamicPaintSurface *surface, BakeNeighPoint *bNeighs, float timescale)
{
	PaintSurfaceData *sData = surface->data;
	int index;
	int steps, ss;
	float dt, min_dist, damp_factor;
	float wave_speed = surface->wave_speed;
	double average_dist = 0.0f;

	/* allocate memory */
	PaintWavePoint *prevPoint = MEM_mallocN(sData->total_points*sizeof(PaintWavePoint), "Temp previous points for wave simulation");
	if (!prevPoint) return;

	/* calculate average neigh distance (single thread) */
	for (index = 0; index < sData->total_points; index++)
	{
		int i;
		int numOfNeighs = sData->adj_data->numOf_n[index];

		for (i=0; i<numOfNeighs; i++) {
			average_dist += bNeighs[sData->adj_data->n_index[index]+i].dist;
		}
	}
	average_dist  /= sData->adj_data->total_targets;

	/* determine number of required steps */
	steps = ceil((WAVE_TIME_FAC*timescale*surface->wave_timescale) / (average_dist/wave_speed/3));
	CLAMP(steps, 1, 15);
	timescale /= steps;

	/* apply simulation values for final timescale */
	dt = WAVE_TIME_FAC*timescale*surface->wave_timescale;
	min_dist = wave_speed*dt*1.5f;
	damp_factor = pow((1.0f-surface->wave_damping), 1.0f*timescale);

	for (ss=0; ss<steps; ss++) {

		/* copy previous frame data */
		memcpy(prevPoint, sData->type_data, sData->total_points*sizeof(PaintWavePoint));

		#pragma omp parallel for schedule(static)
		for (index = 0; index < sData->total_points; index++) {
			PaintWavePoint *wPoint = &((PaintWavePoint*)sData->type_data)[index];
			int numOfNeighs = sData->adj_data->numOf_n[index];
			float force = 0.0f, avg_dist = 0.0f, avg_height = 0.0f;
			int numOfN = 0, numOfRN = 0;
			int i;

			if (wPoint->state) continue;

			/* calculate force from surrounding points */
			for (i=0; i<numOfNeighs; i++) {
				int n_index = sData->adj_data->n_index[index]+i;
				float dist = bNeighs[n_index].dist;
				PaintWavePoint *tPoint = &prevPoint[sData->adj_data->n_target[n_index]];

				if (!dist || tPoint->state) continue;
				if (dist<min_dist) dist=min_dist;
				avg_dist += dist;
				numOfN++;

				/* count average height for edge points for open borders */
				if (!(sData->adj_data->flags[sData->adj_data->n_target[n_index]] & ADJ_ON_MESH_EDGE)) {
					avg_height += tPoint->height;
					numOfRN++;
				}

				force += (tPoint->height - wPoint->height) / (dist*dist);
			}
			avg_dist = (numOfN) ? avg_dist/numOfN : 0.0f;

			if (surface->flags & MOD_DPAINT_WAVE_OPEN_BORDERS &&
				sData->adj_data->flags[index] & ADJ_ON_MESH_EDGE) {

				/* if open borders, apply a fake height to keep waves going on */
				avg_height = (numOfRN) ? avg_height/numOfRN : 0.0f;

				wPoint->height = (dt*wave_speed*avg_height + wPoint->height*avg_dist) / (avg_dist + dt*wave_speed);
			}
			/* else, do wave eq */
			else {
				/* add force towards zero height based on average dist */
				if (avg_dist)
					force += (0.0f - wPoint->height) * surface->wave_spring / (avg_dist*avg_dist) / 2.0f;

				/* change point velocity */
				wPoint->velocity += force*dt * wave_speed*wave_speed;
				/* damping */
				wPoint->velocity *= damp_factor;

				/* and new height */
				wPoint->height += wPoint->velocity*dt;
			}
		}
	}

	/* reset d_obs and state */ 
	for (index = 0; index < sData->total_points; index++) {
		PaintWavePoint *wPoint = &((PaintWavePoint*)sData->type_data)[index];
		wPoint->state = 0;
	}

	MEM_freeN(prevPoint);
}

#define VALUE_DISSOLVE(VALUE, SPEED, SCALE, LOG) (VALUE) = (LOG) ? (VALUE) * 1.0f - 1.0f/((SPEED)/(SCALE)) : (VALUE) - 1.0f/(SPEED)*(SCALE)

/* Prepare for surface step by creating PaintBakePoint data */
static int dynamicPaint_prepareSurfaceStep(DynamicPaintSurface *surface, PaintBakeData *bData, Object *ob, DerivedMesh *dm, float timescale) {
	PaintSurfaceData *sData = surface->data;

	MVert *mvert = dm->getVertArray(dm);
	MFace *mface = dm->getFaceArray(dm);

	int index;
	FaceAdv *canvas_inv_normals = NULL;
	Vec3f *canvas_verts = NULL;

	int canvasNumOfVerts = dm->getNumVerts(dm);
	int canvasNumOfFaces = dm->getNumFaces(dm);

	/*
	*	Make a transformed copy of canvas derived mesh vertices to avoid recalculation.
	*/
	canvas_verts =  (struct Vec3f *) MEM_mallocN(canvasNumOfVerts*sizeof(struct Vec3f), "Dynamic Paint transformed canvas verts");
	if (!canvas_verts) return 0;

	#pragma omp parallel for schedule(static)
	for (index=0; index<canvasNumOfVerts; index++) {
		
		/* Multiply coordinates by canvas transformation matrix	*/
		VECCOPY(canvas_verts[index].v, mvert[index].co);
		mul_m4_v3(ob->obmat, canvas_verts[index].v);
	}

	/*
	*	Calculate temp per face normals using those transformed coordinates.
	*	(To not have to calculate same normal for millions of pixels)
	*/
	canvas_inv_normals =  (struct FaceAdv *) MEM_callocN(canvasNumOfFaces*sizeof(struct FaceAdv), "Dynamic Paint canvas normals");
	if (!canvas_inv_normals) {
		MEM_freeN(canvas_verts);
		return 0;
	}

	#pragma omp parallel for schedule(static)
	for (index=0; index<canvasNumOfFaces; index++) {
		if (mface[index].flag & ME_SMOOTH) continue;	/* Only calculate flat faces */
		
		/* Transformed normal */
		normal_tri_v3( canvas_inv_normals[index].no, canvas_verts[mface[index].v3].v,
			canvas_verts[mface[index].v2].v, canvas_verts[mface[index].v1].v);
		if (mface[index].v4) normal_tri_v3(canvas_inv_normals[index].no_q, canvas_verts[mface[index].v4].v,
			canvas_verts[mface[index].v3].v, canvas_verts[mface[index].v1].v);
	}

	/*
	*	Prepare each surface point for a new step
	*/
	#pragma omp parallel for schedule(static)
	for (index=0; index<sData->total_points; index++)
	{
		/* Do drying dissolve effects */
		if (surface->type == MOD_DPAINT_SURFACE_T_PAINT) {
			PaintPoint *pPoint = &((PaintPoint*)sData->type_data)[index];
			/* drying */
			if (pPoint->wetness > 0.0f) {
				/* for every drying step blend wet paint to the background */
				float invAlpha = 1.0f - pPoint->e_alpha;
				pPoint->color[0] = pPoint->color[0]*invAlpha + pPoint->e_color[0]*pPoint->e_alpha;
				pPoint->color[1] = pPoint->color[1]*invAlpha + pPoint->e_color[1]*pPoint->e_alpha;
				pPoint->color[2] = pPoint->color[2]*invAlpha + pPoint->e_color[2]*pPoint->e_alpha;
				pPoint->state = 1;

				/* only increase alpha if wet paint has higher	*/
				if (pPoint->e_alpha > pPoint->alpha) pPoint->alpha = pPoint->e_alpha;

				/* now dry it ;o	*/
				VALUE_DISSOLVE(pPoint->wetness, surface->dry_speed, timescale, (surface->flags & MOD_DPAINT_DRY_LOG));
			}
			/* 	If effect layer is completely dry, make sure it's marked empty */
			if (pPoint->wetness <= 0.0f) {
				pPoint->wetness = 0.0f;
				pPoint->e_alpha = 0.0f;
				pPoint->state = 0;
			}

			if (surface->flags & MOD_DPAINT_DISSOLVE) {

				VALUE_DISSOLVE(pPoint->alpha, surface->diss_speed, timescale, (surface->flags & MOD_DPAINT_DISSOLVE_LOG));
				if (pPoint->alpha < 0.0f) pPoint->alpha = 0.0f;

				VALUE_DISSOLVE(pPoint->e_alpha, surface->diss_speed, timescale, (surface->flags & MOD_DPAINT_DISSOLVE_LOG));
				if (pPoint->e_alpha < 0.0f) pPoint->e_alpha = 0.0f;
			}
		}
		/* dissolve for float types */
		else if (surface->flags & MOD_DPAINT_DISSOLVE &&
				(surface->type == MOD_DPAINT_SURFACE_T_DISPLACE ||
				 surface->type == MOD_DPAINT_SURFACE_T_WEIGHT)) {

			float *point = &((float*)sData->type_data)[index];
			/* log or linear */
			VALUE_DISSOLVE(*point, surface->diss_speed, timescale, (surface->flags & MOD_DPAINT_DISSOLVE_LOG));
			if (*point < 0.0f) *point = 0.0f;
		}

		/*
		*	Calculate current 3D-position of each surface pixel
		*/
		if (surface->format == MOD_DPAINT_SURFACE_F_IMAGESEQ) {
			float n1[3], n2[3], n3[3];
			ImgSeqFormatData *f_data = (ImgSeqFormatData*)sData->format_data;
			PaintUVPoint *tPoint = &((PaintUVPoint*)f_data->uv_p)[index];
			int ss;

			/* per sample coordinates */
			for (ss=0; ss<sData->samples; ss++) {
							interp_v3_v3v3v3(	&bData->realCoord[(sData->total_points*ss+index)*3],
				canvas_verts[tPoint->v1].v,
				canvas_verts[tPoint->v2].v,
				canvas_verts[tPoint->v3].v, f_data->barycentricWeights[index*sData->samples+ss].v);
			}

			/* Calculate current pixel surface normal	*/
			if(mface[tPoint->face_index].flag & ME_SMOOTH) {
				normal_short_to_float_v3(n1, mvert[tPoint->v1].no);
				normal_short_to_float_v3(n2, mvert[tPoint->v2].no);
				normal_short_to_float_v3(n3, mvert[tPoint->v3].no);

				interp_v3_v3v3v3(	bData->bPoint[index].invNorm,
					n1, n2, n3, f_data->barycentricWeights[index*sData->samples].v);
				mul_mat3_m4_v3(ob->obmat, bData->bPoint[index].invNorm);
				normalize_v3(bData->bPoint[index].invNorm);
				negate_v3(bData->bPoint[index].invNorm);
			}
			else {
				if (tPoint->quad) {VECCOPY(bData->bPoint[index].invNorm, canvas_inv_normals[tPoint->face_index].no_q);}
				else {VECCOPY(bData->bPoint[index].invNorm, canvas_inv_normals[tPoint->face_index].no);}
			}
		}
		else if (surface->format == MOD_DPAINT_SURFACE_F_VERTEX) {
			/* In case of verted data */

			/* location, currently vertex format can have only one sample */
			VECCOPY(&bData->realCoord[index*3], canvas_verts[index].v);

			/* normal */
			normal_short_to_float_v3(bData->bPoint[index].invNorm, mvert[index].no);
			mul_mat3_m4_v3(ob->obmat, bData->bPoint[index].invNorm);
			normalize_v3(bData->bPoint[index].invNorm);
			//mul_qt_v3(ob->quat, bPoint[index].invNorm);
			negate_v3(bData->bPoint[index].invNorm);
		}

		/* Prepare special data for surface types */
		if (surface->type == MOD_DPAINT_SURFACE_T_DISPLACE ||
			surface->type == MOD_DPAINT_SURFACE_T_WAVE) {
			float temp_nor[3];
			if (surface->format == MOD_DPAINT_SURFACE_F_VERTEX)
				{normal_short_to_float_v3(temp_nor, mvert[index].no);
				normalize_v3(temp_nor);}
			else {temp_nor[0]=0.0f;temp_nor[1]=0.0f;temp_nor[2]=1.0f;}

			mul_v3_v3 (temp_nor, ob->size);
			bData->bPoint[index].normal_scale = len_v3(temp_nor);
		}

		/*
		*	TODO: paint effect space preparation
		*/
	}

	MEM_freeN(canvas_verts);
	MEM_freeN(canvas_inv_normals);

	return 1;
}

static void scene_updateObject(Scene *scene, Object *ob, float frame) {
	int oflags;

	/* if object has parent, update it too */
	if (ob->parent) scene_updateObject(scene, ob->parent, frame);
	if (ob->track) scene_updateObject(scene, ob->track, frame);

	/* for curve */
	if(ob->type==OB_CURVE) {
		Curve *cu= ob->data;
		BKE_animsys_evaluate_animdata(&cu->id, cu->adt, frame, ADT_RECALC_ANIM);
	}
	
	/* backup object flags */
	oflags = ob->recalc;

	ob->recalc |= OB_RECALC_ALL;
	ob->recalc |= OB_RECALC_DATA;
	BKE_animsys_evaluate_animdata(&ob->id, ob->adt, frame, ADT_RECALC_ANIM);
	object_handle_update(scene, ob);

	/* restore flags */
	ob->recalc = oflags;
}

static void scene_setSubframe(Scene *scene, float subframe) {
	/* dynamic paint subframes must be done on previous frame */
	scene->r.cfra -= 1;
	scene->r.subframe = subframe;
}

/*
*	Do Dynamic Paint Step. Paints scene brush objects of current state/frame to canvas.
*/
static int dynamicPaint_doStep(Scene *scene, Object *ob, DynamicPaintSurface *surface, float timescale, float subframe)
{
	PaintSurfaceData *sData = surface->data;
	DynamicPaintCanvasSettings *canvas = surface->canvas;
	PaintBakeData bData;
	int ret = 1;

	if (!sData || sData->total_points < 1) return 0;

	/* Init surface current frame position data */
	bData.bPoint = (struct PaintBakePoint *) MEM_mallocN(sData->total_points*sizeof(struct PaintBakePoint), "Dynamic Paint step data");
	if (bData.bPoint == NULL) return printError(canvas, "Not enough free memory.");
	bData.realCoord = (float *) MEM_mallocN(sData->total_points*3*sData->samples*sizeof(float), "Dynamic Paint step coords");
	if (bData.realCoord == NULL) return printError(canvas, "Not enough free memory.");

	if (!dynamicPaint_prepareSurfaceStep(surface, &bData, ob, canvas->dm, timescale)) {
		ret = 0;
		printError(canvas, "Not enough free memory.");
	}
	/*
	*	Loop through surface's target paint objects and do painting
	*/
	else
	{
		Base *base = NULL;
		GroupObject *go = NULL;	

		Object *brushObj = NULL;
		ModifierData *md = NULL;

		/* backup current scene frame */
		int scene_frame = scene->r.cfra;
		float scene_subframe = scene->r.subframe;

		/* either from group or from all objects */
		if(surface->brush_group)
			go = surface->brush_group->gobject.first;
		else
			base = scene->base.first;

		while (base || go)
		{
			brushObj = NULL;

			/* select object */
			if(surface->brush_group) {						
				if(go->ob)	brushObj = go->ob;					
			}					
			else						
				brushObj = base->object;

			if(!brushObj)					
			{			
				/* skip item */
				if(surface->brush_group) go = go->next;
				else base= base->next;					
				continue;			
			}

			/* next item */
			if(surface->brush_group)
				go = go->next;
			else
				base= base->next;

			md = modifiers_findByType(brushObj, eModifierType_DynamicPaint);

			/* check if target has an active dp modifier	*/
			if(md && md->mode & (eModifierMode_Realtime | eModifierMode_Render))					
			{
				DynamicPaintModifierData *pmd2 = (DynamicPaintModifierData *)md;

				/* Make sure we're dealing with a painter	*/
				if((pmd2->type & MOD_DYNAMICPAINT_TYPE_BRUSH) && pmd2->brush)
				{
					DynamicPaintBrushSettings *brush = pmd2->brush;

					/* update object position on this subframe */
					if (subframe) {
						scene_setSubframe(scene, subframe);
						scene_updateObject(scene, brushObj, BKE_curframe(scene));
					}

					/* If using material color, update anim data to current (sub)frame	*/
					if (brush->flags & MOD_DPAINT_USE_MATERIAL)
						dynamicPaint_updateObjectMaterials(brushObj, brush->mat, scene);

					/*	Check if painter has a particle system selected
					*	-> if so, do particle painting */
					if (brush->collision == MOD_DPAINT_COL_PSYS)
					{
						if (brush && brush->psys && brush->psys->part && brush->psys->part->type==PART_EMITTER)
						if (psys_check_enabled(brushObj, brush->psys)) {

							/* Paint a particle system */
							BKE_animsys_evaluate_animdata(&brush->psys->part->id, brush->psys->part->adt, BKE_curframe(scene), ADT_RECALC_ANIM);
							dynamicPaint_paintParticles(surface, &bData, brush->psys, brush, ob, timescale);
						}
					}							
					else {
						/* Paint a mesh */
						dynamicPaint_paintMesh(surface, &bData, brush, ob, brushObj, timescale);
					}

					/* return object to it's original state */
					if (subframe) {
						scene->r.cfra = scene_frame;
						scene->r.subframe = scene_subframe;
						scene_updateObject(scene, brushObj, BKE_curframe(scene));
						dynamicPaint_updateObjectMaterials(brushObj, brush->mat, scene);
					}
				} /* end of collision check (Is valid paint modifier) */
			}
		}
	}

	MEM_freeN(bData.bPoint);

	/* neighbour data and surface opretions that use it */
	if (sData->adj_data)
	{
		BakeNeighPoint *bNeighs = MEM_mallocN(sData->adj_data->total_targets*sizeof(struct BakeNeighPoint), "PaintEffectBake");

		if (bNeighs) {
			/* calculate current frame distances and global dirs */
			dynamicPaint_prepareNeighbourData(surface, bNeighs, bData.realCoord);

			/* wave type surface simulation step */
			if (surface->type == MOD_DPAINT_SURFACE_T_WAVE) {
				dynamicPaint_doWaveStep(surface, bNeighs, timescale);
			}

			/* paint movement effects */
			if (surface->effect && surface->type == MOD_DPAINT_SURFACE_T_PAINT)
			{
				int steps = 1, s;
				PaintPoint *prevPoint;
				float *force = NULL;

				/* free antialias sample coords, as theyre not needed anymore */
				if (sData->samples>1) bData.realCoord = MEM_reallocN(bData.realCoord, sData->total_points*3*sizeof(float));
				/* Allocate memory for surface previous points to read unchanged values from	*/
				prevPoint = MEM_mallocN(sData->total_points*sizeof(struct PaintPoint), "PaintSurfaceDataCopy");

				/* in case of error, free already allocated blocks */
				if (!prevPoint || !bData.realCoord) {
					if (!bData.realCoord) MEM_freeN(bData.realCoord);
					if (!prevPoint) MEM_freeN(prevPoint);

					return printError(canvas, "Not enough free memory.");
				}

				/* Prepare effects and get number of required steps */
				steps = dynamicPaint_prepareEffectStep(surface, scene, ob, bNeighs, &force, bData.realCoord, timescale);

				/*
				*	Do Effects steps
				*/
				for (s = 0; s < steps; s++)
				{
					dynamicPaint_doEffectStep(surface, bNeighs, force, prevPoint, timescale/(float)steps);
				}

				/* Free temporary effect data	*/
				if (prevPoint) MEM_freeN(prevPoint);
				if (force) MEM_freeN(force);
			}

			MEM_freeN(bNeighs);
		}
	}

	MEM_freeN(bData.realCoord);

	return ret;
}

/*
*	Calculate a single frame for canvas point cache
*/
static int dynamicPaint_calculateFrame(DynamicPaintSurface *surface, Scene *scene, Object *cObject, int frame)
{
	float timescale = 1.0f;
	
	/* dont do substeps for first frame */
	if (surface->substeps && (frame != surface->start_frame)) {
		int st;
		timescale = 1.0f / (surface->substeps+1);

		for (st = 1; st <= surface->substeps; st++)
		{
			float subframe = ((float) st) / (surface->substeps+1);

			if (!dynamicPaint_doStep(scene, cObject, surface, timescale, subframe)) return 0;
		}
	}

	if (!dynamicPaint_doStep(scene, cObject, surface, timescale, 0.0f)) return 0;
	else  return 1;
}

/***************************** Image Sequence Baking ******************************/

/*
*	Do actual bake operation. Loops through to-be-baked frames.
*	Returns 0 on failture.
*/
static int dynamicPaint_bakeImageSequence(bContext *C, DynamicPaintSurface *surface, Object *cObject)
{
	DynamicPaintCanvasSettings *canvas = surface->canvas;
	Scene *scene= CTX_data_scene(C);
	wmWindow *win = CTX_wm_window(C);
	int frame = 1;
	int frames;

	frames = surface->end_frame - surface->start_frame + 1;
	if (frames <= 0) {return printError(canvas, "No frames to bake.");}

	/*
	*	Set frame to start point (also inits modifier data)
	*/
	frame = surface->start_frame;
	scene->r.cfra = (int)frame;
	ED_update_for_newframe(CTX_data_main(C), scene, win->screen, 1);

	/* Init surface	*/
	if (!dynamicPaint_createUVSurface(surface)) return 0;

	/*
	*	Loop through selected frames
	*/
	for (frame=surface->start_frame; frame<=surface->end_frame; frame++)
	{
		float timescale = 1.0f;
		int st;
		float progress = (frame - surface->start_frame) / (float)frames * 100;
		surface->current_frame = frame;

		/* If user requested stop (esc), quit baking	*/
		if (blender_test_break()) return 0;

		/* Update progress bar cursor */
		WM_timecursor(win, (int)progress);

		printf("DynamicPaint: Baking frame %i\n", frame);

		/*
		*	Do calculations for every substep
		*	Note: these have to be from previous frame
		*/
		if (frame != surface->start_frame) {
			/* change to next frame */
			timescale /= (surface->substeps+1);
			scene->r.cfra = (int)frame;
			scene->r.subframe = 0.0f;
			ED_update_for_newframe(CTX_data_main(C), scene, win->screen, 1);

			for (st = 1; st <= surface->substeps; st++)
			{
				float subframe = ((float) st) / (surface->substeps+1);

				if (!dynamicPaint_doStep(scene, cObject, surface, timescale, subframe)) return 0;
			}
		}

		if (!dynamicPaint_doStep(scene, cObject, surface, timescale, 0.0f)) return 0;

		/*
		*	Save output images
		*/
		{
			char filename[250];
			char pad[4];
			char dir_slash[2];
						/* OpenEXR or PNG	*/
			short format = (surface->image_fileformat & MOD_DPAINT_IMGFORMAT_OPENEXR) ? DPOUTPUT_OPENEXR : DPOUTPUT_PNG;

			/* Add frame number padding	*/
			if (frame<10) sprintf(pad,"000");
			else if (frame<100) sprintf(pad,"00");
			else if (frame<1000) sprintf(pad,"0");
			else pad[0] = '\0';

			/* make sure directory path is valid to append filename */
			if (surface->image_output_path[strlen(surface->image_output_path)-1] != 47 &&
				surface->image_output_path[strlen(surface->image_output_path)-1] != 92)
				strcpy(dir_slash,"/");
			else
				dir_slash[0] = '\0';


			/* color map	*/
			if (surface->type == MOD_DPAINT_SURFACE_T_PAINT) {
				if (surface->flags & MOD_DPAINT_OUT1) {
					sprintf(filename, "%s%s%s%s%i", surface->image_output_path, dir_slash, surface->output_name, pad, (int)frame);
					dynamicPaint_outputImage(surface, filename, format, DPOUTPUT_PAINT);
				}
				if (surface->flags & MOD_DPAINT_OUT2) {
					sprintf(filename, "%s%s%s%s%i", surface->image_output_path, dir_slash, surface->output_name2, pad, (int)frame);
					dynamicPaint_outputImage(surface, filename, format, DPOUTPUT_WET);
				}
			}

			/* displacement map	*/
			else if (surface->type == MOD_DPAINT_SURFACE_T_DISPLACE) {
				sprintf(filename, "%s%s%s%s%i", surface->image_output_path, dir_slash, surface->output_name, pad, (int)frame);
				dynamicPaint_outputImage(surface, filename, format, DPOUTPUT_DISPLACE);
			}

			/* waves	*/
			else if (surface->type == MOD_DPAINT_SURFACE_T_WAVE) {
				sprintf(filename, "%s%s%s%s%i", surface->image_output_path, dir_slash, surface->output_name, pad, (int)frame);
				dynamicPaint_outputImage(surface, filename, format, DPOUTPUT_WAVES);
			}
		}
	}
	return 1;
}


/*
*	An operator call to start baking dynamic paint image sequences for active object
*/
static int dynamicPaint_initBake(bContext *C, wmOperator *op)
{
	DynamicPaintModifierData *pmd = NULL;
	Object *cObject = CTX_data_active_object(C);
	int status = 0;
	double timer = PIL_check_seconds_timer();
	DynamicPaintSurface *surface;

	/*
	*	Get modifier data
	*/
	pmd = (DynamicPaintModifierData *)modifiers_findByType(cObject, eModifierType_DynamicPaint);
	if (!pmd) {
		BKE_report(op->reports, RPT_ERROR, "Bake Failed: No Dynamic Paint modifier found.");
		return 0;
	}

	/* Make sure we're dealing with a canvas */
	if (!pmd->canvas) {
		BKE_report(op->reports, RPT_ERROR, "Bake Failed: Invalid Canvas.");
		return 0;
	}
	surface = get_activeSurface(pmd->canvas);

	/* Set state to baking and init surface */
	pmd->canvas->error[0] = '\0';
	pmd->canvas->flags |= MOD_DPAINT_BAKING;
	G.afbreek= 0;	/* reset blender_test_break*/

	/*  Bake Dynamic Paint	*/
	status = dynamicPaint_bakeImageSequence(C, surface, cObject);
	/* Clear bake */
	pmd->canvas->flags &= ~MOD_DPAINT_BAKING;
	WM_cursor_restore(CTX_wm_window(C));
	dynamicPaint_freeSurfaceData(surface);

	/* Bake was successful:
	*  Report for ended bake and how long it took */
	if (status) {

		/* Format time string	*/
		char timestr[30];
		double time = PIL_check_seconds_timer() - timer;
		int tmp_val;
		timestr[0] = '\0';

		/* days (just in case someone actually has a very slow pc)	*/
		tmp_val = (int)floor(time / 86400.0f);
		if (tmp_val > 0) sprintf(timestr, "%i Day(s) - ", tmp_val);
		/* hours	*/
		time -= 86400.0f * tmp_val;
		tmp_val = (int)floor(time / 3600.0f);
		if (tmp_val > 0) sprintf(timestr, "%s%i h ", timestr, tmp_val);
		/* minutes	*/
		time -= 3600.0f * tmp_val;
		tmp_val = (int)floor(time / 60.0f);
		if (tmp_val > 0) sprintf(timestr, "%s%i min ", timestr, tmp_val);
		/* seconds	*/
		time -= 60.0f * tmp_val;
		tmp_val = (int)ceil(time);
		sprintf(timestr, "%s%i s", timestr, tmp_val);

		/* Show bake info */
		sprintf(pmd->canvas->ui_info, "Bake Complete! (Time: %s)", timestr);
		printf("%s\n", pmd->canvas->ui_info);
	}
	else {
		if (strlen(pmd->canvas->error)) { /* If an error occured */
			sprintf(pmd->canvas->ui_info, "Bake Failed: %s", pmd->canvas->error);
			BKE_report(op->reports, RPT_ERROR, pmd->canvas->ui_info);
		}
		else {	/* User cancelled the bake */
			sprintf(pmd->canvas->ui_info, "Baking Cancelled!");
			BKE_report(op->reports, RPT_WARNING, pmd->canvas->ui_info);
		}

		/* Print failed bake to console */
		printf("Baking Cancelled!\n");
	}

	return status;
}


/***************************** Operators ******************************/

static int dynamicpaint_bake_exec(bContext *C, wmOperator *op)
{

	/* Bake dynamic paint */
	if(!dynamicPaint_initBake(C, op)) {
		return OPERATOR_CANCELLED;}

	return OPERATOR_FINISHED;
}

void DPAINT_OT_bake(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Dynamic Paint Bake";
	ot->description= "Bake dynamic paint";
	ot->idname= "DPAINT_OT_bake";
	
	/* api callbacks */
	ot->exec= dynamicpaint_bake_exec;
	ot->poll= ED_operator_object_active_editable;
}

static int surface_slot_add_exec(bContext *C, wmOperator *op)
{
	DynamicPaintModifierData *pmd = NULL;
	Object *cObject = CTX_data_active_object(C);
	DynamicPaintSurface *surface;

	/* Make sure we're dealing with a canvas */
	pmd = (DynamicPaintModifierData *)modifiers_findByType(cObject, eModifierType_DynamicPaint);
	if (!pmd) return OPERATOR_CANCELLED;
	if (!pmd->canvas) return OPERATOR_CANCELLED;

	surface = dynamicPaint_createNewSurface(pmd->canvas);

	if (!surface) return OPERATOR_CANCELLED;

	/* set preview for this surface only and set active */
	pmd->canvas->active_sur = 0;
	for(surface=surface->prev; surface; surface=surface->prev) {
				surface->flags &= ~MOD_DPAINT_PREVIEW;
				pmd->canvas->active_sur++;
	}

	return OPERATOR_FINISHED;
}

/* add surface slot */
void DPAINT_OT_surface_slot_add(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Add Surface Slot";
	ot->idname= "DPAINT_OT_surface_slot_add";
	ot->description="Add a new Dynamic Paint surface slot";
	
	/* api callbacks */
	ot->exec= surface_slot_add_exec;
	ot->poll= ED_operator_object_active_editable;

	/* flags */
	ot->flag= OPTYPE_REGISTER|OPTYPE_UNDO;
}

static int surface_slot_remove_exec(bContext *C, wmOperator *op)
{
	DynamicPaintModifierData *pmd = NULL;
	Object *cObject = CTX_data_active_object(C);
	DynamicPaintSurface *surface;
	int id=0;

	/* Make sure we're dealing with a canvas */
	pmd = (DynamicPaintModifierData *)modifiers_findByType(cObject, eModifierType_DynamicPaint);
	if (!pmd) return OPERATOR_CANCELLED;
	if (!pmd->canvas) return OPERATOR_CANCELLED;

	surface = pmd->canvas->surfaces.first;

	/* find active surface and remove it */
	for(; surface; surface=surface->next) {
		if(id == pmd->canvas->active_sur) {
				pmd->canvas->active_sur -= 1;
				dynamicPaint_freeSurface(surface);
				break;
			}
		id++;
	}

	dynamicPaint_resetPreview(pmd->canvas);
	DAG_id_tag_update(&cObject->id, OB_RECALC_DATA);
	WM_event_add_notifier(C, NC_OBJECT|ND_MODIFIER, cObject);

	return OPERATOR_FINISHED;
}

/* remove surface slot */
void DPAINT_OT_surface_slot_remove(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Remove Surface Slot";
	ot->idname= "DPAINT_OT_surface_slot_remove";
	ot->description="Remove the selected surface slot";
	
	/* api callbacks */
	ot->exec= surface_slot_remove_exec;
	ot->poll= ED_operator_object_active_editable;

	/* flags */
	ot->flag= OPTYPE_REGISTER|OPTYPE_UNDO;
}

