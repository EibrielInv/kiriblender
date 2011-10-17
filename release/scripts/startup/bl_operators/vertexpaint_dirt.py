# ***** BEGIN GPL LICENSE BLOCK *****
#
# Script copyright (C) Campbell J Barton
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# ***** END GPL LICENSE BLOCK *****
# --------------------------------------------------------------------------

# <pep8 compliant>

# History
#
# Originally written by Campbell Barton aka ideasman42
#
# 2009-11-01: * 2.5 port by Keith "Wahooney" Boshoff
#              * Replaced old method with my own, speed is similar (about 0.001 sec on Suzanne)
#               but results are far more accurate
#


def applyVertexDirt(me, blur_iterations, blur_strength, clamp_dirt, clamp_clean, dirt_only):
    from mathutils import Vector
    from math import acos

    #BPyMesh.meshCalcNormals(me)

    vert_tone = [0.0] * len(me.vertices)

    min_tone = 180.0
    max_tone = 0.0

    # create lookup table for each vertex's connected vertices (via edges)
    con = []

    con = [[] for i in range(len(me.vertices))]

    # add connected verts
    for e in me.edges:
        con[e.vertices[0]].append(e.vertices[1])
        con[e.vertices[1]].append(e.vertices[0])

    for i, v in enumerate(me.vertices):
        vec = Vector()
        no = v.normal
        co = v.co

        # get the direction of the vectors between the vertex and it's connected vertices
        for c in con[i]:
            vec += (me.vertices[c].co - co).normalized()

        # normalize the vector by dividing by the number of connected verts
        tot_con = len(con[i])

        if tot_con == 0:
            continue

        vec /= tot_con

        # angle is the acos() of the dot product between vert and connected verts normals
        ang = acos(no.dot(vec))

        # enforce min/max
        ang = max(clamp_dirt, ang)

        if not dirt_only:
            ang = min(clamp_clean, ang)

        vert_tone[i] = ang

    # blur tones
    for i in range(blur_iterations):
        # backup the original tones
        orig_vert_tone = list(vert_tone)

        # use connected verts look up for blurring
        for j, c in enumerate(con):
            for v in c:
                vert_tone[j] += blur_strength * orig_vert_tone[v]

            vert_tone[j] /= len(c) * blur_strength + 1

    min_tone = min(vert_tone)
    max_tone = max(vert_tone)

    # debug information
    # print(min_tone * 2 * math.pi)
    # print(max_tone * 2 * math.pi)
    # print(clamp_clean)
    # print(clamp_dirt)

    tone_range = max_tone - min_tone

    if not tone_range:
        return

    active_col_layer = None

    if len(me.vertex_colors):
        for lay in me.vertex_colors:
            if lay.active:
                active_col_layer = lay.data
    else:
        bpy.ops.mesh.vertex_color_add()
        me.vertex_colors[0].active = True
        active_col_layer = me.vertex_colors[0].data

    if not active_col_layer:
        return('CANCELLED', )

    for i, f in enumerate(me.faces):
        if not me.use_paint_mask or f.select:

            f_col = active_col_layer[i]

            f_col = [f_col.color1, f_col.color2, f_col.color3, f_col.color4]

            for j, v in enumerate(f.vertices):
                col = f_col[j]
                tone = vert_tone[me.vertices[v].index]
                tone = (tone - min_tone) / tone_range

                if dirt_only:
                    tone = min(tone, 0.5)
                    tone *= 2

                col[0] = tone * col[0]
                col[1] = tone * col[1]
                col[2] = tone * col[2]


import bpy
from bpy.types import Operator
from bpy.props import FloatProperty, IntProperty, BoolProperty


class VertexPaintDirt(Operator):
    bl_idname = "paint.vertex_color_dirt"
    bl_label = "Dirty Vertex Colors"
    bl_options = {'REGISTER', 'UNDO'}

    blur_strength = FloatProperty(
            name="Blur Strength",
            description="Blur strength per iteration",
            min=0.01, max=1.0,
            default=1.0,
            )
    blur_iterations = IntProperty(
            name="Blur Iterations",
            description="Number times to blur the colors. (higher blurs more)",
            min=0, max=40,
            default=1,
            )
    clean_angle = FloatProperty(
            name="Highlight Angle",
            description="Less then 90 limits the angle used in the tonal range",
            min=0.0, max=180.0,
            default=180.0,
            )
    dirt_angle = FloatProperty(
            name="Dirt Angle",
            description="Less then 90 limits the angle used in the tonal range",
            min=0.0, max=180.0,
            default=0.0,
            )
    dirt_only = BoolProperty(
            name="Dirt Only",
            description="Dont calculate cleans for convex areas",
            default=False,
            )

    def execute(self, context):
        import time
        from math import radians
        obj = context.object

        if not obj or obj.type != 'MESH':
            self.report({'ERROR'}, "Error, no active mesh object, aborting")
            return {'CANCELLED'}

        mesh = obj.data

        t = time.time()

        applyVertexDirt(mesh, self.blur_iterations, self.blur_strength, radians(self.dirt_angle), radians(self.clean_angle), self.dirt_only)

        print('Dirt calculated in %.6f' % (time.time() - t))

        return {'FINISHED'}
