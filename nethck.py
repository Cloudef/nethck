import bpy
from bpy.props import BoolProperty
import os.path
import mathutils

bl_info = {
    "name": "NEThck Synchorization",
    "description": "Synchorizes current scene over nethck 'protocol'",
    "author": "Jari Vetoniemi",
    "version": (1, 0),
    "blender": (2, 66, 0),
    "location": "Render > Nethck Render",
    "warning": "Needs blender nethck client running (Linux only)", # used for warning icon and text in addons panel
    "wiki_url": "",
    "tracker_url": "",
    "category": "Development"}

#
# Vertexdata conversion to OpenGL/GLhck friendly format
#

# Round 3d vector
def r3d(v):
   return round(v[0],6), round(v[1],6), round(v[2],6)

# Round 2d vector
def r2d(v):
   return round(v[0],6), round(v[1],6)

def buildData(mesh):
   storedData = {}
   indices = []
   vertices = []
   normals = []
   uvs = []
   vertexCount = 0

   # Check if mesh has UV
   hasUV = True
   if (len(mesh.tessface_uv_textures)>0):
      if (mesh.tessface_uv_textures.active is None):
         hasUV = False
   else:
      hasUV = False

   # Get active UV
   if (hasUV):
      activeUV = mesh.tessface_uv_textures.active.data

   for i,f in enumerate(mesh.tessfaces):
      isSmooth = f.use_smooth
      tmpFaces = []
      for j,v in enumerate(f.vertices):
         # Get vertex
         vertex = mesh.vertices[v].co
         vertex = r3d(vertex)

         # Get normal
         if (isSmooth):
            normal = mesh.vertices[v].normal
         else:
            normal = f.normal;
         normal = r3d(normal)

         # Get uv coordinate
         if (hasUV):
            coord = activeUV[i].uv[j]
            coord = r2d(coord)
         else:
            coord = (0.0, 0.0)

         # Check for duplicate vertex
         key = vertex, normal, coord
         vertexIndex = storedData.get(key)

         if (vertexIndex is None):
            # Store new vertex
            storedData[key] = vertexCount
            vertices.append(vertex)
            normals.append(normal)
            uvs.append(coord)
            tmpFaces.append(vertexCount)
            vertexCount+=1
         else:
            # Reuse the vertex from dictionary
            tmpFaces.append(vertexIndex)

      # Is the format already triangles?
      if (len(tmpFaces)==3):
         indices.append(tmpFaces)
      else:
         indices.append([tmpFaces[0], tmpFaces[1], tmpFaces[2]])
         indices.append([tmpFaces[0], tmpFaces[2], tmpFaces[3]])

   return {'vertices':vertices, 'normals':normals, 'uvs':uvs, 'indices':indices}

#
# FIFO pipe logic with nethck's blender example
#

# Dictionary for storing object's geometry update state
fifoPath='/tmp/blender.fifo'

# Return object's position
def obPosition(ob):
   position = ob.matrix_local.to_translation()
   return [-position[0],position[2],position[1]]

# Return object's rotation
def obRotation(ob):
   x = 'X'
   y = 'Z'
   z = 'Y'
   euler = ob.matrix_local.to_euler()
   crot = mathutils.Matrix().to_3x3()
   xrot = mathutils.Matrix.Rotation(-euler.x, 3, x)
   yrot = mathutils.Matrix.Rotation(euler.y, 3, y)
   zrot = mathutils.Matrix.Rotation(euler.z, 3, z)
   rot = crot * zrot * yrot * xrot
   return rot.to_euler()

# Return object's scaling
def obScaling(ob):
   return [-ob.scale[0],ob.scale[2],ob.scale[1]]

