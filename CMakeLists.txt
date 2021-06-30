cmake_minimum_required(VERSION 3.19)
project(RISK_V_simulator)

set(CMAKE_CXX_STANDARD 14)

add_executable(RISK_V_simulator
        main.cpp IO.hpp Saver.hpp Executer.hpp exception.hpp)
