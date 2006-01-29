#!BPY

"""
Name: 'DEC Object File Format (.off)...'
Blender: 232
Group: 'Export'
Tooltip: 'Export selected mesh to DEC Object File Format (*.off)'
"""

__author__ = "Anthony D'Agostino (Scorpius)"
__url__ = ("blender", "elysiun",
"Author's homepage, http://www.redrival.com/scorpius")
__version__ = "Part of IOSuite 0.5"

__bpydoc__ = """\
This script exports meshes to DEC Object File Format.

The DEC (Digital Equipment Corporation) OFF format is very old and
almost identical to Wavefront's OBJ. I wrote this so I could get my huge
meshes into Moonlight Atelier. (DXF can also be used but the file size
is five times larger than OFF!) Blender/Moonlight users might find this
script to be very useful.

Usage:<br>
	Select meshes to be exported and run this script from "File->Export" menu.

Notes:<br>
	Only exports a single selected mesh.
"""

# $Id:
#
# +---------------------------------------------------------+
# | Copyright (c) 2002 Anthony D'Agostino                   |
# | http://www.redrival.com/scorpius                        |
# | scorpius@netzero.com                                    |
# | February 3, 2001                                        |
# | Read and write Object File Format (*.off)               |
# +---------------------------------------------------------+

# ***** BEGIN GPL LICENSE BLOCK *****
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
# Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# ***** END GPL LICENCE BLOCK *****

import Blender, meshtools
#import time

# ==============================
# ====== Write OFF Format ======
# ==============================
def write(filename):
	#start = time.clock()
	file = open(filename, "wb")

	objects = Blender.Object.GetSelected()
	objname = objects[0].name
	meshname = objects[0].data.name
	mesh = Blender.NMesh.GetRaw(meshname)
	#mesh = Blender.NMesh.GetRawFromObject(meshname)	# for SubSurf
	obj = Blender.Object.Get(objname)

	# === OFF Header ===
	file.write("OFF\n")
	file.write("%d %d %d\n" % (len(mesh.verts), len(mesh.faces), 0))

	# === Vertex List ===
	for i in range(len(mesh.verts)):
		if not i%100 and meshtools.show_progress:
			Blender.Window.DrawProgressBar(float(i)/len(mesh.verts), "Writing Verts")
		x, y, z = mesh.verts[i].co
		file.write("%f %f %f\n" % (x, y, z))

	# === Face List ===
	for i in range(len(mesh.faces)):
		if not i%100 and meshtools.show_progress:
			Blender.Window.DrawProgressBar(float(i)/len(mesh.faces), "Writing Faces")
		file.write(`len(mesh.faces[i].v)`+' ')
		mesh.faces[i].v.reverse()
		for j in range(len(mesh.faces[i].v)):
			file.write(`mesh.faces[i].v[j].index`+' ')
		file.write("\n")


	Blender.Window.DrawProgressBar(1.0, '')  # clear progressbar
	file.close()
	#end = time.clock()
	#seconds = " in %.2f %s" % (end-start, "seconds")
	message = "Successfully exported " + Blender.sys.basename(filename)# + seconds
	meshtools.print_boxed(message)

def fs_callback(filename):
	if filename.find('.off', -4) <= 0: filename += '.off'
	write(filename)

Blender.Window.FileSelector(fs_callback, "Export OFF")
