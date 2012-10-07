#ifndef __nethck_packets_h__
#define __nethck_packets_h__

/* \brief packet type enum */
typedef enum nethckPacketType {
   NETHCK_PACKET_OBJECT,
} _nethckPacketType;

/* \brief representation of object's view */
typedef struct nethckView {
   nethckVector3B translation, target, rotation, scaling;
} nethckView;

/* \brief representation of object's material */
typedef struct nethckMaterial {
   glhckColorb color;
} nethckMaterial;

/* \brief object packet */
typedef struct nethckObjectPacket {
   unsigned char type;
   struct glhckGeometry geometry;
   struct nethckView view;
   struct nethckMaterial material;
} nethckObjectPacket;

/* \brief generic packet */
typedef struct nethckPacket {
   unsigned char type;
} nethckPacket;

#endif /* __nethck_packets_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
