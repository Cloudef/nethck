#ifndef __nethck_packets_h__
#define __nethck_packets_h__

/* \brief packet type enum */
typedef enum nethckPacketType {
   NETHCK_PACKET_OBJECT,
   NETHCK_PACKET_OBJECT_TRANSLATION,
   NETHCK_PACKET_OBJECT_TEXTURE,
} _nethckPacketType;

/* \brief representation of object's material */
typedef struct nethckMaterial {
   glhckColorb color;
} nethckMaterial;

/* \brief representation of object's view */
typedef struct nethckView {
   glhckVector3f translation, rotation, scaling;
} nethckView;

/* \brief represntation of object's geometry */
typedef struct nethckGeometry {
   glhckVector3f bias;
   glhckVector3f scale;
   int vertexCount, indexCount;
   int textureRange;
   glhckGeometryType type;
   glhckGeometryVertexType vertexType;
   glhckGeometryIndexType indexType;
} nethckGeometry;

/* \brief full object packet */
typedef struct nethckObjectPacket {
   unsigned char type;
   struct nethckGeometry geometry;
   struct nethckView view;
   struct nethckMaterial material;
   unsigned int id;
} nethckObjectPacket;

/* \brief object translation packet */
typedef struct nethckObjectTranslationPacket {
   unsigned char type;
   struct nethckView view;
   unsigned int id;
} nethckObjectTranslationPacket;

typedef struct nethckObjectTexturePacket {
   unsigned char type;
   glhckTextureTarget target;
   glhckTextureFormat format;
   glhckDataType dataType;
   int width, height, depth;
   int size;
   unsigned int id;
} nethckObjectTexturePacket;

/* \brief generic packet */
typedef struct nethckPacket {
   unsigned char type;
} nethckPacket;

#endif /* __nethck_packets_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
