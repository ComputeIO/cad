# KiCad README

For specific documentation like Compiling, GUI translation, old
changelogs see the [Documentation](Documentation) subfolder.

## Files
* [AUTHORS.txt](AUTHORS.txt) - The authors, contributors, document writers and translators list
* [CMakeList.txt](CMakeList.txt) - Main CMAKE build tool script
* [copyright.h](copyright.h) - A very short copy of the GNU General Public License to be included in new source files
* [CTestConfig.cmake](CTestConfig.cmake) - Support for CTest and CDash testing tools
* [Doxyfile](Doxyfile) - Doxygen config file for KiCad
* [INSTALL.txt](INSTALL.txt) - The release (binary) installation instructions
* [uncrustify.cfg](uncrustify.cfg) - Uncrustify config file for uncrustify sources formatting tool
* [_clang-format](_clang-format) - clang config file for clang-format sources formatting tool

## Subdirectories

* [3d-viewer](3d-viewer)         - Sourcecode of the 3D viewer
* [bitmap2component](bitmap2component)  - Sourcecode of the bitmap to pcb artwork converter
* [bitmaps_png](bitmaps_png)       - Menu and program icons
* [CMakeModules](CMakeModules)      - Modules for the CMAKE build tool
* [common](common)            - Sourcecode of the common library
* [cvpcb](cvpcb)             - Sourcecode of the CvPCB tool
* [demos](demos)             - Some demo examples
* [Documentation](Documentation)     - Developer documentation. Old changelogs etcetera.
* [eeschema](eeschema)          - Sourcecode of the schematic editor
* [gerbview](gerbview)          - Sourcecode of the gerber viewer
* [helpers](helpers)           - Helper tools and utilities for development
* [include](include)           - Interfaces to the common library
* [kicad](kicad)             - Sourcecode of the project manager
* [lib_dxf](lib_dxf)           - Sourcecode of the dxf reader/writer library
* [new](new)               - Staging area for the new schematic library format
* [pagelayout_editor](pagelayout_editor) - Sourcecode of the pagelayout editor
* [patches](patches)           - Collection of patches for external dependencies
* [pcbnew](pcbnew)           - Sourcecode of the printed circuit board editor
* [plugins](plugins)           - Sourcecode of the new plugin concept
* [polygon](polygon)           - Sourcecode of the polygon library
* [potrace](potrace)           - Sourcecode of the potrace library, used in bitmap2component
* [qa](qa)                - Testcases using the python interface
* [resources](resources)         - Resources for freedesktop mime-types for linux
* [scripting](scripting)         - SWIG Python scripting definitions and build scripts
* [scripts](scripts)           - Example scripts for distribution with KiCad
* [template](template)          - Project and pagelayout templates
* [tools](tools)             - Other miscellaneous helpers for testing
* [utils](utils)             - Small utils for kicad, e.g. IDF tools

