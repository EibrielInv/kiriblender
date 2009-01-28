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
#ifndef ED_IPO_INTERN_H
#define ED_IPO_INTERN_H

struct bContext;
struct wmWindowManager;
struct bAnimContext;
struct SpaceIpo;
struct ARegion;

/* internal exports only */

/* ***************************************** */
/* ipo_draw.c */
void graph_draw_channel_names(struct bAnimContext *ac, struct SpaceIpo *sipo, struct ARegion *ar);
void graph_draw_curves(struct bAnimContext *ac, struct SpaceIpo *sipo, struct ARegion *ar);

/* ***************************************** */
/* ipo_header.c */
void graph_header_buttons(const bContext *C, struct ARegion *ar);

/* ***************************************** */
/* ipo_select.c */

void GRAPHEDIT_OT_keyframes_deselectall(struct wmOperatorType *ot);
void GRAPHEDIT_OT_keyframes_borderselect(struct wmOperatorType *ot);
void GRAPHEDIT_OT_keyframes_columnselect(struct wmOperatorType *ot);
void GRAPHEDIT_OT_keyframes_clickselect(struct wmOperatorType *ot);

/* defines for left-right select tool */
enum {
	GRAPHKEYS_LRSEL_TEST	= -1,
	GRAPHKEYS_LRSEL_NONE,
	GRAPHKEYS_LRSEL_LEFT,
	GRAPHKEYS_LRSEL_RIGHT,
} eGraphKeys_LeftRightSelect_Mode;

/* defines for column-select mode */
enum {
	GRAPHKEYS_COLUMNSEL_KEYS	= 0,
	GRAPHKEYS_COLUMNSEL_CFRA,
	GRAPHKEYS_COLUMNSEL_MARKERS_COLUMN,
	GRAPHKEYS_COLUMNSEL_MARKERS_BETWEEN,
} eGraphKeys_ColumnSelect_Mode;

/* ***************************************** */
/* ipo_edit.c */

void GRAPHEDIT_OT_set_previewrange(struct wmOperatorType *ot);
void GRAPHEDIT_OT_view_all(struct wmOperatorType *ot);

void GRAPHEDIT_OT_keyframes_copy(struct wmOperatorType *ot);
void GRAPHEDIT_OT_keyframes_paste(struct wmOperatorType *ot);

void GRAPHEDIT_OT_keyframes_delete(struct wmOperatorType *ot);
void GRAPHEDIT_OT_keyframes_clean(struct wmOperatorType *ot);
void GRAPHEDIT_OT_keyframes_sample(struct wmOperatorType *ot);

void GRAPHEDIT_OT_keyframes_handletype(struct wmOperatorType *ot);
void GRAPHEDIT_OT_keyframes_interpolation_type(struct wmOperatorType *ot);
void GRAPHEDIT_OT_keyframes_expotype(struct wmOperatorType *ot);

void GRAPHEDIT_OT_keyframes_cfrasnap(struct wmOperatorType *ot);
void GRAPHEDIT_OT_keyframes_snap(struct wmOperatorType *ot);
void GRAPHEDIT_OT_keyframes_mirror(struct wmOperatorType *ot);

/* defines for snap keyframes 
 * NOTE: keep in sync with eEditKeyframes_Snap (in ED_keyframes_edit.h)
 */
enum {
	GRAPHKEYS_SNAP_CFRA = 1,
	GRAPHKEYS_SNAP_NEAREST_FRAME,
	GRAPHKEYS_SNAP_NEAREST_SECOND,
	GRAPHKEYS_SNAP_NEAREST_MARKER,	
	GRAPHKEYS_SNAP_HORIZONTAL,
} eGraphKeys_Snap_Mode;

/* defines for mirror keyframes 
 * NOTE: keep in sync with eEditKeyframes_Mirror (in ED_keyframes_edit.h)
 */
enum {
	GRAPHKEYS_MIRROR_CFRA = 1,
	GRAPHKEYS_MIRROR_YAXIS,
	GRAPHKEYS_MIRROR_XAXIS,
	GRAPHKEYS_MIRROR_MARKER,
} eGraphKeys_Mirror_Mode;

/* ***************************************** */
/* ipo_ops.c */
void graphedit_keymap(struct wmWindowManager *wm);
void graphedit_operatortypes(void);


#endif /* ED_IPO_INTERN_H */

