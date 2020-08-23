/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Brian Piccioni brian@documenteddesigns.com
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Brian Piccioni <brian@documenteddesigns.com>
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

#ifndef DIALOG_BOARD_REANNOTATE_H_
#define DIALOG_BOARD_REANNOTATE_H_

#include <class_board.h>
#include <class_module.h>
#include <class_text_mod.h>
#include <dialogs/dialog_board_reannotate_base.h>
#include <layers_id_colors_and_visibility.h>
#include <netlist_reader/pcb_netlist.h>
#include <pcb_base_frame.h>
#include <pcb_edit_frame.h>
#include <project.h>
#include <stdint.h>
#include <wx/wx.h>
#include <wx_html_report_panel.h>

#include <fctsys.h>
#include <frame_type.h>
#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <tools/selection_tool.h>

#define SORTXFIRST 0b000       //Sort on X
#define SORTYFIRST 0b100       //Sort on Y
#define ASCENDINGFIRST 0b000   //Sort low to high
#define DESCENDINGFIRST 0b010  //Sort high to low
#define ASCENDINGSECOND 0b000  //Sort low to high
#define DESCENDINGSECOND 0b001 //Sort high to low

#define MINGRID 1000
#define MAXERROR 10
#define VALIDPREFIX "_-+=/\\" //Prefixs can be alpha or these symbols

enum ActionCode
{
    UpdateRefDes,
    EmptyRefDes,
    InvalidRefDes,
    Exclude
};

enum AnnotationChoice
{
    AnnotateAll,
    AnnotateFront,
    AnnotateBack,
    AnnotateSelected
};

struct RefDesChange
{
    KIID        Uuid;
    wxString    NewRefDes;       //The new reference designation (F_U21)
    wxString    OldRefDesString; //What the old refdes preamble + number was
    bool        Front;           //True if on the front of the board
    ActionCode  Action;          //Used to skip (if #, etc)
};

struct RefDesInfo
{
    KIID        Uuid;
    bool        Front;              //True if on the front of the board
    wxString    RefDesString;       //What its refdes is R1, C2
    wxString    RefDesType;         //ie R, C, etc
    int         x, y;               //The coordinates
    int         roundedx, roundedy; //The coordinates after rounding.
    ActionCode  Action;             //Used to skip (if #, etc)
    LIB_ID      FPID;
};

struct RefDesTypeStr
{
    wxString     RefDesType;
    unsigned int RefDesCount;
};


class DIALOG_BOARD_REANNOTATE : public DIALOG_BOARD_REANNOTATE_BASE
{
public:
    DIALOG_BOARD_REANNOTATE( PCB_EDIT_FRAME* aParentFrame );
    ~DIALOG_BOARD_REANNOTATE();

private:
    PCB_EDIT_FRAME*  m_frame;
    MODULES          m_modules;
    PCB_SCREEN*      m_screen;
    PCBNEW_SELECTION m_selection;

    std::vector<RefDesChange>  m_ChangeArray;
    std::vector<RefDesInfo>    m_FrontModules;
    std::vector<RefDesInfo>    m_BackModules;
    std::vector<RefDesTypeStr> m_RefDesTypes;
    std::vector<wxString>      m_ExcludeArray;

    EDA_UNITS m_Units;

    bool m_Standalone;

    int m_SortCode;
    int m_GridIndex;
    int m_AnnotationChoice;
    int m_Severity;
    int m_UndoBaseCount; // # of Undo levels available when reannotate dialog is opened

    double m_SortGridx;
    double m_SortGridy;

    wxString m_FrontPrefixString;
    wxString m_BackPrefixString;
    wxString m_ValidPrefixes;

    APP_SETTINGS_BASE* m_Settings;

    std::vector<wxRadioButton*> m_sortButtons = { m_Down_Right, m_Right_Down, m_Down_Left,
        m_Left_Down, m_Up_Right, m_Right_Up, m_Up_Left, m_Left_Up };

    std::vector<wxRadioButton*> AnnotateWhat = { m_AnnotateAll, m_AnnotateFront, m_AnnotateBack,
        m_AnnotateSelection };


    std::vector<wxStaticBitmap*> Bitmaps = { reannotate_down_right_bitmap, reannotate_right_down_bitmap,
        reannotate_down_left_bitmap, reannotate_left_down_bitmap, reannotate_up_right_bitmap,
        reannotate_right_up_bitmap, reannotate_up_left_bitmap, reannotate_left_up_bitmap };

    APP_SETTINGS_BASE* m_Config;


    void GetParameters( void );
    void InitValues( void );
    void updateUndoRedoBtnState( void ); // Undo/Redo buttons visible when reannotation changes made

    void OnApplyClick( wxCommandEvent& event ) override;
    void OnCloseClick( wxCommandEvent& event ) override;
    void onUndoClick( wxCommandEvent& event ) override;
    void onRedoClick( wxCommandEvent& event ) override;
    void onUpdateSchCheckBoxClick( wxCommandEvent& event ) override;
    void FilterFrontPrefix( wxCommandEvent& event ) override;
    void FilterBackPrefix( wxCommandEvent& event ) override;

    void ShowReport( wxString aMessage, SEVERITY aSeverity );
    void LogModules( wxString& aMessage, std::vector<RefDesInfo>& aModules );
    void LogChangePlan( void );

    bool ReannotateBoard( void );
    bool BuildModuleList( std::vector<RefDesInfo>& aBadRefDes );
    void BuildChangeArray( std::vector<RefDesInfo>& aModules, unsigned int aStartRefDes,
            wxString aPrefix, bool aRemovePrefix, std::vector<RefDesInfo>& aBadRefDes );

    RefDesChange* GetNewRefDes( MODULE* aMod );

    int      RoundToGrid( int aCoord, int aGrid );
    wxString CoordTowxString( int aX, int aY );
    void     MakeSampleText( wxString& aMessage );
    void     FilterPrefix( wxTextCtrl* aPrefix );
};

#endif /* DIALOG_BOARD_REANNOTATECLASSES_H_ */
