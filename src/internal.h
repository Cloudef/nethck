#ifndef __nethck_internal_h__
#define __nethck_internal_h__

#if defined(_nethck_c_)
#  define NETHCKGLOBAL
#else
#  define NETHCKGLOBAL extern
#endif

#include <glhck/glhck.h>
#include "glhck/nethck.h"
#include "helper/common.h"

#if defined(_glhck_c_)
   char _nethckInitialized = 0;
#else
   NETHCKGLOBAL char _nethckInitialized;
#endif

/* return variables used throughout library */
typedef enum _nethckReturnValue {
   RETURN_FAIL    =  0,
   RETURN_OK      =  1,
   RETURN_TRUE    =  1,
   RETURN_FALSE   =  !RETURN_TRUE
} _nethckReturnValue;

/* \brief Bams v3 */
typedef struct nethckVector3B {
   unsigned int x, y, z;
} nethckVector3B;

/* \brief Bams v2 */
typedef struct nethckVector2B {
   unsigned int x, y;
} nethckVector2B;

/* fifo types */
typedef enum _nethckFifoType {
   NETHCK_FIFO_TYPE_LISTEN,
   NETHCK_FIFO_TYPE_RELAY,
} _nethckFifoType;

typedef struct _nethckFifo {
   _nethckFifoType type;
   unsigned int fd;
} _nethckFifo;

/* bams conversions */
void _nethckV2FToBams(nethckVector2B *bv2, const glhckVector2f *v2);
void _nethckV3FToBams(nethckVector3B *bv3, const glhckVector3f *v3);
void _nethckBamsToV2F(glhckVector2f *v2, const nethckVector2B *bv2);
void _nethckBamsToV3F(glhckVector3f *v3, const nethckVector3B *bv3);

/* geometry helpers */
void _nethckGeometryVertexDataAndSize(glhckGeometry *geometry, void **data, size_t *size);
void _nethckGeometryIndexDataAndSize(glhckGeometry *geometry, void **data, size_t *size);

#endif /* __nethck_internal_h__ */
