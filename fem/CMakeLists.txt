add_definitions(-DPCBNEW)

include_directories(BEFORE ${INC_BEFORE})
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/pcbnew
    ${CMAKE_SOURCE_DIR}
    ${INC_AFTER}
    )

set(FEM_SRCS
    ${CMAKE_SOURCE_DIR}/common/base_units.cpp
    fem_test.cpp
    fem_app_main.cpp  # just a dummy for now
    )

set_source_files_properties( ${CMAKE_SOURCE_DIR}/common/single_top.cpp PROPERTIES
    COMPILE_DEFINITIONS     "TOP_FRAME=FRAME_FEM"
    )

add_executable( fem WIN32 MACOSX_BUNDLE
    ${FEM_SRCS}
    )

add_dependencies( fem pcbcommon )

target_link_libraries( fem
    ${SPARSELIZARD_LIBRARIES}
    pcbcommon
    common
    kimath
    kiplatform
    ${PCBNEW_IO_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${Boost_LIBRARIES}
    )

# TODO: this is quite a horrible hack because includes are not globally visible right now!!
target_include_directories( fem PRIVATE /home/thomas/SLlibs/petsc/include)
target_include_directories( fem PRIVATE /home/thomas/SLlibs/petsc/arch-linux-c-opt/include)
