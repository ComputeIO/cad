/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SCH_DRAWING_TOOLS_H
#define SCH_DRAWING_TOOLS_H

#include <tools/ee_tool_base.h>
#include <sch_base_frame.h>
#include <status_popup.h>

class SCH_COMPONENT;
class SCH_BUS_WIRE_ENTRY;
class SCH_LABEL;
class SCHLIB_FILTER;
class SCH_EDIT_FRAME;
class EE_SELECTION_TOOL;


/**
 * Tool responsible for drawing/placing items (symbols, wires, buses, labels, etc.).
 */

class SCH_DRAWING_TOOLS : public EE_TOOL_BASE<SCH_EDIT_FRAME>
{
public:
    SCH_DRAWING_TOOLS();
    ~SCH_DRAWING_TOOLS() override {}

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    int PlaceSymbol( const TOOL_EVENT& aEvent );
    int SingleClickPlace( const TOOL_EVENT& aEvent );
    int TwoClickPlace( const TOOL_EVENT& aEvent );
    int DrawSheet( const TOOL_EVENT& aEvent );
    int PlaceImage( const TOOL_EVENT& aEvent );

private:
    /**
     * @return the "free" area of the canvas in pixels.
     * the free area is the area that create no autopan when the mouse cursor
     * is inside this area
     */
    EDA_RECT GetCanvasFreeAreaPixels();

    /**
     * Get the next queued text item.
     *
     * @return next SCH_TEXT* or nullptr if empty
     */
    SCH_TEXT* createNewText( const VECTOR2I& aPosition, int aType );

    SCH_HIERLABEL* importHierLabel( SCH_SHEET* aSheet );

    SCH_SHEET_PIN* createSheetPin( SCH_SHEET* aSheet, SCH_HIERLABEL* aLabel );

    void sizeSheet( SCH_SHEET* aSheet, VECTOR2I aPos );

    ///< Set up handlers for various events.
    void setTransitions() override;

    std::vector<PICKED_SYMBOL> m_symbolHistoryList;
    std::vector<PICKED_SYMBOL> m_powerHistoryList;

    PINSHEETLABEL_SHAPE                   m_lastSheetPinType;
    PINSHEETLABEL_SHAPE                   m_lastGlobalLabelShape;
    EDA_ANGLE                             m_lastTextAngle;
    TEXT_ATTRIBUTES::HORIZONTAL_ALIGNMENT m_lastTextHorizontalAlignment;
    TEXT_ATTRIBUTES::VERTICAL_ALIGNMENT   m_lastTextVerticalAlignment;
    bool                                  m_lastTextBold = false;
    bool                                  m_lastTextItalic = false;
    KIFONT::FONT*                         m_lastFont = nullptr;

    ///< Re-entrancy guards
    bool m_inPlaceSymbol = false;
    bool m_inPlaceImage = false;
    bool m_inSingleClickPlace = false;
    bool m_inTwoClickPlace = false;
    bool m_inDrawSheet = false;

    std::unique_ptr<STATUS_TEXT_POPUP> m_statusPopup;
};

#endif /* SCH_DRAWING_TOOLS_H */
