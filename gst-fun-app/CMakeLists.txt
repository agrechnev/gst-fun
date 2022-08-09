cmake_minimum_required(VERSION 3.1)

project(gst-fun-app)

set(CMAKE_CXX_STANDARD 14)
find_package(PkgConfig)

# GStreamer
pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-audio-1.0 gstreamer-video-1.0  gstreamer-app-1.0)
message("GST_INCLUDE_DIRS = ${GST_INCLUDE_DIRS}")
message("GST_LIBRARY_DIRS = ${GST_LIBRARY_DIRS}")
message("GST_LIBRARIES = ${GST_LIBRARIES}")

# OpenCV
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

link_directories( ${GST_LIBRARY_DIRS}  )
include_directories( ${GST_INCLUDE_DIRS}  )

add_executable(fun1 fun1.cpp)
target_link_libraries(fun1 ${GST_LIBRARIES}  ${OpenCV_LIBS})

add_executable(fun2 fun2.cpp)
target_link_libraries(fun2 ${GST_LIBRARIES}  ${OpenCV_LIBS})

add_executable(fun3 fun3.cpp)
target_link_libraries(fun3 ${GST_LIBRARIES}  ${OpenCV_LIBS})

add_executable(fun4 fun4.cpp)
target_link_libraries(fun4 ${GST_LIBRARIES}  ${OpenCV_LIBS})

add_executable(idiot idiot.cpp)
target_link_libraries(idiot ${GST_LIBRARIES}  ${OpenCV_LIBS})

add_executable(fun4f fun4f.cpp)
target_link_libraries(fun4f ${GST_LIBRARIES}  ${OpenCV_LIBS})

add_executable(fun5 fun5.cpp)
target_link_libraries(fun5 ${GST_LIBRARIES}  ${OpenCV_LIBS})

add_executable(fun6 fun6.cpp)
target_link_libraries(fun6 ${GST_LIBRARIES}  ${OpenCV_LIBS})
