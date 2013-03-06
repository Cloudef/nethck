#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <glhck/glhck.h>
#include <glhck/nethck.h>
#include <fcntl.h>
#include <signal.h>

enum {
   ID,
   TRANSLATION,
   ROTATION,
   SCALING,
   COLOR,
   GEOMETRY,
};

enum {
   VERTEX,
   NORMAL,
   COORD,
   LAST
};

static void removeFifo(void)
{
   unlink("/tmp/blender.fifo");
}

static void sigInt(int sig)
{
   removeFifo();
   exit(sig);
}

static void handleGeometry(FILE *f, glhckObject *object, size_t vertexCount, size_t indexCount)
{
   char buffer[1024];
   size_t v = 0, i = 0;
   unsigned int index = 0;
   glhckVertexData3f *vdata = NULL;
   glhckIndexi *idata = NULL;
   glhckGeometry *geometry = NULL;

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

      /* xzy -> xyz here
       * could be also done in nethck.py, I guess.. */

      printf("%s", buffer);
      switch (index) {
         case VERTEX:
            sscanf(buffer, "%f,%f,%f",
                  &vdata[v].vertex.x, &vdata[v].vertex.z, &vdata[v].vertex.y);
            break;
         case NORMAL:
            sscanf(buffer, "%f,%f,%f",
                  &vdata[v].normal.x, &vdata[v].normal.z, &vdata[v].normal.y);
            break;
         case COORD:
            sscanf(buffer, "%f,%f",
                  &vdata[v].coord.x, &vdata[v].coord.y);
            break;
      }

      vdata[v].color.r = 255;
      vdata[v].color.g = 255;
      vdata[v].color.b = 255;
      vdata[v].color.a = 255;

      if (++index == LAST) {
         index = 0;
         ++v;
      }
   }

   while (i < indexCount) {
      memset(buffer, 0, sizeof(buffer));
      if (!fgets(buffer, sizeof(buffer), f))
         goto fail;

      printf("%s", buffer);
      sscanf(buffer, "%u,%u,%u", &idata[i+0], &idata[i+1], &idata[i+2]);
      i+=3;
   }

   if (!(geometry = glhckObjectNewGeometry(object)))
      goto fail;

   glhckGeometrySetVertices(geometry, GLHCK_VERTEX_V3F, vdata, vertexCount);
   if (indexCount) glhckGeometrySetIndices(geometry, GLHCK_INDEX_INTEGER, idata, indexCount);
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
   unsigned int index, id;
   int vertexCount, indexCount;
   kmVec3 translation, rotation, scaling;
   kmVec4 colorf;

   index = vertexCount = indexCount = 0;
   memset(buffer, 0, sizeof(buffer));
   while (fgets(buffer, sizeof(buffer), f)) {
      printf("%s", buffer);
      switch (index) {
         case ID:
            sscanf(buffer, "%u", &id);
            object = nethckClientObjectForId(id);
            if (object) glhckObjectRef(object);
            if (!object && !(object = glhckObjectNew())) goto fail;
            break;
         case TRANSLATION:
            sscanf(buffer, "%f,%f,%f", &translation.x, &translation.y, &translation.z);
            glhckObjectPosition(object, &translation);
            break;
         case ROTATION:
            sscanf(buffer, "%f,%f,%f", &rotation.x, &rotation.y, &rotation.z);
            rotation.x *= kmPIUnder180;
            rotation.y *= kmPIUnder180;
            rotation.z *= kmPIUnder180;
            printf("ROTATION: %f, %f, %f\n", rotation.x, rotation.y, rotation.z);
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
            sscanf(buffer, "%d,%d", &vertexCount, &indexCount);
            handleGeometry(f, object, vertexCount, indexCount);

            if (object) {
               nethckClientObjectRender(id, object);
               nethckClientUpdate();
               glhckObjectFree(object);
               object = NULL;
               index = vertexCount = indexCount = 0;
            }

            break;
         default:
            break;
      };
      if (object) ++index;
      memset(buffer, 0, sizeof(buffer));
   }

   if (object) glhckObjectFree(object);
   return index;

fail:
   if (object) glhckObjectFree(object);
   return 0;
}

int main(int argc, char **argv)
{
   FILE *f;

   if (!glhckContextCreate(argc, argv))
      return EXIT_FAILURE;

   if (!nethckClientCreate(NULL, 5050))
      return EXIT_FAILURE;

   signal(SIGQUIT, sigInt);
   signal(SIGINT, sigInt);
   atexit(removeFifo);
   removeFifo();
   mkfifo("/tmp/blender.fifo", 0600);
   if (!(f = fopen("/tmp/blender.fifo", "r")))
      return EXIT_FAILURE;

   atexit(removeFifo);
   while (1) {
      nethckClientUpdate();
      handleFifo(f);
   }

   fclose(f);
   removeFifo();
   nethckClientTerminate();
   glhckContextTerminate();
   return EXIT_SUCCESS;
}
