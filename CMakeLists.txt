project(VideoTranscoder)

cmake_minimum_required (VERSION 2.8.6)

# Version Number
set (VIDEO_TRANSCODER_VERSION_MAJOR 1)
set (VIDEO_TRANSCODER_VERSION_MINOR 0)
set (VIDEO_TRANSCODER_VERSION_PATCH 0)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Manually setting Boost because the CMake script is pure shit
set(Boost_INCLUDE_DIR "D:/Desarrollo/Bibliotecas/boost_1_72_0")
set(Boost_LIBRARIES "D:/Desarrollo/Bibliotecas/boost_1_72_0/bin.v2/libs/program_options/build/gcc-8.2.0/release/link-static/threading-multi/visibility-hidden/libboost_program_options-mgw82-mt-x64-1_72.a"
                    "D:/Desarrollo/Bibliotecas/boost_1_72_0/bin.v2/libs/filesystem/build/gcc-8.2.0/release/link-static/threading-multi/visibility-hidden/libboost_filesystem-mgw82-mt-x64-1_72.a")

# Find the QtWidgets library
find_package(Qt5 COMPONENTS Widgets WinExtras)

if (CMAKE_BUILD_TYPE MATCHES Debug)
  set(CORE_EXTERNAL_LIBS ${CORE_EXTERNAL_LIBS} ${QT_QTTEST_LIBRARY})
endif (CMAKE_BUILD_TYPE MATCHES Debug)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

if(DEFINED MINGW)
  configure_file("${PROJECT_SOURCE_DIR}/resources.rc.in" "${PROJECT_BINARY_DIR}/resources.rc")
  set(CORE_SOURCES ${CORE_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/resources.rc)
  set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -o <OBJECT> -i <SOURCE>")
  enable_language(RC)
endif(DEFINED MINGW)

# libav library needs to be compiled with nvenc (Nvidia encoder) and VPX (VP8 & VP9 encoders) support. Check libav compilation options.
set(LIBAV_DIR "D:/Desarrollo/Bibliotecas/libav-12")
set(LIBAV_INCLUDE_DIR ${LIBAV_DIR})
set(LIBAV_LIBRARIES   ${LIBAV_DIR}/libavformat/libavformat.a 
                      ${LIBAV_DIR}/libavcodec/libavcodec.a 
                      ${LIBAV_DIR}/libavutil/libavutil.a 
                      ${LIBAV_DIR}/libavresample/libavresample.a)
                      
# libvpx library
set(LIBVPX_INCLUDE_DIR "D:/Desarrollo/Bibliotecas/libvpx")
set(LIBVPX_LIBRARY ${LIBVPX_INCLUDE_DIR}/libvpx.a)

# zlib
set(ZLIB_INCLUDE_DIR "D:/Desarrollo/Bibliotecas/zlib-1.2.11")
set(ZLIB_LIBRARY "D:/Desarrollo/Bibliotecas/zlib-build/libzlibstatic.a")

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${PROJECT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}          # Generated .h files
  ${CMAKE_CURRENT_BINARY_DIR}  # For wrap/ui files
  ${LIBAV_INCLUDE_DIR}
  ${LIBVPX_INCLUDE_DIR}
  ${ZLIB_INCLUDE_DIR}
  ${Boost_INCLUDE_DIR}
  )

set(CMAKE_CXX_FLAGS " -Wall -Wno-deprecated -std=c++11 -mwindows -m64")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

# Add Qt Resource files
qt5_add_resources(RESOURCES
  rsc/resources.qrc
)

qt5_wrap_ui(CORE_UI
  # .ui for Qt
  VideoTranscoder.ui
  ProcessDialog.ui
  AboutDialog.ui
  ConfigurationDialog.ui
)
	
set (CORE_SOURCES
  # project files
  ${CORE_SOURCES}
  ${RESOURCES}
  ${CORE_UI}
  Main.cpp
  VideoTranscoder.cpp
  AboutDialog.cpp
  Utils.cpp
  ConfigurationDialog.cpp
  ProcessDialog.cpp
  Worker.cpp
)

SET_SOURCE_FILES_PROPERTIES(${CORE_SOURCES} PROPERTIES OBJECT_DEPENDS "${CORE_UI}")

set(CORE_EXTERNAL_LIBS
  Qt5::Widgets
  Qt5::WinExtras
  ${Boost_LIBRARIES}
  ${LIBAV_LIBRARIES}
  ${LIBVPX_LIBRARY}
  ${ZLIB_LIBRARY}
  libws2_32.a
)
  
add_executable(VideoTranscoder ${CORE_SOURCES})
target_link_libraries (VideoTranscoder ${CORE_EXTERNAL_LIBS})
qt5_use_modules(VideoTranscoder Widgets)

add_custom_target(buildNumberDependency
                  COMMAND ${CMAKE_COMMAND} -P "${CMAKE_SOURCE_DIR}/buildnumber.cmake")
add_dependencies(VideoTranscoder buildNumberDependency)
