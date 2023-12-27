cmake_minimum_required(VERSION 3.10)
project(rbbs-ws VERSION 1.0.0)

cmake_policy(SET CMP0024 OLD)
cmake_policy(SET CMP0046 NEW)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(FetchContent)
FetchContent_Declare(
    Libwebsockets
    GIT_REPOSITORY https://github.com/warmcat/libwebsockets.git
    GIT_TAG        v4.3.3
)
FetchContent_MakeAvailable(Libwebsockets)

include(FindOpenSSL)

add_executable(rbbs-ws main.c buffer.c)
add_dependencies(rbbs-ws websockets_shared)

target_include_directories(rbbs-ws PRIVATE "${libwebsockets_SOURCE_DIR}/include")
target_include_directories(rbbs-ws PRIVATE "${libwebsockets_BINARY_DIR}")
target_include_directories(rbbs-ws PRIVATE "${OPENSSL_INCLUDE_DIR}")
target_include_directories(rbbs-ws PRIVATE "include")
target_link_libraries(rbbs-ws PRIVATE websockets_shared)

SET_TARGET_PROPERTIES(websockets PROPERTIES EXCLUDE_FROM_ALL True)
SET_TARGET_PROPERTIES(test-lejp PROPERTIES EXCLUDE_FROM_ALL True)
SET_TARGET_PROPERTIES(test-client PROPERTIES EXCLUDE_FROM_ALL True)
SET_TARGET_PROPERTIES(test-server PROPERTIES EXCLUDE_FROM_ALL True)
SET_TARGET_PROPERTIES(test-server-extpoll PROPERTIES EXCLUDE_FROM_ALL True)

if(MSVC)
  target_compile_options(rbbs-ws PRIVATE /W4 /WX)
else()
  target_compile_options(rbbs-ws PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

set_property(TARGET rbbs-ws PROPERTY C_STANDARD 11)