#ifndef __nethck_common_h__
#define __nethck_common_h__

/* if exists then perform function and set NULL
 * used mainly to shorten if (x) free(x); x = NULL; */
#define IFDO(f, x) { if (x) f(x); x = NULL; }

/* perform function and set NULL (no checks)
 * used mainly to shorten free(x); x = NULL; */
#define NULLDO(f, x) { f(x); x = NULL; }

/*** format macros ***/

#define RECT(r)   (r)?(r)->x:-1, (r)?(r)->y:-1, (r)?(r)->w:-1, (r)?(r)->h:-1
#define RECTS     "rect[%f, %f, %f, %f]"
#define COLB(c)   (c)?(c)->r:0, (c)?(c)->g:0, (c)?(c)->b:0, (c)?(c)->a
#define COLBS     "colb[%d, %d, %d, %d]"
#define VEC2(v)   (v)?(v)->x:-1, (v)?(v)->y:-1
#define VEC2S     "vec2[%f, %f]"
#define VEC3(v)   (v)?(v)->x:-1, (v)?(v)->y:-1, (v)?(v)->z:-1
#define VEC3S     "vec3[%f, %f, %f]"
#define VEC4(v)   (v)?(v)->x:-1, (v)?(v)->y:-1, (v)?(v)->z:-1, (v)?(v)->w:-1
#define VEC4S     "vec3[%f, %f, %f, %f]"

#endif /* __nethck_common_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
