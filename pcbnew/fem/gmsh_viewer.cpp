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
 
// Vertices and faces of a simple cube to demonstrate 3D render
// source: http://www.opengl.org/resources/code/samples/glut_examples/examples/cube.c
GLfloat v[8][3];
GLint faces[6][4] = {  /* Vertex indices for the 6 faces of a cube. */
    {0, 1, 2, 3}, {3, 2, 6, 7}, {7, 6, 5, 4},
    {4, 5, 1, 0}, {5, 6, 2, 1}, {7, 4, 0, 3} };
 

class drawContextWx : public drawContextGlobal{
 public:
  void draw( drawContext* ctx)
  {
    ctx->draw3d();
    ctx->draw2d();
  }
};


GMSH_VIEWER_WX::GMSH_VIEWER_WX(wxFrame* parent, int* args) :
    wxGLCanvas(parent, wxID_ANY, args, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE)
{
	m_context = new wxGLContext(this);

    // prepare a simple cube to demonstrate 3D render
    // source: http://www.opengl.org/resources/code/samples/glut_examples/examples/cube.c

    v[0][0] = v[1][0] = v[2][0] = v[3][0] = -1;
    v[4][0] = v[5][0] = v[6][0] = v[7][0] = 1;
    v[0][1] = v[1][1] = v[4][1] = v[5][1] = -1;
    v[2][1] = v[3][1] = v[6][1] = v[7][1] = 1;
    v[0][2] = v[3][2] = v[4][2] = v[7][2] = 1;
    v[1][2] = v[2][2] = v[5][2] = v[6][2] = -1;    

	// gmsh
	int argc = 2;
	char* argv[]= { "gmsh", "/home/fabien/kicad-git/potential.pos" } ;
	//gmsh::GmshInitialize( argc , argv );*

	new GModel();
	GmshInitialize(argc, argv, true);
	//gmsh::add( "/home/fabien/kicad-git/currentDensity.pos" );

	OpenProject(GModel::current()->getFileName());

	for(unsigned int i = 1; i < CTX::instance()->files.size(); i++){
		if(CTX::instance()->files[i] == "-new"){
			GModel::current()->setVisibility(0);
			new GModel();
		}
		else
			MergeFile(CTX::instance()->files[i]);
	}


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
 
/** Inits the OpenGL viewport for drawing in 3D. */
void GMSH_VIEWER_WX::prepare3DViewport(int topleft_x, int topleft_y, int bottomrigth_x, int bottomrigth_y)
{
	
    glClearColor(0.2f, 0.1f, 0.2f, 1.0f); // Black Background
    glClearDepth(1.0f);	// Depth Buffer Setup
    glEnable(GL_DEPTH_TEST); // Enables Depth Testing
    glDepthFunc(GL_LEQUAL); // The Type Of Depth Testing To Do
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	
    glEnable(GL_COLOR_MATERIAL);
	
    glViewport(topleft_x, topleft_y, bottomrigth_x-topleft_x, bottomrigth_y-topleft_y);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	
    float ratio_w_h = (float)(bottomrigth_x-topleft_x)/(float)(bottomrigth_y-topleft_y);
    gluPerspective(45 /*view angle*/, ratio_w_h, 0.1 /*clip close*/, 200 /*clip far*/);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	
}
 
/** Inits the OpenGL viewport for drawing in 2D. */
void GMSH_VIEWER_WX::prepare2DViewport(int topleft_x, int topleft_y, int bottomrigth_x, int bottomrigth_y)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black Background
    glEnable(GL_TEXTURE_2D);   // textures
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	
    glViewport(topleft_x, topleft_y, bottomrigth_x-topleft_x, bottomrigth_y-topleft_y);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    gluOrtho2D(topleft_x, bottomrigth_x, bottomrigth_y, topleft_y);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
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
    wxPaintDC(this); // only to be used in paint events. use wxClientDC to paint outside the paint event
	/*
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
    // ------------- draw some 2D ----------------
    prepare2DViewport(0,0,getWidth()/2, getHeight());
    glLoadIdentity();
	
    // white background
    glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);
    glVertex3f(0,0,0);
    glVertex3f(getWidth(),0,0);
    glVertex3f(getWidth(),getHeight(),0);
    glVertex3f(0,getHeight(),0);
    glEnd();
	
    // red square
    glColor4f(1, 0, 0, 1);
    glBegin(GL_QUADS);
    glVertex3f(getWidth()/8, getHeight()/3, 0);
    glVertex3f(getWidth()*3/8, getHeight()/3, 0);
    glVertex3f(getWidth()*3/8, getHeight()*2/3, 0);
    glVertex3f(getWidth()/8, getHeight()*2/3, 0);
    glEnd();
    
    // ------------- draw some 3D ----------------
    prepare3DViewport(getWidth()/2,0,getWidth(), getHeight());
    glLoadIdentity();*/
	/*
    glColor4f(0,0,1,1);
    glTranslatef(0,0,-5);
    glRotatef(50.0f, 0.0f, 1.0f, 0.0f);
    */
    glColor4f(1, 0, 0, 1);
    for (int i = 0; i < 6; i++)
    {
        glBegin(GL_LINE_STRIP);
        glVertex3fv(&v[faces[i][0]][0]);
        glVertex3fv(&v[faces[i][1]][0]);
        glVertex3fv(&v[faces[i][2]][0]);
        glVertex3fv(&v[faces[i][3]][0]);
        glVertex3fv(&v[faces[i][0]][0]);
        glEnd();
    }

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