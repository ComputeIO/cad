# - Try to find Sparselizard (https://www.sparselizard.org/)
# Once done this will define
#
#  SPARSELIZARD_FOUND - system has Sparselizard installed
#  SPARSELIZARD_INCLUDE_DIR - the Sparselizard include directory
#  SPARSELIZARD_LIBRARIES - Link these to use Sparselizard

if(SPARSELIZARD_INCLUDE_DIR AND SPARSELIZARD_LIBRARIES)
    set(SPARSELIZARD_FIND_QUIETLY TRUE)
endif(SPARSELIZARD_INCLUDE_DIR AND SPARSELIZARD_LIBRARIES)

find_path(SPARSELIZARD_INCLUDE_DIR sparselizard/sparselizard.h)

find_library(SPARSELIZARD_LIBRARIES sparselizard )

# handle the QUIETLY and REQUIRED arguments and set SPARSELIZARD_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Sparselizard DEFAULT_MSG SPARSELIZARD_LIBRARIES SPARSELIZARD_INCLUDE_DIR)

mark_as_advanced(SPARSELIZARD_INCLUDE_DIR SPARSELIZARD_LIBRARIES)
