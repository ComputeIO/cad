/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Thomas Pointhuber <thomas.pointhuber@gmx.at>
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "gmsh_viewer.h"
#include "wx/wx.h"
#include "wx/sizer.h"
#include "wx/glcanvas.h"
#include <gmsh.h>
#include <gmsh/GmshGlobal.h>
#include <gmsh/GmshMessage.h>
#include <gmsh/GModel.h>
#include <gmsh/CommandLine.h>
#include <gmsh/OpenFile.h>
#include <gmsh/Context.h>



// include OpenGL
#ifdef __WXMAC__
#include "OpenGL/glu.h"
#include "OpenGL/gl.h"
#else
#include <GL/glu.h>
#include <GL/gl.h>
#endif

BEGIN_EVENT_TABLE(GMSH_VIEWER_WX, wxGLCanvas)
EVT_MOTION(GMSH_VIEWER_WX::mouseMoved)
EVT_LEFT_DOWN(GMSH_VIEWER_WX::mouseDown)
EVT_LEFT_UP(GMSH_VIEWER_WX::mouseReleased)
EVT_RIGHT_DOWN(GMSH_VIEWER_WX::rightClick)
EVT_LEAVE_WINDOW(GMSH_VIEWER_WX::mouseLeftWindow)
EVT_SIZE(GMSH_VIEWER_WX::resized)
EVT_KEY_DOWN(GMSH_VIEWER_WX::keyPressed)
EVT_KEY_UP(GMSH_VIEWER_WX::keyReleased)
EVT_MOUSEWHEEL(GMSH_VIEWER_WX::mouseWheelMoved)
EVT_PAINT(GMSH_VIEWER_WX::render)
END_EVENT_TABLE()
 
 
// some useful events to use
void GMSH_VIEWER_WX::mouseMoved(wxMouseEvent& event) {}
void GMSH_VIEWER_WX::mouseDown(wxMouseEvent& event) {}
void GMSH_VIEWER_WX::mouseWheelMoved(wxMouseEvent& event) {}
void GMSH_VIEWER_WX::mouseReleased(wxMouseEvent& event) {}
void GMSH_VIEWER_WX::rightClick(wxMouseEvent& event) {}
void GMSH_VIEWER_WX::mouseLeftWindow(wxMouseEvent& event) {}
void GMSH_VIEWER_WX::keyPressed(wxKeyEvent& event) {}
void GMSH_VIEWER_WX::keyReleased(wxKeyEvent& event) {}
 

class drawContextWx : public drawContextGlobal{
 public:
  void draw( drawContext* ctx)
  {
    ctx->draw3d();
    ctx->draw2d();
  }
};

void GMSH_VIEWER_WX::ClearAllViews()
{
    std::vector<int> tags;
    gmsh::view::getTags( tags );

    for( int tag : tags )
    {
        gmsh::view::remove( tag );
    }
}

void GMSH_VIEWER_WX::Open( std::string aFile )
{
    gmsh::open( aFile );
}

void GMSH_VIEWER_WX::Initialize()
{
    gmsh::initialize();
}

void GMSH_VIEWER_WX::Finalize()
{
    gmsh::finalize();
}

GMSH_VIEWER_WX::GMSH_VIEWER_WX(wxFrame* parent, int* args) :
    wxGLCanvas(parent, wxID_ANY, args, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE)
{
	m_context = new wxGLContext(this);

    Initialize();

    _ctx = new drawContext();

    drawContext::setGlobal( new drawContextWx );

    // To avoid flashing on MSW
    SetBackgroundStyle(wxBG_STYLE_CUSTOM);
}
 
GMSH_VIEWER_WX::~GMSH_VIEWER_WX()
{
	delete m_context;
}

void GMSH_VIEWER_WX::resized(wxSizeEvent& evt)
{
//	wxGLCanvas::OnSize(evt);
	
    Refresh();
}
 
 
int GMSH_VIEWER_WX::getWidth()
{
    return GetSize().x;
}
 
int GMSH_VIEWER_WX::getHeight()
{
    return GetSize().y;
}
 
 
void GMSH_VIEWER_WX::render( wxPaintEvent& evt )
{
    if(!IsShown()) return;
    
    wxGLCanvas::SetCurrent(*m_context);


    //wxPaintDC(this); // only to be used in paint events. use wxClientDC to paint outside the paint event

    // Maybe we could use gmsh::graphics::draw()
    // That could allow us to remove _ctx, the only reason the DENABLE_PRIVATE_API option is on.

    _ctx->viewport[2] = this->getWidth();
	_ctx->viewport[3] = this->getHeight();

	glViewport(_ctx->viewport[0], _ctx->viewport[1],
				_ctx->viewport[2], _ctx->viewport[3]);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	_ctx->draw3d();
	_ctx->draw2d();


    glFlush();
    SwapBuffers();
}