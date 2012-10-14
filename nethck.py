import bpy

def sendObject(ob):
   ic = 0
   f = open('/tmp/blender.fifo', 'w')
   f.write(str(-ob.location[0])+","+
           str(ob.location[2])+","+
           str(ob.location[1])+"\n")
   f.write(str(-ob.rotation_euler[0])+","+
           str(ob.rotation_euler[2])+","+
           str(ob.rotation_euler[1])+"\n")
   f.write(str(ob.scale[0])+","+
           str(ob.scale[2])+","+
           str(ob.scale[1])+"\n")
   f.write(str(ob.color[0])+","+
           str(ob.color[1])+","+
           str(ob.color[2])+","+
           str(ob.color[3])+"\n")
   for p in ob.data.polygons:
       ic += len(p.vertices)
   f.write(str(len(ob.data.vertices))+","+
           str(ic)+"\n");
   for v in ob.data.vertices:
      f.write(str(v.co[0])+","+
              str(v.co[1])+","+
              str(v.co[2])+"\n")
   for p in ob.data.polygons:
      verts = p.vertices[:]
      for v in verts:
         f.write(str(v)+"\n")
   f.flush()
   f.close()

def scene_update(context):
   if bpy.data.objects.is_updated:
      print("One or more objects were updated!")
      for ob in bpy.data.objects:
         if ob.is_updated:
            print("=>", ob.name)
            sendObject(ob)

bpy.app.handlers.scene_update_post.append(scene_update)
