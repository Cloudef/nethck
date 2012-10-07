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
ADD_LIBRARY(nethck ${NETHCK_SRC})
TARGET_LINK_LIBRARIES(nethck kazmath enet ${MATH_LIBRARY})
INSTALL(TARGETS nethck DESTINATION lib)
