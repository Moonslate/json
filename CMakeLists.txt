#Require a minimum version
cmake_minimum_required(VERSION 3.10)

project(uva-json)

add_library(uva-json
	${CMAKE_CURRENT_LIST_DIR}/src/json.cpp
)

include_directories(${CMAKE_CURRENT_LIST_DIR})