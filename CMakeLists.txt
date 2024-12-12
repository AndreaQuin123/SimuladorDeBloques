
cmake_minimum_required(VERSION 3.1)

project(SimuladorDeBloques_AndreaQuin)

#REQS
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

#Variable entre ${}
add_executable(${CMAKE_PROJECT_NAME} main.cpp BlockDevice.cpp)
