#include "internal.h"
#include <glhck/glhck.h>

/* \brief float vector to bams vector */
void _nethckV3FToBams(glhckVector3s *bv3, const glhckVector3f *v3)
{
   bv3->x = htonl(*((unsigned int*)&v3->x)>>16);
   bv3->y = htonl(*((unsigned int*)&v3->y)>>16);
   bv3->z = htonl(*((unsigned int*)&v3->z)>>16);
}

/* \brief bams vector to float vector */
void _nethckBamsToV3F(glhckVector3f *v3, const glhckVector3s *bv3)
{
   unsigned int x = ntohl(bv3->x)<<16;
   unsigned int y = ntohl(bv3->y)<<16;
   unsigned int z = ntohl(bv3->z)<<16;
   v3->x = *((float*)&x);
   v3->y = *((float*)&y);
   v3->z = *((float*)&z);
}

/* \brief init nethck */
NETHCKAPI int nethckInit(int argc, char **argv)
{
   return RETURN_OK;
}

/* vim: set ts=8 sw=3 tw=0 :*/
