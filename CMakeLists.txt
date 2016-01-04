project(sppedtest)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c99")

set(SERVER_SOURCE_FILES
    server.c)

set(CLIENT_SOURCE_FILES
    client.c)

add_executable(server ${SERVER_SOURCE_FILES})
add_executable(client ${CLIENT_SOURCE_FILES})
