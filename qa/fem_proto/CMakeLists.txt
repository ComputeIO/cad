find_package(Boost COMPONENTS unit_test_framework REQUIRED)
find_package( wxWidgets 3.0.0 COMPONENTS gl aui adv html core net base xml stc REQUIRED )


add_definitions(-DBOOST_TEST_DYN_LINK -DPCBNEW -DFEM_PROTO -DTEST_APP_NO_MAIN)

add_executable( fem_proto
    fem_proto_test.cpp

    ../qa_utils/mocks.cpp
    ../pcbnew_utils/board_file_utils.cpp
    ../qa_utils/test_app_main.cpp
    ../qa_utils/utility_program.cpp
    ../qa_utils/stdstream_line_reader.cpp
    ../../common/base_units.cpp
)

add_dependencies( fem_proto pcbcommon ${PCBNEW_IO_LIBRARIES} )

add_executable( fem_currentDensity
    fem_currentDensity.cpp

    ../qa_utils/mocks.cpp
    ../pcbnew_utils/board_file_utils.cpp
    ../qa_utils/test_app_main.cpp
    ../qa_utils/utility_program.cpp
    ../qa_utils/stdstream_line_reader.cpp
    ../../common/base_units.cpp
)

add_dependencies( fem_currentDensity pcbcommon ${PCBNEW_IO_LIBRARIES} )

include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/3d-viewer
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/pcbnew
    ${CMAKE_SOURCE_DIR}/pcbnew/router
    ${CMAKE_SOURCE_DIR}/pcbnew/tools
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs
    ${CMAKE_SOURCE_DIR}/polygon
    ${CMAKE_SOURCE_DIR}/common/geometry
    ${CMAKE_SOURCE_DIR}/libs/kimath/include/math
    ${CMAKE_SOURCE_DIR}/qa/common
    ${CMAKE_SOURCE_DIR}/qa
    ${CMAKE_SOURCE_DIR}/qa/qa_utils
    ${CMAKE_SOURCE_DIR}/qa/qa_utils/include
    ${CMAKE_SOURCE_DIR}/qa/pcbnew_utils/include
    ${Boost_INCLUDE_DIR}
    ${INC_AFTER}
)

target_link_libraries( fem_proto
    qa_pcbnew_utils
    3d-viewer
    connectivity
    pcbcommon
    pnsrouter
    gal
    common
    gal
    qa_utils
    dxflib_qcad
    tinyspline_lib
    nanosvg
    idf3
    unit_test_utils
    ${PCBNEW_IO_LIBRARIES}
    ${PCBNEW_FEM_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
    ${PYTHON_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PCBNEW_EXTRA_LIBS}    # -lrt must follow Boost
)

target_link_libraries( fem_currentDensity
    qa_pcbnew_utils
    3d-viewer
    connectivity
    pcbcommon
    pnsrouter
    gal
    common
    gal
    qa_utils
    dxflib_qcad
    tinyspline_lib
    nanosvg
    idf3
    unit_test_utils
    ${PCBNEW_IO_LIBRARIES}
    ${PCBNEW_FEM_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
    ${PYTHON_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PCBNEW_EXTRA_LIBS}    # -lrt must follow Boost
)

set( QA_FEM_SRCS
    # The main test entry points
    test_module.cpp

    # Shared between programs, but dependent on the BIU
    ${CMAKE_SOURCE_DIR}/qa/common/test_format_units.cpp
    ${CMAKE_SOURCE_DIR}/qa/common/test_array_options.cpp
)

add_executable( sparselizard_DC
    ${QA_FEM_SRCS}
    sparselizard_DC.cpp
    ../qa_utils/mocks.cpp
    ../pcbnew_utils/board_file_utils.cpp
    ../qa_utils/test_app_main.cpp
    ../qa_utils/utility_program.cpp
    ../qa_utils/stdstream_line_reader.cpp
    ../../common/base_units.cpp
)

add_dependencies( sparselizard_DC pcbcommon ${PCBNEW_IO_LIBRARIES} )

target_link_libraries( sparselizard_DC
    qa_pcbnew_utils
    3d-viewer
    connectivity
    pcbcommon
    common
    qa_utils
    unit_test_utils
    ${PCBNEW_IO_LIBRARIES}
    ${PCBNEW_FEM_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PCBNEW_EXTRA_LIBS}    # -lrt must follow Boost
)

kicad_add_boost_test( sparselizard_DC sparselizard_DC )