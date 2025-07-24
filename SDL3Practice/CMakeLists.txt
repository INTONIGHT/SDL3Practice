cmake_minimum_required(VERSION 4.1)

find_package(SDL3 REQUIRED)

project(SDL3Practice)
add_executable(SDL3Practice "Main.cpp" "Timer.h" "Animation.h")



target_include_directories(SDL3Practice PRIVATE "ext/")