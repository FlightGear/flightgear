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

MARGIN = 10 # px
GAP = 10    # px


import Blender, math, random


class Abort(Exception):
	def __init__(self, msg):
		self.msg = msg


def pack():
	image = Blender.Image.GetCurrent()
	if not image:
		raise Abort('No texture image selected')

	imagesize = image.getSize()
	if imagesize[0] != imagesize[1]:
		Blender.Draw.PupMenu("Warning%t|Image isn't a square!")
	gap = (float(GAP) / imagesize[0], float(GAP) / imagesize[1])
	margin = (float(MARGIN) / imagesize[0] - gap[0] * 0.5, float(MARGIN) / imagesize[1] - gap[1] * 0.5)


	def drawrect(x0, y0, x1, y1, color = (255, 255, 255, 255)):
		x0 *= imagesize[0]
		y0 *= imagesize[1]
		x1 *= imagesize[0]
		y1 *= imagesize[1]
		for u in range(int(x0 + 0.5), int(x1 - 0.5)):
			for v in range(int(y0 + 0.5), int(y1 - 0.5)):
				image.setPixelI(u, v, color)

	boxes = []
	meshes = {}

	Blender.Window.DrawProgressBar(0.0, "packing")
	for o in Blender.Scene.GetCurrent().objects.selected:
		if o.type != "Mesh":
			continue

		mesh = o.getData(mesh = 1)
		if not mesh.faceUV:
			continue
		if mesh.name in meshes:
			#print "dropping duplicate mesh", mesh.name, "of object", o.name
			continue
		meshes[mesh.name] = True

		print "\tobject '%s'" % o.name
		xmin = ymin = 1000.0
		xmax = ymax = -1000.0
		for f in mesh.faces:
			for p in f.uv:
				xmin = min(xmin, p[0])
				xmax = max(xmax, p[0])
				ymin = min(ymin, p[1])
				ymax = max(ymax, p[1])

		width = xmax - xmin
		height = ymax - ymin
		boxes.append([0, 0, width + gap[0], height + gap[1], xmin, ymin, mesh, o.name])

	if not boxes:
		raise Abort('No mesh objects selected')


	boxsize = Blender.Geometry.BoxPack2D(boxes)
	xscale = (1.0 - 2.0 * margin[0]) / max(boxsize[0], boxsize[1])
	yscale = (1.0 - 2.0 * margin[1]) / max(boxsize[0], boxsize[1])

	Blender.Window.DrawProgressBar(0.2, "Erasing texture")
	drawrect(0, 0, 1, 1) # erase texture
	for box in boxes:
		xmin = ymin = 1000.0
		xmax = ymax = -1000.0
		for f in box[6].faces:
			for p in f.uv:
				p[0] = (p[0] - box[4] + box[0] + gap[0] * 0.5 + margin[0]) * xscale
				p[1] = (p[1] - box[5] + box[1] + gap[1] * 0.5 + margin[1]) * yscale

				xmin = min(xmin, p[0])
				xmax = max(xmax, p[0])
				ymin = min(ymin, p[1])
				ymax = max(ymax, p[1])

		drawrect(xmin, ymin, xmax, ymax, (random.randint(128, 255), random.randint(128, 255),
				random.randint(128, 255), 255))
	Blender.Window.DrawProgressBar(1.0, "Finished")



editmode = Blender.Window.EditMode()
if editmode:
	Blender.Window.EditMode(0)

try:
	print "box packing ..."
	pack()
	print "done\n"
except Abort, e:
	Blender.Draw.PupMenu("Error%t|" + e.msg)
	print "Error:", e.msg, " -> aborting ...\n"

if editmode:
	Blender.Window.EditMode(1)


