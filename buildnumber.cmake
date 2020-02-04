# build number script
#
# Place this script in the source code directory and add the following line to the project CMakeLists.txt
#
# add_custom_target(buildNumberDependency
#                   COMMAND ${CMAKE_COMMAND} -P "${CMAKE_SOURCE_DIR}/buildnumber.cmake")
# add_dependencies(projectName buildNumberDependency)

set(HEADER_FILE "${CMAKE_BINARY_DIR}/version.h")
set(CACHE_FILE "BuildNumberCache.txt")

#Reading data from file + incrementation
IF(EXISTS ${CACHE_FILE})
    file(READ ${CACHE_FILE} BUILD_NUMBER)
    math(EXPR BUILD_NUMBER "${BUILD_NUMBER}+1")
ELSE()
    set(BUILD_NUMBER "1")
ENDIF()

message("Build number: " ${BUILD_NUMBER})

#Update the cache
file(WRITE ${CACHE_FILE} "${BUILD_NUMBER}")

#Create the header
file(WRITE ${HEADER_FILE} "#ifndef VERSION_H\n#define VERSION_H\n\n#define BUILD_NUMBER ${BUILD_NUMBER}\n\n#endif")