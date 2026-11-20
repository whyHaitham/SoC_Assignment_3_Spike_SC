# Builds the official MESS code-generator libraries (bsc-mem/Mess) for the host
# and links our RISC-V driver against them.  MESS_SRC points to a checkout of
# https://github.com/bsc-mem/Mess at the commit pinned in the Dockerfile.
cmake_minimum_required(VERSION 3.21)
project(mess_riscv_codegen LANGUAGES CXX C)

set(MESS_SRC "/root/riscv-src/mess" CACHE PATH "bsc-mem/Mess checkout")
add_subdirectory(${MESS_SRC} mess EXCLUDE_FROM_ALL)

# Same sources as MESS's own generate_code target, minus its main()
# (GenerateCode.cpp), which auto-detects the host CPU.
include(${MESS_SRC}/cmake/MessSources.cmake)   # source lists only
set(MESS_CODEGEN_SRCS ${MESS_GENERATE_CODE_SRCS})
list(REMOVE_ITEM MESS_CODEGEN_SRCS GenerateCode.cpp)
list(TRANSFORM MESS_CODEGEN_SRCS PREPEND "${MESS_SRC}/")

add_executable(gen_riscv_kernel gen_riscv_kernel.cpp ${MESS_CODEGEN_SRCS})
target_link_libraries(gen_riscv_kernel PRIVATE mess_arch mess_lib mess_platform)
set_target_properties(gen_riscv_kernel PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
