SET(NETHCK_SRC
   nethck.c
   client.c
   server.c)

FIND_PACKAGE(ENet REQUIRED)

# include directories
INCLUDE_DIRECTORIES(
   ${ENet_INCLUDE_DIR}
   ${kazmath_SOURCE_DIR}/src
   ${glhck_SOURCE_DIR}/include
   ${NETHCK_INCLUDE_DIR}
)

ADD_DEFINITIONS(-fPIC)
ADD_LIBRARY(nethckStatic STATIC ${NETHCK_SRC})
ADD_LIBRARY(nethckShared SHARED ${NETHCK_SRC})
SET_TARGET_PROPERTIES(nethckStatic nethckShared PROPERTIES OUTPUT_NAME nethck)
TARGET_LINK_LIBRARIES(nethckStatic kazmath enet glhck ${MATH_LIBRARY})
TARGET_LINK_LIBRARIES(nethckShared kazmath enet glhck ${MATH_LIBRARY})
INSTALL(TARGETS nethckStatic nethckShared DESTINATION lib)
