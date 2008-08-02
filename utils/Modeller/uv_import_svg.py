#!BPY

# """
# Name: 'UV: (Re)Import UV from SVG'
# Blender: 245
# Group: 'Image'
# Tooltip: 'Re-import UV layout from SVG file'
# """

__author__ = "Melchior FRANZ < mfranz # aon : at >"
__url__ = ["http://www.flightgear.org/", "http://cvs.flightgear.org/viewvc/source/utils/Modeller/uv_import_svg.py"]
__version__ = "0.2"
__bpydoc__ = """\
Imports an SVG file containing UV maps, which has been saved by the
uv_export.svg script. This allows to move, scale, and rotate object
mappings in SVG editors like Inkscape. Note that all contained UV maps
will be set, no matter which objects are actually selected at the moment.
The choice has been made when the file was saved!
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


ID_SEPARATOR = '_.:._'


import Blender, BPyMessages, sys, math, re, xml.sax


numwsp = re.compile('(?<=[\d.])\s+(?=[-+.\d])')
commawsp = re.compile('\s+|\s*,\s*')
istrans = re.compile('^\s*(skewX|skewY|scale|translate|rotate|matrix)\s*\(([^\)]*)\)\s*')
isnumber = re.compile('^[-+]?(\d+(\.\d*)?|\.\d+)([eE][-+]?\d+)?$')


class Abort(Exception):
	def __init__(self, msg):
		self.msg = msg


class Matrix:
	def __init__(self, a = 1, b = 0, c = 0, d = 1, e = 0, f = 0):
		self.a = a; self.b = b; self.c = c; self.d = d; self.e = e; self.f = f

	def __str__(self):
		return "[Matrix %f %f %f %f %f %f]" % (self.a, self.b, self.c, self.d, self.e, self.f)

	def multiply(self, mat):
		a = mat.a * self.a + mat.c * self.b
		b = mat.b * self.a + mat.d * self.b
		c = mat.a * self.c + mat.c * self.d
		d = mat.b * self.c + mat.d * self.d
		e = mat.a * self.e + mat.c * self.f + mat.e
		f = mat.b * self.e + mat.d * self.f + mat.f
		self.a = a; self.b = b; self.c = c; self.d = d; self.e = e; self.f = f

	def transform(self, u, v):
		return u * self.a + v * self.c + self.e, u * self.b + v * self.d + self.f

	def translate(self, dx, dy):
		self.multiply(Matrix(1, 0, 0, 1, dx, dy))

	def scale(self, sx, sy):
		self.multiply(Matrix(sx, 0, 0, sy, 0, 0))

	def rotate(self, a):
		a *= math.pi / 180
		self.multiply(Matrix(math.cos(a), math.sin(a), -math.sin(a), math.cos(a), 0, 0))

	def skewX(self, a):
		a *= math.pi / 180
		self.multiply(Matrix(1, 0, math.tan(a), 1, 0, 0))

	def skewY(self, a):
		a *= math.pi / 180
		self.multiply(Matrix(1, math.tan(a), 0, 1, 0, 0))


def parse_transform(s):
	matrix = Matrix()
	while True:
		match = istrans.match(s)
		if not match:
			break
		cmd = match.group(1)
		values = commawsp.split(match.group(2).strip())
		s = s[len(match.group(0)):]
		arg = []

		for value in values:
			match = isnumber.match(value)
			if not match:
				raise Abort("bad transform value")

			arg.append(float(match.group(0)))

		num = len(arg)
		if cmd == "skewX":
			if num == 1:
				matrix.skewX(arg[0])
				continue

		elif cmd == "skewY":
			if num == 1:
				matrix.skewY(arg[0])
				continue

		elif cmd == "scale":
			if num == 1:
				matrix.scale(arg[0], arg[0])
				continue
			if num == 2:
				matrix.scale(arg[0], arg[1])
				continue

		elif cmd == "translate":
			if num == 1:
				matrix.translate(arg[0], 0)
				continue
			if num == 2:
				matrix.translate(arg[0], arg[1])
				continue

		elif cmd == "rotate":
			if num == 1:
				matrix.rotate(arg[0])
				continue
			if num == 3:
				matrix.translate(-arg[1], -arg[2])
				matrix.rotate(arg[0])
				matrix.translate(arg[1], arg[2])
				continue

		elif cmd == "matrix":
			if num == 6:
				matrix.multiply(Matrix(*arg))
				continue

		else:
			print "ERROR: unknown transform", cmd
			continue

		print "ERROR: '%s' with wrong argument number (%d)" % (cmd, num)

	if len(s):
		print "ERROR: transform with trailing garbage (%s)" % s
	return matrix


class import_svg(xml.sax.handler.ContentHandler):
	# err_handler
	def error(self, exception):
		raise Abort(str(exception))

	def fatalError(self, exception):
		raise Abort(str(exception))

	def warning(self, exception):
		print "WARNING: " + str(exception)

	# doc_handler
	def setDocumentLocator(self, whatever):
		pass

	def startDocument(self):
		self.verified = False
		self.scandesc = False
		self.matrices = [None]
		self.meshes = {}
		for o in Blender.Scene.GetCurrent().objects:
			if o.type != "Mesh":
				continue
			mesh = o.getData(mesh = 1)
			if not mesh.faceUV:
				continue
			if mesh.name in self.meshes:
				continue
			self.meshes[mesh.name] = mesh

	def endDocument(self):
		pass

	def characters(self, data):
		if not self.scandesc:
			return
		if data.startswith("uv_export_svg.py"):
			self.verified = True

	def ignorableWhitespace(self, data, start, length):
		pass

	def processingInstruction(self, target, data):
		pass

	def startElement(self, name, attrs):
		currmat = self.matrices[-1]
		if "transform" in attrs:
			m = parse_transform(attrs["transform"])
			if currmat != None:
				m.multiply(currmat)
			self.matrices.append(m)
		else:
			self.matrices.append(currmat)

		if name == "polygon":
			self.handlePolygon(attrs)
		elif name == "svg":
			if "viewBox" in attrs:
				x, y, w, h = commawsp.split(attrs["viewBox"], 4)
				if int(x) or int(y):
					raise Abort("bad viewBox")
				self.width = int(w)
				self.height = int(h)
				if self.width != self.height:
					raise Abort("viewBox isn't a square")
			else:
				raise Abort("no viewBox")
		elif name == "desc" and not self.verified:
			self.scandesc = True

	def endElement(self, name):
		self.scandesc = False
		self.matrices.pop()

	def handlePolygon(self, attrs):
		if not self.verified:
			raise Abort("this file wasn't written by uv_export_svg.py")
		ident = attrs.get("id", None)
		points = attrs.get("points", None)

		if not ident or not points:
			print('bad polygon "%s"' % ident)
			return

		try:
			meshname, num = ident.strip().split(ID_SEPARATOR, 2)
		except:
			print('broken id "%s"' % ident)
			return

		if not meshname in self.meshes:
			print('unknown mesh "%s"' % meshname)
			return

		#print 'mesh %s face %d: ' % (meshname, num)
		matrix = self.matrices[-1]
		transuv = []
		for p in numwsp.split(points.strip()):
			u, v = commawsp.split(p.strip(), 2)
			u = float(u)
			v = float(v)
			if matrix:
				u, v = matrix.transform(u, v)
			transuv.append((u / self.width, 1 - v / self.height))

		for i, uv in enumerate(self.meshes[meshname].faces[int(num)].uv):
			uv[0] = transuv[i][0]
			uv[1] = transuv[i][1]


def run_parser(path):
	if BPyMessages.Error_NoFile(path):
		return

	editmode = Blender.Window.EditMode()
	if editmode:
		Blender.Window.EditMode(0)
	Blender.Window.WaitCursor(1)

	try:
		xml.sax.parse(path, import_svg(), import_svg())
		Blender.Registry.SetKey("UVImportExportSVG", { "path" : path }, False)

	except Abort, e:
		print "Error:", e.msg, "  -> aborting ...\n"
		Blender.Draw.PupMenu("Error%t|" + e.msg)

	Blender.Window.RedrawAll()
	Blender.Window.WaitCursor(0)
	if editmode:
		Blender.Window.EditMode(1)


registry = Blender.Registry.GetKey("UVImportExportSVG", False)
if registry and "path" in registry and Blender.sys.exists(Blender.sys.expandpath(registry["path"])):
	path = registry["path"]
else:
	path = ""

Blender.Window.FileSelector(run_parser, "Import SVG", path)

