cmake_minimum_required(VERSION 3.17)

project(redis-cpp-playground)

#---------------------------------------------------------------------------------------
# compiler config
#---------------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    add_compile_options("-Wall")
    add_compile_options("-Wextra")
    add_compile_options("-Wconversion")
    add_compile_options("-pedantic")
    add_compile_options("-Wfatal-errors")
    
endif()

# fmt library dependency
find_package(fmt CONFIG REQUIRED)

# spdlog library dependency
find_package(spdlog CONFIG REQUIRED)

# <------------ add hiredis dependency --------------->
find_path(HIREDIS_HEADER hiredis)
include_directories(${HIREDIS_HEADER})

find_library(HIREDIS_LIB hiredis)
#target_link_libraries(target ${HIREDIS_LIB})

# <------------ add redis-plus-plus dependency -------------->
# NOTE: this should be *sw* NOT *redis++*
find_path(REDIS_PLUS_PLUS_HEADER sw)
include_directories(${REDIS_PLUS_PLUS_HEADER})
#target_include_directories(target PUBLIC ${REDIS_PLUS_PLUS_HEADER})

find_library(REDIS_PLUS_PLUS_LIB redis++)
#target_link_libraries(target ${REDIS_PLUS_PLUS_LIB})

# Threads
find_package(Threads)

add_subdirectory(hash)

add_subdirectory(zset)

add_subdirectory(scheduler)

add_subdirectory(redlock)
