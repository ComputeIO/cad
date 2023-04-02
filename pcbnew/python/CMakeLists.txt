# SWIG 4.1 or later required for proper C++20 support.
find_package( SWIG 4.0 REQUIRED COMPONENTS python )
include(UseSWIG)

set(SWIG_SOURCES
board.i                  board_item_container.i  netinfo.i        pcb_group.i            pcb_shape.i   plugins.i   version.i
board_connected_item.i   connectivity.i          pad.i            pcb_item_containers.i  pcb_target.i  track.i     zone.i
board_design_settings.i  footprint.i             pcb_bitmap.i     pcb_marker.i           pcb_text.i    typeinfo.i  zone_settings.i
board_item.i             layer_ids.i             pcb_dimension.i  pcb_plot_params.i      pcbnew.i      units.i
)
list(TRANSFORM SWIG_SOURCES PREPEND swig/)
foreach (swig_src IN LISTS SWIG_SOURCES)
    set_property(SOURCE ${swig_src} PROPERTY CPLUSPLUS ON)
endforeach()

set(SWIG_INCLUDES
    ${CMAKE_SOURCE_DIR}/pcbnew
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/scripting
    ${CMAKE_SOURCE_DIR}/pcbnew/python/scripting
    ${CMAKE_SOURCE_DIR}/common/swig
    ${CMAKE_SOURCE_DIR}/libs/kimath/include
    ${CMAKE_BINARY_DIR}
)

list(APPEND SWIG_TARG_DEFS
    $<$<CONFIG:Debug,RelwithDebInfo>:DEBUG>
)

swig_add_library(pcbnew_py
    TYPE SHARED
    LANGUAGE python
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES ${SWIG_SOURCES}
)

set_property(TARGET pcbnew_py
    PROPERTY SWIG_COMPILE_OPTIONS
    -nothreads # required, or pcbnew's threading will break
    -fastdispatch
    -fvirtual
    $<$<BOOL:${DOXYGEN_FOUND}>:-doxygen>
)
set_property(TARGET pcbnew_py
    PROPERTY SWIG_GENERATED_COMPILE_DEFINITIONS
    "${SWIG_TARG_DEFS}"
)

set_property(TARGET pcbnew_py PROPERTY SWIG_USE_TARGET_INCLUDE_DIRECTORIES ON)
target_link_libraries(pcbnew_py PUBLIC
    Python::Python
    nlohmann_json
    clipper
    clipper2
    common
)
target_include_directories(pcbnew_py PUBLIC ${SWIG_INCLUDES})
set_property(TARGET pcbnew_py PROPERTY SWIG_USE_SWIG_DEPENDENCIES ON)
