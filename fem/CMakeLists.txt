add_definitions(-DPCBNEW)

include_directories(BEFORE ${INC_BEFORE})
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}
    ${INC_AFTER}
    )

set(FEM_SRCS
    fem_test.cpp
    )

add_executable( fem WIN32 MACOSX_BUNDLE
    ${FEM_SRCS}
    )
target_link_libraries( fem
    common
    pcbcommon
    ${PCBNEW_IO_LIBRARIES}
    ${SPARSELIZARD_LIBRARIES}
    )

# TODO: this is quite a horrible hack!!
target_include_directories( fem PRIVATE /home/thomas/SLlibs/petsc/include)
target_include_directories( fem PRIVATE /home/thomas/SLlibs/petsc/arch-linux-c-opt/include)
