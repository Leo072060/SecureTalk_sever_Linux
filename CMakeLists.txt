cmake_minimum_required(VERSION 3.10)

# Project name
project(SecureTalk LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# DEBUG 
add_definitions(-DDEBUG)

# Set the build type to Debug if it is not already specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Compiler options for AddressSanitizer
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Define AddressSanitizer flags and keep frame pointer for better stack traces
    set(ASAN_FLAGS "-fsanitize=address -fno-omit-frame-pointer")

    # Add ASan flags and debugging info (-g) to C++ Debug build flags
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${ASAN_FLAGS} -g")

    # Add ASan flags to the linker for Debug build
    set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} ${ASAN_FLAGS}")
    
endif()


# Find package
find_package(OpenSSL REQUIRED)

# Add include directory
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/lib/protobuf/include)
include_directories(${PROJECT_SOURCE_DIR}/lib/protobuf/generated_include)
include_directories(${PROJECT_SOURCE_DIR}/lib/sqlite/include)
include_directories(${PROJECT_SOURCE_DIR}/lib/logger/include)
include_directories(${PROJECT_SOURCE_DIR}/lib/hiredis/include)

# Collect all .cpp files in src directory
file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp)
file(GLOB PROTO_SRC_FILES ${PROJECT_SOURCE_DIR}/lib/protobuf/generated_src/*.cc)
file(GLOB LOGGER_SRC_FILES ${PROJECT_SOURCE_DIR}/lib/logger/src/*.cpp)

# Build an executable from source files
add_executable(SecureTalk ${SRC_FILES} ${PROTO_SRC_FILES} ${LOGGER_SRC_FILES})

# Link against libraries from lib directory
link_directories(
)

# target_link_libraries
target_link_libraries(SecureTalk PRIVATE
    ${PROJECT_SOURCE_DIR}/lib/protobuf/lib/libprotobuf.so   
    ${PROJECT_SOURCE_DIR}/lib/sqlite/lib/libsqlite3.so   
    OpenSSL::Crypto
    ${PROJECT_SOURCE_DIR}/lib/hiredis/lib/libhiredis.so
)

# 链接库
target_link_libraries(SecureTalk PRIVATE ${HIREDIS_LIBRARIES})