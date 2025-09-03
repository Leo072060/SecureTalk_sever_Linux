cmake_minimum_required(VERSION 3.10)

# Project name
project(SecureTalk LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# DEBUG 
add_definitions(-DDEBUG)

# Add include directory
include_directories(${PROJECT_SOURCE_DIR}/include)

# Collect all .cpp files in src directory
file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp)

# Build an executable from source files
add_executable(SecureTalk ${SRC_FILES})

# Link against libraries from lib directory (if any)
link_directories(${PROJECT_SOURCE_DIR}/lib)

# target_link_libraries(SecureTalk protobuf Qt5::Core Qt5::Network)
