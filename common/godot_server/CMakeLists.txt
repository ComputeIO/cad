set(GODOT_SERVER_SRCS
    godot_server.cpp)

include_directories( BEFORE ${INC_BEFORE} )

include_directories(
    ./
)

add_library(godot_server STATIC ${GODOT_SERVER_SRCS})

target_link_libraries(godot_server
    ${Boost_}
    boost_system
    ${CMAKE_THREAD_LIBS_INIT})
