#!BPY

"""
Name: 'Raw Triangle (.raw)...'
Blender: 232
Group: 'Export'
Tooltip: 'Export selected mesh to Raw Triangle Format (.raw)'
"""

__author__ = "Anthony D'Agostino (Scorpius)"
__url__ = ("blender", "elysiun",
"Author's homepage, http://www.redrival.com/scorpius")
__version__ = "Part of IOSuite 0.5"

__bpydoc__ = """\
This script exports meshes to Raw Triangle file format.

Usage:

Select meshes to be exported and run this script from "File->Export" menu.
"""


# $Id$
#
# +---------------------------------------------------------+
# | Copyright (c) 2002 Anthony D'Agostino                   |
# | http://www.redrival.com/scorpius                        |
# | scorpius@netzero.com                                    |
# | April 28, 2002                                          |
# | Released under the Blender Artistic Licence (BAL)       |
# | Import Export Suite v0.5                                |
# +---------------------------------------------------------+
# | Read and write RAW Triangle File Format (*.raw)         |
# +---------------------------------------------------------+

import Blender, mod_meshtools
import sys
#import time

# =================================
# === Write RAW Triangle Format ===
# =================================
def write(filename):
	#start = time.clock()
	file = open(filename, "wb")

	objects = Blender.Object.GetSelected()
	objname = objects[0].name
	meshname = objects[0].data.name
	mesh = Blender.NMesh.GetRaw(meshname)
	obj = Blender.Object.Get(objname)


	std=sys.stdout
	sys.stdout=file
	for face in mesh.faces:
		if len(face.v) == 3:		# triangle
			v1, v2, v3 = face.v
			faceverts = tuple(v1.co) + tuple(v2.co) + tuple(v3.co)
			print "% f % f % f % f % f % f % f % f % f" % faceverts
		else:						# quadrilateral
			v1, v2, v3, v4 = face.v
			faceverts1 = tuple(v1.co) + tuple(v2.co) + tuple(v3.co)
			faceverts2 = tuple(v3.co) + tuple(v4.co) + tuple(v1.co)
			print "% f % f % f % f % f % f % f % f % f" % faceverts1
			print "% f % f % f % f % f % f % f % f % f" % faceverts2
	sys.stdout=std


	Blender.Window.DrawProgressBar(1.0, '')  # clear progressbar
	file.close()
	#end = time.clock()
	#seconds = " in %.2f %s" % (end-start, "seconds")
	message = "Successfully exported " + Blender.sys.basename(filename)# + seconds
	mod_meshtools.print_boxed(message)

def fs_callback(filename):
	if filename.find('.raw', -4) <= 0: filename += '.raw'
	write(filename)

Blender.Window.FileSelector(fs_callback, "Export Raw")
