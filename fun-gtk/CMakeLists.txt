cmake_minimum_required(VERSION 3.1)

project(gst-gtk)

set(CMAKE_CXX_STANDARD 14)
find_package(PkgConfig)

#gstreamer
pkg_check_modules(GST gstreamer-1.0)
                                  
message("GST_INCLUDE_DIRS = ${GST_INCLUDE_DIRS}")
message("GST_LIBRARY_DIRS = ${GST_LIBRARY_DIRS}")
message("GST_LIBRARIES = ${GST_LIBRARIES}")

link_directories( ${GST_LIBRARY_DIRS}  )
include_directories( ${GST_INCLUDE_DIRS}  )

#GTK+
pkg_check_modules(GTK gtk+-3.0)

message("GTK_INCLUDE_DIRS = ${GTK_INCLUDE_DIRS}")
message("GTK_LIBRARY_DIRS = ${GTK_LIBRARY_DIRS}")
message("GTK_LIBRARIES = ${GTK_LIBRARIES}")

link_directories( ${GTK_LIBRARY_DIRS}  )
include_directories( ${GTK_INCLUDE_DIRS}  )

add_executable(fun1 fun1.cpp)
target_link_libraries(fun1 ${GST_LIBRARIES} ${GTK_LIBRARIES})
