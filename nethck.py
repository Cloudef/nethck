import bpy
import mathutils

def r3d(v):
   return round(v[0],6), round(v[1],6), round(v[2],6)

def r2d(v):
   return round(v[0],6), round(v[1],6)

vertexnum={}
lvl=[]
lnl=[]
luvl=[]
lfl=[]

def buildData(msh):
   global lvl
   global lnl
   global luvl
   global lfl

   lvdic = {}
   lfl = []
   lvl = []
   lnl = []
   luvl = []
   lvcnt = 0
   isSmooth = False
   hasUV = True

   if (len(msh.tessface_uv_textures)>0):
      if (msh.tessface_uv_textures.active is None):
         hasUV = False
   else:
      hasUV = False

   if (hasUV):
      activeUV = msh.tessface_uv_textures.active.data

   for i,f in enumerate(msh.tessfaces):
      isSmooth = f.use_smooth
      tmpfaces = []
      for j,v in enumerate(f.vertices):
         vec = msh.vertices[v].co
         vec = r3d(vec)
         if (isSmooth):
            nor = msh.vertices[v].normal
         else:
            nor = f.normal;
         nor = r3d(nor)

         if (hasUV):
            co = activeUV[i].uv[j]
            co = r2d(co)
         else:
            co = (0.0, 0.0)

         key = vec, nor, co
         vinx = lvdic.get(key)

         if (vinx is None):
            lvdic[key] = lvcnt
            lvl.append(vec)
            lnl.append(nor)
            luvl.append(co)
            tmpfaces.append(lvcnt)
            lvcnt+=1
         else:
            inx = lvdic[key]
            tmpfaces.append(inx)

      if (len(tmpfaces)==3):
         lfl.append(tmpfaces)
      else:
         lfl.append([tmpfaces[0], tmpfaces[1], tmpfaces[2]])
         lfl.append([tmpfaces[0], tmpfaces[2], tmpfaces[3]])

def obPosition(ob):
   return ob.matrix_local.to_translation()

def obRotation(ob):
   return ob.matrix_local.to_euler();

def sendObject(ob, edited):
   global vertexnum

   f = open('/tmp/blender.fifo', 'w')
   if f:
      obid = hash(ob.name)
      oldvnum = vertexnum.get(obid, 0)
      position = obPosition(ob)
      rotation = obRotation(ob)
      f.write(str(obid)+"\n")
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

      msh = ob.to_mesh(bpy.context.scene,True,"PREVIEW")
      newvnum = len(msh.vertices)
      if newvnum != oldvnum or edited:
         buildData(msh)
         f.write(str(len(lvl))+","+
                 str(len(lfl)*3)+"\n")
         for i in range(0,len(lvl)):
            f.write("%f,%f,%f\n"%tuple(lvl[i]))
            f.write("%f,%f,%f\n"%tuple(lnl[i]))
            f.write("%f,%f\n"%tuple(luvl[i]))

         for i in lfl:
            f.write("%d,%d,%d\n"%tuple(i))

      vertexnum[obid] = newvnum
      f.flush()
      f.close()

def scene_update(context):
   if bpy.data.objects.is_updated:
      print("One or more objects were updated!")
      for ob in bpy.data.objects:
         if ob.type != 'MESH':
            continue
         if ob.is_updated:
            print("=>", ob.name)
            sendObject(ob, False)
         else:
            mesh = bpy.data.meshes.get(ob.data.name)
            if mesh.is_updated:
               print("=>", mesh)
               sendObject(ob, True)

bpy.app.handlers.scene_update_post.append(scene_update)
