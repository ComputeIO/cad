# - Try to find gmsh (https://gmsh.info/)
# Once done this will define
#
#  GMSH_FOUND - system has gmsh installed
#  GMSH_INCLUDE_DIR - the gmsh include directory
#  GMSH_LIBRARIES - Link these to use gmsh

if(GMSH_INCLUDE_DIR AND GMSH_LIBRARIES)
    set(GMSH_FIND_QUIETLY TRUE)
endif(GMSH_INCLUDE_DIR AND GMSH_LIBRARIES)

find_path(GMSH_INCLUDE_DIR gmsh.h)

find_library(GMSH_LIBRARIES gmsh )

# handle the QUIETLY and REQUIRED arguments and set GMSH_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Gmsh DEFAULT_MSG GMSH_LIBRARIES GMSH_INCLUDE_DIR)

mark_as_advanced(GMSH_INCLUDE_DIR GMSH_LIBRARIES)
