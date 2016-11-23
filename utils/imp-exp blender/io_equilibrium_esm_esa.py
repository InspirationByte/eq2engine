# ##### BEGIN GPL LICENSE BLOCK #####
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# ##### END GPL LICENSE BLOCK #####

bl_info = {
	"name": "Equilibrium Engine ESM/ESA",
	"author": "Shurumov Ilya",
	"version": (0, 5, 0),
	"blender": (2, 68, 4),
	"category": "Import-Export",
	"location": "File > Import/Export, Scene properties, Armature properties",
	"wiki_url": "http://code.google.com/p/blender-ESM/",
	"tracker_url": "http://code.google.com/p/blender-ESM/issues/list",
	"description": "Importer and exporter for Equilibrium Engine by Damage Byte. Supports ESM/ESA. Thanks for Blender ESM tools."}

import math, os, time, bpy, bmesh, random, mathutils, re, struct, subprocess, io
from bpy import ops
from bpy.props import *
from struct import unpack,calcsize
from mathutils import *
from math import *

intsize = calcsize("i")
floatsize = calcsize("f")

rx90 = Matrix.Rotation(radians(90),4,'X')
ry90 = Matrix.Rotation(radians(90),4,'Y')
rz90 = Matrix.Rotation(radians(90),4,'Z')
ryz90 = ry90 * rz90

rx90n = Matrix.Rotation(radians(-90),4,'X')
ry90n = Matrix.Rotation(radians(-90),4,'Y')
rz90n = Matrix.Rotation(radians(-90),4,'Z')

mat_BlenderToESMMirror = Matrix.Scale(-1, 4, Vector((0, 1, 0)))

val_BlenderToESMScale = 1 # TODO: variable in menus
sclX = Matrix.Scale(val_BlenderToESMScale, 4, Vector((1, 0, 0)))
sclY = Matrix.Scale(val_BlenderToESMScale, 4, Vector((0, 1, 0)))
sclZ = Matrix.Scale(val_BlenderToESMScale, 4, Vector((0, 0, 1)))

mat_BlenderToESMScale = sclX * sclY * sclZ

# ESM types
REF = 0x1 # $body, $model, $bodygroup->studio (if before a $body or $model)
REF_ADD = 0x2 # $bodygroup, $lod->replacemodel
PHYS = 0x3 # $collisionmesh, $collisionjoints
ANIM = 0x4 # $sequence, $animation
ANIM_SOLO = 0x5 # for importing animations to scenes without an existing armature
FLEX = 0x6 # $model ESX

mesh_compatible = [ 'MESH', 'TEXT', 'FONT', 'SURFACE', 'META', 'CURVE' ]
exportable_types = mesh_compatible[:]
exportable_types.append('ARMATURE')
shape_types = ['MESH' , 'SURFACE']


# I hate Python's var redefinition habits
class ESM_info:
	def __init__(self):
		self.a = None # Armature object
		self.amod = None # Original armature modifier
		self.m = None # Mesh datablock
		self.shapes = None
		self.g = None # Group being exported
		self.file = None
		self.jobName = None
		self.jobType = None
		self.startTime = 0
		self.uiTime = 0
		self.started_in_editmode = None
		self.append = False
		self.in_block_comment = False
		self.upAxis = 'Y'
		self.upAxisMat = 1 # vec * 1 == vec
		self.truncMaterialNames = []
		self.rotMode = 'EULER' # for creating keyframes during import
		self.materials_used = set() # printed to the console for users' benefit
		self.restPoseMatrices = []

		self.attachments = []
		self.meshes = []
		self.parent_chain = []

		self.frameData = []

		self.bakeInfo = []

		# boneIDs contains the ID-to-name mapping of *this* ESM's bones.
		# - Key: integer ID
		# - Value: bone name (storing object itself is not safe)
		self.boneIDs = {}
		self.boneNameToID = {} # for convenience during export
		self.phantomParentIDs = {} # for bones in animation ESMs but not the ref skeleton


# error reporting
class logger:
	def __init__(self):
		self.warnings = []
		self.errors = []
		self.startTime = time.time()

	def warning(self, *string):
		message = " ".join(str(s) for s in string)
		printColour(STD_YELLOW," WARNING:",message)
		self.warnings.append(message)

	def error(self, *string):
		message = " ".join(str(s) for s in string)
		printColour(STD_RED," ERROR:",message)
		self.errors.append(message)

	def errorReport(self, jobName, output, caller, numOut):
		message = "{} {}{} {}".format(numOut,output,"s" if numOut != 1 else "",jobName)
		if numOut:
			message += " in {} seconds".format( round( time.time() - self.startTime, 1 ) )

		if len(self.errors) or len(self.warnings):
			message += " with {} errors and {} warnings:".format(len(self.errors),len(self.warnings))

			# like it or not, Blender automatically prints operator reports to the console these days
			'''print(message)
			stdOutColour(STD_RED)
			for msg in self.errors:
				print("  " + msg)
			stdOutColour(STD_YELLOW)
			for msg in self.warnings:
				print("  " + msg)
			stdOutReset()'''

			for err in self.errors:
				message += "\nERROR: " + err
			for warn in self.warnings:
				message += "\nWARNING: " + warn
			caller.report({'ERROR'},message)
		else:
			caller.report({'INFO'},message)
			print(message)


log = None # Initialize this so it is easier for ESM_test_suite to access

##################################
#        Shared utilities        #
##################################

def benchReset():
	global benchLast
	global benchStart
	benchStart = benchLast = time.time()
benchReset()
def bench(label):
	global benchLast
	now = time.time()
	print("{}: {:.4f}".format(label,now-benchLast))
	benchLast = now
def benchTotal():
	global benchStart
	return time.time() - benchStart

def ValidateBlenderVersion(op):
	return True # TODO: make better

def getFileExt(ESM_type = None):
	if ESM_type in [REF,REF_ADD,PHYS]:
		return ".esm"
	elif ESM_type in [ANIM,ANIM_SOLO]:
		return ".esa"
	elif ESM_type in [FLEX]:
		return ".esx"
	else:
		return "." + bpy.context.scene.ESM_format.lower()

def getFilename(filepath):
	return filepath.split('\\')[-1].split('/')[-1].rsplit(".")[0]
def getFileDir(filepath):
	return filepath.rstrip(filepath.split('\\')[-1].split('/')[-1])

def isWild(in_str):
	wcards = [ "*", "?", "[", "]" ]
	for char in wcards:
		if in_str.find(char) != -1: return True

# rounds to 6 decimal places, converts between "1e-5" and "0.000001", outputs str
def getESMFloat(fval):
	val = "{:.6f}".format(float(fval))
	return val

# joins up "quoted values" that would otherwise be delimited, removes comments
def parseQuoteBlockedLine(line,lower=True):
	if len(line) == 0:
		return ["\n"]

	words = []
	last_word_start = 0
	in_quote = in_whitespace = False

	# The last char of the last line in the file was missed
	if line[-1] != "\n":
		print(line)
		line += "\n"

	for i in range(len(line)):
		char = line[i]
		nchar = pchar = None
		if i < len(line)-1:
			nchar = line[i+1]
		if i > 0:
			pchar = line[i-1]

		# line comment - precedence over block comment
		if (char == "/" and nchar == "/") or char in ['#',';']:
			if i > 0:
				i = i-1 # last word will be caught after the loop
			break # nothing more this line

		#block comment
		global ESM_manager
		if ESM_manager.in_block_comment:
			if char == "/" and pchar == "*": # done backwards so we don't have to skip two chars
				ESM_manager.in_block_comment = False
			continue
		elif char == "/" and nchar == "*": # note: nchar, not pchar
			ESM_manager.in_block_comment = True
			continue

		# quote block
		if char == "\"" and not pchar == "\\": # quotes can be escaped
			in_quote = (in_quote == False)
		if not in_quote:
			if char in [" ","\t"]:
				cur_word = line[last_word_start:i].strip("\"") # characters between last whitespace and here
				if len(cur_word) > 0:
					if (lower and os.name == 'nt') or cur_word[0] == "$":
						cur_word = cur_word.lower()
					words.append(cur_word)
				last_word_start = i+1 # we are in whitespace, first new char is the next one

	# catch last word and any '{'s crashing into it
	needBracket = False
	cur_word = line[last_word_start:i]
	if cur_word.endswith("{"):
		needBracket = True

	cur_word = cur_word.strip("\"{")
	if len(cur_word) > 0:
		words.append(cur_word)

	if needBracket:
		words.append("{")

	if line.endswith("\\\\\n") and (len(words) == 0 or words[-1] != "\\\\"):
		words.append("\\\\") # macro continuation beats everything

	return words

def appendExt(path,ext):
	if not path.lower().endswith("." + ext) and not path.lower().endswith(".dmx"):
		path += "." + ext
	return path

def printTimeMessage(start_time,name,job,type="ESM"):
	elapsedtime = int(time.time() - start_time)
	if elapsedtime == 1:
		elapsedtime = "1 second"
	elif elapsedtime > 1:
		elapsedtime = str(elapsedtime) + " seconds"
	else:
		elapsedtime = "under 1 second"

	print(type,name,"{}ed successfully in".format(job),elapsedtime,"\n")

def PrintVer(in_seq,sep="."):
		rlist = list(in_seq[:])
		rlist.reverse()
		out = ""
		for val in rlist:
			if int(val) == 0 and not len(out):
				continue
			out = "{}{}{}".format(str(val),sep if sep else "",out) # NB last value!
		if out.count(sep) == 1:
			out += "0" # 1.0 instead of 1
		return out.rstrip(sep)

try:
	import ctypes
	kernel32 = ctypes.windll.kernel32
	STD_RED = 0x04
	STD_YELLOW = 0x02|0x04
	STD_WHITE = 0x01|0x02|0x04
	def stdOutColour(colour):
		kernel32.SetConsoleTextAttribute(kernel32.GetStdHandle(-11),colour|0x08)
	def stdOutReset():
		kernel32.SetConsoleTextAttribute(kernel32.GetStdHandle(-11),STD_WHITE)
	def printColour(colour,*string):
		stdOutColour(colour)
		print(*string)
		stdOutReset()
except:
	STD_RED = STD_YELLOW = STD_WHITE = None
	def stdOutColour(colour):
		pass
	def stdOutReset():
		pass
	def printColour(colour,*string):
		print(*string)
		
def getUpAxisMat(axis):
	if axis.upper() == 'X':
		return Matrix.Rotation(pi/2,4,'Y')
	if axis.upper() == 'Y':
		return Matrix.Rotation(pi/2,4,'X')
	if axis.upper() == 'Z':
		return Matrix.Rotation(0,4,'Z')
	else:
		raise AttributeError("getUpAxisMat got invalid axis argument '{}'".format(axis))

def VecXMat(vec, mat):
	return mat.inverted() * vec

axes = (('X','X',''),('Y','Y',''),('Z','Z',''))

def MakeObjectIcon(object,prefix=None,suffix=None):
	if not (prefix or suffix):
		raise TypeError("A prefix or suffix is required")

	if object.type == 'TEXT':
		type = 'FONT'
	else:
		type = object.type

	out = ""
	if prefix:
		out += prefix
	out += type
	if suffix:
		out += suffix
	return out

def getObExportName(ob):
	if ob.get('ESM_name'):
		return ob['ESM_name']
	else:
		return ob.name

def removeObject(obj):
	d = obj.data
	type = obj.type

	if type == "ARMATURE":
		for child in obj.children:
			if child.type == 'EMPTY':
				removeObject(child)

	bpy.context.scene.objects.unlink(obj)
	if obj.users == 0:
		if type == 'ARMATURE' and obj.animation_data:
			obj.animation_data.action = None # avoid horrible Blender bug that leads to actions being deleted

		bpy.data.objects.remove(obj)
		if d and d.users == 0:
			if type == 'MESH':
				bpy.data.meshes.remove(d)
			if type == 'ARMATURE':
				bpy.data.armatures.remove(d)

	return None if d else type

def hasShapes(ob,groupIndex = -1):
	def _test(t_ob):
		return t_ob.type in shape_types and t_ob.data.shape_keys and len(t_ob.data.shape_keys.key_blocks) > 1

	if groupIndex != -1:
		for g_ob in ob.users_group[groupIndex].objects:
			if _test(g_ob): return True
		return False
	else:
		return _test(ob)

def getDirSep():
	if os.name is 'nt':
		return "\\"
	else:
		return "/"

def getBoneByESMName(bones,name):
	if bones.get(name):
		return bones.get(name)
	for bone in bones:
		if bone.get("ESM_name") == name:
			return bone

def shouldExportGroup(group):
	return group.ESM_export and not group.ESM_mute
	

########################
#        Import        #
########################

