cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
project(ego-gomoku-zero)

if (WIN32)
    string(APPEND CMAKE_CXX_FLAGS " /W4")
else ()
    string(APPEND CMAKE_CXX_FLAGS " -Wall -Wextra")
    string(APPEND CMAKE_C_FLAGS " -Wall -Wextra")
endif ()

# ========== 推理后端选项 ==========
option(USE_LIBTORCH "Build with libtorch+MPS backend (macOS only)" OFF)

#onnxruntime providers
option(onnxruntime_USE_CUDA "Build with CUDA support" OFF)
option(onnxruntime_USE_TENSORRT "Build with TensorRT support" OFF)
option(LIBPNG_ROOTDIR "libpng root dir")
option(ONNXRUNTIME_ROOTDIR "onnxruntime root dir")
include(FetchContent)

set(CMAKE_CXX_STANDARD 17)

# ========== ONNX Runtime（默认后端） ==========
if(NOT USE_LIBTORCH)
    if(NOT DEFINED ENV{ONNXRUNTIME_ROOTDIR})
        message(FATAL_ERROR "ONNXRUNTIME_ROOTDIR environment variable is not set.")
    endif()

    set(ONNXRUNTIME_ROOTDIR $ENV{ONNXRUNTIME_ROOTDIR})
    message(STATUS "ONNXRUNTIME_ROOTDIR: ${ONNXRUNTIME_ROOTDIR}")

    include_directories("${ONNXRUNTIME_ROOTDIR}/include"
            "${ONNXRUNTIME_ROOTDIR}/include/onnxruntime"
            "${ONNXRUNTIME_ROOTDIR}/include/onnxruntime/core/session")

    link_directories("${ONNXRUNTIME_ROOTDIR}/lib")
endif()

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

# ========== 源文件 ==========
set(COMMON_SOURCES
    SelfPlay.cpp Game.h Game.cpp MCTS.h MCTS.cpp SelfPlay.h Main.cpp
    Analyzer.cpp Analyzer.h Piskvork.cpp Pisqpipe.h
    Pisqpipe.cpp Utils.cpp Utils.h Shape.cpp Shape.h
    Model.cpp Model.h ConfigReader.cpp ConfigReader.h
    Bridge.cpp Bridge.h Evaluate.cpp Evaluate.h
    GenerateOpenings.cpp GenerateOpenings.h
)

# ========== libtorch + MPS 后端 ==========
if(USE_LIBTORCH)
    message(STATUS "=== Building with libtorch + MPS backend ===")
    add_definitions(-DUSE_LIBTORCH)

    # 查找 libtorch
    if(NOT DEFINED ENV{LIBTORCH_ROOTDIR})
        message(FATAL_ERROR "LIBTORCH_ROOTDIR environment variable is not set. "
                            "Set it to your libtorch installation directory.")
    endif()
    set(LIBTORCH_ROOTDIR $ENV{LIBTORCH_ROOTDIR})
    message(STATUS "LIBTORCH_ROOTDIR: ${LIBTORCH_ROOTDIR}")

    # C++17 required by libtorch
    set(CMAKE_CXX_STANDARD 17)

    find_package(Torch REQUIRED PATHS ${LIBTORCH_ROOTDIR})
    include_directories(${TORCH_INCLUDE_DIRS})

    add_executable(ego-gomoku-zero
        ${COMMON_SOURCES}
        TorchBackend.cpp TorchBackend.h
    )
    set_property(TARGET ego-gomoku-zero PROPERTY CXX_STANDARD 17)

    target_link_libraries(ego-gomoku-zero ${TORCH_LIBRARIES})

# ========== ONNX Runtime 后端（默认） ==========
else()
    message(STATUS "=== Building with ONNX Runtime backend ===")

    add_executable(ego-gomoku-zero
        ${COMMON_SOURCES}
        ONNXBackend.cpp ONNXBackend.h
    )
    set_property(TARGET ego-gomoku-zero PROPERTY CXX_STANDARD 17)

    # ONNX
    find_library(MY_LIBRARY
            NAMES onnxruntime onnxruntime_providers_tensorrt onnxruntime_providers_shared onnxruntime_providers_cuda
            PATHS ${ONNXRUNTIME_ROOTDIR}/lib)

    target_link_libraries(ego-gomoku-zero ${MY_LIBRARY})
endif()
