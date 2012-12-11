#include "internal.h"
#include <glhck/glhck.h>
#include <arpa/inet.h>
#include <malloc.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* \brief float vector to bams v2 vector */
void _nethckV2FToBams(nethckVector2B *bv2, const glhckVector2f *v2)
{
   bv2->x = htonl(*((unsigned int*)&v2->x)>>16);
   bv2->y = htonl(*((unsigned int*)&v2->y)>>16);
}

/* \brief float vector to bams v3 vector */
void _nethckV3FToBams(nethckVector3B *bv3, const glhckVector3f *v3)
{
   _nethckV2FToBams((nethckVector2B*)bv3, (glhckVector2f*)v3);
   bv3->z = htonl(*((unsigned int*)&v3->z)>>16);
}

/* \brief bams v2 vector to float vector */
void _nethckBamsToV2F(glhckVector2f *v2, const nethckVector2B *bv2)
{
   unsigned int x = ntohl(bv2->x)<<16;
   unsigned int y = ntohl(bv2->y)<<16;
   v2->x = *((float*)&x);
   v2->y = *((float*)&y);
}

/* \brief bams v3 vector to float vector */
void _nethckBamsToV3F(glhckVector3f *v3, const nethckVector3B *bv3)
{
   _nethckBamsToV2F((glhckVector2f*)v3, (nethckVector2B*)bv3);
   unsigned int z = ntohl(bv3->z)<<16;
   v3->z = *((float*)&z);
}

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