# Identifies what type of ESM this is. Cannot tell between reference/lod/collision meshes!
def scanESM():
	for line in ESM.file:
		if line == "triangles\n":
			ESM.jobType = REF
			print("- This is a mesh")
			break
		if line == "vertexanimation\n":
			print("- This is a flex animation library")
			ESM.jobType = FLEX
			break

	# Finished the file

	if ESM.jobType == None:
		print("- This is a skeltal animation or pose") # No triangles, no flex - must be animation
		if ESM.append:
			for object in bpy.context.scene.objects:
				if object.type == 'ARMATURE':
					ESM.jobType = ANIM
		if ESM.jobType == None: # support importing animations on their own
			ESM.jobType = ANIM_SOLO

	ESM.file.seek(0,0) # rewind to start of file

def uniqueName(name, nameList, limit):
	if name not in nameList and len(name) <= limit:
		return name
	name_orig = name[:limit-3]
	i = 1
	name = '%s_%.2d' % (name_orig, i)
	while name in nameList:
		i += 1
		name = '%s_%.2d' % (name_orig, i)
	return name

# Runs instead of readBones if an armature already exists, testing the current ESM's nodes block against it.
def validateBones(target):
	missing = 0
	validated = 0
	for line in ESM.file:
		if line == "end\n":
			break
	
		values = parseQuoteBlockedLine(line,lower=False)
		values[0] = int(values[0])
		values[2] = int(values[2])

		targetBone = target.data.bones.get(values[1]) # names, not IDs, are the key
		if not targetBone:
			for bone in target.data.bones:
				if bone.get("ESM_name") == values[1]:
					targetBone = bone
		
		if targetBone:
			validated += 1
		else:
			missing += 1
			parentName = targetBone.parent.name if targetBone and targetBone.parent else ""
			if ESM.boneIDs.get(values[2]) != parentName:
				ESM.phantomParentIDs[values[0]] = values[2]		

		ESM.boneIDs[values[0]] = targetBone.name if targetBone else values[1]
	
	if ESM.a != target:
		removeObject(ESM.a)
		ESM.a = target

	print("- Validated {} bones against armature \"{}\"{}".format(validated, ESM.a.name, " (could not find {})".format(missing) if missing > 0 else ""))

# nodes
def readNodes():
	if ESM.append and findArmature():
		if ESM.jobType == REF:
			ESM.jobType = REF_ADD
		validateBones(ESM.a)
		return

	# Got this far? Then this is a fresh import which needs a new armature.
	ESM.a = createArmature(ESM_manager.jobName)
	ESM.a.data.ESM_implicit_zero_bone = False # Too easy to break compatibility, plus the skeleton is probably set up already

	try:
		qc.a = ESM.a
	except NameError:
		pass

	boneParents = {}
	renamedBones = []

	bpy.ops.object.mode_set(mode='EDIT',toggle=False)

	# Read bone definitions from disc
	for line in ESM.file:
		if line == "end\n":
			break

		values = parseQuoteBlockedLine(line,lower=False)
	
		safeName = values[1].replace("ValveBiped.","")
		bone = ESM.a.data.edit_bones.new(safeName[:29]) # avoid Blender hang
		bone.tail = 0,5,0 # Blender removes zero-length bones

		if bone.name != values[1]:
			bone['ESM_name'] = values[1]
			renamedBones.append(bone)

		ESM.boneIDs[int(values[0])] = bone.name
		boneParents[bone.name] = int(values[2])

	# Apply parents now that all bones exist
	for bone in ESM.a.data.edit_bones:
		parentID = boneParents[bone.name]
		if parentID != -1:	
			bone.parent = ESM.a.data.edit_bones[ ESM.boneIDs[parentID] ]
	numRenamed = len(renamedBones)
	if numRenamed > 0:
		log.warning("{} bone name{} were changed".format(numRenamed,'s were' if numRenamed > 1 else ' was'))
		for bone in renamedBones:
			print('  {} -> {}'.format(bone['ESM_name'],bone.name))

	bpy.ops.object.mode_set(mode='OBJECT')
	print("- Imported {} new bones".format(len(ESM.a.data.bones)) )

	if len(ESM.a.data.bones) > 128:
		log.warning("Source only supports 128 bones!")

def findArmature():
	# Search the current scene for an existing armature - there can only be one skeleton in a Source model
	if bpy.context.active_object and bpy.context.active_object.type == 'ARMATURE':
		try:
			ESM.a = bpy.context.active_object
		except:
			return bpy.context.active_object
	else:
		def isArmIn(list):
			for ob in list:
				if ob.type == 'ARMATURE':
					ESM.a = ob
					return True

		isArmIn(bpy.context.selected_objects) # armature in the selection?

		if not ESM.a:
			for ob in bpy.context.selected_objects:
				if ob.type == 'MESH':
					ESM.a = ob.find_armature() # armature modifying a selected object?
					if ESM.a:
						break
		if not ESM.a:
			isArmIn(bpy.context.scene.objects) # armature in the scene at all?

	return ESM.a

def createArmature(armature_name):

	if bpy.context.active_object:
		bpy.ops.object.mode_set(mode='OBJECT',toggle=False)
	a = bpy.data.objects.new(armature_name,bpy.data.armatures.new(armature_name))
	a.show_x_ray = True
	a.data.draw_type = 'STICK'
	bpy.context.scene.objects.link(a)
	for i in bpy.context.selected_objects: i.select = False #deselect all objects
	a.select = True
	bpy.context.scene.objects.active = a

	ops.object.mode_set(mode='OBJECT')

	return a

def readFrames():
	# We only care about the pose data in some ESM types
	if ESM.jobType not in [ REF, ANIM, ANIM_SOLO ]:
		for line in ESM.file:
			if line == "end\n":
				return

	a = ESM.a
	bones = a.data.bones
	scn = bpy.context.scene
	bpy.context.scene.objects.active = ESM.a
	ops.object.mode_set(mode='POSE')

	act = None
	if ESM.jobType in [ANIM,ANIM_SOLO]:
		if not a.animation_data:
			a.animation_data_create()
		act = a.animation_data.action = bpy.data.actions.new(ESM.jobName)
		a.animation_data.action.use_fake_user = True

	num_frames = 0
	ops.object.mode_set(mode='POSE')	
	bone_mats = {}
	phantom_mats = {} # bones that aren't in the reference skeleton
	
	for bone in ESM.a.pose.bones:
		bone_mats[bone] = []
	
	for line in ESM.file:
		if line == "end\n":
			break
			
		values = line.split()

		if values[0] == "time": # frame number is a dummy value, all frames are equally spaced
			if num_frames > 0:
				if ESM.jobType == ANIM_SOLO and num_frames == 1:
					bpy.ops.pose.armature_apply()
				if ESM.jobType == REF:
					log.warning("Found animation in reference mesh \"{}\", ignoring!".format(ESM.jobName))
					for line in ESM.file: # skip to end of block
						if line == "end\n":
							break
			num_frames += 1
			
			# this way bones which are immobile some of the time are supported
			for mat_list in bone_mats.values():
				mat_list.append(None)
			for mat_list in phantom_mats.values():
				mat_list.append(None)
			continue
			
		# Read ESM data
		pos = Vector([float(values[1]), float(values[2]), float(values[3])])
		rot = Euler([float(values[4]), float(values[5]), float(values[6])])
		mat = Matrix.Translation(pos) * rot.to_matrix().to_4x4()
		
		# store the matrix
		values[0] = int(values[0])
		try:
			bone = ESM.a.pose.bones[ ESM.boneIDs[values[0]] ]
			if not bone.parent:
				mat = getUpAxisMat(ESM.upAxis) * mat
			bone_mats[bone][-1] = mat
		except KeyError:
			if not phantom_mats.get(values[0]):
				phantom_mats[values[0]] = [None] * num_frames
			if not ESM.phantomParentIDs.get(values[0]):
				mat = getUpAxisMat(ESM.upAxis) * mat
			phantom_mats[values[0]][-1] = mat
		
	# All frames read, apply phantom bones
	for ID, parentID in ESM.phantomParentIDs.items():		
		bone = getBoneByESMName(ESM.a.pose.bones, ESM.boneIDs.get(ID) )
		if not bone: continue		
		for frame in range(num_frames):
			cur_parent = parentID
			if bone_mats[bone][frame]: # is there a keyframe to modify?
				while phantom_mats.get(cur_parent): # parents are recursive
					phantom_frame = frame					
					while not phantom_mats[cur_parent][phantom_frame]: # rewind to the last value
						if phantom_frame == 0: continue # should never happen
						phantom_frame -= 1
					# Apply the phantom bone, then recurse
					bone_mats[bone][frame] = phantom_mats[cur_parent][phantom_frame] * bone_mats[bone][frame]					
					cur_parent = ESM.phantomParentIDs.get(cur_parent)
					
	applyFrames(bone_mats,num_frames,act)

def applyFrames(bone_mats,num_frames,action, dmx_key_sets = None): # this is called during DMX import too
	if ESM.jobType in [REF,ANIM_SOLO]:	
		# Apply the reference pose
		for bone in ESM.a.pose.bones:
			bone.matrix = bone_mats[bone][0]
		bpy.ops.pose.armature_apply()
	
		# Get sphere bone mesh
		bone_vis = bpy.data.objects.get("ESM_bone_vis")
		if not bone_vis:
			bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=3,size=2)
			bone_vis = bpy.context.active_object
			bone_vis.data.name = bone_vis.name = "ESM_bone_vis"
			bone_vis.use_fake_user = True
			bpy.context.scene.objects.unlink(bone_vis) # don't want the user deleting this
			bpy.context.scene.objects.active = ESM.a    
	
		# Calculate armature dimensions...Blender should be doing this!
		maxs = [0,0,0]
		mins = [0,0,0]
		for bone in ESM.a.data.bones:
			for i in range(3):
				maxs[i] = max(maxs[i],bone.head_local[i])
				mins[i] = min(mins[i],bone.head_local[i])
    
		ESM_manager.dimensions = dimensions = []
		for i in range(3):
			dimensions.append(maxs[i] - mins[i])
    
		length = (dimensions[0] + dimensions[1] + dimensions[2]) / 600 # very small indeed, but a custom bone is used for display
		if length < 0.001: # Blender silently deletes bones whose length is <= 0.000001
			length = 0.001 # could be only a single bone (static prop for example)
	
		# Apply spheres
		ops.object.mode_set(mode='EDIT')
		for bone in ESM.a.data.edit_bones:
			bone.tail = bone.head + (bone.tail - bone.head).normalized() * length # Resize loose bone tails based on armature size
			ESM.a.pose.bones[bone.name].custom_shape = bone_vis # apply bone shape
    
		ops.object.mode_set(mode='POSE')
	
	if ESM.jobType in [ANIM,ANIM_SOLO]:
		# Create an animation
		bpy.context.scene.frame_start = 0
		bpy.context.scene.frame_end = num_frames - 1		
		
		for bone in ESM.a.pose.bones:
			bone.rotation_mode = ESM.rotMode
			
		# Remove every point but the first unless there is motion
		still_bones = ESM.a.pose.bones[:]
		for bone in ESM.a.pose.bones:
			for i in range( 1, num_frames ): # skip first frame
				if not bone in still_bones: break
				if not bone_mats.get(bone) or not bone_mats[bone][i] or not bone_mats[bone][i-1]: continue
				diff = bone_mats[bone][i].inverted() * bone_mats[bone][i-1]
				if diff.to_translation().length > 0.00001 or abs(sum(diff.to_euler())) > 0.0001:
					still_bones.remove(bone)
		
		# Create keyframes
		def ApplyRecursive(bone):
			if bone_mats.get(bone):
				group = action.groups.new(name=bone.name)
				
				# Only set the one keyframe if this bone doesn't move
				if bone in still_bones:
					frame_range = range(1)
				else:
					frame_range = range(num_frames)
				
				# Generate curves
				curvesLoc = []
				curvesRot = []
				bone_string = "pose.bones[\"{}\"].".format(bone.name)
				for i in range(3):
					curve = action.fcurves.new(data_path=bone_string + "location",index=i)
					curve.group = group
					curvesLoc.append(curve)
				for i in range(3 if ESM.rotMode == 'XYZ' else 4):
					curve = action.fcurves.new(data_path=bone_string + "rotation_" + ("euler" if ESM.rotMode == 'XYZ' else "quaternion"),index=i)
					curve.group = group
					curvesRot.append(curve)					
				
				# Key each frame
				for f in frame_range:
					if not bone_mats[bone][f]:
						continue
					
					# Transform		
					if bone.parent:
						parentMat = bone.parent.matrix
						bone.matrix = parentMat * bone_mats[bone][f]
					else:
						bone.matrix = getUpAxisMat(ESM.upAxis) * bone_mats[bone][f]
						
					# Key location					
					if not dmx_key_sets or dmx_key_sets[bone][f]['p']:
						for i in range(3):
							curvesLoc[i].keyframe_points.add(1)
							curvesLoc[i].keyframe_points[-1].co = [f, bone.location[i]]
					
					# Key rotation
					if not dmx_key_sets or dmx_key_sets[bone][f]['o']:
						if ESM.rotMode == 'XYZ':							
							for i in range(3):
								curvesRot[i].keyframe_points.add(1)
								curvesRot[i].keyframe_points[-1].co = [f, bone.rotation_euler[i]]								
						else:
							for i in range(4):
								curvesRot[i].keyframe_points.add(1)
								curvesRot[i].keyframe_points[-1].co = [f, bone.rotation_quaternion[i]]

			# Recurse
			for child in bone.children:
				ApplyRecursive(child)
		
		# Start keying
		for bone in ESM.a.pose.bones:
			if not bone.parent:					
				ApplyRecursive(bone)
		
		# Smooth keyframe handles
		oldType = bpy.context.area.type
		bpy.context.area.type = 'GRAPH_EDITOR'
		try:
			bpy.ops.graph.handle_type(type='AUTO')
		except RuntimeError: # no frames?
			pass
		bpy.context.area.type = oldType # in Blender 2.59 this leaves context.region blank, making some future ops calls (e.g. view3d.view_all) fail!

	# clear any unkeyed poses
	for bone in ESM.a.pose.bones:
		bone.location.zero()
		if ESM.rotMode == 'XYZ': bone.rotation_euler.zero()
		else: bone.rotation_quaternion.identity()
	scn = bpy.context.scene
	
	if scn.frame_current == 1: # Blender starts on 1, Source starts on 0
		scn.frame_set(0)
	else:
		scn.frame_set(scn.frame_current)
	ops.object.mode_set(mode='OBJECT')
	
	print( "- Imported {} frames of animation".format(num_frames) )

