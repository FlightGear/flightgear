#!BPY

# """
# Name: 'FlightGear Animation (.xml)'
# Blender: 243
# Group: 'Export'
# Submenu: 'Generate textured lights'			LIGHTS
# Submenu: 'Rotation'					ROTATE
# Submenu: 'Range (LOD)'				RANGE
# Submenu: 'Translation from origin'			TRANS0
# Submenu: 'Translation from cursor'			TRANSC
# Tooltip: 'FlightGear Animation'
# """

# BLENDER PLUGIN
#
# Put this file into ~/.blender/scripts/. You'll then find
# it in Blender under "File->Export->FlightGear Animation (*.xml)"
#
# For the script to work properly, make sure a directory
# ~/.blender/scripts/bpydata/config/ exists. To change the
# script parameters, edit file fgfs_animation.cfg in that
# dir, or in blender, switch to "Scripts Window" view, select
# "Scripts->System->Scripts Config Editor" and there select
# "Export->FlightGear Animation (*.xml)". Don't forget to
# "apply" the changes.
#
# This file is Public Domain.


__author__ = "Melchior FRANZ < mfranz # aon : at >"
__url__ = "http://members.aon.at/mfranz/flightgear/"
__version__ = "$Revision$ -- $Date$"
__bpydoc__ = """\
== Generate textured lights ==

Adds linked "light objects" (square consisting of two triangles at
origin) for each selected vertex, and creates FlightGear animation
code that scales all faces according to view distance, moves them to
the vertex location, turns the face towards the viewer and blends
it with other (semi)transparent objects. All lights are turned off
at daylight.


== Translation from origin ==

Generates "translate" animation for each selected vertex and for the
cursor, if it is not at origin. This mode is thought for moving
objects to their proper location after scaling them at origin, a
technique that is used for lights.


== Translation from cursor ==

Same as above, except that movements from the cursor location are
calculated.


== Rotation ==

Generates one "rotate" animation from two selected vertices
(rotation axis), and the cursor (rotation center).


== Range ==

Generates "range" animation skeleton with list of all selected objects.
"""


#==================================================================================================


import sys
import Blender
from math import sqrt
from Blender import Mathutils
from Blender.Mathutils import Vector, VecMultMat


REG_KEY = 'fgfs_animation'
MODE = __script__['arg']
ORIGIN = [0, 0, 0]

FILENAME = Blender.Get("filename")
(BASENAME, EXTNAME) = Blender.sys.splitext(FILENAME)
DIRNAME = Blender.sys.dirname(BASENAME)
FILENAME = Blender.sys.basename(BASENAME) + "-export.xml"

TOOLTIPS = {
	'PATHNAME': "where exported data should be saved (current dir if empty)",
	'INDENT': "number of spaces per indentation level, or 0 for tabs",
}

reg = Blender.Registry.GetKey(REG_KEY, True)
if not reg:
	print "WRITING REGISTRY"
	Blender.Registry.SetKey(REG_KEY, {
		'INDENT': 0,
		'PATHNAME': "",
		'tooltips': TOOLTIPS,
	}, True)




#==================================================================================================


class Error(Exception):
	pass


class BlenderSetup:
	def __init__(self):
		#Blender.Window.WaitCursor(1)
		self.is_editmode = Blender.Window.EditMode()
		if self.is_editmode:
			Blender.Window.EditMode(0)

	def __del__(self):
		#Blender.Window.WaitCursor(0)
		if self.is_editmode:
			Blender.Window.EditMode(1)


