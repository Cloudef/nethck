#ifndef __nethck_packets_h__
#define __nethck_packets_h__

/* \brief packet type enum */
typedef enum nethckPacketType {
   NETHCK_PACKET_OBJECT,
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
   unsigned int type;
   glhckGeometryVertexType vertexType;
   glhckGeometryIndexType indexType;
   size_t vertexCount;
   size_t indexCount;
   glhckVector3f bias;
   glhckVector3f scale;
   unsigned short textureRange;
} nethckGeometry;

/* \brief object packet */
typedef struct nethckObjectPacket {
   unsigned char type;
   struct nethckGeometry geometry;
   struct nethckView view;
   struct nethckMaterial material;
} nethckObjectPacket;

/* \brief generic packet */
typedef struct nethckPacket {
   unsigned char type;
} nethckPacket;

#endif /* __nethck_packets_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