def getMeshMaterial(in_name):
	if in_name == "": # buggered ESM
		in_name = "Material"
	md = ESM.m.data
	long_name = len(in_name) > 21
	mat = None
	for candidate in bpy.data.materials: # Do we have this material already?
		if candidate.name == in_name or candidate.get('ESM_name') == in_name:
			mat = candidate
	if mat:
		if md.materials.get(mat.name): # Look for it on this mesh
			for i in range(len(md.materials)):
				if md.materials[i].name == mat.name:
					mat_ind = i
					break
		else: # material exists, but not on this mesh
			md.materials.append(mat)
			mat_ind = len(md.materials) - 1
	else: # material does not exist
		print("- New material: %s" % in_name)
		mat = bpy.data.materials.new(in_name)
		md.materials.append(mat)
		# Give it a random colour
		randCol = []
		for i in range(3):
			randCol.append(random.uniform(.4,1))
		mat.diffuse_color = randCol
		if ESM.jobType != PHYS:
			mat.use_face_texture = True # in case the uninitated user wants a quick rendering
		else:
			ESM.m.draw_type = 'SOLID'
		mat_ind = len(md.materials) - 1
		if long_name: # Save the original name as a custom property.
			mat['ESM_name'] = in_name
			if not in_name in ESM.truncMaterialNames:
				ESM.truncMaterialNames.append(in_name)

	return mat, mat_ind

def setLayer():
	layers = [False] * len(ESM.m.layers)
	layers[ESM.layer] = bpy.context.scene.layers[ESM.layer] = True
	ESM.m.layers = layers
	if ESM.jobType == PHYS:
		ESM.a.layers[ESM.layer] = True
		for child in ESM.a.children:
			if child.type == 'EMPTY':
				child.layers[ESM.layer] = True

# triangles block
def readPolys():
	if ESM.jobType not in [ REF, REF_ADD, PHYS ]:
		return

	# Create a new mesh object, disable double-sided rendering, link it to the current scene
	if ESM.jobType == REF and not ESM.jobName.lower().find("reference") and not ESM.jobName.lower().endswith("ref"):
		meshName = ESM.jobName + " ref"
	else:
		meshName = ESM.jobName

	ESM.m = bpy.data.objects.new(meshName,bpy.data.meshes.new(meshName))	
	if ESM.m.name != ESM.jobName:
		ESM.m['ESM_name'] = ESM.jobName
	ESM.m.data.show_double_sided = False
	ESM.m.parent = ESM.a
	bpy.context.scene.objects.link(ESM.m)
	setLayer()
	if ESM.jobType == REF: # can only have flex on a ref mesh
		try:
			qc.ref_mesh = ESM.m # for VTA import
		except NameError:
			pass

	# Create weightmap groups
	for bone in ESM.a.data.bones.values():
		ESM.m.vertex_groups.new(bone.name)

	# Apply armature modifier
	modifier = ESM.m.modifiers.new(type="ARMATURE",name="Armature")
	modifier.object = ESM.a

	# Initialisation
	md = ESM.m.data
	lastWindowUpdate = time.time()
	# Vertex values
	cos = []
	norms = []
	weights = []
	# Face values
	uvs = []
	mats = []

	ESM.ESMNameToMatName = {}
	mat = None
	for mat in bpy.data.materials:
		ESM_name = getObExportName(mat)
		ESM.ESMNameToMatName[ESM_name] = mat.name

	bm = bmesh.new()
	bm.from_mesh(md)
	
	# *************************************************************************************************
	# There are two loops in this function: one for polygons which continues until the "end" keyword
	# and one for the vertices on each polygon that loops three times. We're entering the poly one now.	
	countPolys = 0
	badWeights = 0
	for line in ESM.file:
		line = line.rstrip("\n")

		if line == "end" or "":
			break

		mat, mat_ind = getMeshMaterial(line)
		mats.append(mat_ind)

		# ***************************************************************
		# Enter the vertex loop. This will run three times for each poly.
		vertexCount = 0
		faceVerts = []
		for line in ESM.file:
			values = line.split()
			vertexCount+= 1
			co = []

			# Read co-ordinates and normals
			for i in range(1,4): # 0 is the deprecated bone weight value
				co.append( float(values[i]) )
				#norm.append( float(values[i+3]) ) # Blender currenty ignores this data!
			
			faceVerts.append( bm.verts.new(co) )
			
			# Can't do these in the above for loop since there's only two
			uvs.append( ( float(values[7]), float(values[8]) ) )

			# Read weightmap data
			weights.append( [] ) # Blank array, needed in case there's only one weightlink
			if len(values) > 10 and values[9] != "0": # got weight links?
				for i in range(10, 10 + (int(values[9]) * 2), 2): # The range between the first and last weightlinks (each of which is *two* values)
					try:
						bone = ESM.a.data.bones[ ESM.boneIDs[int(values[i])] ]
						weights[-1].append( [ ESM.m.vertex_groups[bone.name], float(values[i+1]) ] )
					except KeyError:
						badWeights += 1
			else: # Fall back on the deprecated value at the start of the line
				try:
					bone = ESM.a.data.bones[ ESM.boneIDs[int(values[0])] ]				
					weights[-1].append( [ESM.m.vertex_groups[bone.name], 1.0] )
				except KeyError:
					badWeights += 1

			# Three verts? It's time for a new poly
			if vertexCount == 3:
				bm.faces.new(faceVerts)
				break

		# Back in polyland now, with three verts processed.
		countPolys+= 1

	# Warn about truncated material names
	length = len(ESM.truncMaterialNames)
	if length > 0:
		log.warning('%d material name%s truncated to 21 characters' % (length,'s were' if length > 1 else ' was'))
		print("The following material names were truncated to 21 characters:")
		for ESMName in ESM.truncMaterialNames:
			print('  ',ESMName)

	bm.to_mesh(md)
	bm.free()
	md.update()
	
	if countPolys:	
		md.polygons.foreach_set("material_index", mats)
		
		md.uv_textures.new()
		uv_data = md.uv_layers[0].data
		for i in range(len(uv_data)):
			uv_data[i].uv = uvs[md.loops[i].vertex_index]
		
		# Apply vertex groups
		for i in range(len(md.vertices)):
			for link in weights[i]:
				link[0].add( [i], link[1], 'REPLACE' )
        
		# Remove doubles and smooth...is there an easier way?		
		bpy.ops.object.select_all(action="DESELECT")
		ESM.m.select = True
		bpy.context.scene.objects.active = ESM.m
		ops.object.shade_smooth()		
		ops.object.mode_set(mode='EDIT')
		bpy.ops.mesh.select_all(action='SELECT')
		ops.mesh.remove_doubles()
		bpy.ops.mesh.select_all(action='DESELECT')
		ops.object.mode_set(mode='OBJECT')
						
		if ESM.jobType == PHYS:
			ESM.m.show_wire = True

		if ESM_manager.upAxis == 'Y':
			md.transform(rx90)
			md.update()

		if badWeights:
			log.warning("{} vertices weighted to invalid bones on {}".format(badWeights,ESM.jobName))
		print("- Imported %i polys" % countPolys)

# vertexanimation block
def readShapes():
	if ESM.jobType is not FLEX:
		return

	if not ESM.m:
		try:
			ESM.m = qc.ref_mesh
		except NameError:
			ESM.m = bpy.context.active_object # user selection
			
	if not ESM.m:
		log.error("Could not import shape keys: no target mesh found") # FIXME: this could actually be supported
		return
	
	ESM.m.show_only_shape_key = True # easier to view each shape, less confusion when several are active at once
	
	co_map = {}
	mesh_cos = []
	for vert in ESM.m.data.vertices:
		mesh_cos.append(vert.co)
	
	making_base_shape = True
	bad_vta_verts = num_shapes = 0
	md = ESM.m.data

	for line in ESM.file:
		line = line.rstrip("\n")
		if line == "end" or "":
			break
		values = line.split()

		if values[0] == "time":
			if len(co_map):
				making_base_shape = False
				if bad_vta_verts > 0:
					log.warning(bad_vta_verts,"VTA vertices were not matched to a mesh vertex!")

			if making_base_shape:
				ESM.m.shape_key_add("Basis")
			else:
				ESM.m.shape_key_add("Unnamed")
				num_shapes += 1

			continue # to the first vertex of the new shape

		cur_id = int(values[0])
		vta_co = Vector([ float(values[1]), float(values[2]), float(values[3]) ])

		if making_base_shape: # create VTA vert ID -> mesh vert ID dictionary
			try:
				co_map[cur_id] = mesh_cos.index(vta_co)
			except ValueError:
				bad_vta_verts += 1
		else: # write to the shapekey
			try:
				md.shape_keys.key_blocks[-1].data[ co_map[cur_id] ].co = vta_co
			except KeyError:
				pass

	print("- Imported",num_shapes,"flex shapes")

def initESM(filepath,ESM_type,append,upAxis,rotMode,from_qc,target_layer):
	global ESM
	ESM	= ESM_info()
	ESM.jobName = getFilename(filepath)
	ESM.jobType = ESM_type
	ESM.append = append
	ESM.startTime = time.time()
	ESM.layer = target_layer
	ESM.rotMode = rotMode
	if upAxis:
		ESM.upAxis = upAxis
		ESM.upAxisMat = getUpAxisMat(upAxis)
	ESM.uiTime = 0
	if not from_qc:
		global ESM_manager
		ESM_manager = ESM

	return ESM

# Parses an ESM file
def readESM( context, filepath, upAxis, rotMode, newscene = False, ESM_type = None, append = True, from_qc = False,target_layer = 0):
	global ESM
	initESM(filepath,ESM_type,append,upAxis,rotMode,from_qc,target_layer)

	try:
		ESM.file = file = open(filepath, 'r')
	except IOError as err: # TODO: work out why errors are swallowed if I don't do this!
		message = "Could not open ESM file \"{}\": {}".format(ESM.jobName,err)
		log.error(message)
		return 0

	if newscene:
		bpy.context.screen.scene = bpy.data.scenes.new(ESM.jobName) # BLENDER BUG: this currently doesn't update bpy.context.scene
	elif bpy.context.scene.name == "Scene":
		bpy.context.scene.name = ESM.jobName

	print("\nESM IMPORTER: now working on",ESM.jobName)
	if file.readline() != "version 1\n":
		log.warning ("Unrecognised/invalid ESM file. Import will proceed, but may fail!")

	if ESM.jobType == None:
		scanESM() # What are we dealing with?

	for line in file:
		if line == "nodes\n": readNodes()
		if line == "skeleton\n": readFrames()
		if line == "triangles\n": readPolys()
		if line == "vertexanimation\n": readShapes()

	file.close()
	bpy.ops.object.select_all(action='DESELECT')
	ESM.a.select = True
	'''
	if ESM.m and ESM.upAxisMat and ESM.upAxisMat != 1:
		ESM.m.rotation_euler = ESM.upAxisMat.to_euler()
		ESM.m.select = True
		bpy.context.scene.update()
		bpy.ops.object.transform_apply(rotation=True)
	'''
	printTimeMessage(ESM.startTime,ESM.jobName,"import")

	return 1

