import bpy
from bpy.props import BoolProperty
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
geometryUpdate={}
fifoPath='/tmp/blender.fifo'

# Return object's position
def obPosition(ob):
   return ob.matrix_local.to_translation()

# Return object's rotation
def obRotation(ob):
   return ob.matrix_local.to_euler();

# Dump object to FIFO
def sendObject(ob, edited):
   global geometryUpdate
   global fifoPath

   f = open(fifoPath, 'w')
   if f:
      obId = hash(ob.name)
      shouldUpdateGeometry = geometryUpdate.get(obId, True)
      position = obPosition(ob)
      rotation = obRotation(ob)

      if ob.mode == 'OBJECT':
         f.write(str(obId)+"\n")
         f.write(str(position[0])+","+
                 str(position[1])+","+
                 str(position[2])+"\n")
         f.write(str(rotation[0])+","+
                 str(rotation[1])+","+
                 str(rotation[2])+"\n")
         f.write(str(ob.scale[0])+","+
                 str(ob.scale[1])+","+
                 str(ob.scale[2])+"\n")
         f.write("%f,%f,%f,%f\n"%tuple(ob.color))

         # Convert the object to mesh and write it's vertexData if needed
         if shouldUpdateGeometry:
            mesh = ob.to_mesh(bpy.context.scene, True, "PREVIEW")

            # Convert to OpenGL/GLhck friendly vertex data
            vertexData = buildData(mesh)
            vertices = vertexData['vertices']
            normals = vertexData['normals']
            uvs = vertexData['uvs']
            indices = vertexData['indices']

            # Length of vertices/indices <int, int>
            f.write(str(len(vertices))+","+
                    str(len(indices)*3)+"\n")

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

         # Flush and close FIFO
         f.flush()
         f.close()

         # Store object edit state
         geometryUpdate[obId] = edited

# UpdateAPI callback
def sceneUpdate(context):
   if bpy.data.objects.is_updated:
      print("One or more objects were updated!")
      for ob in bpy.data.objects:
         if ob.type == 'MESH':
            if ob.is_updated:
               print("=>", ob.name)
               sendObject(ob, False)
            else:
               mesh = bpy.data.meshes.get(ob.data.name)
               if mesh.is_updated:
                  print("=>", mesh)
                  sendObject(ob, True)

# Update whole scene
def sceneUpdateFull():
   for ob in bpy.data.objects:
      if ob.type == 'MESH':
         sendObject(ob, False)

# Register update callback
def registerUpdateCallback():
   removeUpdateCallback()
   bpy.app.handlers.scene_update_post.append(sceneUpdate)

# Remove update callback
def removeUpdateCallback():
   for x in bpy.app.handlers.scene_update_post[:]:
      if x == sceneUpdate:
         bpy.app.handlers.scene_update_post.remove(sceneUpdate)

# Nethck synchorization check box
class nethckCheck(bpy.types.Operator):
   bl_idname = 'render.nethck'
   bl_label = 'NEThck Synchorization'
   bl_description = 'Synchorize over nethck \'protocol\''

   def execute(self, context):
      global geometryUpdate
      global fifoPath
      if bpy.types.Scene.Nethck:
         try:
            with open('fifoPath') as f: pass
         except IOError as e:
            self.report({'ERROR'}, "Blender nethck client is not running!")
            return {'FINISHED'}

         # Activated
         geometryUpdate={}
         sceneUpdateFull()
         registerUpdateCallback()
      else:
         # Disabled
         geometryUpdate={}
         removeUpdateCallback()

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
   global geometryUpdate
   geometryUpdate={}
   removeUpdateCallback()
   bpy.types.Scene.Nethck = BoolProperty(
         name=nethckCheck.bl_label,
         description=nethckCheck.bl_description,
         default=False)
   bpy.utils.register_class(nethckCheck)
   bpy.types.RENDER_PT_render.prepend(nethckDraw)

# Plugin's unregister method
def unregister():
   global geometryUpdate
   geometryUpdate={}
   removeUpdateCallback()
   del bpy.types.Scene.Nethck
   bpy.utils.unregister_class(nethckCheck)
   bpy.types.RENDER_PT_render.remove(nethckDraw)

if __name__ == "__main__":
   register()
