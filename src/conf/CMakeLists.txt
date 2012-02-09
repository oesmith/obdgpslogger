INCLUDE_DIRECTORIES(
	.
)


FILE(GLOB OBDCONFIGFILE_SRCS
	obdconfigfile*
)

ADD_LIBRARY(ckobdconfigfile STATIC ${OBDCONFIGFILE_SRCS})


SET(OBD_ENABLE_CONFIGTEST false CACHE BOOL "Enable configtest executable")
IF(OBD_ENABLE_CONFIGTEST)
	ADD_EXECUTABLE(testobdconfig testobdconfig.c)
	TARGET_LINK_LIBRARIES(testobdconfig ckobdconfigfile ckobdinfo)
ENDIF(OBD_ENABLE_CONFIGTEST)

