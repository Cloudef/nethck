#ifndef __nethck_h__
#define __nethck_h__

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_WIN32) && (defined(__WIN32__) || defined(WIN32) || defined(__CYGWIN__))
#  define _WIN32
#endif /* _WIN32 */

/* GLHCK specific defines */

#if defined(NETHCK_DLL) && defined(_NETHCK_BUILD_DLL)
#  error "You must not have both NETHCK_DLL and _NETHCK_BUILD_DLL defined"
#endif

#if defined(_WIN32) && defined(_NETHCK_BUILD_DLL)
   /* We are building a Win32 DLL */
#  define NETHCKAPI __declspec(dllexport)
#elif defined(_WIN32) && defined(NETHCK_DLL)
   /* We are calling a Win32 DLL */
#  if defined(__LCC__)
#     define NETHCKAPI extern
#  else
#     define NETHCKAPI __declspec(dllimport)
#  endif
#else
   /* We are either building/calling a static lib or we are non-win32 */
#  define NETHCKAPI
#endif

/***
 * public api
 ***/

/* init */
NETHCKAPI int nethckInit(int argc, char **argv);

/* client */
NETHCKAPI int nethckClientInit(const char *host, int port);
NETHCKAPI void nethckClientKill(void);
NETHCKAPI int nethckClientUpdate(void);
NETHCKAPI void nethckClientObjectRender(const glhckObject *object);

/* server */
NETHCKAPI int nethckServerInit(const char *host, int port);
NETHCKAPI void nethckServerKill(void);
NETHCKAPI int nethckServerUpdate(void);

#ifdef __cplusplus
}
#endif

#endif /* __nethck_h__ */