class ESMImporter(bpy.types.Operator):
	bl_idname = "import_scene.esm"
	bl_label = "Import ESM/ESA"
	bl_description = "Imports uncompiled Equilibrium model data"
	bl_options = {'UNDO'}

	# Properties used by the file browser
	filepath = StringProperty(name="File path", description="File filepath used for importing the ESM/ESA file", maxlen=1024, default="")
	filter_folder = BoolProperty(name="Filter folders", description="", default=True, options={'HIDDEN'})
	filter_glob = StringProperty(default="*.ESM;*.ESA;", options={'HIDDEN'})

	# Custom properties
	append = BoolProperty(name="Extend any existing model", description="Whether imports will latch onto an existing armature or create their own", default=True)
	doAnim = BoolProperty(name="Import animations (slow/bulky)", default=True)
	upAxis = EnumProperty(name="Up axis",items=axes,default='Z',description="Which axis represents 'up'")
	makeCamera = BoolProperty(name="Make camera at $origin",description="For use in viewmodel editing; if not set, an empty will be created instead",default=False)
	rotModes = ( ('XYZ', "Euler XYZ", ''), ('QUATERNION', "Quaternion", "") )
	rotMode = EnumProperty(name="Rotation mode",items=rotModes,default='XYZ',description="Keyframes will be inserted in this rotation mode")
	
	def execute(self, context):
		if not ValidateBlenderVersion(self):
			return {'CANCELLED'}

		global log
		log = logger()

		filepath_lc = self.properties.filepath.lower()
		if filepath_lc.endswith('.ESM'):
			self.countESMs = readESM(context, self.properties.filepath, self.properties.upAxis, self.properties.rotMode, append=self.properties.append)
		elif filepath_lc.endswith ('.ESA'):
			self.countESMs = readESM(context, self.properties.filepath, self.properties.upAxis, self.properties.rotMode, append=self.properties.append)
		else:
			if len(filepath_lc) == 0:
				self.report({'ERROR'},"No file selected")
			else:
				self.report({'ERROR'},"Format of {} not recognised".format(getFilename(self.properties.filepath)))
			return {'CANCELLED'}

		log.errorReport("imported","file",self,self.countESMs)
		if self.countESMs and ESM.m:
			ESM.m.select = True
			for area in context.screen.areas:
				if area.type == 'VIEW_3D':
					space = area.spaces.active
					# FIXME: small meshes offset from their origins won't extend things far enough
					xy = int(max(ESM.m.dimensions[0],ESM.m.dimensions[1]))
					space.grid_lines = max(space.grid_lines, xy)
					space.clip_end = max(space.clip_end, max(xy,int(ESM.m.dimensions[2])))
		if bpy.context.area.type == 'VIEW_3D' and bpy.context.region:
			bpy.ops.view3d.view_all()
		return {'FINISHED'}

	def invoke(self, context, event):
		if not ValidateBlenderVersion(self):
			return {'CANCELLED'}
		self.properties.upAxis = context.scene.ESM_up_axis
		bpy.context.window_manager.fileselect_add(self)
		return {'RUNNING_MODAL'}

########################
#        Export        #
########################

# nodes block
def writeBones(quiet=False):

	ESM.file.write("bones\n{\n")

	if not ESM.a:
		ESM.file.write("\"root\" 0 -1 0 0 0 0 0 0\n")
		ESM.file.write("}\n")
		if not quiet: print("- No skeleton to export")
		return
		
	curID = 0
	
	scene = bpy.context.scene
	prev_frame = scene.frame_current

	# remove any non-keyframed positions
	for posebone in ESM.a.pose.bones:
		posebone.matrix_basis.identity()
		
		# mute all pose constraints
		for con in posebone.constraints:
			con.mute = True

	bpy.context.scene.update()

	# Final bone positions are only available in pose mode
	bpy.ops.object.mode_set(mode='POSE')
	
	armatureWorld = ESM.a.matrix_world

	for posebone in ESM.a.pose.bones:
		if not posebone.bone.use_deform: continue

		parent = posebone.parent	
		# Skip over any non-deforming parents
		while parent:
			if parent.bone.use_deform:
				break
			parent = parent.parent

		# Get the bone's Matrix from the current pose
		localBoneMatrix = posebone.matrix

		if parent:
			parentMat = parent.matrix
			localBoneMatrix = parentMat.inverted() * localBoneMatrix
		else:
			localBoneMatrix = armatureWorld * localBoneMatrix


		ESM.restPoseMatrices.append(localBoneMatrix)

			
		# Get position
		pos = localBoneMatrix.to_translation()

		# Apply armature scale
		#if posebone.parent: # already applied to root bones
		#	scale = armatureWorld.to_scale()
		#	for j in range(3):
		#		pos[j] *= scale[j]

		# Get Rotation
		rot = localBoneMatrix.to_euler()
		
		# Construct the string
		pos_str = rot_str = ""
		for j in [0,1,2]:
			pos_str += " " + getESMFloat(pos[j]*val_BlenderToESMScale)
			rot_str += " " + getESMFloat(rot[j])
			
		bone_name = posebone.bone.get('ESM_name')
		if bone_name:
			comment = " # ESM_name override from \"{}\"".format(posebone.bone.name)
		else:
			bone_name = posebone.bone.name
			comment = ""	
		line = "\"" + bone_name + "\" "
		
		line += "{} ".format(curID)
		ESM.boneNameToID[posebone.bone.name] = curID
		curID += 1

		if parent:
			line += str(ESM.boneNameToID[parent.name])
		else:
			line += "-1"

		ESM.file.write(line + pos_str + rot_str + comment + "\n")
			

	ESM.file.write("}\n")
	
	scene.frame_set(prev_frame)

	if ESM.a.animation_data and ESM.a.animation_data.action:
		ESM.a.animation_data.action.user_clear() # otherwise the UI shows 100s of users!
		ESM.a.animation_data.action.use_fake_user = True
	bpy.ops.object.mode_set(mode='OBJECT')

	if not quiet: print("- Exported",len(ESM.a.data.bones),"bones")
	if len(ESM.a.data.bones) > 128:
		log.warning("Equilibrium Engine only supports 128 bones!")

# skeleton block
def writeFrames():
	if ESM.jobType in [REF,PHYS,FLEX]: # writeShapes() does its own skeleton block
		return

	ESM.file.write("frames\n{\n")

	if not ESM.a:
		ESM.file.write("0\n{\n")
		ESM.file.write("0 0 0 0 0 0 0\n")
		ESM.file.write("}\n")
		return

	scene = bpy.context.scene
	prev_frame = scene.frame_current

	# remove any non-keyframed positions
	for posebone in ESM.a.pose.bones:
		posebone.matrix_basis.identity()
		
		# mute all pose constraints
		# fixme: what if it's enabled?
		for con in posebone.constraints:
			con.mute = False

	bpy.context.scene.update()

	# Get the working frame range
	num_frames = 1
	if ESM.jobType == ANIM:
		action = ESM.a.animation_data.action
		start_frame, last_frame = action.frame_range
		num_frames = int(last_frame - start_frame) + 1 # add 1 due to the way range() counts
		scene.frame_set(start_frame)
			
		if 'fps' in dir(action):
			bpy.context.scene.render.fps = action.fps
			bpy.context.scene.render.fps_base = 1

	# Final bone positions are only available in pose mode
	bpy.ops.object.mode_set(mode='POSE')
	
	armatureWorld = ESM.a.matrix_world

	# Start writing out the animation
	for i in range(num_frames):
		ESM.file.write("{}\n".format(i)) # write time
		ESM.file.write("{\n")

		boneId = 0
		
		for posebone in ESM.a.pose.bones:
			if not posebone.bone.use_deform: continue
	
			parent = posebone.parent	
			# Skip over any non-deforming parents
			while parent:
				if parent.bone.use_deform:
					break
				parent = parent.parent

			# Get the bone's Matrix from the current pose
			localBoneMatrix = posebone.matrix
			
			if parent:
				parentMat = parent.matrix
				localBoneMatrix = parentMat.inverted() * localBoneMatrix
			else:
				localBoneMatrix = armatureWorld * localBoneMatrix
				
			# convert to local transformation
			localBoneMatrix = ESM.restPoseMatrices[boneId].inverted() * localBoneMatrix
				
			boneId += 1

			# Get position
			pos = localBoneMatrix.to_translation()

			# Apply armature scale
			#if posebone.parent: # already applied to root bones
			#	scale = armatureWorld.to_scale()
			#	for j in range(3):
			#		pos[j] *= scale[j]

			# Get Rotation
			rot = localBoneMatrix.to_euler()
			
			# HACK: this is a weird rotation fix and you should get rid of it
			rot[1] = -rot[1]
			rot[2] = -rot[2]

			# Construct the string
			pos_str = rot_str = ""
			for j in [0,1,2]:
				pos_str += " " + getESMFloat(pos[j]*val_BlenderToESMScale)
				rot_str += " " + getESMFloat(rot[j])
	
			# Write!
			ESM.file.write( str(ESM.boneNameToID[posebone.name]) + pos_str + rot_str + "\n" )

		# All bones processed, advance the frame
		scene.frame_set(scene.frame_current + 1)	
		ESM.file.write("}\n")

	ESM.file.write("}\n")
	scene.frame_set(prev_frame)

	if ESM.a.animation_data and ESM.a.animation_data.action:
		ESM.a.animation_data.action.user_clear() # otherwise the UI shows 100s of users!
		ESM.a.animation_data.action.use_fake_user = True
	bpy.ops.object.mode_set(mode='OBJECT')
	
	print("- Exported {} frames".format(num_frames))
	return

