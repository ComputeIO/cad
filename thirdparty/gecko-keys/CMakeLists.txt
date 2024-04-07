add_library( gecko-keys OBJECT
    gecko_keys.cpp
)

target_include_directories( gecko-keys
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)