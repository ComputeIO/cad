# CMake script for finding wxPython/Phoenix library

# Copyright (C) 2018 CERN
# Author: Maciej Suminski <maciej.suminski@cern.ch>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, you may find one here:
# http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
# or you may search the http://www.gnu.org website for the version 2 license,
# or you may write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

# Exported variables:
#   WXPYTHON_VERSION: wxPython/Phoenix version,
#                     normally 3.x.x for wxPython, 4.x.x for Phoenix
#   WXPYTHON_FLAVOR: 'Phoenix' or 'wxPython'
#   WXPYTHON_TOOLKIT: base library toolkit (e.g. gtk2, gtk3, msw, osx_cocoa)
#   WXPYTHON_WXVERSION: wxWidgets version used by wxPython/Phoenix

# Create a CMake list containing wxPython version
find_package(Python3 3.6 COMPONENTS Interpreter REQUIRED)
execute_process(
    COMMAND ${Python3_EXECUTABLE} -c "import wx;print(wx.version())"
    RESULT_VARIABLE wxPython_VERSION_RESULT
    OUTPUT_VARIABLE wxPython_VERSION_FOUND
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY
)
execute_process(COMMAND ${Python3_EXECUTABLE} -c "import wx;print(wx.wxWidgets_version.split(' ')[1])"
    RESULT_VARIABLE wxPython_wxVersion_RESULT
    OUTPUT_VARIABLE wxPython_wxVersion_FOUND
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY
)

# Check to see if any version of wxPython is installed on the system.
if(wxPython_version_RESULT GREATER 0)
    message(FATAL_ERROR "wxPython/Phoenix does not appear to be installed")
endif()

# Turn the version string to a list for easier processing
separate_arguments(wxPython_VERSION_FOUND)
list(LENGTH wxPython_VERSION_FOUND _VERSION_LIST_LEN)

if(_VERSION_LIST_LEN LESS 3)
    message(FATAL_ERROR "Unknown wxPython version string: ${wxPython_version_FOUND}")
endif()

# Pheonix style: e.g. 4.0.1;gtk3;(phoenix)
list(GET wxPython_VERSION_FOUND 0 wxPython_VERSION)
list(GET wxPython_VERSION_FOUND 1 wxPython_TOOLKIT)
list(GET wxPython_VERSION_FOUND 2 wxPython_FLAVOR)

# Fix an inconsistency between the toolkit names reported by wx.version() and wx-config for cocoa
if(wxPython_TOOLKIT STREQUAL "osx-cocoa")
    set(wxPython_TOOLKIT "osx_cocoa")
endif()

set(wxPython_${wxPython_TOOLKIT}_FOUND true)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(wxPython
    VERSION_VAR wxPython_VERSION
    HANDLE_VERSION_RANGE
    HANDLE_COMPONENTS
)
