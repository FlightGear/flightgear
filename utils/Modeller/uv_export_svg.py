#!BPY

# """
# Name: 'UV: Export to SVG'
# Blender: 245
# Group: 'Image'
# Tooltip: 'Export selected objects to SVG file'
# """

__author__ = "Melchior FRANZ < mfranz # aon : at >"
__url__ = ["http://www.flightgear.org/", "http://cvs.flightgear.org/viewvc/source/utils/Modeller/uv_export_svg.py"]
__version__ = "0.1"
__bpydoc__ = """\
Saves the UV mappings of all selected objects to an SVG file. The uv_import_svg.py
script can be used to re-import such a file. Each object and each group of adjacent
faces therein will be made a separate SVG group.
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


FILL_COLOR = None  # 'yellow' or '#ffa000' e.g. for uni-color, None for random color
ID_SEPARATOR = '_.:._'


import Blender, BPyMessages, sys, random


class Abort(Exception):
	def __init__(self, msg):
		self.msg = msg


class UVFaceGroups:
	def __init__(self, mesh):
		faces = dict([(f.index, f) for f in mesh.faces])
		self.groups = []
		while faces:
			self.groups.append(self.adjacent(faces))

	def __len__(self):
		return len(self.groups)

	def __iter__(self):
		return self.groups.__iter__()

	def adjacent(self, faces):
		uvcoords = {}
		face = faces.popitem()[1]
		group = [face]
		for c in face.uv:
			uvcoords[tuple(c)] = True

		while True:
			found = []
			for face in faces.itervalues():
				for c in face.uv:
					if tuple(c) in uvcoords:
						for c in face.uv:
							uvcoords[tuple(c)] = True
						found.append(face)
						break
			if not found:
				return group
			for face in found:
				group.append(face)
				del faces[face.index]


def stringcolor(string):
	random.seed(hash(string))
	c = [random.randint(220, 255), random.randint(120, 240), random.randint(120, 240)]
	random.shuffle(c)
	return "#%02x%02x%02x" % tuple(c)


def write_svg(path):
	size = Blender.Draw.PupMenu("Image size%t|128|256|512|1024|2048|4096|8192")
	if size < 0:
		raise Abort('no image size chosen')
	size = 1 << (size + 6)

	svg = open(path, "w")
	svg.write('<?xml version="1.0" standalone="no"?>\n')
	svg.write('<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">\n\n')
	svg.write('<svg width="%spx" height="%spx" viewBox="0 0 %d %d" xmlns="http://www.w3.org/2000/svg"' \
			' version="1.1" xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape">\n'
			% (size, size, size, size))
	svg.write("\t<desc>uv_export_svg.py: %s</desc>\n" % path);
	svg.write('\t<rect x="0" y="0" width="%d" height="%d" fill="none" stroke="blue" stroke-width="%f"/>\n'
			% (size, size, 1.0))

	objects = {}
	for o in Blender.Scene.GetCurrent().objects.selected:
		if o.type != "Mesh":
			continue

		mesh = o.getData(mesh = 1)
		if mesh.faceUV:
			objects[mesh.name] = (o.name, mesh)

	for meshname, v in objects.iteritems():
		objname, mesh = v
		color = FILL_COLOR or stringcolor(meshname)

		svg.write('\t<g style="fill:%s; stroke:black stroke-width:1px" inkscape:label="%s" ' \
				'id="%s">\n' % (color, objname, objname))

		facegroups = UVFaceGroups(mesh)
		for faces in facegroups:
			indent = '\t\t'
			if len(facegroups) > 1:
				svg.write('\t\t<g>\n')
				indent = '\t\t\t'
			for f in faces:
				svg.write('%s<polygon id="%s%s%d" points="' % (indent, meshname, ID_SEPARATOR, f.index))
				for p in f.uv:
					svg.write('%.8f,%.8f ' % (p[0] * size, size - p[1] * size))
				svg.write('"/>\n')
			if len(facegroups) > 1:
				svg.write('\t\t</g>\n')

		svg.write("\t</g>\n")

	svg.write('</svg>\n')
	svg.close()


def export(path):
	if not BPyMessages.Warning_SaveOver(path):
		return

	editmode = Blender.Window.EditMode()
	if editmode:
		Blender.Window.EditMode(0)

	try:
		write_svg(path)
		Blender.Registry.SetKey("UVImportExportSVG", { "path" : path }, False)

	except Abort, e:
		print "Error:", e.msg, "  -> aborting ...\n"
		Blender.Draw.PupMenu("Error%t|" + e.msg)

	if editmode:
		Blender.Window.EditMode(1)


registry = Blender.Registry.GetKey("UVImportExportSVG", False)
if registry and "path" in registry:
	path = registry["path"]
else:
	active = Blender.Scene.GetCurrent().objects.active
	basename = Blender.sys.basename(Blender.sys.splitext(Blender.Get("filename"))[0])
	path = basename + "-" + active.name + ".svg"

Blender.Window.FileSelector(export, "Export to SVG", path)

