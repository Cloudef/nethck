#include "internal.h"
#include "packets.h"
#include <stdio.h>
#include <assert.h>    /* for assert */
#include <enet/enet.h> /* for enet   */
#include <string.h>

/* \brief local client struct */
typedef struct __NETHCKclient {
   ENetHost *enet;
   ENetPeer *peer;
} __NETHCKclient;
static __NETHCKclient _NETHCKclient;
static char _nethckClientInitialized = 0;

#define DEBUG(x, y, ...) printf(y"\n", ##__VA_ARGS__)
#define CALL(x, y, ...) ;
#define RET(x, y, ...) ;
#define TRACE(x) ;

/* \brief manage incoming object packet */
static void _nethckClientManagePacketObject(unsigned char *data)
{
   nethckObjectPacket *packet;
   unsigned char *offset;
   void *vdata = NULL, *idata = NULL;
   size_t vsize = 0, isize = 0;
   unsigned int tmp;
   glhckVector3f translation, rotation, scaling;
   glhckObject *object = NULL;
   glhckGeometry *geometry = NULL;

   if (!(object = glhckObjectNew()))
      goto fail;

   if (!(geometry = glhckObjectNewGeometry(object)))
      goto fail;

   offset  = data;
   packet  = (nethckObjectPacket*)data;
   offset += sizeof(nethckObjectPacket);

   geometry->type        = packet->geometry.type;
   geometry->vertexType  = packet->geometry.vertexType;
   geometry->indexType   = packet->geometry.indexType;
   geometry->vertexCount = packet->geometry.vertexCount;
   geometry->indexCount  = packet->geometry.indexCount;
   _nethckBamsToV3F(&geometry->bias, &packet->geometry.bias);
   _nethckBamsToV3F(&geometry->scale, &packet->geometry.scale);
   tmp = ntohl(packet->geometry.textureRange)<<16;
   geometry->textureRange = *((float*)&tmp);

   _nethckBamsToV3F(&translation, &packet->view.translation);
   _nethckBamsToV3F(&rotation, &packet->view.rotation);
   _nethckBamsToV3F(&scaling, &packet->view.scaling);

   vsize = geometry->vertexCount * glhckVertexTypeElementSize(geometry->vertexType);
   if (!(vdata = malloc(vsize)))
      goto fail;

   memcpy(vdata, offset, vsize); offset += vsize;
   glhckGeometrySetVertices(geometry, geometry->vertexType, vdata, geometry->vertexCount);
   NULLDO(free, vdata);

   isize = geometry->indexCount * glhckIndexTypeElementSize(geometry->indexType);
   if (!(idata = malloc(isize)))
      goto fail;

   memcpy(idata, offset, isize); offset += isize;
   glhckGeometrySetIndices(geometry, geometry->indexType, idata, geometry->indexCount);
   NULLDO(free, idata);

   glhckObjectScale(object, (kmVec3*)&scaling);
   glhckObjectPosition(object, (kmVec3*)&translation);
   glhckObjectRotation(object, (kmVec3*)&rotation);
   glhckObjectColor(object, &packet->material.color);

   glhckObjectUpdate(object);
   glhckObjectDraw(object);

   printf("-- Echo %p from Server -->\n", packet);
   printf("[] Geometry type: %u\n", geometry->type);
   printf("[] Vertex type: %d\n", geometry->vertexType);
   printf("[] Index type: %d\n", geometry->indexType);
   printf("[] Vertex count: %zu\n", geometry->vertexCount);
   printf("[] Index count: %zu\n", geometry->indexCount);
   printf("[] Bias: "VEC3S"\n", VEC3(&geometry->bias));
   printf("[] Scale: "VEC3S"\n", VEC3(&geometry->scale));
   printf("[] Texture range: %.0f\n", geometry->textureRange);
   printf("[] Translation: "VEC3S"\n", VEC3(glhckObjectGetPosition(object)));
   printf("[] Rotation: "VEC3S"\n", VEC3(glhckObjectGetRotation(object)));
   printf("[] Scaling: "VEC3S"\n", VEC3(glhckObjectGetScale(object)));
   printf("[] Color: "COLBS"\n", COLB(glhckObjectGetColor(object)));
   printf("<---\n");

   glhckObjectFree(object);
   return;

fail:
   IFDO(glhckObjectFree, object);
   IFDO(free, vdata);
   IFDO(free, idata);
}

