#ifndef __nethck_internal_h__
#define __nethck_internal_h__

#include <glhck/glhck.h>
#include "glhck/nethck.h"
#include "helper/common.h"

/* return variables used throughout library */
typedef enum _nethckReturnValue {
   RETURN_FAIL    =  0,
   RETURN_OK      =  1,
   RETURN_TRUE    =  1,
   RETURN_FALSE   =  !RETURN_TRUE
} _nethckReturnValue;

/* geometry helpers */
void _nethckGeometryVertexDataAndSize(glhckGeometry *geometry, void **data, size_t *size);
void _nethckGeometryIndexDataAndSize(glhckGeometry *geometry, void **data, size_t *size);

#endif /* __nethck_internal_h__ */
