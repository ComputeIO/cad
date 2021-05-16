
# Sources for the pcbnew PLUGIN called ALTIUM_DESIGNER_PLUGIN, ALTIUM_CIRCUIT_STUDIO_PLUGIN and ALTIUM_CIRCUIT_MAKER_PLUGIN

set( PCBNEW_FEM_SRCS
    common/fem_descriptor.cpp
    common/fem_port.cpp
    common/fem_result.cpp
    sparselizard/sparselizard_DC_CurrentDensity.cpp
    sparselizard/sparselizard_mesher.cpp
    gmsh_mesher.cpp
    )

add_library( kicadfem STATIC ${PCBNEW_FEM_SRCS} )

target_link_libraries( kicadfem pcbcommon ${GMSH_LIBRARIES} ${SPARSELIZARD_LIBRARIES} )

target_include_directories( kicadfem PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        $<TARGET_PROPERTY:compoundfilereader,INTERFACE_INCLUDE_DIRECTORIES>
        )
