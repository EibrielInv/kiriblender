#include "MEM_guardedalloc.h"
#include "BKE_customdata.h" 
#include "DNA_listBase.h"
#include "DNA_customdata_types.h"
#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_object_types.h"
#include "DNA_scene_types.h"
#include <string.h>
#include "BKE_utildefines.h"
#include "BKE_mesh.h"
#include "BKE_global.h"
#include "BKE_DerivedMesh.h"
#include "BKE_cdderivedmesh.h"

#include "BLI_editVert.h"
#include "mesh_intern.h"
#include "ED_mesh.h"

#include "BLI_arithb.h"
#include "BLI_blenlib.h"
#include "BLI_edgehash.h"

#include "bmesh.h"

/*
 * UTILS.C
 *
 * utility bmesh operators, e.g. transform, 
 * translate, rotate, scale, etc.
 *
*/

void bmesh_makevert_exec(BMesh *bm, BMOperator *op) {
	float vec[3];

	BMO_Get_Vec(op, "co", vec);

	BMO_SetFlag(bm, BM_Make_Vert(bm, vec, NULL), 1);	
	BMO_Flag_To_Slot(bm, op, "newvertout", 1, BM_VERT);
}

void bmesh_transform_exec(BMesh *bm, BMOperator *op) {
	BMOIter iter;
	BMVert *v;
	float mat[4][4];

	BMO_Get_Mat4(op, "mat", mat);

	BMO_ITER(v, &iter, bm, op, "verts", BM_VERT) {
		Mat4MulVecfl(mat, v->co);
	}
}

/*this operator calls the transform operator, which
  is a little complex, but makes it easier to make
  sure the transform op is working, since initially
  only this one will be used.*/
void bmesh_translate_exec(BMesh *bm, BMOperator *op) {
	BMOIter iter;
	BMVert *v;
	float mat[4][4], vec[3];
	
	BMO_Get_Vec(op, "vec", vec);

	Mat4One(mat);
	VECCOPY(mat[3], vec);

	BMO_CallOpf(bm, "transform mat=%m4 verts=%s", mat, op, "verts");
}

void bmesh_rotate_exec(BMesh *bm, BMOperator *op) {
	BMOIter iter;
	BMVert *v;
	float mat[4][4], vec[3];
	
	BMO_Get_Vec(op, "cent", vec);
	
	/*there has to be a proper matrix way to do this, but
	  this is how editmesh did it and I'm too tired to think
	  through the math right now.*/
	VecMulf(vec, -1);
	BMO_CallOpf(bm, "translate verts=%s vec=%v", op, "verts", vec);

	BMO_CallOpf(bm, "transform mat=%s verts=%s", op, "mat", op, "verts");

	VecMulf(vec, -1);
	BMO_CallOpf(bm, "translate verts=%s vec=%v", op, "verts", vec);
}

void bmesh_reversefaces_exec(BMesh *bm, BMOperator *op) {
	BMOIter iter;
	BMFace *f;

	BMO_ITER(f, &iter, bm, op, "faces", BM_FACE) {
		BM_flip_normal(bm, f);
	}
}