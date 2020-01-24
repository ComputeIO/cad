# Version 6 Road Map

This document is the KiCad version 6 Developer's road map document.  It is
living document that should be maintained during the version 6 development
cycle.  The goal of this document is to provide an overview for developers
of the goals for the project for the version 6 release of KiCad.  It is
broken into sections for each major component of the KiCad source code and
documentation.  It defines tasks that developers an use to contribute to the
project and provides updated status information.  Tasks should define clear
objectives and avoid vague generalizations so that a new developer can complete
the task.  It is not a place for developers to add their own personal wish.
list.  It should only be updated with approval of the project manager after
discussion with the lead developers.

Each entry in the road map is made up of four sections.  The goal should
be a brief description of the what the road map entry will accomplish.  The
task section should be a list of deliverable items that are specific enough
hat they can be documented as completed.  The dependencies sections is a list
of requirements that must be completed before work can begin on any of the
tasks.  The status section should include a list of completed tasks or marked
as complete as when the goal is met.

<!-- [TOC] Note: as of 2020-01-24, GitLab does not support generating tables
of contents in markdown documents located in the source tree.  See:
https://gitlab.com/gitlab-org/gitlab/issues/21901 -->

# General
This section defines the tasks that affect all or most of KiCad or do not
fit under as specific part of the code such as the board editor or the
schematic editor.

## User Interface Modernization
**Goal:**

Give KiCad a more modern user interface with dockable tool bars and windows.
Create perspectives to allow users to arrange dockable windows as they prefer.

**Task:**
- Take advantage of the advanced UI features in wxAui such as detaching and
  hiding.
- Clean up menu structure. Menus must allow access to all features of the
  program in a clear and logical way. Currently some functions of Pcbnew are
  accessible only through tool bars
- Redesign dialogs, make sure they are following same style rules.
- Check quality of translations. Either fix or remove bad quality translations.
- Develop a global shortcut manager that allows the user assign arbitrary
  shortcuts for any tool or action.

**Dependencies:**
- None

**Status:**
- No progress.

## Object Properties and Introspection
**Goal:**

Provide an object introspection system using properties.

**Task:**
- Select existing or develop property system.
- Add definable properties to base objects.
- Create introspection framework for manipulating object properties.
- Serialization of properties to and from files and/or other I/O structures.
- Create tool to edit property namespace/object name/name/type/value table.

**Dependencies:**
- None

**Status:**
- In progress.

**Team:**
- Orson

## New project and user settings framework
**Goal:**

Improve management of project and user settings, removing settings that are
not associated with design data from the design files.

**Task:**
- Switch from wxConfig to JSON-based settings files
- Centralize settings file management (new SETTINGS_MANAGER object)
- Add support for versioned settings and automatic migration
- Create new project settings directory
- Migrate settings out of the board file (etc) into new settings files

**Dependencies:**
- None

**Status:**
- In progress

**Team:**
- Jon

## Color theme support
**Goal:**

Support storing color themes in standalone settings files, easy switching
between color themes, and a better color editor across all applications.

**Task:**
- Migrate color settings storage into a unified color theme file
- New color settings GUI that supports managing themes

