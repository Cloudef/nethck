#include "internal.h"
#include "packets.h"
#include <stdio.h>
#include <assert.h>    /* for assert */
#include <enet/enet.h> /* for enet   */
#include <string.h>

typedef struct __NETHCKclient {
   char host[46];
   struct __NETHCKclient *next;
} __NETHCKclient;

/* \brief local server struct */
typedef struct __NETHCKserver {
   ENetHost *enet;
   __NETHCKclient *clients;
} __NETHCKserver;
static __NETHCKserver _NETHCKserver;
static char _nethckServerInitialized = 0;

#define DEBUG(x, y, ...) printf(y"\n", ##__VA_ARGS__)
#define CALL(x, y, ...) ;
#define RET(x, y, ...) ;
#define TRACE(x) ;

/* \brief add new client */
static __NETHCKclient* _nethckServerNewClient(__NETHCKclient *params)
{
   __NETHCKclient *c;

   /* add to list */
   for (c = _NETHCKserver.clients; c && c->next; c = c->next);
   if (c) c = c->next = malloc(sizeof(__NETHCKclient));
   else c = _NETHCKserver.clients = malloc(sizeof(__NETHCKclient));

   memcpy(c, params, sizeof(__NETHCKclient));
   return c;
}

/* \brief free client */
static void _nethckServerFreeClient(__NETHCKclient *client)
{
   __NETHCKclient *c;

   /* remove from list */
   for (c = _NETHCKserver.clients; c != client && c->next != client; c = c->next);
   if (c == client) _NETHCKserver.clients = client->next;
   else if (c) c->next = client->next;

   free(client);
}

