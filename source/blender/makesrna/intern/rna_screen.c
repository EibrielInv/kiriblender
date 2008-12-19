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
 * Contributor(s): Blender Foundation (2008), Nathan Letwory
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include <stdlib.h>

#include "RNA_define.h"
#include "RNA_types.h"

#include "rna_internal.h"

#include "DNA_screen_types.h"
#include "DNA_scene_types.h"

#ifdef RNA_RUNTIME


static void *rna_Screen_scene_get(PointerRNA *ptr)
{
	bScreen *sc= (bScreen*)ptr->data;
	return sc->scene;
}

static void rna_Screen_verts_begin(CollectionPropertyIterator *iter, PointerRNA *ptr)
{
	bScreen *scr= (bScreen*)ptr->data;
	rna_iterator_listbase_begin(iter, &scr->vertbase, NULL);
}

static void rna_Screen_edges_begin(CollectionPropertyIterator *iter, PointerRNA *ptr)
{
	bScreen *scr= (bScreen*)ptr->data;
	rna_iterator_listbase_begin(iter, &scr->edgebase, NULL);
}

static void rna_Screen_areas_begin(CollectionPropertyIterator *iter, PointerRNA *ptr)
{
	bScreen *scr= (bScreen*)ptr->data;
	rna_iterator_listbase_begin(iter, &scr->areabase, NULL);
}

#else

static void RNA_def_vectypes(BlenderRNA *brna)
{
	StructRNA *srna;
	
	srna= RNA_def_struct(brna, "vec2s", NULL);
	RNA_def_struct_ui_text(srna, "vec2s", "DOC_BROKEN");
	RNA_def_struct_sdna(srna, "vec2s");	
}

static void RNA_def_scrvert(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;
	
	srna= RNA_def_struct(brna, "ScrVert", NULL);
	RNA_def_struct_ui_text(srna, "Screen Vertex", "DOC_BROKEN");
	RNA_def_struct_sdna(srna, "ScrVert");
	
	prop= RNA_def_property(srna, "Location", PROP_INT, PROP_VECTOR);
	RNA_def_property_flag(prop, PROP_NOT_EDITABLE);
	RNA_def_property_int_sdna(prop, NULL, "vec.x");
	RNA_def_property_array(prop, 2);
	RNA_def_property_ui_text(prop, "Location", "Screen Vert Location");
}

static void RNA_def_scredge(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;
	
	srna= RNA_def_struct(brna, "ScrEdge", NULL);
	RNA_def_struct_ui_text(srna, "Screen Edge", "DOC_BROKEN");
	RNA_def_struct_sdna(srna, "ScrEdge");
	
	prop= RNA_def_property(srna, "start", PROP_INT, PROP_VECTOR);
	RNA_def_property_flag(prop, PROP_NOT_EDITABLE);
	RNA_def_property_int_sdna(prop, NULL, "v1->vec.x");
	RNA_def_property_array(prop, 2);
	RNA_def_property_ui_text(prop, "Edge Start", "Screen Edge Start");
	
	prop= RNA_def_property(srna, "end", PROP_INT, PROP_VECTOR);
	RNA_def_property_flag(prop, PROP_NOT_EDITABLE);
	RNA_def_property_int_sdna(prop, NULL, "v2->vec.x");
	RNA_def_property_array(prop, 2);
	RNA_def_property_ui_text(prop, "Edge End", "Screen Edge End");
}