class XMLExporter:
	def __init__(self, filename):
		cursor = Blender.Window.GetCursorPos()
		self.file = open(filename, "w")
		self.write('<?xml version="1.0"?>\n')
		self.write("<!-- project: %s -->\n" % Blender.Get("filename"))
		self.write("<!-- cursor: %0.12f %0.12f %0.12f -->\n\n" % (cursor[0], cursor[1], cursor[2]))
		self.write("<PropertyList>\n")
		self.write("\t<path>%s.ac</path>\n\n" % BASENAME)

	def __del__(self):
		try:
			self.write("</PropertyList>\n")
			self.file.close()
		except AttributeError:		# opening in __init__ failed
			pass

	def write(self, s):
		global INDENT
		if INDENT <= 0:
			self.file.write(s)
		else:
			self.file.write(s.expandtabs(INDENT))

	def comment(self, s, e = "", a = ""):
		self.write(a + "<!-- " + s + " -->\n" + e)

	def translate(self, name, va, vb):
		x = vb[0] - va[0]
		y = vb[1] - va[1]
		z = vb[2] - va[2]
		length = sqrt(x * x + y * y + z * z)
		s = """\
	<animation>
		<type>translate</type>
		<object-name>%s</object-name>
		<offset-m>%s</offset-m>
		<axis>
			<x>%s</x>
			<y>%s</y>
			<z>%s</z>
		</axis>
	</animation>\n\n"""
		self.write(s % (name, Round(length), Round(x), Round(y), Round(z)))

	def rotate(self, name, center, va, vb):
		x = Round(vb[0] - va[0])
		y = Round(vb[1] - va[1])
		z = Round(vb[2] - va[2])
		self.write("\t<!-- Vertex 0: %f %f %f -->\n" % (va[0], va[1], va[2]))
		self.write("\t<!-- Vertex 1: %f %f %f -->\n" % (vb[0], vb[1], vb[2]))
		s = """\
	<animation>
		<type>rotate</type>
		<object-name>%s</object-name>
		<property>null</property>
		<!--min-deg>0</min-deg-->
		<!--max-deg>360</max-deg-->
		<!--factor>1</factor-->
		<center>
			<x-m>%s</x-m>
			<y-m>%s</y-m>
			<z-m>%s</z-m>
		</center>
		<axis>
			<x>%s</x>
			<y>%s</y>
			<z>%s</z>
		</axis>
	</animation>\n\n"""
		self.write(s % (name, Round(center[0]), Round(center[1]), Round(center[2]), x, y, z))

	def billboard(self, name, spherical = "true"):
		s = """\
	<animation>
		<type>billboard</type>
		<object-name>%s</object-name>
		<spherical type="bool">%s</spherical>
	</animation>\n\n"""
		self.write(s % (name, spherical))

	def group(self, name, objects):
		self.write("\t<animation>\n");
		self.write("\t\t<name>%s</name>\n" % name);
		for o in objects:
			self.write("\t\t<object-name>%s</object-name>\n" % o.name)
		self.write("\t</animation>\n\n");

	def alphatest(self, name, factor = 0.001):
		s = """\
	<animation>
		<type>alpha-test</type>
		<object-name>%s</object-name>
		<alpha-factor>%s</alpha-factor>
	</animation>\n\n"""
		self.write(s % (name, factor))

	def sunselect(self, name, angle = 1.57):
		s = """\
	<animation>
		<type>select</type>
		<name>%s</name>
		<object-name>%s</object-name>
		<condition>
			<greater-than>
				<property>/sim/time/sun-angle-rad</property>
				<value>%s</value>
			</greater-than>
		</condition>
	</animation>\n\n"""
		self.write(s % (name + "Night", name, angle))

	def distscale(self, name, base):
		s = """\
	<animation>
		<type>dist-scale</type>
		<object-name>%s</object-name>
		<interpolation>
			<entry>
				<ind>0</ind>
				<dep alias="../../../../%sparams/light-near"/>
			</entry>
			<entry>
				<ind>500</ind>
				<dep alias="../../../../%sparams/light-med"/>
			</entry>
			<entry>
				<ind>16000</ind>
				<dep alias="../../../../%sparams/light-far"/>
			</entry>
		</interpolation>
	</animation>\n\n"""
		self.write(s % (name, base, base, base))

	def range(self, objects):
		self.write("\t<animation>\n")
		self.write("\t\t<type>range</type>\n")
		for o in objects:
			self.write("\t\t<object-name>%s</object-name>\n" % o.name)
		self.write("""\
		<min-m>0</min-m>
		<!--
		<max-property>/sim/rendering/static-lod/detailed</max-property>

		<min-property>/sim/rendering/static-lod/detailed</min-property>
		<max-property>/sim/rendering/static-lod/rough</max-property>

		<min-property>/sim/rendering/static-lod/rough</min-property>
		-->
		<max-property>/sim/rendering/static-lod/bare</max-property>\n""")
		self.write("\t</animation>\n\n")

	def lightrange(self, dist = 25000):
		s = """\
	<animation>
		<type>range</type>
		<min-m>0</min-m>
		<max-m>%s</max-m>
	</animation>\n\n"""
		self.write(s % dist)


