#include "internal.h"
#include "packets.h"
#include <stdio.h>
#include <assert.h>    /* for assert */
#include <enet/enet.h> /* for enet   */
#include <string.h>

/* \brief object wrapper struct */
typedef struct nethckObject {
   unsigned int id;
   glhckObject *object;
   struct nethckObject *next;
} nethckObject;

/* \brief local client struct */
typedef struct __NETHCKclient {
   ENetHost *enet;
   ENetPeer *peer;
   nethckObject *objects;
   glhckObject **objectList;
} __NETHCKclient;
static __NETHCKclient _NETHCKclient;
static char _nethckClientInitialized = 0;

#define DEBUG(x, y, ...) printf(y"\n", ##__VA_ARGS__)
#define CALL(x, y, ...) ;
#define RET(x, y, ...) ;
#define TRACE(x) ;

/* \brief get object with id from tracking list */
static nethckObject* _nethckTrackObjectForId(unsigned int id)
{
   nethckObject *o;
   for (o = _NETHCKclient.objects; o; o = o->next)
      if (o->id == id) return o;
   return NULL;
}

/* \brief add object with id to tracking list */
static int _nethckTrackObject(unsigned int id, glhckObject *object)
{
   nethckObject *o;

   /* don't add duplicates */
   if (_nethckTrackObjectForId(id))
      return RETURN_OK;

   /* add to tracking list */
   for (o = _NETHCKclient.objects; o && o->next; o = o->next);
   if (!o) o = _NETHCKclient.objects = calloc(1, sizeof(nethckObject));
   else o = o->next = calloc(1, sizeof(nethckObject));
   if (!o) return RETURN_FAIL;

   o->id = id;
   o->object = glhckObjectRef(object);
   return RETURN_OK;
}

