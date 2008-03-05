#!BPY

# """
# Name: 'SVG: Export UV layout to SVG file'
# Blender: 245
# Group: 'UV'
# Tooltip: 'Export selected objects to SVG file'
# """

__author__ = "Melchior FRANZ < mfranz # aon : at >"
__url__ = "http://members.aon.at/mfranz/flightgear/"
__version__ = "0.1"
__bpydoc__ = """\
Saves the UV mappings of all selected objects to an SVG file. The uv_import_svg.py
script can be used to re-import such a file. Each object and each group of adjacent
faces therein will be made a separate SVG group.
"""

ID_SEPARATOR = '_____'


import Blender, sys


class Abort(Exception):
	def __init__(self, msg):
		self.msg = msg


def get_adjacent(pool):
	i, face = pool.popitem()
	group = [face]

	uvcoords = {}
	for c in face.uv:
		uvcoords[(c[0], c[1])] = True

	while True:
		found = []
		for face in pool.itervalues():
			for c in face.uv:
				if (c[0], c[1]) in uvcoords:
					for d in face.uv:
						uvcoords[(d[0], d[1])] = True
					found.append(face)
					break
		if not found:
			break
		for face in found:
			group.append(face)
			del pool[face.index]

	return group


def write_svg(filename):
	size = Blender.Draw.PupMenu("Image size%t|128|256|512|1024|2048|4096|8192")
	if size < 0:
		raise Abort('no image size chosen')
	size = 1 << (size + 6)

	print "exporting to '%s' (size %d) ... " % (filename, size),
	svg = open(filename, "w")
	svg.write('<?xml version="1.0" standalone="no"?>\n')
	svg.write('<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">\n\n')
	svg.write('<svg width="%spx" height="%spx" viewBox="0 0 %d %d" xmlns="http://www.w3.org/2000/svg"' \
			'version="1.1" xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape">\n'
			% (size, size, size, size))
	svg.write("\t<desc>uv_export_svg.py: %s</desc>\n" % filename);
	svg.write('\t<rect x="0" y="0" width="%d" height="%d" fill="none" stroke="blue" stroke-width="%f"/>\n'
			% (size, size, 1.0))

	unique_meshes = {}
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

		svg.write('\t<g style="fill:yellow; stroke:black stroke-width:1px" inkscape:label="%s" ' \
				'id="%s">\n' % (o.name, o.name))

		pool = {}
		for f in mesh.faces:
			pool[f.index] = f

		while len(pool):
			svg.write('\t\t<g>\n')
			for f in get_adjacent(pool):
				svg.write('\t\t\t<polygon points="')
				for p in f.uv:
					svg.write('%.8f,%.8f ' % (p[0] * size, size - p[1] * size))
				svg.write('" id="%s%s%d"/>\n' % (mesh.name, ID_SEPARATOR, f.index))
			svg.write('\t\t</g>\n')

		svg.write("\t</g>\n")

	svg.write('</svg>\n')
	svg.close()
	print "done."


def export(filename):
	registry = {}
	registry[basename] = Blender.sys.basename(filename)
	Blender.Registry.SetKey("UVImportExportSVG", registry, False)

	editmode = Blender.Window.EditMode()
	if editmode:
		Blender.Window.EditMode(0)

	try:
		write_svg(filename)
	except Abort, e:
		print "Error:", e.msg, "  -> aborting ...\n"
		Blender.Draw.PupMenu("Error%t|" + e.msg)

	if editmode:
		Blender.Window.EditMode(1)


active = Blender.Scene.GetCurrent().objects.active
(basename, extname) = Blender.sys.splitext(Blender.Get("filename"))
filename = Blender.sys.basename(basename) + "-" + active.name + ".svg"
Blender.Window.FileSelector(export, "Export to SVG", filename)

