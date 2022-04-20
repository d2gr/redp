cmake_minimum_required (VERSION 3.1)
set (REDIS_CLIENT_VERSION_MAJOR 0)
set (REDIS_CLIENT_VERSION_MINOR 1)
project(redis_client VERSION "${REDIS_CLIENT_VERSION_MAJOR}.${REDIS_CLIENT_VERSION_MINOR}")

configure_file("${PROJECT_SOURCE_DIR}/definitions.hpp.in" "${CMAKE_CURRENT_SOURCE_DIR}/include/redis/definitions.hpp")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin) 

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DREDIS_DEBUG")
#set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")

set(ENABLE_TESTING OFF CACHE BOOL "Enable testing")

option(REDIS_CLIENT_BUILD_EXAMPLES "Builds examples listed on the examples folder." ON)
# option(REDIS_CLIENT_USE_STRING_VIEW "Use string_view as much as possible" ON)
option(REDIS_CLIENT_SHARED_PTR "Enables `enabled_shared_from_this` in redis::client" OFF)


add_library(${PROJECT_NAME} "${PROJECT_SOURCE_DIR}/src/stream.cc"
                            "${PROJECT_SOURCE_DIR}/src/basic_stream.cc"
                            "${PROJECT_SOURCE_DIR}/src/subscribed_stream.cc"
                            "${PROJECT_SOURCE_DIR}/src/types/array.cc"
                            "${PROJECT_SOURCE_DIR}/src/types/error.cc"
                            "${PROJECT_SOURCE_DIR}/src/types/integer.cc"
                            "${PROJECT_SOURCE_DIR}/src/types/string.cc"
                            "${PROJECT_SOURCE_DIR}/src/parser.cc")
add_library(redis::client ALIAS redis_client)

target_include_directories(redis_client PUBLIC "${PROJECT_SOURCE_DIR}/include")

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

#Link boost URL and JSON
if(REDIS_CLIENT_SHARED_PTR)
    target_compile_definitions(${PROJECT_NAME} INTERFACE REDIS_CLIENT_SHARED_PTR=1)
endif()

#Find Boost dependencies
if(REDIS_CLIENT_BUILD_EXAMPLES)
  find_package(Boost COMPONENTS system program_options)
endif()

#Include and link Boost
if(Boost_FOUND)
  target_include_directories(${PROJECT_NAME} INTERFACE ${Boost_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} INTERFACE ${Boost_LIBRARIES})
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(OPENSSL_USE_STATIC_LIBS true)
  set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
endif()

## TODO: make it optional
# Find OpenSSL
find_package(OpenSSL REQUIRED)

#Include and link OpenSSL
if(OPENSSL_FOUND)
  if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    target_include_directories(${PROJECT_NAME} INTERFACE "/usr/local/opt/openssl/include/")
  endif()
  target_include_directories(${PROJECT_NAME} INTERFACE ${OPENSSL_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} INTERFACE ${OPENSSL_LIBRARIES})
endif()

find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} INTERFACE Threads::Threads)

if(REDIS_CLIENT_BUILD_EXAMPLES)
    add_subdirectory(${PROJECT_SOURCE_DIR}/examples)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC ${CONAN_LIBS})
