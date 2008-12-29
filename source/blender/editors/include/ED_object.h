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
#ifndef ED_OBJECT_H
#define ED_OBJECT_H

struct wmWindowManager;
struct Object;
struct bContext;
struct Base;

void ED_operatortypes_object(void);
void ED_keymap_object(struct wmWindowManager *wm);

	/* send your own notifier for select! */
void ED_base_object_select(struct Base *base, short mode);
	/* includes notifier */
void ED_base_object_activate(struct bContext *C, struct Base *base);

/* cleanup */
int object_data_is_libdata(struct Object *ob);
int object_is_libdata(struct Object *ob);

#endif /* ED_OBJECT_H */