#==================================================================================================


def Round(f, digits = 6):
	r = round(f, digits)
	if r == int(r):
		return str(int(r))
	else:
		return str(r)


def serial(i, max = 0):
	if max == 1:
		return ""
	if max < 10:
		return "%d" % i
	if max < 100:
		return "%02d" % i
	if max < 1000:
		return "%03d" % i
	if max < 10000:
		return "%04d" % i
	return "%d" % i


def needsObjects(objects, n):
	def error(e):
		raise Error("wrong number of selected mesh objects: please select " + e)
	if n < 0 and len(objects) < -n:
		if n == -1:
			error("at least one object")
		else:
			error("at least %d objects" % -n)
	elif n > 0 and len(objects) != n:
		if n == 1:
			error("exactly one object")
		else:
			error("exactly %d objects" % n)


def checkName(name):
	""" check if name is already in use """
	try:
		Blender.Object.Get(name)
		raise Error("can't generate object '" + name + "'; name already in use")
	except AttributeError:
		pass
	except ValueError:
		pass


def selectedVertices(object):
	verts = []
	mat = object.getMatrix('worldspace')
	for v in object.getData().verts:
		if not v.sel:
			continue
		vec = Vector([v[0], v[1], v[2]])
		vec.resize4D()
		vec *= mat
		v[0], v[1], v[2] = vec[0], vec[1], vec[2]
		verts.append(v)
	return verts


def createLightMesh(name, size = 1):
	mesh = Blender.NMesh.New(name + "mesh")
	mesh.mode = Blender.NMesh.Modes.NOVNORMALSFLIP
	mesh.verts.append(Blender.NMesh.Vert(-size, 0, -size))
	mesh.verts.append(Blender.NMesh.Vert(size, 0, -size))
	mesh.verts.append(Blender.NMesh.Vert(size, 0, size))
	mesh.verts.append(Blender.NMesh.Vert(-size, 0, size))

	face1 = Blender.NMesh.Face()
	face1.v.append(mesh.verts[0])
	face1.v.append(mesh.verts[1])
	face1.v.append(mesh.verts[2])
	mesh.faces.append(face1)

	face2 = Blender.NMesh.Face()
	face2.v.append(mesh.verts[0])
	face2.v.append(mesh.verts[2])
	face2.v.append(mesh.verts[3])
	mesh.faces.append(face2)

	mat = Blender.Material.New(name + "mat")
	mat.setRGBCol(1, 1, 1)
	mat.setMirCol(1, 1, 1)
	mat.setAlpha(1)
	mat.setEmit(1)
	mat.setSpecCol(0, 0, 0)
	mat.setSpec(0)
	mat.setAmb(0)
	mesh.setMaterials([mat])
	return mesh


def createLight(mesh, name):
	object = Blender.Object.New("Mesh", name)
	object.link(mesh)
	Blender.Scene.getCurrent().link(object)
	return object


# modes ===========================================================================================


class mode:
	def __init__(self, xml = None):
		self.cursor = Blender.Window.GetCursorPos()
		self.objects = [o for o in Blender.Object.GetSelected() if o.getType() == 'Mesh']
		if xml != None:
			BlenderSetup()
			self.test()
			self.execute(xml)

	def test(self):
		pass


class translationFromOrigin(mode):
	def execute(self, xml):
		if self.cursor != ORIGIN:
			xml.translate("BlenderCursor", ORIGIN, self.cursor)

		needsObjects(self.objects, 1)
		object = self.objects[0]
		verts = selectedVertices(object)
		if len(verts):
			xml.comment('[%s] translate from origin: "%s"' % (BASENAME, object.name), '\n')
			for i, v in enumerate(verts):
				xml.translate("X%d" % i, ORIGIN, v)


class translationFromCursor(mode):
	def test(self):
		needsObjects(self.objects, 1)
		self.object = self.objects[0]
		self.verts = selectedVertices(self.object)
		if not len(self.verts):
			raise Error("no vertex selected")

	def execute(self, xml):
		xml.comment('[%s] translate from cursor: "%s"' % (BASENAME, self.object.name), '\n')
		for i, v in enumerate(self.verts):
			xml.translate("X%d" % i, self.cursor, v)


