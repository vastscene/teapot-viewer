/****************************************************************************
 * Copyright (C) 2007-2010 by E.Heidt  http://teapot-viewer.sourceforge.net/ *
 * All rights reserved.                                                      *
 *                                                                           *
 * This program is free software; you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License as published by      *
 * the Free Software Foundation; either version 2 of the License, or         *
 * (at your option) any later version.                                       *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License (http://www.gnu.org/licenses/gpl.txt)          *
 * for more details.                                                         *
 ****************************************************************************/

#pragma once
#include <wx/wx.h>
#include <wx/glcanvas.h>

#if defined(_MSC_VER)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <Viewport.h>
#include <Controller.h>
using namespace eh;

class wx3DWnd: public wxGLCanvas
{
	typedef IDriver* (*CreateDriverFunc)(int* pWindow);
	CreateDriverFunc CreateOpenGL1Driver;
public:
	wx3DWnd(wxWindow* parent) :
		wxGLCanvas(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize), m_aTimer(this, wxID_ANY),
		m_pViewport(new Viewport(NULL))
	{

#if defined(_MSC_VER)
		HMODULE hModule = LoadLibraryA("OpenGLDriver.dll");
		assert(hModule);
		CreateOpenGL1Driver = (CreateDriverFunc)GetProcAddress(hModule, "CreateOpenGL1Driver");
#else
		void* hModule = dlopen("OpenGLDriver.so", RTLD_GLOBAL);
		CreateOpenGL1Driver = (CreateDriverFunc) dlsym(hModule, "CreateOpenGL1Driver");
#endif

		Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler( wx3DWnd::OnEraseBG ));
		Connect(wxEVT_PAINT, wxPaintEventHandler( wx3DWnd::OnPaint ));
		Connect(wxEVT_SIZE, wxSizeEventHandler( wx3DWnd::OnSize ));
		Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(wx3DWnd::OnMouseEvent));
		Connect(wxEVT_LEFT_UP, wxMouseEventHandler(wx3DWnd::OnMouseEvent));
		Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(wx3DWnd::OnMouseEvent));
		Connect(wxEVT_RIGHT_UP, wxMouseEventHandler(wx3DWnd::OnMouseEvent));
		Connect(wxEVT_MOTION, wxMouseEventHandler(wx3DWnd::OnMouseEvent));
		Connect(wxEVT_MOUSEWHEEL, wxMouseEventHandler(wx3DWnd::OnMouseEvent));

		Connect(wxEVT_TIMER, wxTimerEventHandler(wx3DWnd::OnTimer));

		Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(wx3DWnd::OnKeyEvent));
		this->SetFocus();
	}

	void OnKeyEvent(wxKeyEvent& event)
	{
		m_pViewport->control().OnKeyDown( event.GetUnicodeKey() );
		Refresh();
	}

	void OnMouseEvent(wxMouseEvent& event)
	{
		if (event.LeftDown() || event.RightDown())
		{
			m_pViewport->control().OnMouseDown(Controller::LBUTTON,
					event.GetX(), event.GetY());
#if defined(_MSC_VER)
			::SetCapture((HWND)this->GetHWND());
#endif
			Refresh();
		}
		else if (event.LeftUp() || event.RightDown())
		{
#if defined(_MSC_VER)
			if ( ::GetCapture() == (HWND)this->GetHWND() )
#endif
			{
				m_pViewport->control().OnMouseUp(
						Controller::LBUTTON,
						event.GetX(), event.GetY());
#if defined(_MSC_VER)
				ReleaseCapture();
#endif
				Refresh();
			}
		}
		else if (event.GetWheelRotation())
		{
			m_pViewport->control().OnMouseWheel(
					0,
					event.GetWheelRotation()
							* event.GetWheelDelta(),
					event.GetX(), event.GetY());
			Refresh();
		}
		else if (event.Dragging())
		{
#if defined(_MSC_VER)
			if ( ::GetCapture() == (HWND)this->GetHWND() )
#else
			if (1)
#endif
			{
				eh::Controller::Flags flags = 0;

				if (event.LeftIsDown())
					flags |= Controller::LBUTTON;

				if (event.RightIsDown())
					flags |= Controller::RBUTTON;

				m_pViewport->control().OnMouseMove(flags,
						event.GetX(), event.GetY());

				Refresh();

				//if(m_pViewport->getModeFlag(Camera::MODE_DRAW_NOT_SIMPLE) == false)
				//	SetTimer(0, 300,0);
			}
			else
				m_pViewport->control().OnMouseMove(0,
						event.GetX(), event.GetY());

			if (!m_pViewport->isValid())
				Refresh();
		}

	}

	void OnEraseBG(wxEraseEvent& event)
	{
		//Flicker on Windows? event.Skip();
	}
	void OnPaint(wxPaintEvent& event)
	{
		wxPaintDC dc(this);

		if (!GetContext())
			return;

		SetCurrent();

		if (m_pViewport->getDriver() == NULL)
		{
			if (CreateOpenGL1Driver != NULL)
				if (IDriver::ptr pDriver = CreateOpenGL1Driver( NULL ) )
				{
					m_pViewport->setDriver( pDriver );
					m_pViewport->setDisplayRect(
							GetClientRect().x,
							GetClientRect().y,
							GetClientRect().width,
							GetClientRect().height);
				}
		}

		if (m_pViewport->getDriver())
			m_pViewport->drawScene();

		SwapBuffers();

		event.Skip();

	}

	void OnSize(wxSizeEvent& event)
	{
		wxGLCanvas::OnSize(event);

		if (m_pViewport->getDriver() != NULL)
		{
			if (!GetContext())
				return;

			SetCurrent();
			if (m_pViewport.get())
				m_pViewport->setDisplayRect(GetClientRect().x,
						GetClientRect().y,
						GetClientRect().width,
						GetClientRect().height);
			Refresh();
		}

	}

	void ResetView()
	{
		if (m_pViewport->getDriver() == NULL)
			return;

		Refresh();

		struct animated
		{
			bool test(const SceneNode::vec& nodes)
			{
				for (SceneNode::vec::const_iterator it =
						nodes.begin(); it
						!= nodes.end(); it++)
				{
					if (GroupNode* pGroup = dynamic_cast<GroupNode*>((*it).get()))
					{
						if (pGroup->isAnimated())
							return true;

						if (test(
								pGroup->getChildNodes()))
							return true;
					}
				}

				return false;
			}
		};

		if (animated().test(m_pViewport->getScene()->getNodes()))
			m_aTimer.Start(20, true);
		else
			m_aTimer.Stop();
	}

	Viewport& GetViewport()
	{
		return *m_pViewport;
	}

	void OnTimer(wxTimerEvent& event)
	{
		m_pViewport->control().Animate();

		if (!m_pViewport->isValid())
			this->Refresh();

		m_aTimer.Start(20, true);
	}
private:
	wxTimer m_aTimer;
	std::auto_ptr<Viewport> m_pViewport;
};