/* \brief initialize enet internally */
static int _nethckEnetInit(const char *host, int port)
{
   ENetEvent event;
   ENetAddress address;
   CALL(0, "%s, %d", host, port);

   /* initialize enet */
   if (enet_initialize() != 0)
      goto enet_init_fail;

   /* set host parameters */
   address.host = ENET_HOST_ANY;
   address.port = port;

   /* set host address, if specified */
   if (host) enet_address_set_host(&address, host);
   else      enet_address_set_host(&address, "127.0.0.1");

   /* create client */
   _NETHCKclient.enet = enet_host_create(NULL,
         1     /* outgoing connections */,
         1     /* max channels */,
         0     /* download bandwidth */,
         0     /* upload bandwidth */);

   if (!_NETHCKclient.enet)
      goto enet_client_fail;

   /* initialize connection */
   _NETHCKclient.peer = enet_host_connect(_NETHCKclient.enet,
         &address, 1, 0);

   if (!_NETHCKclient.peer)
      goto enet_client_init_fail;

   if (enet_host_service(_NETHCKclient.enet, &event, 5000) <= 0 ||
         event.type != ENET_EVENT_TYPE_CONNECT)
      goto fail;

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

enet_init_fail:
   DEBUG(NETHCK_DBG_ERROR, "Failed to initialize ENet");
   goto fail;
enet_client_fail:
   DEBUG(NETHCK_DBG_ERROR, "Failed to create ENet client");
   goto fail;
enet_client_init_fail:
   DEBUG(NETHCK_DBG_ERROR, "No available peers for iniating an ENet connection");
fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief disconnect enet internally */
static void _nethckEnetDisconnect(void)
{
   ENetEvent event;
   TRACE(0);

   if (!_NETHCKclient.peer)
      return;

   if (!_nethckClientInitialized)
      goto force_disconnect;

   /* soft disconnect */
   enet_peer_disconnect(_NETHCKclient.peer, 0);
   while (enet_host_service(_NETHCKclient.enet, &event, 3000) > 0) {
      switch (event.type) {
         case ENET_EVENT_TYPE_RECEIVE:
            enet_packet_destroy(event.packet);
            break;

         case ENET_EVENT_TYPE_DISCONNECT:
            DEBUG(NETHCK_DBG_CRAP, "Disconnected from server");
            return;

         default:
            break;
      }
   }

force_disconnect:
   /* force disconnect */
   enet_peer_reset(_NETHCKclient.peer);
   DEBUG(NETHCK_DBG_CRAP, "Disconnected from server with force");
}

/* \brief destroy enet internally */
static void _nethckEnetDestroy(void)
{
   TRACE(0);

   if (!_NETHCKclient.enet)
      return;

   /* disconnect && kill */
   _nethckEnetDisconnect();
   enet_host_destroy(_NETHCKclient.enet);
   _NETHCKclient.enet = NULL;
   _NETHCKclient.peer = NULL;
}

/* \brief update enet state internally */
static int _nethckEnetUpdate(void)
{
   ENetEvent event;
   int packets = 0;
   TRACE(2);

   /* check events */
   while (enet_host_service(_NETHCKclient.enet, &event, 0) > 0) {
      switch (event.type) {
         case ENET_EVENT_TYPE_RECEIVE:
            printf("A packet of length %zu containing %s was received from server on channel %u.\n",
                  event.packet->dataLength,
                  (char*)event.packet->data,
                  event.channelID);

            /* manage packet by kind */
            printf("ID: %d\n", ((nethckPacket*)event.packet->data)->type);
            switch (((nethckPacket*)event.packet->data)->type) {
               case NETHCK_PACKET_OBJECT:
                  _nethckClientManagePacketObject(event.packet->data);
                  break;

               default:
                  printf("A packet of length %zu containing %s was received from server on channel %u.\n",
                     event.packet->dataLength,
                     (char*)event.packet->data,
                     event.channelID);
                  break;
            }

            /* Clean up the packet now that we're done using it. */
            enet_packet_destroy(event.packet);
            break;

         case ENET_EVENT_TYPE_DISCONNECT:
            printf("%s disconected.\n", (char*)event.peer->data);

            /* Reset the peer's client information. */
            event.peer->data = NULL;
            break;

         default:
            break;
      }
      ++packets;
   }

   RET(2, "%d", packets);
   return packets;
}

/* \brief send packet */
static void _nethckEnetSend(unsigned char *data, size_t size)
{
   ENetPacket *packet;
   packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
   enet_peer_send(_NETHCKclient.peer, 0, packet);
   enet_host_flush(_NETHCKclient.enet);
}

/* public api */

/* \brief initialize nethck server */
NETHCKAPI int nethckClientInit(const char *host, int port)
{
   CALL(0, "%s, %d", host, port);

   /* reinit, if initialized */
   if (_nethckClientInitialized)
      nethckClientKill();

   /* null struct */
   memset(&_NETHCKclient, 0, sizeof(__NETHCKclient));

   /* initialize enet */
   if (_nethckEnetInit(host, port) != RETURN_OK)
      goto fail_connect;

   _nethckClientInitialized = 1;
   DEBUG(NETHCK_DBG_CRAP, "Connected to server [%s:%d]",
         host?host:"127.0.0.1", port);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail_connect:
   DEBUG(NETHCK_DBG_ERROR, "Failed to connect to server [%s:%d]",
         host?host:"127.0.0.1", port);
fail:
   _nethckEnetDestroy();
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief kill nethck server */
NETHCKAPI void nethckClientKill(void)
{
   TRACE(0);

   /* is initialized? */
   if (!_nethckClientInitialized)
      return;

   /* kill enet */
   _nethckEnetDestroy();

   _nethckClientInitialized = 0;
   DEBUG(NETHCK_DBG_CRAP, "Killed ENet client");
}

/* \brief update server state */
NETHCKAPI int nethckClientUpdate(void)
{
   TRACE(2);

   /* is initialized? */
   if (!_nethckClientInitialized)
      return -1;

   /* updat enet */
   return _nethckEnetUpdate();
}

/* \brief 'render' object to network */
NETHCKAPI void nethckClientObjectRender(const glhckObject *object)
{
   glhckGeometry *geometry;
   nethckObjectPacket *packet;
   unsigned char *data = NULL, *offset;
   void *vdata = NULL, *idata = NULL;
   size_t size, vsize = 0, isize = 0;

   if (!(geometry = glhckObjectGetGeometry(object)))
      goto fail;

   size = sizeof(nethckObjectPacket);

   _nethckGeometryVertexDataAndSize(geometry, &vdata, &vsize);
   _nethckGeometryVertexDataAndSize(geometry, &idata, &isize);
   size += vsize + isize;

   if (!(data = malloc(size)))
      goto fail;

   offset  = data;
   packet  = (nethckObjectPacket*)data;
   offset += sizeof(nethckObjectPacket);

   packet->type = NETHCK_PACKET_OBJECT;
   packet->geometry.type        = geometry->type;
   packet->geometry.vertexType  = geometry->vertexType;
   packet->geometry.indexType   = geometry->indexType;
   packet->geometry.vertexCount = geometry->vertexCount;
   packet->geometry.indexCount  = geometry->indexCount;
   _nethckV3FToBams(&packet->geometry.scale, &geometry->scale);
   _nethckV3FToBams(&packet->geometry.bias, &geometry->bias);
   packet->geometry.textureRange = htonl(*((unsigned int*)&geometry->textureRange)>>16);

   _nethckV3FToBams(&packet->view.translation, (glhckVector3f*)glhckObjectGetPosition(object));
   _nethckV3FToBams(&packet->view.rotation, (glhckVector3f*)glhckObjectGetRotation(object));
   _nethckV3FToBams(&packet->view.scaling, (glhckVector3f*)glhckObjectGetScale(object));
   memcpy(&packet->material.color, glhckObjectGetColor(object), sizeof(glhckColorb));

   printf("-- Echo %p to Server -->\n", object);
   printf("[] Geometry type: %u\n", geometry->type);
   printf("[] Vertex type: %d\n", geometry->vertexType);
   printf("[] Index type: %d\n", geometry->indexType);
   printf("[] Vertex count: %zu\n", geometry->vertexCount);
   printf("[] Index count: %zu\n", geometry->indexCount);
   printf("[] Bias: "VEC3S"\n", VEC3(&geometry->bias));
   printf("[] Scale: "VEC3S"\n", VEC3(&geometry->scale));
   printf("[] Texture range: %.0f\n", geometry->textureRange);
   printf("[] Translation: "VEC3S"\n", VEC3(glhckObjectGetPosition(object)));
   printf("[] Rotation: "VEC3S"\n", VEC3(glhckObjectGetRotation(object)));
   printf("[] Scaling: "VEC3S"\n", VEC3(glhckObjectGetScale(object)));
   printf("[] Color: "COLBS"\n", COLB(glhckObjectGetColor(object)));
   printf("<---\n");

   memcpy(offset, vdata, vsize); offset += vsize;
   memcpy(offset, idata, isize); offset += isize;

   _nethckEnetSend(data, size);
   NULLDO(free, data);
   return;

fail:
   IFDO(free, data);
}

/* vim: set ts=8 sw=3 tw=0 :*/