#include "internal.h"
#include "packets.h"
#include <stdio.h>
#include <assert.h>    /* for assert */
#include <enet/enet.h> /* for enet   */
#include <string.h>

/* \brief local server struct */
typedef struct __NETHCKserver {
   ENetHost *enet;
} __NETHCKserver;
static __NETHCKserver _NETHCKserver;
static char _nethckServerInitialized = 0;

#define DEBUG(x, y, ...) printf(y"\n", ##__VA_ARGS__)
#define CALL(x, y, ...) ;
#define RET(x, y, ...) ;
#define TRACE(x) ;

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
}

/* \brief update enet state internally */
static int _nethckEnetUpdate(void)
{
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

            /* Store any relevant client information here. */
            event.peer->data = "Client information";
            break;

         case ENET_EVENT_TYPE_RECEIVE:
            /* manage packet by kind */
            printf("ID: %d\n", ((nethckPacket*)event.packet->data)->type);
            switch (((nethckPacket*)event.packet->data)->type) {
               case NETHCK_PACKET_OBJECT:
                  _nethckServerManagePacketObject(event.packet->data);
                  break;

               default:
                  printf("A packet of length %zu containing %s was received from %s on channel %u.\n",
                     event.packet->dataLength,
                     (char*)event.packet->data,
                     (char*)event.peer->data,
                     event.channelID);
                  break;
            }

            /* Echo the packet to clients */
            enet_host_broadcast(_NETHCKserver.enet, 0, event.packet);

            /* Clean up the packet now that we're done using it. */
            //enet_packet_destroy(event.packet);
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

   RET(2, "%u", packets);
   return packets;
}

/* public api */

/* \brief initialize nethck server */
NETHCKAPI int nethckServerInit(const char *host, int port)
{
   CALL(0, "%s, %d", host, port);

   /* reinit, if initialized */
   if (_nethckServerInitialized)
      nethckServerKill();

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
NETHCKAPI void nethckServerKill(void)
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

