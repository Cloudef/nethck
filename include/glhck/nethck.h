#ifndef __nethck_h__
#define __nethck_h__

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_WIN32) && (defined(__WIN32__) || defined(WIN32) || defined(__CYGWIN__))
#  define _WIN32
#endif /* _WIN32 */

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

/* \brief import structs for easier data passing
 * from other applications that are not glhck.
 *
 * These structs only use public types mostly from glhck header. */

typedef struct nethckImportView {
   glhckVector3f translation, rotation, scaling;
} nethckImportView;

typedef struct nethckImportMaterial {
   glhckColorb color;
} nethckImportMaterial;

typedef struct nethckImportGeometry {
   unsigned int type;
   size_t vertexCount;
   size_t indexCount;
   glhckImportVertexData *vertexData;
   glhckImportIndexData *indexData;
} nethckImportGeometry;

typedef struct nethckImportObject {
   struct nethckImportGeometry geometry;
   struct nethckImportView view;
   struct nethckImportMaterial material;
} nethckImportObject;

/***
 * public api
 ***/

/* client */
NETHCKAPI int nethckClientCreate(const char *host, int port);
NETHCKAPI void nethckClientTerminate(void);
NETHCKAPI int nethckClientUpdate(void);
NETHCKAPI glhckObject** nethckClientObjects(unsigned int *objectCount);
NETHCKAPI glhckObject* nethckClientObjectForId(unsigned int id);
NETHCKAPI void nethckClientObjectRender(unsigned int id, glhckObject *object);
NETHCKAPI void nethckClientImportObject(nethckImportObject *import);

/* server */
NETHCKAPI int nethckServerCreate(const char *host, int port);
NETHCKAPI void nethckServerTerminate(void);
NETHCKAPI int nethckServerUpdate(void);
NETHCKAPI unsigned int nethckServerClientCount(void);

#ifdef __cplusplus
}
#endif

#endif /* __nethck_h__ */
