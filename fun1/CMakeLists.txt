cmake_minimum_required(VERSION 3.1)

project(gst-fun1)

set(CMAKE_CXX_STANDARD 14)
find_package(PkgConfig)

pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-audio-1.0)

message("GST_INCLUDE_DIRS = ${GST_INCLUDE_DIRS}")
message("GST_LIBRARY_DIRS = ${GST_LIBRARY_DIRS}")
message("GST_LIBRARIES = ${GST_LIBRARIES}")

link_directories( ${GST_LIBRARY_DIRS}  )
include_directories( ${GST_INCLUDE_DIRS}  )

add_executable(fun0 fun0.cpp)
target_link_libraries(fun0 ${GST_LIBRARIES})

add_executable(fun1 fun1.cpp)
target_link_libraries(fun1 ${GST_LIBRARIES})

add_executable(fun2 fun2.cpp)
target_link_libraries(fun2 ${GST_LIBRARIES})

add_executable(fun3 fun3.cpp)
target_link_libraries(fun3 ${GST_LIBRARIES})

add_executable(fun4 fun4.cpp)
target_link_libraries(fun4 ${GST_LIBRARIES})

add_executable(fun5 fun5.cpp)
target_link_libraries(fun5 ${GST_LIBRARIES})
