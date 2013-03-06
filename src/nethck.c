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

      case GLHCK_VERTEX_V3FS:
         *size = geometry->vertexCount * sizeof(glhckVertexData3fs);
         *data = geometry->vertices.v3fs;
         break;

      case GLHCK_VERTEX_V2FS:
         *size = geometry->vertexCount * sizeof(glhckVertexData2fs);
         *data = geometry->vertices.v2fs;
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

/* dirty hack for hashing the floats */
unsigned int hashf(float x)
{
   union {
      float f;
      unsigned u;
   } u;
   u.f = x;
   return u.u;
}
#define HASH(hash, x) hash+hashf(x)
#define HASHV2(hash, v) HASH(hash, (&v)->x)+HASH(hash, (&v)->y)
#define HASHV3(hash, v) HASHV2(hash, v)+HASH(hash, (&v)->z)
#define HASHCB(hash, v) HASH(hash, (&v)->r)+HASH(hash, (&v)->g)+HASH(hash, (&v)->b)+HASH(hash, (&v)->a)
unsigned int _nethckGeometryVertexDataHash(glhckGeometry *geometry)
{
   int i;
   unsigned int hash = 0;

   switch (geometry->vertexType) {
      case GLHCK_VERTEX_V3B:
         for (i = 0; i != geometry->vertexCount; ++i)
            hash = HASHV3(hash, geometry->vertices.v3b[i].vertex);
            hash = HASHV3(hash,geometry->vertices.v3b[i].normal);
            hash = HASHV2(hash,geometry->vertices.v3b[i].coord);
            hash = HASHCB(hash,geometry->vertices.v3b[i].color);
         break;

      case GLHCK_VERTEX_V2B:
         for (i = 0; i != geometry->vertexCount; ++i)
            hash = HASHV2(hash,geometry->vertices.v2b[i].vertex);
            hash = HASHV3(hash,geometry->vertices.v2b[i].normal);
            hash = HASHV2(hash,geometry->vertices.v2b[i].coord);
            hash = HASHCB(hash,geometry->vertices.v2b[i].color);
         break;

      case GLHCK_VERTEX_V3S:
         for (i = 0; i != geometry->vertexCount; ++i)
            hash = HASHV3(hash,geometry->vertices.v3s[i].vertex);
            hash = HASHV3(hash,geometry->vertices.v3s[i].normal);
            hash = HASHV2(hash,geometry->vertices.v3s[i].coord);
            hash = HASHCB(hash,geometry->vertices.v3s[i].color);
         break;

      case GLHCK_VERTEX_V2S:
         for (i = 0; i != geometry->vertexCount; ++i)
            hash = HASHV2(hash,geometry->vertices.v2s[i].vertex);
            hash = HASHV3(hash,geometry->vertices.v2s[i].normal);
            hash = HASHV2(hash,geometry->vertices.v2s[i].coord);
            hash = HASHCB(hash,geometry->vertices.v2s[i].color);
         break;

      case GLHCK_VERTEX_V3FS:
         for (i = 0; i != geometry->vertexCount; ++i)
            hash = HASHV3(hash,geometry->vertices.v3fs[i].vertex);
            hash = HASHV3(hash,geometry->vertices.v3fs[i].normal);
            hash = HASHV2(hash,geometry->vertices.v3fs[i].coord);
            hash = HASHCB(hash,geometry->vertices.v3fs[i].color);
         break;

      case GLHCK_VERTEX_V2FS:
         for (i = 0; i != geometry->vertexCount; ++i)
            hash = HASHV2(hash,geometry->vertices.v2fs[i].vertex);
            hash = HASHV3(hash,geometry->vertices.v2fs[i].normal);
            hash = HASHV2(hash,geometry->vertices.v2fs[i].coord);
            hash = HASHCB(hash,geometry->vertices.v2fs[i].color);
         break;

      case GLHCK_VERTEX_V3F:
         for (i = 0; i != geometry->vertexCount; ++i)
            hash = HASHV3(hash,geometry->vertices.v3f[i].vertex);
            hash = HASHV3(hash,geometry->vertices.v3f[i].normal);
            hash = HASHV2(hash,geometry->vertices.v3f[i].coord);
            hash = HASHCB(hash,geometry->vertices.v3f[i].color);
         break;

      case GLHCK_VERTEX_V2F:
         for (i = 0; i != geometry->vertexCount; ++i)
            hash = HASHV2(hash,geometry->vertices.v2f[i].vertex);
            hash = HASHV3(hash,geometry->vertices.v2f[i].normal);
            hash = HASHV2(hash,geometry->vertices.v2f[i].coord);
            hash = HASHCB(hash,geometry->vertices.v2f[i].color);
         break;

      default:
         break;
   }

   return hash;
}
#undef HASH
#undef HASHV2
#undef HASHV3
#undef HASHCB

/* vim: set ts=8 sw=3 tw=0 :*/
