cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
project(ego-gomoku-zero)

if (WIN32)
    string(APPEND CMAKE_CXX_FLAGS " /W4")
else ()
    string(APPEND CMAKE_CXX_FLAGS " -Wall -Wextra")
    string(APPEND CMAKE_C_FLAGS " -Wall -Wextra")
endif ()

#onnxruntime providers
option(onnxruntime_USE_CUDA "Build with CUDA support" OFF)
option(onnxruntime_USE_TENSORRT "Build with TensorRT support" OFF)
option(LIBPNG_ROOTDIR "libpng root dir")
option(ONNXRUNTIME_ROOTDIR "onnxruntime root dir")
include(FetchContent)

set(CMAKE_CXX_STANDARD 17)

if(NOT DEFINED ENV{ONNXRUNTIME_ROOTDIR})
    message(FATAL_ERROR "ONNXRUNTIME_ROOTDIR environment variable is not set.")
endif()

set(ONNXRUNTIME_ROOTDIR $ENV{ONNXRUNTIME_ROOTDIR})

message(STATUS "ONNXRUNTIME_ROOTDIR: ${ONNXRUNTIME_ROOTDIR}")

# The ORT package has a different include directory structure to a local install via cmake.
# We added the path for the pre-built package above. Add the path for a local install to support either usage.
# TODO: If we want to support additional EPs being loadable from a local install we also need to add EP specific
# directories under /include/onnxruntime/core/providers
include_directories("${ONNXRUNTIME_ROOTDIR}/include"                           # Pre-built package
        "${ONNXRUNTIME_ROOTDIR}/include/onnxruntime"               # Linux local install to /usr/local
        "${ONNXRUNTIME_ROOTDIR}/include/onnxruntime/core/session") # Windows local install

link_directories("${ONNXRUNTIME_ROOTDIR}/lib")

if (WIN32)
    add_library(wil INTERFACE)

    FetchContent_Declare(
            microsoft_wil
            URL https://github.com/microsoft/wil/archive/refs/tags/v1.0.220914.1.zip
    )
    FetchContent_Populate(microsoft_wil)
    target_include_directories(wil INTERFACE ${microsoft_wil_SOURCE_DIR}/include)
    set(WIL_LIB wil)
endif ()

if (onnxruntime_USE_CUDA)
    add_definitions(-DUSE_CUDA)
endif ()
if (onnxruntime_USE_TENSORRT)
    add_definitions(-DUSE_TENSORRT)
endif ()
if (onnxruntime_USE_DML)
    message("Enabling DML")
    add_definitions(-DUSE_DML)
endif ()

# Windows might have an onnxruntime.dll in the system directory so it's more robust to manually copy the dlls to
# the output dir. Define a function to do so. This is called from the cmake file in the subdirectories.
function(copy_ort_dlls target_name)
if (MSVC)
        file(GLOB ORT_DLLS ${ONNXRUNTIME_ROOTDIR}/lib/*.dll)
        foreach (ORT_DLL ${ORT_DLLS})
            add_custom_command(TARGET ${target_name} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy ${ORT_DLL} $<TARGET_FILE_DIR:${target_name}>)
        endforeach ()
    endif ()
endfunction()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -O3 -funroll-loops -ftree-vectorize -finline-functions -std=c++17")

add_executable(ego-gomoku-zero SelfPlay.cpp Game.h Game.cpp MCTS.h MCTS.cpp SelfPlay.h Main.cpp Analyzer.cpp Analyzer.h AnalyzerTest.cpp AnalyzerTest.h Piskvork.cpp Pisqpipe.h Pisqpipe.cpp GameTest.cpp GameTest.h Console.cpp Console.cpp Console.cpp Console.h Utils.cpp Utils.h Shape.cpp Shape.h ShapeTest.cpp ShapeTest.h Model.cpp Model.h Pruner.cpp Pruner.h)
set_property(TARGET ego-gomoku-zero PROPERTY CXX_STANDARD 17)

# ONNX
find_library(MY_LIBRARY
        NAMES onnxruntime onnxruntime_providers_tensorrt onnxruntime_providers_shared onnxruntime_providers_cuda
        PATHS ${ONNXRUNTIME_ROOTDIR}/lib)

# 链接第三方静态库到你的目标
target_link_libraries(ego-gomoku-zero ${MY_LIBRARY})

# TENSORRT
#if(DEFINED ENV{TENSORRT_ROOTDIR})
#    message(STATUS "TENSORRT_ROOTDIR environment variable set $ENV{TENSORRT_ROOTDIR}")
#    find_library(TENSORRT_LIBRARY
#            NAMES nvinfer nvcaffe_parser nvinfer_dispatch nvinfer_lean nvinfer_plugin nvinfer_vc_plugin nvparsers onnx_proto
#            PATHS $ENV{TENSORRT_ROOTDIR}/lib)
#    target_link_libraries(ego-gomoku-zero ${TENSORRT_LIBRARY})
#endif()