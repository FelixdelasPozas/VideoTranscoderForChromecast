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

# Find the QtWidgets library
find_package(Qt5 COMPONENTS Widgets)

if (CMAKE_BUILD_TYPE MATCHES Debug)
  set(CORE_EXTERNAL_LIBS ${CORE_EXTERNAL_LIBS} ${QT_QTTEST_LIBRARY})
endif (CMAKE_BUILD_TYPE MATCHES Debug)

if(DEFINED MINGW)
  configure_file("${PROJECT_SOURCE_DIR}/resources.rc.in" "${PROJECT_BINARY_DIR}/resources.rc")
  set(CORE_SOURCES ${CORE_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/resources.rc)
  set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -o <OBJECT> -i <SOURCE>")
  enable_language(RC)
endif(DEFINED MINGW)

# libav library
set(LIBAV_DIR "D:/Desarrollo/Bibliotecas/libav-12")
set(LIBAV_INCLUDE_DIR ${LIBAV_DIR})
set(LIBAV_LIBRARIES   ${LIBAV_DIR}/libavformat/libavformat.a 
                      ${LIBAV_DIR}/libavcodec/libavcodec.a 
                      ${LIBAV_DIR}/libavutil/libavutil.a 
                      ${LIBAV_DIR}/libavresample/libavresample.a)
                      
# libvpx library
set(LIBVPX_INCLUDE_DIR "D:/Desarrollo/Bibliotecas/libvpx")
set(LIBVPX_LIBRARY ${LIBVPX_INCLUDE_DIR}/libvpx.a)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${PROJECT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}          # Generated .h files
  ${CMAKE_CURRENT_BINARY_DIR}  # For wrap/ui files
  ${LIBAV_INCLUDE_DIR}
  ${LIBVPX_INCLUDE_DIR}
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
)

SET_SOURCE_FILES_PROPERTIES(${CORE_SOURCES} PROPERTIES OBJECT_DEPENDS "${CORE_UI}")

set(CORE_EXTERNAL_LIBS
  ${QT_LIBRARIES}
  ${LIBAV_LIBRARIES}
  ${LIBVPX_LIBRARY}
)
  
add_executable(VideoTranscoder ${CORE_SOURCES})
target_link_libraries (VideoTranscoder ${CORE_EXTERNAL_LIBS})
qt5_use_modules(VideoTranscoder Widgets)