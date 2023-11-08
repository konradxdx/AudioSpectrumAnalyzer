cmake_minimum_required(VERSION 3.20)

project("AudioSpectrumAnalyzer" C CXX ASM)

include(cmake/st-project.cmake)

add_executable(${PROJECT_NAME} core/src/main.c core/src/clean_main.c)
add_st_target_properties(${PROJECT_NAME})
