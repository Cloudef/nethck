CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(nethck)
SET(NETHCK_NAME "NEThck Network Transparency")
SET(NETHCK_DESCRIPTION "Provides networking functionality to glhck for sharing geometry beetwen applications")
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${nethck_SOURCE_DIR}/CMake/modules)

# GLHCK options
SET(GLHCK_IMPORT_ASSIMP OFF CACHE BOOL "We don't need assimp")
SET(GLHCK_IMPORT_OPENCTM OFF CACHE BOOL "We don't need openctm")
SET(GLHCK_IMPORT_MMD OFF CACHE BOOL "We don't need mmd")

# Debug build
SET(CMAKE_BUILD_TYPE Debug)

# Release build
# SET(CMAKE_BUILD_TYPE Release)

ADD_DEFINITIONS(-DGLHCK_KAZMATH_FLOAT -DUSE_SINGLE_PRECISION)

# Build checks
# Abort git submodule init && git submodule update has not been ran.
SET(SUBMODULE_ERROR "Run 'git submodule update --init --recursive' update first.")
IF (NOT EXISTS ${nethck_SOURCE_DIR}/lib/glhck/.git)
   SET(SUBMODULE_ERROR_THROW 1)
ENDIF ()

# Throw the error
IF (SUBMODULE_ERROR_THROW)
   MESSAGE(FATAL_ERROR ${SUBMODULE_ERROR})
ENDIF ()

# Set nethck's include dir
SET(NETHCK_INCLUDE_DIR ${nethck_SOURCE_DIR}/include)

# Do it (tm)
ADD_SUBDIRECTORY(lib)

# Warnings
IF (MSVC)
   IF (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
      STRING(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
   ELSE ()
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
   ENDIF ()
ELSEIF (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
   SET(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wall -Wno-long-long")
   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long")
ENDIF ()

# Do nethck
ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(example)

# Install
FILE(COPY include/glhck/nethck.h DESTINATION include/glhck)
