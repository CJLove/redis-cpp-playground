cmake_minimum_required(VERSION 3.17)

project (scheduler)

add_executable(scheduler main.cpp Scheduler.cpp Dispatcher.cpp Worker.cpp)

target_link_libraries( scheduler Threads::Threads hiredis redis++)


install(TARGETS scheduler DESTINATION bin)
