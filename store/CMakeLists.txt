cmake_minimum_required(VERSION 3.17)

project (store)

add_executable(store-driver 
    store-driver.cpp
    entry.cpp
    store.cpp
)

target_link_libraries( store-driver Threads::Threads hiredis redis++)


install(TARGETS store-driver DESTINATION bin)
