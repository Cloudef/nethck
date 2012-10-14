#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <glhck/glhck.h>
#include <glhck/nethck.h>
#include <fcntl.h>

enum {
   TRANSLATION,
   ROTATION,
   SCALING,
   COLOR,
   GEOMETRY,
};

static void removeFifo(void)
{
   unlink("/tmp/blender.fifo");
}

static void handleGeometry(FILE *f, glhckObject *object, size_t vertexCount, size_t indexCount)
{
   char buffer[1024];
   size_t v = 0, i = 0;
   glhckImportVertexData *vdata = NULL;
   glhckImportIndexData *idata = NULL;
   glhckGeometry *geometry = NULL;
   kmVec4 colorf;

   if (!vertexCount)
      goto fail;

   if (!(vdata = malloc(sizeof(glhckImportVertexData) * vertexCount)))
      goto fail;

   if (indexCount && !(idata = malloc(sizeof(glhckImportIndexData) * indexCount)))
      goto fail;

   memset(vdata, 0, sizeof(glhckImportVertexData) * vertexCount);
   if (indexCount) memset(idata, 0, sizeof(glhckImportIndexData) * indexCount);

   while (v != vertexCount) {
      memset(buffer, 0, sizeof(buffer));
      if (!fgets(buffer, sizeof(buffer), f))
         goto fail;

      sscanf(buffer, "%f,%f,%f",
            &vdata[v].vertex.x, &vdata[v].vertex.y, &vdata[v].vertex.z);
      printf("%f, %f, %f\n",
            vdata[v].vertex.x, vdata[v].vertex.y, vdata[v].vertex.z);
      vdata[v].color.r = 255;
      vdata[v].color.g = 255;
      vdata[v].color.b = 255;
      vdata[v].color.a = 255;

#if 0
      vdata[v].color.r = colorf.x * 255.0f;
      vdata[v].color.g = colorf.y * 255.0f;
      vdata[v].color.b = colorf.z * 255.0f;
      vdata[v].color.a = colorf.w * 255.0f;
#endif

      ++v;
   }

   while (i != indexCount) {
      memset(buffer, 0, sizeof(buffer));
      if (!fgets(buffer, sizeof(buffer), f))
         goto fail;

      sscanf(buffer, "%u", &idata[i]);
      printf("%u\n", idata[i]);
      ++i;
   }

   glhckObjectInsertVertices(object, vertexCount, GLHCK_VERTEX_V3F, vdata);
   if (indexCount) glhckObjectInsertIndices(object, indexCount, GLHCK_INDEX_INTEGER, idata);

   if (!(geometry = glhckObjectGetGeometry(object)))
      goto fail;

   geometry->type = GLHCK_TRIANGLES;
   free(vdata);
   free(idata);
   return;

fail:
   if (vdata) free(vdata);
   if (idata) free(idata);
}

static unsigned int handleFifo(FILE *f)
{
   glhckObject *object = NULL;
   char buffer[1024];
   unsigned int index;
   size_t vertexCount, indexCount;
   kmVec3 translation, rotation, scaling;
   kmVec4 colorf;

   index = vertexCount = indexCount = 0;
   memset(buffer, 0, sizeof(buffer));
   while (fgets(buffer, sizeof(buffer), f)) {
     printf("%s", buffer);
      switch (index) {
         case TRANSLATION:
            if (!(object = glhckObjectNew())) goto fail;
            sscanf(buffer, "%f,%f,%f", &translation.x, &translation.y, &translation.z);
            glhckObjectPosition(object, &translation);
            break;
         case ROTATION:
            sscanf(buffer, "%f,%f,%f", &rotation.x, &rotation.y, &rotation.z);
            rotation.x *= kmPIUnder180;
            rotation.y *= kmPIUnder180;
            rotation.z *= kmPIUnder180;
            glhckObjectRotation(object, &rotation);
            break;
         case SCALING:
            sscanf(buffer, "%f,%f,%f", &scaling.x, &scaling.y, &scaling.z);
            glhckObjectScale(object, &scaling);
            break;
         case COLOR:
            sscanf(buffer, "%f,%f,%f,%f", &colorf.x, &colorf.y, &colorf.z, &colorf.w);
            glhckObjectColorb(object, colorf.x*255.0f, colorf.y*255.0f, colorf.z*255.0f, colorf.w*255.0f);
            break;
         case GEOMETRY:
            sscanf(buffer, "%zu,%zu", &vertexCount, &indexCount);
            handleGeometry(f, object, vertexCount, indexCount);
            break;
         default:
            break;
      }; ++index;
      memset(buffer, 0, sizeof(buffer));
   }

   if (object) {
      nethckClientObjectRender(object);
      glhckObjectFree(object);
   }
   return index;

fail:
   if (object) glhckObjectFree(object);
   return 0;
}

int main(int argc, char **argv)
{
   FILE *f;

   if (!glhckInit(argc, argv))
      return EXIT_FAILURE;

   nethckInit(argc, argv);
   if (!nethckClientInit(NULL, 5050))
      return EXIT_FAILURE;

   removeFifo();
   mkfifo("/tmp/blender.fifo", 0600);
   if (!(f = fopen("/tmp/blender.fifo", "r")))
      return EXIT_FAILURE;

   atexit(removeFifo);
   while (1) {
      handleFifo(f);
      nethckClientUpdate();
   }

   fclose(f);
   removeFifo();

   nethckClientKill();
   glhckTerminate();
   return EXIT_SUCCESS;
}
