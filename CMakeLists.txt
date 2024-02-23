cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
project(ego-gomoku-zero)

execute_process(
        COMMAND python3 -c "import torch; print(torch.utils.cmake_prefix_path)"
        OUTPUT_VARIABLE TORCH_CMAKE_PREFIX_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(CMAKE_PREFIX_PATH ${TORCH_CMAKE_PREFIX_PATH})
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_OSX_ARCHITECTURES x86_64)

find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS} -O3 -funroll-loops -ftree-vectorize -finline-functions -std=c++17")

add_executable(ego-gomoku-zero SelfPlay.cpp Game.h Game.cpp MCTS.h MCTS.cpp SelfPlay.h Main.cpp Analyzer.cpp Analyzer.h AnalyzerTest.cpp AnalyzerTest.h Piskvork.cpp Pisqpipe.h Pisqpipe.cpp GameTest.cpp GameTest.h Console.cpp Console.cpp Console.cpp Console.h Utils.cpp Utils.h)
target_link_libraries(ego-gomoku-zero "${TORCH_LIBRARIES}")
set_property(TARGET ego-gomoku-zero PROPERTY CXX_STANDARD 17)

# The following code block is suggested to be used on Windows.
# According to https://github.com/pytorch/pytorch/issues/25457,
# the DLLs need to be copied to avoid memory errors.
if (MSVC)
  file(GLOB TORCH_DLLS "${TORCH_INSTALL_PREFIX}/lib/*.dll")
  add_custom_command(TARGET ego-gomoku-zero
                     POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy_if_different
                     ${TORCH_DLLS}
                     $<TARGET_FILE_DIR:ego-gomoku-zero>)
endif (MSVC)