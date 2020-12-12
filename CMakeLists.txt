cmake_minimum_required(VERSION 3.10)

#########################################################################
# Setup gtest

project(Gtest VERSION 1.123)

# Download and unpack googletest at configure time
configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/googletest-download"
)
execute_process(COMMAND "${CMAKE_COMMAND}" --build .
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/googletest-download"
)

# Add googletest directly to our build. This adds the following targets:
# gtest, gtest_main, gmock and gmock_main
add_subdirectory("${CMAKE_BINARY_DIR}/googletest-src"
                 "${CMAKE_BINARY_DIR}/googletest-build"
)
include_directories("${gtest_SOURCE_DIR}/include"
                    "${gmock_SOURCE_DIR}/include"
)

#########################################################################
# Project Specific

# project name
project(ImageAnalysis VERSION 1.123)
message("CMAKE_PROJECT_VERSION = ${CMAKE_PROJECT_VERSION}")

# setup environment
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_VERBOSE_MAKEFILE OFF)

# setup protobuf processing
find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS detection_results_v2.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS annotations.proto)

#####################
# Test

include(GoogleTest)
add_executable(FooTest 
    test/test.cpp
    test/algoTest.cpp
    detection_results_v2.pb.cc
)
target_compile_options(FooTest PRIVATE -Werror -Wall -Wextra -mavx2)

target_include_directories(FooTest PUBLIC
	"${PROJECT_SRC_DIR}"
	"${PROJECT_BINARY_DIR}"
)

# add dependent libraries
target_link_libraries(FooTest ${Protobuf_LIBRARIES})
target_link_libraries(FooTest gtest_main)
# Now simply link your own targets against gtest, gmock,
# etc. as appropriate
#gtest_add_tests(TARGET      FooTest
#    TEST_SUFFIX .noArgs
#    TEST_LIST   noArgsTests
#)

#####################
# Application

find_package(Qt5 COMPONENTS Widgets Charts REQUIRED)

# add a executable to be created
add_executable(ImageAnalysis
    main.cpp
    window.cpp
    image_label.cpp
    annotations.cpp
    detection_results_v2.pb.cc
    annotations.pb.cc
)
#target_compile_options(ImageAnalysis PRIVATE -Werror -Wall -Wextra -mavx2)
target_compile_options(ImageAnalysis PRIVATE -Wall -Wextra -mavx2)

# add dependent libraries
target_link_libraries(ImageAnalysis ${Protobuf_LIBRARIES})

# files to be processed by autotools
configure_file (
 "${PROJECT_SOURCE_DIR}/Config.h.in"
 "${PROJECT_BINARY_DIR}/Config.h"
 )
target_include_directories(ImageAnalysis PUBLIC
	"${PROJECT_BINARY_DIR}"
)

target_link_libraries(ImageAnalysis Qt5::Widgets Qt5::Charts)
