/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Janito V. Ferreira Filho
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "svg_import_plugin.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#include <wx/gdicmn.h>
#include <math/vector2d.h>

#include "convert_to_biu.h"
#include <eda_item.h>
#include "graphics_importer.h"

SVG_IMPORT_PLUGIN::SVG_IMPORT_PLUGIN() : doc([this](std::vector<std::array<double, 2>> vec, bool grb_dark){
        /* FIXME handle grb_dark */
        if (!grb_dark)
            return;
        std::vector<VECTOR2D> newvec;
        for (auto &p : vec)
            newvec.push_back(VECTOR2D(p[0], p[1]));
        std::cerr << "adding primitive" << std::endl;
        this->DrawPolygon(newvec, 0.01);
    }) {
}

bool SVG_IMPORT_PLUGIN::Load( const wxString& aFileName )
{
    wxCHECK( m_importer, false );
    return doc.load(aFileName.ToStdString());
}

bool SVG_IMPORT_PLUGIN::Import()
{
    doc.do_export();
    return true;
}

double SVG_IMPORT_PLUGIN::GetImageHeight() const
{
    if( !doc ) {
        wxASSERT_MSG(false, "SVG must have been loaded before checking height");
        return 0.0;
    }

    return doc.height();
}

double SVG_IMPORT_PLUGIN::GetImageWidth() const
{
    if( !doc ) {
        wxASSERT_MSG(false, "SVG must have been loaded before checking width");
        return 0.0;
    }

    return doc.width();
}

void SVG_IMPORT_PLUGIN::DrawPolygon( const std::vector< VECTOR2D >& aPoints, double aWidth )
{
    m_importer->AddPolygon( aPoints, aWidth );
}

