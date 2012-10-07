#include "internal.h"
#include <glhck/glhck.h>
#include <arpa/inet.h>

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

/* \brief init nethck */
NETHCKAPI int nethckInit(int argc, char **argv)
{
   return RETURN_OK;
}

/* vim: set ts=8 sw=3 tw=0 :*/