# Dump object to FIFO
def sendObject(ob, force):
   global fifoPath

   # Check for fifo
   if not os.path.exists(fifoPath):
      removeUpdateCallback()
      bpy.types.Scene.Nethck = True
      return

   f = open(fifoPath, 'w')
   if f:
      obId = hash(ob.name) % (2**32)
      position = obPosition(ob)
      rotation = obRotation(ob)
      scaling  = obScaling(ob)

      f.write("%u\n"%obId)
      f.write("%f,%f,%f\n"%tuple(position))
      f.write("%f,%f,%f\n"%tuple(rotation))
      f.write("%f,%f,%f\n"%tuple(scaling))

      # Send material parameters
      if ob.active_material:
         diffuse = ob.active_material.diffuse_color
         f.write("%f,%f,%f,%f\n"%(diffuse[0], diffuse[1], diffuse[2], ob.active_material.alpha))

         updateTexture = (ob.active_material.active_texture and ob.active_material.active_texture.is_updated)
         if force or updateTexture:
            texture = ob.active_material.active_texture
            if "image" in dir(texture) and texture.image is not None:
               f.write(bpy.path.abspath(texture.image.filepath)+"\n")
            else:
               f.write("NULL\n")
         else:
            f.write("NULL\n")
      else:
         f.write("%f,%f,%f,%f\n"%tuple(ob.color))
         f.write("NULL\n")

      # Convert the object to mesh and write it's vertexData if needed
      if force or ob.is_updated_data:
         mesh = ob.to_mesh(bpy.context.scene, True, "PREVIEW")

         # Convert to OpenGL/GLhck friendly vertex data
         vertexData = buildData(mesh)
         vertices = vertexData['vertices']
         normals = vertexData['normals']
         uvs = vertexData['uvs']
         indices = vertexData['indices']

         # Length of vertices/indices <int, int>
         f.write("%d,%d\n"%(len(vertices),len(indices)*3))

         # Write vertices, normals and uvs
         for i in range(0,len(vertices)):
            f.write("%f,%f,%f\n"%tuple(vertices[i]))
            f.write("%f,%f,%f\n"%tuple(normals[i]))
            f.write("%f,%f\n"%tuple(uvs[i]))

         # Write indices
         for i in indices:
            f.write("%d,%d,%d\n"%tuple(i))
      else:
         # No need to send the geometry data, so inform 0,0 length
         f.write("0,0\n")

      # Close FIFO
      f.close()

# UpdateAPI callback
def sceneUpdate(context):
   if bpy.data.objects.is_updated:
      print("One or more objects were updated!")
      for ob in bpy.data.objects:
         if ob.type == 'MESH' and ob.is_updated:
            # ob.is_updated_data for edit mode updates!
            print("=>", ob.name)
            sendObject(ob, False)
   if bpy.data.textures.is_updated:
      for texture in bpy.data.textures:
         if texture.is_updated:
            for ob in bpy.data.objects:
               if ob.type == 'MESH' and ob.active_material:
                  if ob.active_material.active_texture and ob.active_material.active_texture.name == texture.name:
                     sendObject(ob, False)
            print("=>", texture.name);
   if bpy.data.materials.is_updated:
      for material in bpy.data.materials:
         if material.is_updated:
            for ob in bpy.data.objects:
               if ob.type == 'MESH' and ob.active_material and ob.active_material.name == material.name:
                  sendObject(ob, False)
            print("=>", material.name);

# Update whole scene
def sceneUpdateFull():
   for ob in bpy.data.objects:
      if ob.type == 'MESH':
         sendObject(ob, True)

# Register update callback
def registerUpdateCallback():
   removeUpdateCallback()
   bpy.app.handlers.scene_update_post.append(sceneUpdate)

# Remove update callback
def removeUpdateCallback():
   for x in bpy.app.handlers.scene_update_post[:]:
      if x == sceneUpdate:
         bpy.app.handlers.scene_update_post.remove(x)

# Nethck synchorization check box
class nethckCheck(bpy.types.Operator):
   bl_idname = 'render.nethck'
   bl_label = 'NEThck Synchorization'
   bl_description = 'Synchorize over nethck \'protocol\''

   def execute(self, context):
      global fifoPath
      if bpy.types.Scene.Nethck:
         if not os.path.exists(fifoPath):
            self.report({'ERROR'}, "Blender nethck client is not running!")
            return {'FINISHED'}

         # Activated
         sceneUpdateFull()
         registerUpdateCallback()
         bpy.types.Scene.Nethck = False
      else:
         # Disabled
         removeUpdateCallback()
         bpy.types.Scene.Nethck = True

      return {'FINISHED'}

# Draw nethck settings
def nethckDraw(self, context):
   nethckOk = not bpy.types.Scene.Nethck
   rnl = context.scene.render.layers.active
   split = self.layout.split()
   col = split.column()
   col.operator(nethckCheck.bl_idname, emboss=False, icon='CHECKBOX_HLT' if nethckOk else 'CHECKBOX_DEHLT')
   self.layout.separator()

# Plugin's register method
def register():
   removeUpdateCallback()
   bpy.types.Scene.Nethck = BoolProperty(
         name=nethckCheck.bl_label,
         description=nethckCheck.bl_description,
         default=False)
   bpy.utils.register_class(nethckCheck)
   bpy.types.RENDER_PT_render.prepend(nethckDraw)

# Plugin's unregister method
def unregister():
   removeUpdateCallback()
   del bpy.types.Scene.Nethck
   bpy.utils.unregister_class(nethckCheck)
   bpy.types.RENDER_PT_render.remove(nethckDraw)

if __name__ == "__main__":
   register()
