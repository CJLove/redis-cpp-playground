cmake_minimum_required(VERSION 3.17)

project (redlock)

add_executable(redlock-driver redlock-driver.cpp)

target_link_libraries( redlock-driver Threads::Threads hiredis redis++)


install(TARGETS redlock-driver DESTINATION bin)
