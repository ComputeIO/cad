/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Brian Piccioni brian@documenteddesigns.com
 * Copyright (C) 2004-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include    <sch_edit_frame.h>
#include    <tools/backannotate.h>
#include    <renum_type.h>
#include    <renum.h>

WX_STRING_REPORTER_FILTERED::WX_STRING_REPORTER_FILTERED(SEVERITY aSeverity) :
        m_MinSeverity(aSeverity) {
}

WX_STRING_REPORTER_FILTERED::~WX_STRING_REPORTER_FILTERED() {
}

REPORTER& WX_STRING_REPORTER_FILTERED::Report(const wxString &aText,
        REPORTER::SEVERITY aSeverity) {
    if (aSeverity < m_MinSeverity)
        return *this;

    m_string << aText << "\n";
    return *this;
}

bool WX_STRING_REPORTER_FILTERED::HasMessage() const {
    return !m_string.IsEmpty();
}

void RenumberFromPCBNew(SCH_EDIT_FRAME *aFrame, std::string &aNetlist) {

    wxString annotateerrors;
    WX_STRING_REPORTER_FILTERED reporter(REPORTER::SEVERITY::RPT_ERROR);

    BACK_ANNOTATE::SETTINGS settings = { reporter, false,    //processFootprints
            false,      //processValues
            true,       //processReferences
            false,      //ignoreStandaloneFootprints
            false,      //ignoreOtherProjects
            false };    //dryRun (not required)

    BACK_ANNOTATE backAnno(aFrame, settings);

    if (true != backAnno.BackAnnotateSymbols(aNetlist)) {
        aNetlist = _("Errors reported by eeSchema:\n")
                + reporter.m_string.ToStdString();     //Assume the worst
        aNetlist += _("\nAnnotation not performed!\n");
    } else {
        aNetlist = RENUM_OK;                    //All is well
    }

    g_CurrentSheet->UpdateAllScreenReferences();
    aFrame->SetSheetNumberAndCount();
    aFrame->SyncView();
    aFrame->OnModify();
    aFrame->GetCanvas()->Refresh();

}