# triangles block
def writePolys(internal=False):

	if not internal:
		ESM.file.write("faces\n{\n")
		prev_frame = bpy.context.scene.frame_current
		have_cleared_pose = False

		if not bpy.context.scene.ESM_use_image_names:
			materials = []
			for bi in ESM.bakeInfo:
				if bi['baked'].type == 'MESH':
					for mat_slot in bi['baked'].material_slots:
						mat = mat_slot.material
						if mat and mat.get('ESM_name') and mat not in materials:
							ESM.file.write( "// Blender material \"{}\" has esm_name \"{}\"\n".format(mat.name,mat['ESM_name']) )
							materials.append(mat)

		for bi in ESM.bakeInfo:
			if bi['baked'].type == 'MESH':
				# write out each object in turn. Joining them would destroy unique armature modifier settings
				ESM.m = bi['baked']
				if bi.get('arm_mod') and bi.get('arm_mod').object:
					ESM.amod = bi['arm_mod']
				else:
					ESM.amod = None
				if len(ESM.m.data.polygons) == 0:
					log.error("Object {} has no faces, cannot export".format(ESM.jobName))
					continue

				if ESM.amod and not have_cleared_pose:
					# This is needed due to a Blender bug. Setting the armature to Rest mode doesn't actually
					# change the pose bones' data!
					bpy.context.scene.objects.active = ESM.amod.object
					bpy.ops.object.mode_set(mode='POSE')
					bpy.ops.pose.select_all()
					bpy.ops.pose.transforms_clear()
					have_cleared_pose = True
				bpy.ops.object.mode_set(mode='OBJECT')

				writePolys(internal=True)

		ESM.file.write("}\n")
		bpy.context.scene.frame_set(prev_frame)
		return

	# internal mode:

	md = ESM.m.data

	face_index = 0
	# bone parent?
	ob_weight_str = " 1 {} 1".format(ESM.boneNameToID[ESM.m['bp']]) if ESM.m.get('bp') else None

	uv_loop = md.uv_layers.active.data
	uv_tex = md.uv_textures.active.data
	
	bad_face_mats = 0

	curmat_name = None
	
	# need to run in back order
	for poly in md.polygons:
		mat_name = None
		
		if not bpy.context.scene.ESM_use_image_names and len(ESM.m.material_slots) > poly.material_index:
			mat = ESM.m.material_slots[poly.material_index].material
			if mat:
				mat_name = getObExportName(mat)
		if not mat_name and uv_tex:
			image = uv_tex[face_index].image
			if image:
				mat_name = getFilename(image.filepath) # not using data name as it can be truncated and custom props can't be used here
		if mat_name:
			ESM.materials_used.add(mat_name)
		else:
			mat_name = "no_material"
			bad_face_mats += 1
			
		if curmat_name != mat_name:
			ESM.file.write("mat \"" + mat_name + "\"\n")
			curmat_name = mat_name
		
		for idx in range(len(poly.vertices)):
			i = (len(poly.vertices) - idx) - 1
		
			# Vertex locations, normal directions
			loc = norms = ""
			v = md.vertices[poly.vertices[i]]
			
			pos = v.co

			norm = v.normal if poly.use_smooth else poly.normal
			
			norm = (getUpAxisMat(bpy.context.scene.ESM_up_axis).inverted() * mat_BlenderToESMMirror * Matrix.Translation(norm)).to_translation();

			for j in range(3):
				loc += " " + getESMFloat(pos[j] * val_BlenderToESMScale)
				norms += " " + getESMFloat(norm[j])

			# UVs
			uv = ""
			uv += " " + getESMFloat(uv_loop[poly.loop_start + i].uv[0])
			uv += " " + getESMFloat(1.0 - uv_loop[poly.loop_start + i].uv[1])

			# Weightmaps
			weights = []
			if ob_weight_str:
				weight_string = ob_weight_str
			elif ESM.amod:		
				am_vertex_group_weight = 0

				if ESM.amod.use_vertex_groups:
					for j in range(len(v.groups)):
						group_index = v.groups[j].group
						if group_index < len(ESM.m.vertex_groups):
							# Vertex group might not exist on object if it's re-using a datablock
							group_name = ESM.m.vertex_groups[group_index].name
							group_weight = v.groups[j].weight
						else:
							continue

						if group_name == ESM.amod.vertex_group:
							am_vertex_group_weight = group_weight

						bone = ESM.amod.object.data.bones.get(group_name)
						if bone and bone.use_deform:
							weights.append([ESM.boneNameToID[bone.name], group_weight])

				if ESM.amod.use_bone_envelopes and not weights: # vertex groups completely override envelopes
					for pose_bone in ESM.amod.object.pose.bones:
						if not pose_bone.bone.use_deform:
							continue
						weight = pose_bone.bone.envelope_weight * pose_bone.evaluate_envelope( v.co * ESM.m.matrix_world )
						if weight:
							weights.append([ESM.boneNameToID[pose_bone.name], weight])
					
			total_weight = 0
			if ESM.amod:
				for link in weights:
					total_weight += link[1]

			if not ESM.amod or total_weight == 0:
				if not ob_weight_str: weight_string = " 0"
				# In Source, unlike in Blender, verts HAVE to be attached to bones. This means that if you have only one bone,
				# all verts will be 100% attached to it. To transform only some verts you need a second bone that stays put.
			else:
				# Shares out unused weight between extant bones, like Blender does, otherwise egfCA puts it onto the root		
				for link in weights:
					link[1] *= 1/total_weight # This also handles weights totalling more than 100%

				weight_string = " " + str(len(weights))
				for link in weights: # one link on one vertex
					if ESM.amod.vertex_group: # strength modifier
						link[1] *= am_vertex_group_weight
						if ESM.amod.invert_vertex_group:
							link[1] = 1 - link[1]

					weight_string += " " + str(link[0]) + " " + getESMFloat(link[1])

			# Finally, write it all to file
			ESM.file.write("vtx " + loc + norms + uv + weight_string + "\n")

		face_index += 1

	if bad_face_mats:
		log.warning("{} faces on {} did not have a texture{} assigned".format(bad_face_mats,ESM.jobName,"" if bpy.context.scene.ESM_use_image_names else " or material"))
	print("- Exported",face_index,"polys")
	return

# vertexanimation block
def writeShapes(cur_shape = 0):
	num_verts = 0

	def _writeTime(time, shape = None):
		ESM.file.write( "key {} {}\n".format(time, shape['shape_name'] if shape else "basis") )

	# ESXs are always separate files. The nodes block is handled by the normal function, but skeleton is done here to afford a nice little hack
	#ESM.file.write("skeleton\n")
	#_writeTime(0) # shared basis
	#num_shapes = 1
	#shapes_written = []
	#for bi in ESM.bakeInfo:
	#	cur_shapes = len(bi['shapes']) - 1 # don't include basis shapes
	#	for i in range(cur_shapes):
	#		shape = bi['shapes'][i+1] # i+1 to skip basis
	#		if shape['shape_name'] in shapes_written: continue
	#		_writeTime(num_shapes + i, shape )
	#		shapes_written.append(shape['shape_name'])
	#	num_shapes += cur_shapes
	#ESM.file.write("end\n")

	ref_name = ""
	for bi in ESM.bakeInfo:
		for shape_candidate in bi['shapes']:
			if shape_candidate['shape_name'] == bi['shapes'][0]['shape_name']:
				ref_name += shape_candidate['src_name'] + ", "
	ref_name = ref_name[:-2]
	
	ESM.file.write("reference {}.esm\n".format(ref_name))
	ESM.file.write("verts\n")
	ESM.file.write("{\n")
	def _writeVerts(shape, ESM_vert_id = 0):
		start_time = time.time()
		num_verts = 0
		num_bad_verts = 0
		for poly in ESM.m.data.polygons:
			for vert in poly.vertices:
				shape_vert = shape.data.vertices[vert]
				mesh_vert = ESM.m.data.vertices[vert]
				if cur_shape != 0:
					diff_vec = shape_vert.co - mesh_vert.co
					bad_vert = False
					for ordinate in diff_vec:
						if not bad_vert and ordinate > 8:
							num_bad_verts += 1
							bad_vert = True

				if cur_shape == 0 or (shape_vert.co != mesh_vert.co or shape_vert.normal != mesh_vert.normal):
					cos = norms = ""
					
					norm = shape_vert.normal
					norm = (getUpAxisMat(bpy.context.scene.ESM_up_axis).inverted() * mat_BlenderToESMMirror * Matrix.Translation(shape_vert.normal)).to_translation();
					
					for i in range(3):
						cos += " " + getESMFloat(shape_vert.co[i])
						norms += " " + getESMFloat(norm[i])
					ESM.file.write( "vtx " + str(ESM_vert_id) + cos + norms + "\n")
					num_verts += 1
			
				ESM_vert_id +=1
		if num_bad_verts:
			log.error("Shape \"{}\" has {} vertex movements that exceed eight units!".format(shape['shape_name'],num_bad_verts))
		return num_verts

	cur_shape = 0
	vert_offset = 0
	total_verts = 0

	# Basis
	_writeTime(0)
	for bi in ESM.bakeInfo:
		ESM.m = bi['shapes'][0]
		total_verts += _writeVerts(bi['shapes'][0], total_verts) # write out every vert in the group
	cur_shape += 1

	# Everything else
	shapes_written = []
	for bi in ESM.bakeInfo:
		ESM.shapes = bi['shapes']
		ESM.m = bi['shapes'][0]
		for shape in ESM.shapes:
			if shape == bi['shapes'][0] or shape['shape_name'] in shapes_written:
				continue # don't write basis shapes, don't re-write shared shapes
			_writeTime(cur_shape,shape)
			cur_shape += 1
			vert_offset = 0
			for bi in ESM.bakeInfo:
				for shape_candidate in bi['shapes']:
					if shape_candidate['shape_name'] == shape['shape_name']:
						last_verts = _writeVerts(shape_candidate,vert_offset)
						total_verts += last_verts
				for face in bi['baked'].data.polygons:
					for vert in face.vertices:
						vert_offset += 1 # len(verts) doesn't give doubles
			shapes_written.append(shape['shape_name'])

	ESM.file.write("}\n")
	print("- Exported {} flex shapes ({} verts)".format(cur_shape,total_verts))
	return

# Creates a mesh with object transformations and modifiers applied
def bakeObj(in_object):
	bi = {}
	bi['src'] = in_object

	for object in bpy.context.selected_objects:
		object.select = False

	def _ApplyVisualTransform(obj):
		if obj.data.users > 1:
			obj.data = obj.data.copy()

		top_parent = cur_parent = obj
		while(cur_parent):
			if not cur_parent.parent:
				top_parent = cur_parent
			cur_parent = cur_parent.parent

		bpy.context.scene.objects.active = obj
		bpy.ops.object.parent_clear(type='CLEAR_KEEP_TRANSFORM')
		obj.location -= top_parent.location # undo location of topmost parent
		bpy.ops.object.transform_apply(location=True)

	# Indirection to support groups
	def _ObjectCopy(obj):
		solidify_fill_rim = False

		selection_backup = bpy.context.selected_objects

		if obj.type in shape_types and obj.data.shape_keys:
			shape_keys = obj.data.shape_keys.key_blocks
		else:
			shape_keys = None

		if ESM.jobType == FLEX:
			num_out = len(shape_keys)
			bi['shapes'] = []
	
			if num_out:
				# create an intermediate object without shapes
				obj_name = obj.name
				obj = obj.copy()
				obj.data = obj.data.copy()
				
				if not bi.get('baked'):
					bi['baked'] = obj
					
				bpy.context.scene.objects.link(obj)		
				bpy.context.scene.objects.active = obj
				bpy.ops.object.select_all(action="DESELECT")
				obj.select = True
		
				for shape in obj.data.shape_keys.key_blocks:
					bpy.ops.object.shape_key_remove('EXEC_SCREEN')
		else:
			num_out = 1

		for i in range(num_out):
			if shape_keys:
				cur_shape = shape_keys[i]
			if ESM.jobType == FLEX and cur_shape.mute:
				print("- Skipping muted shape \"{}\"".format(cur_shape.name))
				continue

			baked = obj.copy()
			if not bi.get('baked'):
				bi['baked'] = baked
				
			bpy.context.scene.objects.link(baked)
			bpy.context.scene.objects.active = baked
			bpy.ops.object.select_all(action="DESELECT")
			baked.select = True

			if shape_keys:
				baked.data = obj.data.copy()
		
				# transfer desired shape to copy
				if baked.type == 'MESH':			
					import array
					cos = array.array('f', [0] * len(baked.data.vertices) * 3 )
					cur_shape.data.foreach_get("co",cos)
					baked.data.vertices.foreach_set("co",cos)
				elif baked.type == 'SURFACE':
					# assuming shape keys only modify spline locations here...
					for i in range( len(baked.data.splines) ):
						for j in range( len(baked.data.splines[i].points) ):
							baked.data.splines[i].points[j].co[:3] = cur_shape.data[(i*2)+j].co # what is the 4th value?
				else:
					raise TypeError( "Shapes found on unsupported object type (\"{}\", {})".format(obj.name,obj.type) )
	
			if obj.type == 'ARMATURE':
				baked.data = baked.data.copy()
				for posebone in baked.pose.bones: posebone.matrix_basis.identity()

				bpy.ops.object.transform_apply(location=True,scale=True,rotation=True)
				baked.data.transform(getUpAxisMat(bpy.context.scene.ESM_up_axis).inverted() * mat_BlenderToESMMirror)
				
			elif obj.type in mesh_compatible:
				has_edge_split = False
				for mod in obj.modifiers:
					if mod.type == 'EDGE_SPLIT':
						has_edge_split = True
					if mod.type == 'SOLIDIFY' and not solidify_fill_rim:
						solidify_fill_rim = mod.use_rim

				if not has_edge_split and obj.type == 'MESH':
					edgesplit = baked.modifiers.new(name="ESM Edge Split",type='EDGE_SPLIT') # creates sharp edges
					edgesplit.use_edge_angle = False

				#baked.data = baked.to_mesh(bpy.context.scene, True, 'PREVIEW') # bake it!
				
				#if ESM.jobType == FLEX:
				#	baked.name = baked.data.name = baked['shape_name'] = cur_shape.name
				#	baked['src_name'] = obj_name
				#	bi['shapes'].append(baked)
				
				bpy.ops.object.convert(keep_original=True) # bake it!

				# did convert() make a new object?
				if bpy.context.active_object == baked: # no
					baked.data = baked.data.copy()
				else: # yes
					removeObject(baked)
					baked = bi['baked'] = bpy.context.active_object

				if ESM.jobType == FLEX:
					baked.name = baked.data.name = baked['shape_name'] = cur_shape.name
					baked['src_name'] = obj_name
					bi['shapes'].append(baked)
				else:
					baked.name = baked.data.name = "{} (baked)".format(obj.name)
			
				# Handle bone parents / armature modifiers, and warn of multiple associations
				baked['bp'] = ""
				envelope_described = False
				if obj.parent_bone and obj.parent_type == 'BONE':
					baked['bp'] = obj.parent_bone
					ESM.a = obj.parent
				for con in obj.constraints:
					if con.mute:
						continue
					if con.type in ['CHILD_OF','COPY_TRANSFORMS'] and con.target.type == 'ARMATURE' and con.subtarget:
						if baked['bp']:
							if not envelope_described:
								print(" - Enveloped to bone \"{}\"".format(baked['bp']))
							log.warning("Bone constraint \"{}\" found on \"{}\", which already has an envelope. Ignoring.".format(con.name,obj.name))
						else:
							baked['bp'] = con.subtarget
							ESM.a = con.target
				for mod in obj.modifiers:
					if mod.type == 'ARMATURE':
						if (ESM.a and mod.object != ESM.a) or baked['bp']:
							if not envelope_described:
								msg = " - Enveloped to {} \"{}\""
								if baked['bp']: print( msg.format("bone",baked['bp']) )
								else: print( msg.format("armature",ESM.a.name) )
							log.warning("Armature modifier \"{}\" found on \"{}\", which already has an envelope. Ignoring.".format(mod.name,obj.name))
						else:
							ESM.a = mod.object
							bi['arm_mod'] = mod
		
				# work on the vertices
				bpy.ops.object.mode_set(mode='EDIT')
				bpy.ops.mesh.reveal()
				
				if obj.matrix_world.is_negative:
					bpy.ops.mesh.flip_normals()

				bpy.ops.mesh.select_all(action='SELECT')
		
				bpy.ops.mesh.quads_convert_to_tris()
		
				# handle which sides of a curve should have polys
				if obj.type == 'CURVE':
					if obj.data.ESM_faces == 'RIGHT':
						bpy.ops.mesh.duplicate()
						bpy.ops.mesh.flip_normals()
					if not obj.data.ESM_faces == 'BOTH':
						bpy.ops.mesh.select_inverse()
						bpy.ops.mesh.delete()
					elif solidify_fill_rim:
						log.warning("Curve {} has the Solidify modifier with rim fill, but is still exporting polys on both sides.".format(obj.name))

				# project a UV map
				if len(baked.data.uv_textures) == 0 and ESM.jobType != FLEX:
					if len(baked.data.vertices) < 2000:
						bpy.ops.object.mode_set(mode='OBJECT')
						bpy.ops.object.select_all(action='DESELECT')
						baked.select = True
						bpy.ops.uv.smart_project()
					else:
						bpy.ops.object.mode_set(mode='EDIT')
						bpy.ops.mesh.select_all(action='SELECT')
						bpy.ops.uv.unwrap()
						
					bpy.ops.object.mode_set(mode='OBJECT')

					for object in selection_backup:
						object.select = True

			# Apply object transforms to the baked data
			bpy.ops.object.mode_set(mode='OBJECT')
			top_parent = cur_parent = baked
			while(cur_parent):
				if not cur_parent.parent:
					top_parent = cur_parent
				cur_parent = cur_parent.parent

			bpy.ops.object.parent_clear(type='CLEAR_KEEP_TRANSFORM')
			baked.location -= top_parent.location # undo location of topmost parent
			bpy.context.scene.update() # another rare case where this actually does something
				
			if baked.type == 'MESH': # don't apply to armatures (until/unless actions are baked too)
				bpy.ops.object.transform_apply(location=True,scale=True,rotation=True)
				baked.data.transform(getUpAxisMat(bpy.context.scene.ESM_up_axis).inverted() * mat_BlenderToESMMirror)
		if ESM.jobType == FLEX:
			removeObject(obj)
		return baked
		# END _ObjectCopy()

	if in_object.type == 'ARMATURE':
		_ObjectCopy(in_object)
		ESM.a = bi['baked']
	elif in_object.type in mesh_compatible:
		# hide all metaballs that we don't want
		metaballs = []
		for object in bpy.context.scene.objects:
			if (ESM.g or object != in_object) and object.type == 'META' and (not object.ESM_export or not (ESM.g and ESM.g in object.users_group)):
				element_states = []
				for i in range(len(object.data.elements)):
					element_states.append(object.data.elements[i].hide)
					object.data.elements[i].hide = True
				metaballs.append( dict( ob=object, states = element_states) )
		bpy.context.scene.update() # actually found a use for this!!

		if not ESM.g:
			_ObjectCopy(in_object)
		else:
			have_baked_metaballs = False
			validObs = getValidObs()
			for object in ESM.g.objects:
				if object.ESM_export and object in validObs and not (object.type == 'META' and have_baked_metaballs):
					if ESM.jobType == FLEX and not object.data.shape_keys:
						continue
					bi['baked'] = _ObjectCopy(object)
					ESM.bakeInfo.append(bi) # save to manager
					bi = dict(src=object)
					if not have_baked_metaballs: have_baked_metaballs = object.type == 'META'
			
		# restore metaball state
		for meta_state in metaballs:
			for i in range(len(meta_state['states'])):
				meta_state['ob'].data.elements[i].hide = meta_state['states'][i]

	if bi.get('baked') or (bi.get('shapes') and len(bi.get('shapes'))):
		ESM.bakeInfo.append(bi) # save to manager

