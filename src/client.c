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

/* manage object packet */
static void _nethckClientManagePacketObject(unsigned char *data)
{
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
            printf("A packet of length %zu containing %s was received from %s on channel %u.\n",
                  event.packet->dataLength,
                  (char*)event.packet->data,
                  (char*)event.peer->data,
                  event.channelID);

            /* manage packet by kind */
            printf("ID: %d\n", ((nethckPacket*)event.packet->data)->type);
            switch (((nethckPacket*)event.packet->data)->type) {
               case NETHCK_PACKET_OBJECT:
                  _nethckClientManagePacketObject(event.packet->data);
                  break;

               default:
                  printf("A packet of length %zu containing %s was received from %s on channel %u.\n",
                     event.packet->dataLength,
                     (char*)event.packet->data,
                     (char*)event.peer->data,
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
}

/* vim: set ts=8 sw=3 tw=0 :*/
