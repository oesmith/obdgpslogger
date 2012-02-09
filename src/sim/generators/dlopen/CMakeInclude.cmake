SET(OBD_SIMGEN_DLOPEN "On" CACHE BOOL "Enable dlopen obdsim generator")

INCLUDE(CheckFunctionExists)
SET(CMAKE_REQUIRED_LIBRARIES dl)
CHECK_FUNCTION_EXISTS(dlopen HAVE_DLOPEN)

IF(NOT HAVE_DLOPEN)
	SET(OBD_SIMGEN_DLOPEN "Off" CACHE BOOL "Enable dlopen obdsim generator" FORCE)
	MESSAGE(STATUS "Couldn't find dlopen. Not building dlopen sim generator")
ELSE(NOT HAVE_DLOPEN)
	MESSAGE(STATUS "Found dlopen. Building dlopen sim generator")
ENDIF(NOT HAVE_DLOPEN)

IF(OBD_SIMGEN_DLOPEN)
	FILE(GLOB OBDSIMGEN_DLOPEN_SRCS
		generators/dlopen/*.c
	)
	ADD_LIBRARY(ckobdsim_dlopen STATIC ${OBDSIMGEN_DLOPEN_SRCS})
	SET(GENERATOR_LIBS ${GENERATOR_LIBS} ckobdsim_dlopen dl)
	ADD_DEFINITIONS(-DOBDSIMGEN_DLOPEN)

	INCLUDE_DIRECTORIES(generators/dlopen/)

# This is just the test library
	ADD_LIBRARY(obdsim_dltest MODULE generators/dlopen/test/test_dlopen.c)
ENDIF(OBD_SIMGEN_DLOPEN)

