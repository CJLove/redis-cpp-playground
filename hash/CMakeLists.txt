cmake_minimum_required(VERSION 3.17)

project (hash)

add_executable(hash-driver hash-driver.cpp)

target_link_libraries( hash-driver Threads::Threads hiredis redis++)


install(TARGETS hash-driver DESTINATION bin)