class rotation(mode):
	def test(self):
		needsObjects(self.objects, 1)
		self.object = self.objects[0]
		self.verts = selectedVertices(self.object)
		if len(self.verts) != 2:
			raise Error("you have to select two vertices that define the rotation axis!")

	def execute(self, xml):
		xml.comment('[%s] rotate "%s"' % (BASENAME, self.object.name), '\n')
		if self.cursor == ORIGIN:
			Blender.Draw.PupMenu("The cursor is still at origin!%t|"\
					"But nevertheless, I pretend it is the rotation center.")
		xml.rotate(self.object.name, self.cursor, self.verts[0], self.verts[1])


class levelOfDetail(mode):
	def test(self):
		needsObjects(self.objects, -1)

	def execute(self, xml):
		xml.comment('[%s] level of detail' % BASENAME, '\n')
		xml.range(self.objects)


class interpolation(mode):
	def test(self):
		needsObjects(self.objects, -2)

	def execute(self, xml):
		print
		for i, o in enumerate(self.objects):
			print "%d: %s" % (i, o.name)

		raise Error("this mode doesn't do anything useful yet")


class texturedLights(mode):
	def test(self):
		needsObjects(self.objects, 1)
		self.object = self.objects[0]
		self.verts = selectedVertices(self.object)
		if not len(self.verts):
			raise Error("there are no vertices to put lights at")
		self.lightname = self.object.name + "X"

		checkName(self.lightname)
		for i, v in enumerate(self.verts):
			checkName(self.lightname + serial(i, len(self.verts)))

	def execute(self, xml):
		lightname = self.lightname
		verts = self.verts
		object = self.object

		lightmesh = createLightMesh(lightname)

		lights = []
		for i, v in enumerate(verts):
			lights.append(createLight(lightmesh, lightname + serial(i, len(verts))))

		parent = object.getParent()
		if parent != None:
			parent.makeParent(lights)

		for l in lights:
			l.Layer = object.Layer

		xml.lightrange()
		xml.write("\t<%sparams>\n" % lightname)
		xml.write("\t\t<light-near>%s</light-near>\n" % "0.4")
		xml.write("\t\t<light-med>%s</light-med>\n" % "0.8")
		xml.write("\t\t<light-far>%s</light-far>\n" % "10")
		xml.write("\t</%sparams>\n\n" % lightname)

		if len(lights) == 1:
			xml.sunselect(lightname)
			xml.alphatest(lightname)
		else:
			xml.group(lightname + "Group", lights)
			xml.sunselect(lightname + "Group")
			xml.alphatest(lightname + "Group")

		for i, l in enumerate(lights):
			xml.translate(l.name, ORIGIN, verts[i])

		for l in lights:
			xml.billboard(l.name)

		for l in lights:
			xml.distscale(l.name, lightname)

		Blender.Redraw(-1)


execute = {
	'LIGHTS'   : texturedLights,
	'TRANS0'   : translationFromOrigin,
	'TRANSC'   : translationFromCursor,
	'ROTATE'   : rotation,
	'RANGE'    : levelOfDetail,
	'INTERPOL' : interpolation
}



# main() ==========================================================================================


def dofile(filename):
	try:
		xml = XMLExporter(filename)
		execute[MODE](xml)

	except Error, e:
		xml.comment("ERROR: " + e.args[0], '\n')
		raise Error(e.args[0])

	except IOError, (errno, strerror):
		raise Error(strerror)


def main():
	try:
		global FILENAME, INDENT
		reg = Blender.Registry.GetKey(REG_KEY, True)
		if reg:
			PATHNAME = reg['PATHNAME'] or ""
			INDENT = reg['INDENT'] or 0
		else:
			PATHNAME = ""
			INDENT = 0

		if PATHNAME:
			FILENAME = Blender.sys.join(PATHNAME, FILENAME)

		print 'writing to "' + FILENAME + '"'
		dofile(FILENAME)

	except Error, e:
		print "ERROR: " + e.args[0]
		Blender.Draw.PupMenu("ERROR%t|" + e.args[0])



if MODE:
	main()