**Dependencies:**
- [New project and user settings framework](#object-properties-and-introspection)

**Status:**
- In progress

**Team:**
- Jon

## String substitution
**Goal:**

Allow substituting object properties into strings through an escape sequence.

**Task:**
- Migrate color settings storage into a unified color theme file
- New color settings GUI that supports managing themes

**Dependencies:**
- [Object Properties and Introspection](#new-project-and-user-settings-framework)

**Status:**
- No progress.

**Team:**
- Jon


# Eeschema: Schematic Editor
This section applies to the source code for the Eeschema schematic editor.

## Coherent SCHEMATIC Object
**Goal:**

Clean up the code related to the schematic object(s) into a coherent object for
managing and manipulating the schematic that can be used by third party tools
and Python scripting.

**Task:**
- Move handling of root sheet object to SCHEMATIC object.
- Move SCH_SCREENS code into SCH_OBJECT.
- Build and maintain schematic hierarchy in SCHEMATIC object rather than
  recreating on the fly every time the hierarchical information is required.
- Optionally build and maintain netlist during editing for extended editing
  features.
- Add any missing functionality to the SCHEMATIC object.

**Dependencies:**
- None

**Status:**
- In progress.

**Team:**
- Wayne

## Implement Sweet (S-Expression) Symbol Libraries
**Goal:**

Make symbol library design more robust and feature rich.  Use s-expressions
to make component library files more readable.

**Task:**
- Use sweet component file format for component libraries.

**Dependencies:**
- None

**Status:**
- Initial SWEET library file format document  written.

**Team:**
- Wayne

## S-Expression File Format
**Goal:**

Make schematic file format more readable, add new features, and take advantage
of the s-expression parser and formatter capability used in Pcbnew.

**Task:**
- Finalize feature set and file format.
- Discuss the possibility of dropping the unit-less proposal temporarily to get
  the s-expression file format and SWEET library format implemented without
  completely rewriting Eeschema.
- Add new s-expression file format to plugin.

**Dependencies:**
- [S-expression file format](#s-expression-file-format).

**Status:**
- File format document initial draft complete.

**Team:**
- Wayne

## Move Common Schematic Code into a Shared Object
**Goal:**

Refactor all schematic object code so that it can be built into a shared object
for use by the schematic editor, Python module, and linked into third party
programs.

**Task**
- Split schematic object code from schematic and component editor code.
- Generate shared object from schematic object code.
- Update build configuration to build schematic and component editors
  against new schematic shared object.

**Dependencies:**
- None

**Progress:**
- No progress.

**Team:**
- Wayne

## ERC Improvements
**Goal:**

Improve the coverage and usability of the electrical rules checker (ERC).

**Task:**
- Refactor and clean up the ERC codebase
- Store ERC settings with a project
- Improve ERC UI and UX
- Add new ERC features requested by users

**Dependencies:**
- [New project and user settings framework](#new-project-and-user-settings-framework)

**Status:**
- [Specification written](https://docs.google.com/document/d/19Dg5e_Ez25AnGzMN5f1XsZkz1DjTY1g-kkRJDx4e_GQ/edit#)

**Team:**
- Jon

## Port Editing Tools to New Tool Framework
**Goal:**

Convert all editing tool to new tool framework.

**Task:**
- Rewrite existing editing tools using the new tool framework.
- Add new capabilities supported by the new tool framework to existing
  editing tools.
- Remove legacy tool framework.

**Dependencies:**
- None

**Status:**
- Complete

## Net Highlighting
**Goal:**
Highlight wires, buses, and junctions when corresponding net in Pcbnew is selected.

**Task:**
- Add communications link to handle net selection from Pcbnew.
- Implement highlight algorithm for net objects.
- Highlight objects connected to net selected in Pcbnew.

**Dependencies:**
- None.

**Status:**
- Complete.

## Allow Use of System Fonts
**Goal:**

Currently the schematic editor uses the stroke drawn fonts which aren't really
necessary for accurate printing of schematics.  Allow the use of system fonts
for schematic text.

**Task:**
- Determine which library for font handling makes the most sense, wxWidgets or
  freetype.
- Add support for selecting text object fonts.

**Dependencies:**
- [S-expression schematic file format](#s-expression-file-format).

**Status:**
- No progress.

## Symbol and Netlist Attributes
**Goal:**

Provide a method of passing information to other tools via the net list.

**Task:**
- Add virtual components and attributes to netlist to define properties that
  can be used by other tools besides the board editor.
- Attributes (properties) are automatically included as part of the new file
  format.

**Dependencies:**
- [S-expression schematic file format](#s-expression-file-format).

**Status:**
- No progress.

## Orthogonal Wire Drag
**Goal:**

Keep wires and buses orthogonal when dragging a symbol.

**Task:**
- Add code to new tool framework to allow for orthogonal dragging of symbols.

**Dependencies:**
- [New tool framework](#port-editing-tools-to-new-tool-framework).

**Status:**
- No progress.

## Custom Wire and Bus Attributes
**Goal:**

Allow for wires and buses to have different widths, colors, and line types.

**Task:**
- Add code to support custom wire and bus attributes.
- Add user interface element to support changing wire and bus attributes.

**Dependencies:**
- [S-Expression File Format](#s-expression-file-format).

**Status:**
- No progress.

## Connectivity Improvements
**Goal:**

Support structured buses, real time netlist calculations, and other
connectivity improvements.

**Task:**
- Keep netlist up to date real time.
- Add support for structured bus definitions.
- Possible real time ERC checking.

**Dependencies:**
- None.

**Status:**
- Real time netlist and structured bus support complete.

**Team:**
- Jon

## Python Support
**Goal:**

SWIG all schematic low level objects and coherent schematic object to
provide Python interface for manipulating schematic objects.

**Task:**-
- Create SWIG wrappers for all low level schematic, symbol library, and
coherent schematic object code.
- Add Python interpreter launcher.

**Dependencies:**
- [Coherent Schematic Object](#coherent-schematic-object).

**Status:**
- No Progress.


# CvPcb: Footprint Association Tool
This section covers the source code of the footprint assignment tool CvPcb.


# Pcbnew: Circuit Board Editor
This section covers the source code of the board editing application Pcbnew.

## Push and Shove Router Improvements

**Goal:**

Add finishing touches to push and shove router.

**Task:**
- Delete and backspace in idle mode
- Differential pair clearance fixes.
- Differential pair optimizer improvements (recognize differential pairs)
- Persistent differential pair gap/width setting.
- Walk around in drag mode.
- Optimize trace being dragged too. (currently no optimization)
- Auto-finish traces (if time permits)
- Additional optimization pass for spring back algorithm using area-minimization
  strategy. (improves tightness of routing)
- Restrict optimization area to view port (if user wants to)
- Support 45 degree tuning meanders.
- Respect trace/via locking!
- Keep out zone support.
- Microwave tools to be added as parameterized shapes generated by Python
  scripts.
- BGA fan out support.
- Drag footprints with traces connected.

**Dependencies:**
- None.

**Status:**
- In progress.

**Team:**
- Tom
- Seth

## Selection Filtering
**Goal:**

Make the selection tool easier for the user to determine which object(s) are
being selected by filtering.

**Task:**
- Provide filtered object selection by adding a third tab to the layer manager
  or possibly some other UI element to provide filtered selection options.

**Dependencies:**
- [Object Properties and Introspection](#object-properties-and-introspection)

**Status:**
- In progress.

**Team:**
- Orson

## Design Rule Check (DRC) Improvements.
**Goal:**

Create additional DRC tests for improved error checking.

**Task:**
- Remove floating point code from clearance calculations to prevent rounding
  errors.
- Add checks for component, silk screen, and mask clearances.
- Add checks for keep out zones.
- Remove DRC related limitations such as no arc or text on copper layers.
- Add option for saving and loading DRC options.

**Dependencies:**
- [Constraint Management System](#constraint-management-system).

**Progress:**
- In progress.

**Team:**
- Tom
- Orson
- Jon

## Linked Objects
**Goal:**

Provide a way to allow external objects such as footprints to be externally
linked in the board file so that changes in the footprint are automatically
updated.  This will allow a one to many object relationship which can pave
the way for reusable board modules.

**Task:**
- Add externally and internally linked objects to the file format to allow for
  footprints and/or other board objects to be shared (one to many relationship)
  instead of only supporting embedded objects (one to one relationship) that
  can only be edited in place.

**Dependencies:**
- None.

**Status:**
- No progress.

## Pin and Part Swapping
**Goal:**

Allow Pcbnew to perform pin and/or part swapping during layout so the user
does not have to do it in Eeschema and re-import the net list.

**Task:**
- Provide forward and back annotation between the schematic and board editors.
- Define netlist file format changes required to handle pin/part swapping.
- Update netlist file formatter and parser to handle file format changes.
- Develop a netlist comparison engine that will produce a netlist diff that
  can be passed between the schematic and board editors.
- Create pin/part swap dialog to manipulate swappable pins and parts.
- Add support to handle net label back annotation changes.

**Dependencies:**
- [S-Expression File Format](#s-expression-file-format).

**Status:**
- No progress.

## Keepout Zones.
**Goal:**

Add support for keepout zones on boards and footprints.

**Task:**
- Add keepout support to zone classes.
- Add keepout zone support to board editor.
- Add keepout zone support to library editor.

**Dependencies:**
- None

**Progress:**
- Complete.

## Clipboard Support
**Goal:**

Provide clipboard cut and paste for footprints.

**Task:**
- Clipboard cut and paste to and from clipboard of footprints in footprint
  editor.

**Dependencies:**
- None.

**Status:**
- Complete.

## Net Highlighting
**Goal:**

Highlight rats nest links and/or traces when corresponding net in Eeschema is selected.

**Task:**
- Add communications link to handle net selection from Eeschema.
- Implement highlight algorithm for objects connected to the selected net.
- Highlight objects connected to net selected in Eeschema

**Dependencies:**
- None.

**Status:**
- Complete.

## Visibility and Colors Improvements
**Goal:**

Improve the controls and features of layer and object color / visibility.
[Specification document](https://docs.google.com/document/d/1iEFqhLd4CHjx86uqAUxH37lhfppWQ7E0Hi2oMn_T4CE/edit#)

**Task:**
- Add the concept of "saved views" to store a snapshot of current visibility
- Support setting object opacity independently of copper layer opacity
- Create new appearance widget to replace the current layers manager

**Dependencies:**
- [New project and user settings framework](#new-project-and-user-settings-framework)
- [Color theme support](#color-theme-support)

**Status:**
- In development

**Team:**
- Jon

## Hatched Zone Filling
**Goal:**

Currently Pcbnew only supports solid zone files.  Add option to fill zones
with hatching.

**Task:**
- Determine zone fill method, required filling code, and file format requirements.
- Add hatch option and hatch configuration to zone dialog.

**Dependencies:**
- None.

**Status:**
- Complete.

## Board Stack Up Impedance Calculator
**Goal:**

Provide a calculator to compute trace impedances using a full board stackup.
Maybe this should be included in the PCB calculator application.

**Task:**
- Design a trace impedance calculator that includes full board stackup.

**Dependencies:**
- None.

**Status:**
- In progress.

## Net Class Improvements
**Goal:**

Add support for route impedance, color selection, etc in net class object.

**Task:**
- Determine parameters to add to net class object.
- Implement file parser and formatter changes to support net class object
  changes.
- Implement tools to work with new net class parameters.
- Create UI elements to configure new net class parameters.
- Update the render tab UI code to view traces by net class.

**Dependencies:**
- [S-Expression File Format](#s-expression-file-format)
- [New project and user settings framework](#new-project-and-user-settings-framework)

**Status:**
- No progress.

**Team:**
- Jon (colors and UI)

## Ratsnest Improvements
**Goal:**

Add support for curved rats and per net color and visibility settings.

**Task:**
- Implement rat curving to minimize overlapped rats.
- Implement UI code to configure ratsnest color and visibility.
- Update ratsnest code to handle per net color and visibility.

**Dependencies:**
- [New project and user settings framework](#new-project-and-user-settings-framework)

**Status:**
- Curved rat support complete.
- Ratsnest colors and visibility in progress.

**Team:**
- Jon (ratsnest colors and visibility)

## DXF Improvements
**Goal:**

- Allow for anchor point setting and layer mapping support on DXF import and
  export multiple board layers to a single DXF file.

**Task:**
- Provide method to select DXF import anchor point.
- Add user interface to allow mapping DXF layers to board layers.
- Modify DXF plotting to export multiple layers to a single file.

**Dependencies:**
- None.

**Status:**
- No progress.

## Improve Dimension Tool
**Goal:**

Make dimensions link to objects and change when objects are modified and add
basic mechanical constraints.

**Task:**
- Add code to link dimension to objects.
- Add basic mechanical constraints like linear distance and angle.

**Dependencies:**
- None.

**Status:**
- In progress.

**Team:**
- Tom

## Constraint Management System
**Goal:**

Implement full featured constraint management system to allow for complex
board constraints instead of netclass only constraints.

**Task:**
- Write specification to define requirement of new constraint system.
- Implement new constraint system including file format changes.
- Allow constraints to be defined in schematic editor and passed to board
  editor via netlist.
- Update netlist file format to support constraints.
- Update DRC to test new constraints.

**Dependencies:**
- None.

**Status**
- Specification being written

**Team:**
- Tom
- Jon

## Append Board in Project Mode
**Goal:**

Allow appending to the board when running Pcbnew in the project mode.

**Task:**
- Enable append board feature in project mode.
- Extend copy/paste feature to introduce paste special tool to add prefix
  and/or suffix to nets of pasted/appended objects.

**Dependencies:**
- None.

**Status:**
- No progress.

## Grid and Auxiliary Origin Improvements
**Goal:**

Allow reset grid and auxiliary origin without hotkey only.  Add support to
make all coordinates relative to the plot origin.

**Task:**
- Add reset grid and auxiliary origin commands to menu entry and/or toolbar
  button.
- Add code to dialogs to allow coordinates to be specified relative to the
  plot origin.

**Dependencies:**
- None.

**Status:**
- Relative coordinate entry in progress.

## Addition Mechanical Layers
**Goal:**

Add more mechanical layers.

**Task:**
- Add remaining mechanical layers for a total of 32.

**Dependencies:**
- None.

**Status:**
- No progress.

**Team:**
- Wayne

## Layer Renaming
**Goal:**

Allow mechanical layers to be renamed.

**Task:**
- Quote layer names in file format to support any printable characters in
  layer names.
- Add user interface to allow mechanical layers to be renamed.

**Dependencies:**
- None.

**Status:**
- Quoted layer names complete.

**Team:**
- Wayne

## Stable Python API
**Goal:**

Create a Python wrapper to hide the SWIG generated API.

**Task:**
- Document new Python API.
- Write Python API.

**Dependencies:**
- None.

**Status:**
- Initial technical specification drafted.

## Track Refining
**Goal:**

Add support for teardrops and automatically updating length tuning
meandering.

**Task:**
- Draft specification for track refining.
- Implement support for teardrops.
- Implement support for changing tuned length meandering.

**Dependencies:**
- None.

**Status:**
- Initial technical specification drafted.

**Team:**
- Seth

## Groups and Rooms
**Goal:**

Support grouping board objects into reusable snippets.

**Task:**
- Write design specification.
- Update board file format to support grouped objects.
- Add user interface code to support grouped board objects.

**Dependencies:**
- None.

**Status:**
- Initial technical specification drafted.

**Team:**
- Seth

## Pad Stack Support
**Goal:**

Add padstack support.

**Task:**
- Write pad stack design specification.
- Update board file format to support pad stacks.
- Add user interface code to support designing pad stack objects.
- Update push and shove router to handle pad stacks.
- Update zone filling to handle pad stacks.
- Update DRC to handle pad stacks.

**Dependencies:**
- None.

**Status:**
- Initial technical specification drafted.

**Team:**
- Orson

## Net Ties
**Goal:**

Add support for net ties.

**Task:**
- Write net tie design specification.
- Implement board file support for net ties.
- Implement schematic file support for net ties.
- Update ERC and DRC to handle net ties.
- Update netlist to pass net tie information from schematic to board.
- Add user interface support for net ties to editors.

**Dependencies:**
- [S-Expression File Format](#s-expression-file-format).

**Status:**
- No Progress.

## Anti-pad Improvements
**Goal:**

Use anti-pads on vias and through hold pads on internal layers as required.

**Task:**-
- Revise zone filling algorithm to create anti-pad on internal layers.

**Dependencies:**
- None.

**Status:**
- No Progress.

## Thermal Relief Improvements
**Goal:**

Allow for custom thermal reliefs in zones and custom pad shapes.

**Task:**-
- Write technical specification to define requirements, alternate unions,
  knockouts, union spokes, etc.
- Revise zone filling thermal relief support to handle new requirements.
- Update board file format for new thermal relief requirements.
- Add user interface support for thermal relief definitions.

**Dependencies:**
- None.

**Status:**
- No Progress.

## Merge KiCad2Step
**Goal:**

Merge export to STEP file code from KiCad2Step so that conversion does
not run in a separate process.

**Task:**-
- Merge KiCad2Step code into Pcbnew code base.
- Remove unused parser code.

**Dependencies:**
- None.

**Status:**
- No Progress.

## 3D Model Improvements
**Goal:**

Add opacity to 3D model support and convert from path look up to library
table to access 3D models.

**Task:**-
- Add opacity support to footprint library file format.
- Add library table 3D model support to footprint library file format.
- Create remapping utility to map from path look up to library table look up.
- Add user interface support for 3D model opacity.
- Add user interface support accessing 3D models via library table.

**Dependencies:**
- None.

**Status:**
- No Progress.

## IPC-2581 Support
**Goal:**

Add support for exporting to and importing from IPC-2581.

**Task:**-
- Add IPC-2581 export code.
- Add IPC-2581 import code.

**Dependencies:**
- None.

**Status:**
- No Progress.

**Team:**
- Seth

## Curved Trace Support
**Goal:**

Add curved trace support to the board editor.

**Task:**-
- Add curved trace support to track object code.
- Add support to board file format for curved traces.
- Update zone fill algorithm to support curved fills.
- Update router to support curved traces.
- Update DRC to handle curved traces and fills.

**Dependencies:**
- None.

**Status:**
- In Development

**Team:**
- Seth


# GerbView: Gerber File Viewer

This section covers the source code for the GerbView gerber file viewer.

There are currently no roadmap features specific to GerbView for KiCad 6.0