def unBake():
	bpy.ops.object.mode_set(mode='OBJECT')
	for bi in ESM.bakeInfo:
		if bi.get('shapes'):
			for shape in bi['shapes']:
				type = removeObject(shape)
		else:
			type = removeObject(bi['baked'])

		if type == 'MESH':
			ESM.m = bi['src']
		elif type == 'ARMATURE':
			ESM.a = bi['src']

		del bi

# Creates an ESM file
def writeESM( context, object, groupIndex, filepath, ESM_type = None, quiet = False ):

	global ESM
	ESM	= ESM_info()
	ESM.jobType = ESM_type
	if groupIndex != -1:
		ESM.g = object.users_group[groupIndex]
	ESM.startTime = time.time()
	ESM.uiTime = 0

	def _workStartNotice():
		if not quiet:
			print( "\nESM EXPORTER: now working on {}{}".format(ESM.jobName," (shape keys)" if ESM.jobType == FLEX else "") )

	if object.type in mesh_compatible:
		# We don't want to bake any meshes with poses applied
		# NOTE: this won't change the posebone values, but it will remove deformations
		armatures = []
		for scene_object in bpy.context.scene.objects:
			if scene_object.type == 'ARMATURE' and scene_object.data.pose_position == 'POSE':
				scene_object.data.pose_position = 'REST'
				armatures.append(scene_object)

		if not ESM.jobType:
			ESM.jobType = REF
		if ESM.g:
			ESM.jobName = ESM.g.name
		else:
			ESM.jobName = getObExportName(object)
		ESM.m = object
		_workStartNotice()
		#ESM.a = ESM.m.find_armature() # Blender bug: only works on meshes
		bakeObj(ESM.m)

		# re-enable poses
		for object in armatures:
			object.data.pose_position = 'POSE'
		bpy.context.scene.update()
	elif object.type == 'ARMATURE':
		if not ESM.jobType:
			ESM.jobType = ANIM
		ESM.a = object
		ESM.jobName = getObExportName(object.animation_data.action)
		_workStartNotice()
	else:
		raise TypeError("PROGRAMMER ERROR: writeESM() has object not in",exportable_types)

	if ESM.a:
		if ESM.jobType != FLEX:
			bakeObj(ESM.a) # MUST be baked after the mesh		

	ESM.file = open(filepath, 'w')
	print("-",filepath)
	
	if ESM.jobType == ANIM:
		ESM.file.write("ESA1\n")
	elif ESM.jobType == FLEX:
		ESM.file.write("ESX1\n")
	else:
		ESM.file.write("ESM1\n")
		
	# these write empty blocks if no armature is found. Required!
	if ESM.jobType in [REF,PHYS,ANIM]:
		writeBones(quiet = ESM.jobType == FLEX)
		writeFrames()
	
	if ESM.m:
		if ESM.jobType in [REF,PHYS]:
			writePolys()
			print("- Exported {} materials".format(len(ESM.materials_used)))
			for mat in ESM.materials_used:
				print("   " + mat)
		elif ESM.jobType == FLEX:
			writeShapes()

	unBake()
	ESM.file.close()
	if not quiet: printTimeMessage(ESM.startTime,ESM.jobName,"export")

	return True

class ESM_MT_ExportChoice(bpy.types.Menu):
	bl_label = "ESM export mode"

	# returns an icon, a label, and the number of valid actions
	# supports single actions, NLA tracks, or nothing
	def getActionSingleTextIcon(self,context,ob = None):
		icon = "OUTLINER_DATA_ARMATURE"
		count = 0
		text = "No Actions or NLA"
		export_name = False
		if not ob:
			ob = context.active_object
			export_name = True # slight hack since having ob currently aligns with wanting a short name
		if ob:
			ad = ob.animation_data
			if ad:
				if ad.action:
					icon = "ACTION"
					count = 1
					if export_name:
						text = ob.ESM_subdir + (getDirSep() if ob.ESM_subdir else "") + ad.action.name + getFileExt(ANIM)
					else:
						text = ad.action.name
				elif ad.nla_tracks:
					nla_actions = []
					for track in ad.nla_tracks:
						if not track.mute:
							for strip in track.strips:
								if not strip.mute and strip.action not in nla_actions:
									nla_actions.append(strip.action)
					icon = "NLA"
					count = len(nla_actions)
					text = "NLA actions (" + str(count) + ")"

		return text,icon,count

	# returns the appropriate text for the filtered list of all action
	def getActionFilterText(self,context):
		ob = context.active_object
		if ob.ESM_action_filter:
			global cached_action_filter_list
			global cached_action_count
			if ob.ESM_action_filter != cached_action_filter_list:
				cached_action_filter_list = ob.ESM_action_filter
				cached_action_count = 0
				for action in bpy.data.actions:
					if action.name.lower().find(ob.ESM_action_filter.lower()) != -1:
						cached_action_count += 1
			return "\"" + ob.ESM_action_filter + "\" actions (" + str(cached_action_count) + ")"
		else:
			return "All actions (" + str(len(bpy.data.actions)) + ")", len(bpy.data.actions)

	def draw(self, context):
		# This function is also embedded in property panels on scenes and armatures
		l = self.layout
		ob = context.active_object

		try:
			l = self.embed_scene
			embed_scene = True
		except AttributeError:
			embed_scene = False
		try:
			l = self.embed_arm
			embed_arm = True
		except AttributeError:
			embed_arm = False

		if embed_scene and (len(context.selected_objects) == 0 or not ob):
			row = l.row()
			row.operator(ESMExporter.bl_idname, text="No selection") # filler to stop the scene button moving
			row.enabled = False

		# Normal processing
		# FIXME: in the properties panel, hidden objects appear in context.selected_objects...in the 3D view they do not
		elif (ob and len(context.selected_objects) == 1) or embed_arm:
			subdir = ob.get('ESM_subdir')
			if subdir:
				label = subdir + getDirSep()
			else:
				label = ""


			if ob.type in mesh_compatible:
				want_single_export = True
				# Groups
				if ob.users_group:
					for i in range(len(ob.users_group)):
						group = ob.users_group[i]
						if not group.ESM_mute:
							want_single_export = False
							label = group.name + getFileExt(REF)
							if bpy.context.scene.ESM_format == 'ESM':
								if hasShapes(ob,i):
									label += "/.ESX"

							op = l.operator(ESMExporter.bl_idname, text=label, icon="GROUP") # group
							op.exportMode = 'SINGLE' # will be merged and exported as one
							op.groupIndex = i
				# Single
				if want_single_export:
					label = getObExportName(ob) + getFileExt(REF)
					if bpy.context.scene.ESM_format == 'ESM' and hasShapes(ob):
						label += "/.ESX"
					l.operator(ESMExporter.bl_idname, text=label, icon=MakeObjectIcon(ob,prefix="OUTLINER_OB_")).exportMode = 'SINGLE'


			elif ob.type == 'ARMATURE':
				if embed_arm or ob.data.ESM_action_selection == 'CURRENT':
					text,icon,count = ESM_MT_ExportChoice.getActionSingleTextIcon(self,context)
					if count:
						l.operator(ESMExporter.bl_idname, text=text, icon=icon).exportMode = 'SINGLE_ANIM'
					else:
						l.label(text=text, icon=icon)
				if embed_arm or (len(bpy.data.actions) and ob.data.ESM_action_selection == 'FILTERED'):
					# filtered action list
					l.operator(ESMExporter.bl_idname, text=ESM_MT_ExportChoice.getActionFilterText(self,context)[0], icon='ACTION').exportMode= 'SINGLE'

			else: # invalid object
				label = "Cannot export " + ob.name
				try:
					l.label(text=label,icon=MakeObjectIcon(ob,prefix='OUTLINER_OB_'))
				except: # bad icon
					l.label(text=label,icon='ERROR')

		# Multiple objects
		elif len(context.selected_objects) > 1 and not embed_arm:
			l.operator(ESMExporter.bl_idname, text="Selected objects\\groups", icon='GROUP').exportMode = 'MULTI' # multiple obects


		if not embed_arm:
			l.operator(ESMExporter.bl_idname, text="Scene as configured", icon='SCENE_DATA').exportMode = 'SCENE'
		#l.operator(ESMExporter.bl_idname, text="Whole .blend", icon='FILE_BLEND').exportMode = 'FILE' # can't do this until scene changes become possible


