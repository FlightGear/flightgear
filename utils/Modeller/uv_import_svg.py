#!BPY

# """
# Name: 'SVG: Re-Import UV layout from SVG file'
# Blender: 245
# Group: 'UV'
# Tooltip: 'Re-import UV layout from SVG file'
# """

__author__ = "Melchior FRANZ < mfranz # aon : at >"
__url__ = "http://members.aon.at/mfranz/flightgear/"
__version__ = "0.1"
__bpydoc__ = """\
Imports an SVG file containing UV maps, which has been saved by the
uv_export.svg script. This allows to move, scale, and rotate object
mapping in SVG editors like Inkscape. Note that all contained UV maps
will be set, no matter which objects are actually selected at the moment.
The choice has been made when the file was saved!
"""


ID_SEPARATOR = '#'


import Blender, sys, math, re
from xml.sax import saxexts


registry = {}
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
		a = self.a * mat.a + self.c * mat.b
		b = self.b * mat.a + self.d * mat.b
		c = self.a * mat.c + self.c * mat.d
		d = self.b * mat.c + self.d * mat.d
		e = self.a * mat.e + self.c * mat.f + self.e
		f = self.b * mat.e + self.d * mat.f + self.f
		self.a = a; self.b = b; self.c = c; self.d = d; self.e = e; self.f = f

	def transform(self, u, v):
		x = u * self.a + v * self.c + self.e
		y = u * self.b + v * self.d + self.f
		return (x, y)

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
				matrix.multiply(Matrix(arg[0], arg[1], arg[2], arg[3], arg[4], arg[5]))
				continue

		else:
			print "ERROR: unknown transform", cmd
			continue

		print "ERROR: '%s' with wrong argument number (%d)" % (cmd, num)

	if len(s):
		print "ERROR: transform with trailing garbage (%s)" % s
	return matrix


class import_svg:
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

	def characters(self, data, start, length):
		if not self.scandesc:
			return
		if data[start:start + length].startswith("uv_export_svg.py"):
			self.verified = True

	def ignorableWhitespace(self, data, start, length):
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
		self.matrices = self.matrices[:-1]

	def handlePolygon(self, attrs):
		if not self.verified:
			raise Abort("this file wasn't written by uv_export_svg.py")
		ident = attrs.get("id", None)
		points = attrs.get("points", None)

		if not ident or not points:
			print('bad polygon "%s"' % ident)
			return

		sep = ident.find(ID_SEPARATOR)
		if sep < 0:
			print('broken id "%s"' % ident)
			return

		meshname = str(ident[:sep])
		num = int(ident[sep + 1:])

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

		for i, uv in enumerate(self.meshes[meshname].faces[num].uv):
			uv[0] = transuv[i][0]
			uv[1] = transuv[i][1]


def run_parser(filename):
	editmode = Blender.Window.EditMode()
	if editmode:
		Blender.Window.EditMode(0)
	Blender.Window.WaitCursor(1)

	try:
		svg = saxexts.ParserFactory().make_parser("xml.sax.drivers.drv_xmlproc")
		svg.setDocumentHandler(import_svg())
		svg.setErrorHandler(import_svg())
		svg.parse(filename)

	except Abort, e:
		print "Error:", e.msg, "  -> aborting ...\n"
		Blender.Draw.PupMenu("Error%t|" + e.msg)

	Blender.Window.RedrawAll()
	Blender.Window.WaitCursor(0)
	if editmode:
		Blender.Window.EditMode(1)


active = Blender.Scene.GetCurrent().objects.active
(basename, extname) = Blender.sys.splitext(Blender.Get("filename"))
filename = Blender.sys.basename(basename) + "-" + active.name + ".svg"

registry = Blender.Registry.GetKey("UVImportExportSVG", False)
if registry and basename in registry:
	filename = registry[basename]

Blender.Window.FileSelector(run_parser, "Import SVG", filename)