/* \brief initialize enet internally */
static int _nethckEnetInit(const char *host, int port)
{
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

   /* create host */
   _NETHCKserver.enet = enet_host_create(&address,
         32    /* max clients */,
         1     /* max channels */,
         0     /* download bandwidth */,
         0     /* upload bandwidth */);

   if (!_NETHCKserver.enet)
      goto enet_host_fail;

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

enet_init_fail:
   DEBUG(NETHCK_DBG_ERROR, "Failed to initialize ENet");
   goto fail;
enet_host_fail:
   DEBUG(NETHCK_DBG_ERROR, "Failed to create ENet host");
fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief destroy enet internally */
static void _nethckEnetDestroy(void)
{
   TRACE(0);

   if (!_NETHCKserver.enet)
      return;

   /* kill */
   enet_host_destroy(_NETHCKserver.enet);
   _NETHCKserver.enet = NULL;
}

static void _nethckServerManagePacketObject(unsigned char *data)
{
   nethckObjectPacket *packet = (nethckObjectPacket*)data;
   unsigned int tmp;

   printf("-- Echo %p from Client -->\n", packet);
   printf("[] Geometry type: %u\n", packet->geometry.type);
   printf("[] Vertex type: %d\n", packet->geometry.vertexType);
   printf("[] Index type: %d\n", packet->geometry.indexType);
   printf("[] Vertex count: %zu\n", packet->geometry.vertexCount);
   printf("[] Index count: %zu\n", packet->geometry.indexCount);
   printf("[] Bias: "VEC3S"\n", VEC3(&packet->geometry.bias));
   printf("[] Scale: "VEC3S"\n", VEC3(&packet->geometry.scale));
   printf("[] Texture range: %u\n", packet->geometry.textureRange);
   printf("[] Translation: "VEC3S"\n", VEC3(&packet->view.translation));
   printf("[] Rotation: "VEC3S"\n", VEC3(&packet->view.rotation));
   printf("[] Scaling: "VEC3S"\n", VEC3(&packet->view.scaling));
   printf("[] Color: "COLBS"\n", COLB(&packet->material.color));
   printf("<---\n");
}

/* \brief update enet state internally */
static int _nethckEnetUpdate(void)
{
   __NETHCKclient client;
   ENetEvent event;
   unsigned int packets = 0;
   TRACE(2);

   /* wait up to 1000 milliseconds for an event */
   while (enet_host_service(_NETHCKserver.enet, &event, 1000) > 0) {
      switch (event.type) {
         case ENET_EVENT_TYPE_CONNECT:
            printf("A new client connected from %x:%u.\n",
                  event.peer->address.host,
                  event.peer->address.port);

            /* fill new client struct */
            memset(&client, 0, sizeof(__NETHCKclient));
            enet_address_get_host_ip(&event.peer->address, client.host, sizeof(client.host));
            event.peer->data = _nethckServerNewClient(&client);
            break;

         case ENET_EVENT_TYPE_RECEIVE:
            /* manage packet by kind */
            printf("A packet of length %zu was received from %s on channel %u.\n",
                  event.packet->dataLength,
                  ((__NETHCKclient*)event.peer->data)->host,
                  event.channelID);
            printf("ID: %d\n", ((nethckPacket*)event.packet->data)->type);
            switch (((nethckPacket*)event.packet->data)->type) {
               case NETHCK_PACKET_OBJECT:
                  _nethckServerManagePacketObject(event.packet->data);
                  break;

               default:
                  printf("A packet of length %zu containing %s was received from %s on channel %u.\n",
                     event.packet->dataLength,
                     (char*)event.packet->data,
                     ((__NETHCKclient*)event.peer->data)->host,
                     event.channelID);
                  break;
            }

            /* echo the packet to clients */
            enet_host_broadcast(_NETHCKserver.enet, 0, event.packet);

            /* clean up the packet now that we're done using it. */
            //enet_packet_destroy(event.packet);
            break;

         case ENET_EVENT_TYPE_DISCONNECT:
            printf("%s disconected.\n",
                  ((__NETHCKclient*)event.peer->data)->host);

            /* free the client */
            _nethckServerFreeClient(event.peer->data);
            event.peer->data = NULL;
            break;

         default:
            break;
      }
      ++packets;
   }

   RET(2, "%u", packets);
   return packets;
}

/* public api */

/* \brief initialize nethck server */
NETHCKAPI int nethckServerCreate(const char *host, int port)
{
   CALL(0, "%s, %d", host, port);

   /* reinit, if initialized */
   if (_nethckServerInitialized)
      nethckServerTerminate();

   /* null struct */
   memset(&_NETHCKserver, 0, sizeof(__NETHCKserver));

   /* initialize enet */
   if (_nethckEnetInit(host, port) != RETURN_OK)
      goto fail_host;

   _nethckServerInitialized = 1;
   DEBUG(NETHCK_DBG_CRAP, "Started ENet server [%s:%d]",
         host?host:"0.0.0.0", port);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail_host:
   DEBUG(NETHCK_DBG_ERROR, "Failed to create ENet server [%s:%d]",
         host?host:"0.0.0.0", port);
fail:
   _nethckEnetDestroy();
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief kill nethck server */
NETHCKAPI void nethckServerTerminate(void)
{
   TRACE(0);

   /* is initialized? */
   if (!_nethckServerInitialized)
      return;

   /* kill enet */
   _nethckEnetDestroy();

   _nethckServerInitialized = 0;
   DEBUG(NETHCK_DBG_CRAP, "Killed ENet server");
}

/* \brief update server state */
NETHCKAPI int nethckServerUpdate(void)
{
   TRACE(2);

   /* is initialized? */
   if (!_nethckServerInitialized)
      return -1;

   /* updat enet */
   return _nethckEnetUpdate();
}

/* \brief get number of connected clients */
NETHCKAPI unsigned int nethckServerClientCount(void)
{
   unsigned int cnt;
   __NETHCKclient *c;
   TRACE(2);

   /* is initialized? */
   if (!_nethckServerInitialized)
      return 0;

   for (cnt = 0, c = _NETHCKserver.clients; c; c = c->next, ++cnt);
   return cnt;
}

