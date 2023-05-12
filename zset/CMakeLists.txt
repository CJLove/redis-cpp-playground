cmake_minimum_required(VERSION 3.17)

project (zset)

add_executable(zset-driver zset-driver.cpp)

target_link_libraries( zset-driver Threads::Threads hiredis redis++)


install(TARGETS zset-driver DESTINATION bin)
