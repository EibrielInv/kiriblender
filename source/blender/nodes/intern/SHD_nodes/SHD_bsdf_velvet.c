/**
 * $Id: SHD_output.c 32517 2010-10-16 14:32:17Z campbellbarton $
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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2005 Blender Foundation.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include "../SHD_util.h"

/* **************** OUTPUT ******************** */

static bNodeSocketType sh_node_bsdf_velvet_in[]= {
	{	SOCK_RGBA, 1, "Color",		0.8f, 0.8f, 0.8f, 1.0f, 0.0f, 1.0f},
	{	SOCK_VALUE, 1, "Sigma",		1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
	{	SOCK_VALUE, 1, "Fresnel",	1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
	{	-1, 0, ""	}
};

static bNodeSocketType sh_node_bsdf_velvet_out[]= {
	{	SOCK_CLOSURE, 0, "BSDF",		0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
	{	-1, 0, ""	}
};

static void node_shader_exec_bsdf_velvet(void *UNUSED(data), bNode *UNUSED(node), bNodeStack **UNUSED(in), bNodeStack **UNUSED(out))
{
}

static int node_shader_gpu_bsdf_velvet(GPUMaterial *mat, bNode *UNUSED(node), GPUNodeStack *in, GPUNodeStack *out)
{
	return GPU_stack_link(mat, "node_bsdf_velvet", in, out, GPU_builtin(GPU_VIEW_NORMAL));
}

/* node type definition */
void register_node_type_sh_bsdf_velvet(ListBase *lb)
{
	static bNodeType ntype;

	node_type_base(&ntype, SH_NODE_BSDF_VELVET, "Velvet BSDF", NODE_CLASS_CLOSURE, 0,
		sh_node_bsdf_velvet_in, sh_node_bsdf_velvet_out);
	node_type_size(&ntype, 150, 60, 200);
	node_type_init(&ntype, NULL);
	node_type_storage(&ntype, "", NULL, NULL);
	node_type_exec(&ntype, node_shader_exec_bsdf_velvet);
	node_type_gpu(&ntype, node_shader_gpu_bsdf_velvet);

	nodeRegisterType(lb, &ntype);
};

