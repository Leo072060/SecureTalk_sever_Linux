cmake_minimum_required(VERSION 3.10)

# Project name
project(SecureTalk LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# DEBUG 
# add_definitions(-DDEBUG)

# Find package
find_package(OpenSSL REQUIRED)

# Add include directory
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/lib/protobuf/include)
include_directories(${PROJECT_SOURCE_DIR}/lib/protobuf/generated_include)
include_directories(${PROJECT_SOURCE_DIR}/lib/sqlite/include)

# Collect all .cpp files in src directory
file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp)
file(GLOB PROTO_FILES ${PROJECT_SOURCE_DIR}/lib/protobuf/generated_src/*.cc)

# Build an executable from source files
add_executable(SecureTalk ${SRC_FILES} ${PROTO_FILES})

# Link against libraries from lib directory
link_directories(
)

# target_link_libraries
target_link_libraries(SecureTalk PRIVATE
    ${PROJECT_SOURCE_DIR}/lib/protobuf/lib/libprotobuf.so   
    ${PROJECT_SOURCE_DIR}/lib/sqlite/lib/libsqlite3.so   
    OpenSSL::Crypto   
)