static void RNA_def_scrarea(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;
	
	srna= RNA_def_struct(brna, "ScrArea", NULL);
	RNA_def_struct_ui_text(srna, "Area", "DOC_BROKEN");
	RNA_def_struct_sdna(srna, "ScrArea");
	
	prop= RNA_def_property(srna, "v1", PROP_INT, PROP_VECTOR);
	RNA_def_property_flag(prop, PROP_NOT_EDITABLE);
	RNA_def_property_int_sdna(prop, NULL, "v1->vec.x");
	RNA_def_property_array(prop, 2);
	RNA_def_property_ui_text(prop, "Area v1", "Screen Area v1");
	
	prop= RNA_def_property(srna, "v2", PROP_INT, PROP_VECTOR);
	RNA_def_property_flag(prop, PROP_NOT_EDITABLE);
	RNA_def_property_int_sdna(prop, NULL, "v2->vec.x");
	RNA_def_property_array(prop, 2);
	RNA_def_property_ui_text(prop, "Area v2", "Screen Area v2");
	
	prop= RNA_def_property(srna, "v3", PROP_INT, PROP_VECTOR);
	RNA_def_property_flag(prop, PROP_NOT_EDITABLE);
	RNA_def_property_int_sdna(prop, NULL, "v3->vec.x");
	RNA_def_property_array(prop, 2);
	RNA_def_property_ui_text(prop, "Area v3", "Screen Area v3");
	
	prop= RNA_def_property(srna, "v4", PROP_INT, PROP_VECTOR);
	RNA_def_property_flag(prop, PROP_NOT_EDITABLE);
	RNA_def_property_int_sdna(prop, NULL, "v4->vec.x");
	RNA_def_property_array(prop, 2);
	RNA_def_property_ui_text(prop, "Area v4", "Screen Area v4");
}

static void RNA_def_panel(BlenderRNA *brna)
{
	StructRNA *srna;
	
	srna= RNA_def_struct(brna, "Panel", NULL);
	RNA_def_struct_ui_text(srna, "Panel", "DOC_BROKEN");
	RNA_def_struct_sdna(srna, "Panel");
}

static void RNA_def_region(BlenderRNA *brna)
{
	StructRNA *srna;
	
	srna= RNA_def_struct(brna, "Region", NULL);
	RNA_def_struct_ui_text(srna, "Area Region", "DOC_BROKEN");
	RNA_def_struct_sdna(srna, "ARegion");
}

static void RNA_def_bscreen(BlenderRNA *brna)
{
	StructRNA *srna;
	PropertyRNA *prop;
	
	srna= RNA_def_struct(brna, "Screen", "ID");
	RNA_def_struct_ui_text(srna, "Screen", "DOC_BROKEN");
	RNA_def_struct_sdna(srna, "bScreen");
	
	prop= RNA_def_property(srna, "scene", PROP_POINTER, PROP_NONE);
	RNA_def_property_flag(prop, PROP_NOT_EDITABLE);
	RNA_def_property_struct_type(prop, "Scene");
	RNA_def_property_pointer_funcs(prop, "rna_Screen_scene_get", NULL, NULL);
	RNA_def_property_ui_text(prop, "Scene", "Active scene.");
	
	prop= RNA_def_property(srna, "vertbase", PROP_COLLECTION, PROP_NONE);
	RNA_def_property_flag(prop, PROP_NOT_EDITABLE);
	RNA_def_property_struct_type(prop, "ScrVert");
	RNA_def_property_collection_funcs(prop, "rna_Screen_verts_begin", "rna_iterator_listbase_next", "rna_iterator_listbase_end", "rna_iterator_listbase_get", 0, 0, 0, 0);
	RNA_def_property_ui_text(prop, "Verts", "All Screen Verts");
	
	prop= RNA_def_property(srna, "edgebase", PROP_COLLECTION, PROP_NONE);
	RNA_def_property_flag(prop, PROP_NOT_EDITABLE);
	RNA_def_property_struct_type(prop, "ScrEdge");
	RNA_def_property_collection_funcs(prop, "rna_Screen_edges_begin", "rna_iterator_listbase_next", "rna_iterator_listbase_end", "rna_iterator_listbase_get", 0, 0, 0, 0);
	RNA_def_property_ui_text(prop, "Edges", "All Screen Edges");
	
	prop= RNA_def_property(srna, "areabase", PROP_COLLECTION, PROP_NONE);
	RNA_def_property_flag(prop, PROP_NOT_EDITABLE);
	RNA_def_property_struct_type(prop, "ScrArea");
	RNA_def_property_collection_funcs(prop, "rna_Screen_areas_begin", "rna_iterator_listbase_next", "rna_iterator_listbase_end", "rna_iterator_listbase_get", 0, 0, 0, 0);
	RNA_def_property_ui_text(prop, "Areas", "All Screen Areas");
}

void RNA_def_screen(BlenderRNA *brna)
{
	RNA_def_bscreen(brna);
	RNA_def_vectypes(brna);
	RNA_def_scrvert(brna);
	RNA_def_scredge(brna);
	RNA_def_scrarea(brna);
	RNA_def_panel(brna);
	RNA_def_region(brna);
}

#endif


