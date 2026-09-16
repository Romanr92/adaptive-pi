cmake_minimum_required(VERSION 3.20)

project(hello-adaptive LANGUAGES CXX)

add_executable(hello-adaptive
    src/main.cpp
)

target_compile_features(hello-adaptive PRIVATE cxx_std_17)