/* \brief manage incoming object packet */
static void _nethckClientManagePacketObject(unsigned char *data)
{
   nethckObjectPacket *packet;
   unsigned char *offset;
   void *vdata = NULL, *idata = NULL;
   size_t vsize = 0, isize = 0;
   glhckObject *object = NULL;
   glhckGeometry *geometry = NULL;

   offset  = data;
   packet  = (nethckObjectPacket*)data;
   offset += sizeof(nethckObjectPacket);

   if ((object = nethckClientObjectForId(packet->id)))
      glhckObjectRef(object);

   if (!object && !(object = glhckObjectNew()))
      goto fail;

   if (!(geometry = glhckObjectGetGeometry(object)) && !(geometry = glhckObjectNewGeometry(object)))
      goto fail;

   if (_nethckTrackObject(packet->id, object) != RETURN_OK)
      goto fail;

   geometry->type        = packet->geometry.type;
   geometry->vertexType  = packet->geometry.vertexType;
   geometry->indexType   = packet->geometry.indexType;
   geometry->vertexCount = packet->geometry.vertexCount;
   geometry->indexCount  = packet->geometry.indexCount;
   memcpy(&geometry->bias, &packet->geometry.bias, sizeof(glhckVector3f));
   memcpy(&geometry->scale, &packet->geometry.scale, sizeof(glhckVector3f));
   geometry->textureRange = packet->geometry.textureRange;

   vsize = geometry->vertexCount * glhckVertexTypeElementSize(geometry->vertexType);
   if (!(vdata = malloc(vsize)))
      goto fail;

   memcpy(vdata, offset, vsize); offset += vsize;
   glhckGeometrySetVertices(geometry, geometry->vertexType, vdata, geometry->vertexCount);
   NULLDO(free, vdata);

   if (geometry->indexCount) {
      isize = geometry->indexCount * glhckIndexTypeElementSize(geometry->indexType);
      if (!(idata = malloc(isize)))
         goto fail;

      memcpy(idata, offset, isize); offset += isize;
      glhckGeometrySetIndices(geometry, geometry->indexType, idata, geometry->indexCount);
      NULLDO(free, idata);
   }

   glhckObjectScale(object, (kmVec3*)&packet->view.scaling);
   glhckObjectPosition(object, (kmVec3*)&packet->view.translation);
   glhckObjectRotation(object, (kmVec3*)&packet->view.rotation);
   glhckObjectColor(object, &packet->material.color);
   glhckObjectUpdate(object);
   glhckObjectFree(object);

#if 1
   printf("-- Echo %p from Server -->\n", packet);
   printf("[] ID: %u\n", packet->id);
   printf("[] Geometry type: %u\n", geometry->type);
   printf("[] Vertex type: %d\n", geometry->vertexType);
   printf("[] Index type: %d\n", geometry->indexType);
   printf("[] Vertex count: %d\n", geometry->vertexCount);
   printf("[] Index count: %d\n", geometry->indexCount);
   printf("[] Bias: "VEC3S"\n", VEC3(&geometry->bias));
   printf("[] Scale: "VEC3S"\n", VEC3(&geometry->scale));
   printf("[] Texture range: %u\n", geometry->textureRange);
   printf("[] Translation: "VEC3S"\n", VEC3(glhckObjectGetPosition(object)));
   printf("[] Rotation: "VEC3S"\n", VEC3(glhckObjectGetRotation(object)));
   printf("[] Scaling: "VEC3S"\n", VEC3(glhckObjectGetScale(object)));
   printf("[] Color: "COLBS"\n", COLB(glhckObjectGetColor(object)));
   printf("<---\n");
#endif
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
#if 0
            printf("A packet of length %zu containing %s was received from server on channel %u.\n",
                  event.packet->dataLength,
                  (char*)event.packet->data,
                  event.channelID);

            /* manage packet by kind */
            printf("ID: %d\n", ((nethckPacket*)event.packet->data)->type);
#endif
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

/* \brief send object packet */
static void nethckClientSendObjectPacket(nethckObjectPacket *packet,
      void *vdata, size_t vsize, void *idata, size_t isize)
{
   size_t size;
   unsigned char *data = NULL, *offset;
   assert(packet);

   size = sizeof(nethckObjectPacket) + vsize + isize;
   if (!(offset = data = malloc(size)))
      goto fail;

   packet->type = NETHCK_PACKET_OBJECT;
   memcpy(offset, packet, sizeof(nethckObjectPacket)); offset += sizeof(nethckObjectPacket);
   memcpy(offset, vdata, vsize); offset += vsize;
   memcpy(offset, idata, isize); offset += isize;

   _nethckEnetSend(data, size);
   NULLDO(free, data);
   return;

fail:
   IFDO(free, data);
}

/* public api */

/* \brief initialize nethck server */
NETHCKAPI int nethckClientCreate(const char *host, int port)
{
   CALL(0, "%s, %d", host, port);

   /* reinit, if initialized */
   if (_nethckClientInitialized)
      nethckClientTerminate();

   /* null struct */
   memset(&_NETHCKclient, 0, sizeof(__NETHCKclient));

   /* initialize enet */
   if (_nethckEnetInit(host, port) != RETURN_OK)
      goto fail;

   _nethckClientInitialized = 1;
   DEBUG(NETHCK_DBG_CRAP, "Connected to server [%s:%d]",
         host?host:"127.0.0.1", port);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   DEBUG(NETHCK_DBG_ERROR, "Failed to connect to server [%s:%d]",
         host?host:"127.0.0.1", port);
   _nethckEnetDestroy();
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief kill nethck server */
NETHCKAPI void nethckClientTerminate(void)
{
   nethckObject *o, *on;
   TRACE(0);

   /* is initialized? */
   if (!_nethckClientInitialized)
      return;

   /* free objects */
   for (o = _NETHCKclient.objects; o; o = on) {
      on = o->next;
      glhckObjectFree(o->object);
      free(o);
   }
   IFDO(free, _NETHCKclient.objectList);

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

/* \brief 'import' object to network */
NETHCKAPI void nethckClientImportObject(nethckImportObject *import)
{
   nethckObjectPacket packet;
   size_t vsize, isize;

   packet.geometry.type         = import->geometry.type;
   packet.geometry.vertexType   = GLHCK_VERTEX_V3F;
   packet.geometry.indexType    = GLHCK_INDEX_INTEGER;
   packet.geometry.vertexCount  = import->geometry.vertexCount;
   packet.geometry.indexCount   = import->geometry.indexCount;
   packet.geometry.textureRange = 1;

   memcpy(&packet.view.translation, &import->view.translation, sizeof(glhckVector3f));
   memcpy(&packet.view.rotation, &import->view.rotation, sizeof(glhckVector3f));
   memcpy(&packet.view.scaling, &import->view.scaling, sizeof(glhckVector3f));
   memcpy(&packet.material.color, &import->material.color, sizeof(glhckColorb));

   vsize = packet.geometry.vertexCount * glhckVertexTypeElementSize(packet.geometry.vertexType);
   isize = packet.geometry.indexCount * glhckIndexTypeElementSize(packet.geometry.indexType);
   nethckClientSendObjectPacket(&packet,
         import->geometry.vertexData, vsize,
         import->geometry.indexData, isize);
}

/* \brief return objects in network */
NETHCKAPI glhckObject** nethckClientObjects(unsigned int *objectCount)
{
   nethckObject *o;
   glhckObject **objects;
   unsigned int count = 0, index = 0;

   IFDO(free, _NETHCKclient.objectList);
   for (o = _NETHCKclient.objects; o; o = o->next)
      ++count;

   if (count) objects = calloc(count, sizeof(glhckObject*));
   for (o = _NETHCKclient.objects; o; o = o->next,++index)
      objects[index] = o->object;

   if (objectCount) *objectCount = count;
   _NETHCKclient.objectList = objects;
   return objects;
}

/* \brief return 'rendered' object for id */
NETHCKAPI glhckObject* nethckClientObjectForId(unsigned int id)
{
   nethckObject *o;
   o = _nethckTrackObjectForId(id);
   if (o) return o->object;
   return NULL;
}

/* \brief 'render' object to network */
NETHCKAPI void nethckClientObjectRender(unsigned int id, glhckObject *object)
{
   glhckGeometry *geometry;
   nethckObjectPacket packet;
   void *vdata = NULL, *idata = NULL;
   size_t vsize = 0, isize = 0;
   assert(object);

   if (_nethckTrackObject(id, object) != RETURN_OK)
      return;

   if (!(geometry = glhckObjectGetGeometry(object)))
      return;

   _nethckGeometryVertexDataAndSize(geometry, &vdata, &vsize);
   _nethckGeometryIndexDataAndSize(geometry, &idata, &isize);

   packet.id = id;
   packet.geometry.type        = geometry->type;
   packet.geometry.vertexType  = geometry->vertexType;
   packet.geometry.indexType   = geometry->indexType;
   packet.geometry.vertexCount = geometry->vertexCount;
   packet.geometry.indexCount  = geometry->indexCount;
   memcpy(&packet.geometry.scale, &geometry->scale, sizeof(glhckVector3f));
   memcpy(&packet.geometry.bias, &geometry->bias, sizeof(glhckVector3f));
   packet.geometry.textureRange = geometry->textureRange;

   memcpy(&packet.view.translation, (glhckVector3f*)glhckObjectGetPosition(object), sizeof(glhckVector3f));
   memcpy(&packet.view.rotation, (glhckVector3f*)glhckObjectGetRotation(object), sizeof(glhckVector3f));
   memcpy(&packet.view.scaling, (glhckVector3f*)glhckObjectGetScale(object), sizeof(glhckVector3f));
   memcpy(&packet.material.color, glhckObjectGetColor(object), sizeof(glhckColorb));

#if 0
   printf("-- Echo %p to Server -->\n", object);
   printf("[] ID: %u\n", id);
   printf("[] Geometry type: %u\n", geometry->type);
   printf("[] Vertex type: %d\n", geometry->vertexType);
   printf("[] Index type: %d\n", geometry->indexType);
   printf("[] Vertex count: %zu\n", geometry->vertexCount);
   printf("[] Index count: %zu\n", geometry->indexCount);
   printf("[] Bias: "VEC3S"\n", VEC3(&geometry->bias));
   printf("[] Scale: "VEC3S"\n", VEC3(&geometry->scale));
   printf("[] Texture range: %u\n", geometry->textureRange);
   printf("[] Translation: "VEC3S"\n", VEC3(glhckObjectGetPosition(object)));
   printf("[] Rotation: "VEC3S"\n", VEC3(glhckObjectGetRotation(object)));
   printf("[] Scaling: "VEC3S"\n", VEC3(glhckObjectGetScale(object)));
   printf("[] Color: "COLBS"\n", COLB(glhckObjectGetColor(object)));
   printf("<---\n");
#endif

   /* send */
   nethckClientSendObjectPacket(&packet, vdata, vsize, idata, isize);
}

/* vim: set ts=8 sw=3 tw=0 :*/
