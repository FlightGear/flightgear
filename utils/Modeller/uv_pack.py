#!BPY

# """
# Name: 'Pack selected objects on a square'
# Blender: 245
# Group: 'UV'
# Tooltip: 'Pack UV maps of all selected objects onto an empty square texture'
# """

__author__ = "Melchior FRANZ < mfranz # aon : at >"
__url__ = "http://members.aon.at/mfranz/flightgear/"
__version__ = "0.1"
__bpydoc__ = """\
Script for mapping multiple objects onto one square texture.

Usage:
(1) create new square texture in the UV editor
(2) map all objects individually, choosing the most appropriate technique
(3) scale each of the mappings to the appropriate size, relative to the
    other object mappings
(4) select all objects and switch to edit mode (consider to use
    Select->Linked->Material or similar methods)
(5) start this script with UVs->Scripts->Pack objects on a square

    [now the texture image will first be erased, then colored rectangles
    will appear for each object]

(6) rescale and/or remap objects that you aren't happy with (the
    relative size of a mapping will be kept)

    [continue with (5) until you like the result]

(7) export UV layout to SVG (UVs->Scripts->Save UV Face Layout)
"""


#--------------------------------------------------------------------------------
# Copyright (C) 2008  Melchior FRANZ  < mfranz # aon : at >
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#--------------------------------------------------------------------------------


MARGIN = 10 # px
GAP = 10    # px


import Blender
from random import randint as rand


class Abort(Exception):
	def __init__(self, msg):
		self.msg = msg


def pack():
	image = Blender.Image.GetCurrent()
	if not image:
		raise Abort('No texture image selected')

	imgwidth, imgheight = image.getSize()
	if imgwidth != imgheight:
		Blender.Draw.PupMenu("Warning%t|Image isn't a square!")
	gap = (float(GAP) / imgwidth, float(GAP) / imgheight)
	margin = (float(MARGIN) / imgwidth - gap[0] * 0.5, float(MARGIN) / imgheight - gap[1] * 0.5)

	def drawrect(x0, y0, x1, y1, color = (255, 255, 255, 255)):
		x0 *= imgwidth
		x1 *= imgwidth
		y0 *= imgheight
		y1 *= imgheight
		for u in range(int(x0 + 0.5), int(x1 - 0.5)):
			for v in range(int(y0 + 0.5), int(y1 - 0.5)):
				image.setPixelI(u, v, color)


	boxes = []
	unique_meshes = {}

	BIG = 1<<30
	Blender.Window.DrawProgressBar(0.0, "packing")
	for o in Blender.Scene.GetCurrent().objects.selected:
		if o.type != "Mesh":
			continue

		mesh = o.getData(mesh = 1)
		if not mesh.faceUV:
			continue
		if mesh.name in unique_meshes:
			#print "dropping duplicate mesh", mesh.name, "of object", o.name
			continue
		unique_meshes[mesh.name] = True

		print "\tobject '%s'" % o.name
		xmin = ymin = BIG
		xmax = ymax = -BIG
		for f in mesh.faces:
			for p in f.uv:
				xmin = min(xmin, p[0])
				xmax = max(xmax, p[0])
				ymin = min(ymin, p[1])
				ymax = max(ymax, p[1])

		width = xmax - xmin
		height = ymax - ymin
		boxes.append([0, 0, width + gap[0], height + gap[1], xmin, ymin, mesh])

	if not boxes:
		raise Abort('No mesh objects selected')


	boxwidth, boxheight = Blender.Geometry.BoxPack2D(boxes)
	boxmax = max(boxwidth, boxheight)
	xscale = (1.0 - 2.0 * margin[0]) / boxmax
	yscale = (1.0 - 2.0 * margin[1]) / boxmax

	image.reload()
	#drawrect(0, 0, 1, 1) # erase texture

	for i, box in enumerate(boxes):
		Blender.Window.DrawProgressBar(float(i) * len(boxes), "Drawing")
		xmin = ymin = BIG
		xmax = ymax = -BIG
		for f in box[6].faces:
			for p in f.uv:
				p[0] = (p[0] - box[4] + box[0] + gap[0] * 0.5 + margin[0]) * xscale
				p[1] = (p[1] - box[5] + box[1] + gap[1] * 0.5 + margin[1]) * yscale

				xmin = min(xmin, p[0])
				xmax = max(xmax, p[0])
				ymin = min(ymin, p[1])
				ymax = max(ymax, p[1])

		drawrect(xmin, ymin, xmax, ymax, (rand(128, 255), rand(128, 255), rand(128, 255), 255))
		box[6].update()

	Blender.Window.RedrawAll()
	Blender.Window.DrawProgressBar(1.0, "Finished")



editmode = Blender.Window.EditMode()
if editmode:
	Blender.Window.EditMode(0)
Blender.Window.WaitCursor(1)

try:
	print "box packing ..."
	pack()
	print "done\n"
except Abort, e:
	print "Error:", e.msg, "  -> aborting ...\n"
	Blender.Draw.PupMenu("Error%t|" + e.msg)

Blender.Window.WaitCursor(0)
if editmode:
	Blender.Window.EditMode(1)


