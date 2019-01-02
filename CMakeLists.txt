cmake_minimum_required(VERSION 3.0)

project(combine)


add_executable(combine combine.cpp)
target_link_libraries(combine "libIlmImf.so")
target_link_libraries(combine "libHalf.so")

install(TARGETS combine RUNTIME DESTINATION bin)
