/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <panel_common_file_settings.h>

#include <bitmap_types.h>
#include <bitmaps.h>
#include <dialog_shim.h>
#include <kiface_i.h>
#include <pgm_base.h>
#include <id.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>


PANEL_COMMON_FILE_SETTINGS::PANEL_COMMON_FILE_SETTINGS( DIALOG_SHIM* aDialog, wxWindow* aParent )
        : PANEL_COMMON_FILE_SETTINGS_BASE( aParent ),
          m_dialog( aDialog )
{
    m_textEditorBtn->SetBitmap( KiBitmap( folder_xpm ) );
    m_pdfViewerBtn->SetBitmap( KiBitmap( folder_xpm ) );
}


PANEL_COMMON_FILE_SETTINGS::~PANEL_COMMON_FILE_SETTINGS()
{
}


bool PANEL_COMMON_FILE_SETTINGS::TransferDataToWindow()
{
    COMMON_SETTINGS* commonSettings = Pgm().GetCommonSettings();

    int timevalue = commonSettings->m_System.autosave_interval;
    wxString msg;

    msg << timevalue / 60;
    m_SaveTime->SetValue( msg );

    m_fileHistorySize->SetValue( commonSettings->m_System.file_history_size );

    m_preferCompressedPcb->SetValue( commonSettings->m_FileHandling.preferCompressedPcbFiles );
    m_compressionLevel->SetValue( commonSettings->m_FileHandling.compressionLevel );

    // TODO(JE) Move these into COMMON_SETTINGS probably
    m_textEditorPath->SetValue( Pgm().GetEditorName( false ) );
    m_defaultPDFViewer->SetValue( Pgm().UseSystemPdfBrowser() );
    m_otherPDFViewer->SetValue( !Pgm().UseSystemPdfBrowser() );
    m_PDFViewerPath->SetValue( Pgm().GetPdfBrowserName() );

    return true;
}


bool PANEL_COMMON_FILE_SETTINGS::TransferDataFromWindow()
{
    COMMON_SETTINGS* commonSettings = Pgm().GetCommonSettings();

    commonSettings->m_System.autosave_interval = m_SaveTime->GetValue() * 60;
    commonSettings->m_System.file_history_size = m_fileHistorySize->GetValue();

    commonSettings->m_FileHandling.preferCompressedPcbFiles = m_preferCompressedPcb->GetValue();
    commonSettings->m_FileHandling.compressionLevel = m_compressionLevel->GetValue();

    Pgm().SetEditorName( m_textEditorPath->GetValue() );

    Pgm().SetPdfBrowserName( m_PDFViewerPath->GetValue() );
    Pgm().ForceSystemPdfBrowser( m_defaultPDFViewer->GetValue() );
    Pgm().WritePdfBrowserInfos();

    Pgm().GetSettingsManager().Save( commonSettings );

    return true;
}


void PANEL_COMMON_FILE_SETTINGS::OnTextEditorClick( wxCommandEvent& event )
{
    // Ask the user to select a new editor, but suggest the current one as the default.
    wxString editorname = Pgm().AskUserForPreferredEditor( m_textEditorPath->GetValue() );

    // If we have a new editor name request it to be copied to m_editor_name and saved
    // to the preferences file. If the user cancelled the dialog then the previous
    // value will be retained.
    if( !editorname.IsEmpty() )
        m_textEditorPath->SetValue( editorname );
}


void PANEL_COMMON_FILE_SETTINGS::OnPDFViewerClick( wxCommandEvent& event )
{
    wxString mask( wxT( "*" ) );

#ifdef __WINDOWS__
    mask += wxT( ".exe" );
#endif

    wxString wildcard = _( "Executable files (" ) + mask + wxT( ")|" ) + mask;

    Pgm().ReadPdfBrowserInfos();
    wxFileName fn = Pgm().GetPdfBrowserName();

    wxFileDialog dlg( this, _( "Select Preferred PDF Browser" ), fn.GetPath(), fn.GetFullPath(),
                      wildcard, wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    m_otherPDFViewer->SetValue( true );
    m_PDFViewerPath->SetValue( dlg.GetPath() );
}


void PANEL_COMMON_FILE_SETTINGS::onUpdateUIPdfPath( wxUpdateUIEvent& event )
{
    bool enabled = m_otherPDFViewer->GetValue();
    m_PDFViewerPath->Enable( enabled );
    m_pdfViewerBtn->Enable( enabled );
}

