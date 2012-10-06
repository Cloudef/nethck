#ifndef __nethck_internal_h__
#define __nethck_internal_h__

#if defined(_nethck_c_)
#  define NETHCKGLOBAL
#else
#  define NETHCKGLOBAL extern
#endif

#include "glhck/nethck.h"

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

#endif /* __nethck_internal_h__ */
