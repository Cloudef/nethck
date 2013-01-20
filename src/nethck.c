#include "internal.h"
#include <glhck/glhck.h>
#include <arpa/inet.h>
#include <malloc.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* \brief get size and data pointer for vertex data from geometry */
void _nethckGeometryVertexDataAndSize(glhckGeometry *geometry, void **data, size_t *size)
{
   *data = NULL;
   *size = 0;
   switch (geometry->vertexType) {
      case GLHCK_VERTEX_V3B:
         *size = geometry->vertexCount * sizeof(glhckVertexData3b);
         *data = geometry->vertices.v3b;
         break;

      case GLHCK_VERTEX_V2B:
         *size = geometry->vertexCount * sizeof(glhckVertexData2b);
         *data = geometry->vertices.v2b;
         break;

      case GLHCK_VERTEX_V3S:
         *size = geometry->vertexCount * sizeof(glhckVertexData3s);
         *data = geometry->vertices.v3s;
         break;

      case GLHCK_VERTEX_V2S:
         *size = geometry->vertexCount * sizeof(glhckVertexData2s);
         *data = geometry->vertices.v2s;
         break;

      case GLHCK_VERTEX_V3F:
         *size = geometry->vertexCount * sizeof(glhckVertexData3f);
         *data = geometry->vertices.v3f;
         break;

      case GLHCK_VERTEX_V2F:
         *size = geometry->vertexCount * sizeof(glhckVertexData2f);
         *data = geometry->vertices.v2f;
         break;

      default:
         break;
   }
}

/* \brief get size and data pointer for index data from geometry */
void _nethckGeometryIndexDataAndSize(glhckGeometry *geometry, void **data, size_t *size)
{
   *data = NULL;
   *size = 0;
   switch (geometry->indexType) {
      case GLHCK_INDEX_BYTE:
         *size = geometry->indexCount * sizeof(glhckIndexb);
         *data = geometry->indices.ivb;
         break;

      case GLHCK_INDEX_SHORT:
         *size = geometry->indexCount * sizeof(glhckIndexs);
         *data = geometry->indices.ivs;
         break;

      case GLHCK_INDEX_INTEGER:
         *size = geometry->indexCount * sizeof(glhckIndexi);
         *data = geometry->indices.ivi;
         break;

      default:
         break;
   }
}

/* vim: set ts=8 sw=3 tw=0 :*/
