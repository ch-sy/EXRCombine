cmake_minimum_required(VERSION 3.0)

project(exrcombine)

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

add_executable(exrcombine combine.cpp)
target_link_libraries(exrcombine "libIlmImf.so")
target_link_libraries(exrcombine "libHalf.so")

install(TARGETS exrcombine RUNTIME DESTINATION bin)