def getValidObs():
	validObs = []
	s = bpy.context.scene
	for o in s.objects:
		if o.type in exportable_types:
			if s.ESM_layer_filter:
				for i in range( len(o.layers) ):
					if o.layers[i] and s.layers[i]:
						validObs.append(o)
						break
			else:
				validObs.append(o)
	return validObs

class ESM_PT_Scene(bpy.types.Panel):
	bl_label = "ESM Export"
	bl_space_type = "PROPERTIES"
	bl_region_type = "WINDOW"
	bl_context = "scene"
	bl_default_closed = True

	def draw(self, context):
		l = self.layout
		scene = context.scene
		num_to_export = 0

		self.embed_scene = l.row()
		ESM_MT_ExportChoice.draw(self,context)

		l.prop(scene,"ESM_path",text="Output Folder")
		row = l.row().split(0.33)
		row.label(text="Target Up Axis:")
		row.row().prop(scene,"ESM_up_axis", expand=True)

		validObs = getValidObs()
		l.row()
		scene_config_row = l.row()

		if len(validObs):
			validObs.sort(key=lambda ob: ob.name.lower())
			box = l.box()
			columns = box.column()
			header = columns.split(0.6)
			header.label(text="Object / Group:")
			header.label(text="Subfolder:")

			had_groups = False
			groups_sorted = []
			for group in bpy.data.groups:
				groups_sorted.append(group)
			groups_sorted.sort(key=lambda g: g.name.lower())
			want_sep = False
			for group in groups_sorted:
				if want_sep: columns.separator()
				want_sep = False
				for object in group.objects:
					if object in validObs:
						had_groups = True
						def _drawMuteButton(layout_button,layout_text):
							layout_button.prop(group,"ESM_mute",emboss=False,text="",icon='MUTE_IPO_ON' if group.ESM_mute else 'MUTE_IPO_OFF')
							if layout_text:
								layout_text.label(text="{} is ignored".format(group.name))
						
						if not group.ESM_mute:	
							row = columns.split(0.6)
							row.prop(group,"ESM_export",icon="GROUP",emboss=True,text=group.name)
							row.prop(group,"ESM_subdir",text="")
							row.enabled = not group.ESM_mute

							if shouldExportGroup(group):
								num_to_export += 1
								group_objects = []
								group_active= 0									
								for object in group.objects:
									if object in validObs and object.type != 'ARMATURE':
										group_objects.append(object)
										if object.ESM_export: group_active += 1
								
								row = columns.row(align=not group.ESM_expand)
								button_column = row.column()
								button_column.prop(group,"ESM_expand",emboss=False,icon='ZOOMOUT' if group.ESM_expand else 'ZOOMIN',text="")
									
								if group.ESM_expand:
									rows = row.column_flow(0) # in theory automatic columns, in practice always 2
									for object in group_objects:
										rows.prop(object,"ESM_export",icon=MakeObjectIcon(object,suffix="_DATA"),emboss=False,text=object.name)
								else:
									row.label(text="({} of {} objects active)".format(group_active,len(group_objects)))
								want_sep = True
						break # we've found an object in the scene and drawn the list

			if had_groups:
				columns.separator()
		
			had_obs = False
			for object in validObs: # meshes
				in_active_group = False
				if object.type in mesh_compatible:
					for group in object.users_group:
						if not group.ESM_mute:
							in_active_group = True
				if not in_active_group:
					if object.type == 'ARMATURE':
						continue

					row = columns.split(0.6)
					row.prop(object,"ESM_export",icon=MakeObjectIcon(object,prefix="OUTLINER_OB_"),emboss=True,text=getObExportName(object))
					row.prop(object,"ESM_subdir",text="")
					had_obs = True
					if object.ESM_export: num_to_export += 1
			
			if had_obs:
				columns.separator()

			had_armatures = False
			want_sep = False
			for object in validObs:
				if object.type == 'ARMATURE' and object.animation_data:
					if want_sep: columns.separator()
					want_sep = False
					had_armatures = True
					row = columns.split(0.6)
					row.prop(object,"ESM_export",icon=MakeObjectIcon(object,prefix="OUTLINER_OB_"),emboss=True,text=object.name)
					row.prop(object,"ESM_subdir",text="")
					if object.ESM_export:
						row = columns.split(0.6)
						if object.data.ESM_action_selection == 'CURRENT':
							text,icon,count = ESM_MT_ExportChoice.getActionSingleTextIcon(self,context,object)
							if object.ESM_export: num_to_export += 1
						elif object.data.ESM_action_selection == 'FILTERED':
							text, num = ESM_MT_ExportChoice.getActionFilterText(self,context)
							if object.ESM_export: num_to_export += num
							icon = "ACTION"
						row.prop(object,"ESM_export",text=text,icon=icon,emboss=False)
						row.prop(object.data,"ESM_action_selection",text="")
						want_sep = True

		scene_config_row.label(text=" Scene Exports ({} file{})".format(num_to_export,"" if num_to_export == 1 else "s"),icon='SCENE_DATA')
	
		r = l.row()
		r.prop(scene,"ESM_use_image_names",text="Ignore materials")
		r.prop(scene,"ESM_layer_filter",text="Visible layer(s) only")
		l.row()

class ESM_PT_Data(bpy.types.Panel):
	bl_label = "ESM Export"
	bl_space_type = "PROPERTIES"
	bl_region_type = "WINDOW"
	bl_context = "data"

	@classmethod
	def poll(self,context):
		return context.active_object.type in ['ARMATURE','CURVE'] # the panel isn't displayed unless there is an active object

	def draw(self, context):
		if context.active_object.type == 'ARMATURE':
			self.draw_Armature(context)
		elif context.active_object.type == 'CURVE':
			self.draw_Curve(context)

	def draw_Curve(self, context):
		c = context.active_object

		self.layout.label(text="Generate polygons on:")

		row = self.layout.row()
		row.prop(c.data,"ESM_faces",expand=True)

	def draw_Armature(self, context):
		l = self.layout
		arm = context.active_object
		anim_data = arm.animation_data

		l.prop(arm,"ESM_subdir",text="Export Subfolder")

		l.prop(arm.data,"ESM_action_selection")
		l.prop(arm,"ESM_action_filter",text="Action Filter")

		row = l.row()
		row.prop(arm.data,"ESM_implicit_zero_bone")

		self.embed_arm = l.row()
		ESM_MT_ExportChoice.draw(self,context)

		if anim_data:
			l.template_ID(anim_data, "action", new="action.new")

		l.separator()
		l.operator(ESMClean.bl_idname,text="Clean ESM names/IDs from bones",icon='BONE_DATA').mode = 'ARMATURE'

class ESMExporter(bpy.types.Operator):
	'''Export egfCA/animCA Data files'''
	bl_idname = "export_scene.esm"
	bl_label = "Export ESM/ESA"
	bl_options = { 'UNDO', 'REGISTER' }

	# This should really be a directory select, but there is no way to explain to the user that a dir is needed except through the default filename!
	directory = StringProperty(name="Export root", description="The root folder into which ESMs from this scene are written", subtype='FILE_PATH')	
	filename = StringProperty(default="", options={'HIDDEN'})

	exportMode_enum = (
		('NONE','No mode','The user will be prompted to choose a mode'),
		('SINGLE','Active','Only the active object'),
		('SINGLE_ANIM','Current action',"Exports the active Armature's current Action"),
		('MULTI','Selection','All selected objects'),
		('SCENE','Scene','Export the objects and animations selected in Scene Properties'),
		#('FILE','Whole .blend file','Export absolutely everything, from all scenes'),
		)
	exportMode = EnumProperty(items=exportMode_enum,options={'HIDDEN'})
	groupIndex = IntProperty(default=-1,options={'HIDDEN'})

	def execute(self, context):
		if not ValidateBlenderVersion(self):
			return {'CANCELLED'}

		props = self.properties

		if props.exportMode == 'NONE':
			self.report({'ERROR'},"Programmer error: bpy.ops.{} called without exportMode".format(ESMExporter.bl_idname))
			return {'CANCELLED'}

		# Handle export root path
		if len(props.directory):
			# We've got a file path from the file selector (or direct invocation)
			context.scene['ESM_path'] = props.directory
		else:
			# Get a path from the scene object
			export_root = context.scene.get("ESM_path")

			# No root defined, pop up a file select
			if not export_root:
				props.filename = "*** [Please choose a root folder for exports from this scene] ***"
				context.window_manager.fileselect_add(self)
				return {'RUNNING_MODAL'}

			if export_root.startswith("//") and not bpy.context.blend_data.filepath:
				self.report({'ERROR'},"Relative scene output path, but .blend not saved")
				return {'CANCELLED'}

			if export_root[-1] not in ['\\','/']: # append trailing slash
				export_root += getDirSep()		

			props.directory = export_root

		global log
		log = logger()
				
		had_auto_keyframe = bpy.context.tool_settings.use_keyframe_insert_auto
		had_auto_keyset = bpy.context.tool_settings.use_keyframe_insert_keyingset
		bpy.context.tool_settings.use_keyframe_insert_auto = False
		bpy.context.tool_settings.use_keyframe_insert_keyingset = False
		
		prev_arm_mode = None
		if props.exportMode == 'SINGLE_ANIM': # really hacky, hopefully this will stay a one-off!
			ob = context.active_object
			if ob.type == 'ARMATURE':
				prev_arm_mode = ob.data.ESM_action_selection
				ob.data.ESM_action_selection = 'CURRENT'
			props.exportMode = 'SINGLE'

		print("\nESM EXPORTER RUNNING")
		prev_active_ob = context.active_object
		if prev_active_ob and prev_active_ob.type == 'ARMATURE':
			prev_bone_selection = bpy.context.selected_pose_bones
			prev_active_bone = context.active_bone
		prev_selection = context.selected_objects
		prev_visible = [] # visible_objects isn't useful due to layers
		for ob in context.scene.objects:
			if not ob.hide: prev_visible.append(ob)
		prev_frame = context.scene.frame_current

		# store Blender mode user was in before export
		prev_mode = bpy.context.mode
		if prev_mode.startswith("EDIT"):
			prev_mode = "EDIT" # remove any suffixes
		if prev_active_ob:
			prev_active_ob.hide = False
			ops.object.mode_set(mode='OBJECT')

		pose_backups = {}
		for object in bpy.context.scene.objects:
			object.hide = False # lots of operators only work on visible objects
			if object.type == 'ARMATURE' and object.animation_data:
				context.scene.objects.active = object
				ops.object.mode_set(mode='POSE')
				context.scene.objects.active = object
				# Back up any unkeyed pose. I'd use the pose library, but it can't be deleted if empty!
				pb_act = bpy.data.actions.new(name=object.name+" pose backup")
				pb_act.user_clear()
				pose_backups[object.name] = [ object.animation_data.action, pb_act ]
				bpy.ops.pose.copy()
				object.animation_data.action = pose_backups[object.name][1]
				bpy.ops.pose.paste()
				bpy.ops.pose.select_all(action='SELECT')
				bpy.ops.anim.keyframe_insert(type='LocRotScale')
				object.animation_data.action = pose_backups[object.name][0]
				ops.object.mode_set(mode='OBJECT')
		context.scene.objects.active = prev_active_ob

		# check export mode and perform appropriate jobs
		self.countESMs = 0
		if props.exportMode == 'SINGLE':
			ob = context.active_object
			group_name = None
			if props.groupIndex != -1:
				# handle the selected object being in a group, but disabled
				group_name = ob.users_group[props.groupIndex].name
				for g_ob in ob.users_group[props.groupIndex].objects:
					if g_ob.ESM_export:
						ob = g_ob
						break
					else:
						ob = None

			if ob:
				self.exportObject(context,context.active_object,groupIndex=props.groupIndex)
			else:
				log.error("The group \"" + group_name + "\" has no active objects")
				return {'CANCELLED'}


		elif props.exportMode == 'MULTI':
			exported_groups = []
			for object in context.selected_objects:
				if object.type in mesh_compatible:
					if object.users_group:
						for i in range(len(object.users_group)):
							if object.ESM_export and object.users_group[i] not in exported_groups:
								self.exportObject(context,object,groupIndex=i)
								exported_groups.append(object.users_group[i])
					else:
						self.exportObject(context,object)
				elif object.type == 'ARMATURE':
					self.exportObject(context,object)

		elif props.exportMode == 'SCENE':
			for group in bpy.data.groups:
				if shouldExportGroup(group):
					for object in group.objects:
						if object.ESM_export and object.type in exportable_types and object.type != 'ARMATURE' and bpy.context.scene in object.users_scene:
							g_index = -1
							for i in range(len(object.users_group)):
								if object.users_group[i] == group:
									g_index = i
									break
							self.exportObject(context,object,groupIndex=g_index)
							break
			for object in bpy.context.scene.objects:
				if object.ESM_export:
					should_export = True
					if object.users_group and object.type != 'ARMATURE':
						for group in object.users_group:
							if not group.ESM_mute:
								should_export = False
								break
					if should_export:
						self.exportObject(context,object)

		elif props.exportMode == 'FILE': # can't be done until Blender scripts become able to change the scene
			for scene in bpy.data.scenes:
				scene_root = scene.get("ESM_path")
				if not scene_root:
					log.warning("Skipped unconfigured scene",scene.name)
					continue
				else:
					props.directory = scene_root

				for object in bpy.data.objects:
					if object.type in ['MESH', 'ARMATURE']:
						self.exportObject(context,object)

		# Export jobs complete! Clean up...
		context.scene.objects.active = prev_active_ob
		if prev_active_ob and context.scene.objects.active:
			ops.object.mode_set(mode=prev_mode)
			if prev_active_ob.type == 'ARMATURE' and prev_active_bone:
				prev_active_ob.data.bones.active = prev_active_ob.data.bones[ prev_active_bone.name ]
				for i in range( len(prev_active_ob.pose.bones) ):
					prev_active_ob.data.bones[i].select = True if prev_bone_selection and prev_active_ob.pose.bones[i] in prev_bone_selection else False

		for object in context.scene.objects:
			object.select = object in prev_selection
			object.hide = object not in prev_visible
			if object.type == 'ARMATURE' and object.animation_data:
				object.animation_data.action = pose_backups[object.name][1] # backed up pose

		context.scene.frame_set(prev_frame) # apply backup pose
		for object in context.scene.objects:
			if object.type == 'ARMATURE' and object.animation_data:
				object.animation_data.action = pose_backups[object.name][0] # switch to original action, don't apply
				pose_backups[object.name][1].use_fake_user = False
				pose_backups[object.name][1].user_clear()
				bpy.data.actions.remove(pose_backups[object.name][1]) # remove backup
		
		if prev_arm_mode:
			prev_active_ob.data.ESM_action_selection = prev_arm_mode
			
		bpy.context.tool_settings.use_keyframe_insert_auto = had_auto_keyframe
		bpy.context.tool_settings.use_keyframe_insert_keyingset = had_auto_keyset

		jobMessage = "exported"

		if self.countESMs == 0:
			log.error("Found no valid objects for export")

		log.errorReport(jobMessage,"file",self,self.countESMs)
		return {'FINISHED'}

	# indirection to support batch exporting
	def exportObject(self,context,object,flex=False,groupIndex=-1):
		props = self.properties

		validObs = getValidObs()
		if groupIndex == -1:
			if not object in validObs:
				return
		else:
			if len(set(validObs).intersection( set(object.users_group[groupIndex].objects) )) == 0:
				return

		# handle subfolder
		if len(object.ESM_subdir) == 0 and object.type == 'ARMATURE':
			object.ESM_subdir = "anims"
		object.ESM_subdir = object.ESM_subdir.lstrip("/") # don't want //s here!

		if object.type == 'ARMATURE' and not object.animation_data:
			return; # otherwise we create a folder but put nothing in it

		# assemble filename
		path = bpy.path.abspath(getFileDir(props.directory) + object.ESM_subdir)
		if path and path[-1] not in ['/','\\']:
			path += getDirSep()
		if not os.path.exists(path):
			os.makedirs(path)

		if object.type in mesh_compatible:
			if groupIndex == -1:
				path += getObExportName(object)
			else:
				path += object.users_group[groupIndex].name

			if writeESM(context, object, groupIndex, path + getFileExt(REF)):
				self.countESMs += 1
			if bpy.context.scene.ESM_format == 'ESM' and hasShapes(object,groupIndex): # DMX will export mesh and shapes to the same file
				if writeESM(context, object, groupIndex, path + getFileExt(FLEX), FLEX):
					self.countESMs += 1
		elif object.type == 'ARMATURE':
			ad = object.animation_data
			prev_action = None
			if ad.action: prev_action = ad.action

			if object.data.ESM_action_selection == 'FILTERED':
				for action in bpy.data.actions:
					if action.users and (not object.ESM_action_filter or action.name.lower().find(object.ESM_action_filter.lower()) != -1):
						ad.action = action
						if writeESM(context,object, -1, path + action.name + getFileExt(ANIM), ANIM):
							self.countESMs += 1
			elif object.animation_data:
				if ad.action:
					if writeESM(context,object,-1,path + ad.action.name + getFileExt(ANIM), ANIM):
						self.countESMs += 1
				elif len(ad.nla_tracks):
					nla_actions = []
					for track in ad.nla_tracks:
						if not track.mute:
							for strip in track.strips:
								if not strip.mute and strip.action not in nla_actions:
									nla_actions.append(strip.action)
									ad.action = strip.action
									if writeESM(context,object,-1,path + ad.action.name + getFileExt(ANIM), ANIM):
										self.countESMs += 1
			ad.action = prev_action

	def invoke(self, context, event):
		if not ValidateBlenderVersion(self):
			return {'CANCELLED'}
		if self.properties.exportMode == 'NONE':
			bpy.ops.wm.call_menu(name="ESM_MT_ExportChoice")
			return {'PASS_THROUGH'}
		else: # a UI element has chosen a mode for us
			return self.execute(context)

class ESMClean(bpy.types.Operator):
	bl_idname = "esm.clean"
	bl_label = "Clean ESM data"
	bl_description = "Deletes ESM-related properties"
	bl_options = {'REGISTER', 'UNDO'}

	mode = EnumProperty(items=( ('OBJECT','Object','Active object'), ('ARMATURE','Armature','Armature bones and actions'), ('SCENE','Scene','Scene and all contents') ),default='SCENE')

	def execute(self,context):
		self.numPropsRemoved = 0
		def removeProps(object,bones=False):
			if not bones:
				for prop in object.items():
					if prop[0].startswith("ESM_"):
						del object[prop[0]]
						self.numPropsRemoved += 1

			if bones and object.type == 'ARMATURE':
				# For some reason deleting custom properties from bones doesn't work well in Edit Mode
				bpy.context.scene.objects.active = object
				object_mode = object.mode
				bpy.ops.object.mode_set(mode='OBJECT')
				for bone in object.data.bones:
					removeProps(bone)
				bpy.ops.object.mode_set(mode=object_mode)
			
			if type(object) == bpy.types.Object and object.type == 'ARMATURE': # clean from actions too
				if object.data.ESM_action_selection == 'CURRENT':
					if object.animation_data and object.animation_data.action:
						removeProps(object.animation_data.action)
				else:
					for action in bpy.data.actions:
						if action.name.lower().find( object.data.ESM_action_filter.lower() ) != -1:
							removeProps(action)

		active_obj = bpy.context.active_object
		active_mode = active_obj.mode if active_obj else None

		if self.properties.mode == 'SCENE':
			for object in context.scene.objects:
				removeProps(object)
					
			for group in bpy.data.groups:
				for g_ob in group.objects:
					if context.scene in g_ob.users_scene:
						removeProps(group)
			removeProps(context.scene)

		elif self.properties.mode == 'OBJECT':
			removeProps(active_obj)

		elif self.properties.mode == 'ARMATURE':
			assert(active_obj.type == 'ARMATURE')
			removeProps(active_obj,bones=True)			

		bpy.context.scene.objects.active = active_obj
		if active_obj:
			bpy.ops.object.mode_set(mode=active_mode)

		self.report({'INFO'},"Deleted {} ESM properties".format(self.numPropsRemoved))
		return {'FINISHED'}

#####################################
#        Shared registration        #
#####################################

def menu_func_import(self, context):
	self.layout.operator(ESMImporter.bl_idname, text="Equilibrium Engine (.ESM, .ESA)")

def menu_func_export(self, context):
	self.layout.operator(ESMExporter.bl_idname, text="Equilibrium Engine (.ESM, .ESA)")

def panel_func_group_mute(self,context):
	# This is crap
	if len(context.active_object.users_group):
		self.layout.separator()
		self.layout.label(text="ESM export ignored groups")
		cols = self.layout.box().column_flow(0)
		for group in context.active_object.users_group:
			cols.prop(group,"ESM_mute",text=group.name)
	
def register():
	bpy.utils.register_module(__name__)
	bpy.types.INFO_MT_file_import.append(menu_func_import)
	bpy.types.INFO_MT_file_export.append(menu_func_export)
	bpy.types.OBJECT_PT_groups.append(panel_func_group_mute)
	
	

	global cached_action_filter_list
	cached_action_filter_list = 0

	bpy.types.Scene.ESM_path = StringProperty(name="ESM Export Root",description="The root folder into which ESMs from this scene are written", subtype='DIR_PATH')
	bpy.types.Scene.ESM_up_axis = EnumProperty(name="ESM Target Up Axis",items=axes,default='Y',description="Use for compatibility with existing ESMs")
	formats = (
	('ESM', "ESM", "egfCA model" ),
	('ESA', "ESA", "animCA animation" ),
	)
	bpy.types.Scene.ESM_format = EnumProperty(name="ESM Export Format",items=formats,default='ESM',description="Currently unused")
	bpy.types.Scene.ESM_use_image_names = BoolProperty(name="ESM Ignore Materials",description="Only export face-assigned image filenames",default=True)
	bpy.types.Scene.ESM_layer_filter = BoolProperty(name="ESM Export visible layers only",description="Only consider objects in active viewport layers for export",default=False)

	bpy.types.Object.ESM_export = BoolProperty(name="ESM Scene Export",description="Export this object with the scene",default=True)
	bpy.types.Object.ESM_subdir = StringProperty(name="ESM Subfolder",description="Location, relative to scene root, for ESMs from this object")
	bpy.types.Object.ESM_action_filter = StringProperty(name="ESM Action Filter",description="Only actions with names matching this filter will be exported")

	bpy.types.Armature.ESM_implicit_zero_bone = BoolProperty(name="Implicit motionless bone",default=True,description="Start bone IDs at one, allowing egfCA to put any unweighted vertices on bone zero. Emulates Blender's behaviour, but may break compatibility with existing ESMs")
	arm_modes = (
	('CURRENT',"Current / NLA","The armature's assigned action, or everything in an NLA track"),
	('FILTERED',"Action Filter","All actions that match the armature's filter term")
	)
	bpy.types.Armature.ESM_action_selection = EnumProperty(name="Action Selection", items=arm_modes,description="How actions are selected for export",default='CURRENT')

	bpy.types.Group.ESM_export = BoolProperty(name="ESM Export Combined",description="Export the members of this group to a single ESM",default=True)
	bpy.types.Group.ESM_subdir = StringProperty(name="ESM Subfolder",description="Location, relative to scene root, for ESMs from this group")
	bpy.types.Group.ESM_expand = BoolProperty(name="ESM show expanded",description="Show the contents of this group in the UI",default=False)
	bpy.types.Group.ESM_mute = BoolProperty(name="ESM ignore",description="Prevents the ESM exporter from merging the objects in this group together",default=False)
	
	bpy.types.Curve.ESM_faces = EnumProperty(name="ESM export which faces",items=(
	('LEFT', 'Left side', 'Generate polygons on the left side'),
	('RIGHT', 'Right side', 'Generate polygons on the right side'),
	('BOTH', 'Both  sides', 'Generate polygons on both sides'),
	), description="Determines which sides of the mesh resulting from this curve will have polygons",default='LEFT')

def unregister():
	bpy.utils.unregister_module(__name__)
	bpy.types.INFO_MT_file_import.remove(menu_func_import)
	bpy.types.INFO_MT_file_export.remove(menu_func_export)
	bpy.types.OBJECT_PT_groups.remove(panel_func_group_mute)

	Scene = bpy.types.Scene
	del Scene.ESM_path
	del Scene.ESM_up_axis
	del Scene.ESM_format
	del Scene.ESM_use_image_names
	del Scene.ESM_layer_filter

	Object = bpy.types.Object
	del Object.ESM_export
	del Object.ESM_subdir
	del Object.ESM_action_filter

	del bpy.types.Armature.ESM_implicit_zero_bone
	del bpy.types.Armature.ESM_action_selection

	del bpy.types.Group.ESM_export
	del bpy.types.Group.ESM_subdir
	del bpy.types.Group.ESM_expand
	del bpy.types.Group.ESM_mute

	del bpy.types.Curve.ESM_faces

if __name__ == "__main__":
	register()
