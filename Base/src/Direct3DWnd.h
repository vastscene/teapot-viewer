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

#include <Controller.h>
#include <Viewport.h>
using namespace eh;

#include "../../Direct3D9Driver/Direct3D9Driver.h"

class wx3DWnd: public wxWindow
{
public:
	wx3DWnd(wxWindow* parent): wxWindow(parent, wxID_ANY), m_aTimer(this, wxID_ANY)
	{
		if(IDriver::ptr pDriver = createDirect3D9Driver( (int*)this->GetHWND() ) )
		{
			m_pViewport.reset( new Viewport(pDriver) );
			m_pViewport->setDisplayRect(GetClientRect().x,GetClientRect().y, GetClientRect().width, GetClientRect().height);		
		
			Connect( -1, wxEVT_ERASE_BACKGROUND, wxEraseEventHandler( wx3DWnd::OnDraw ) );
			Connect( -1, wxEVT_SIZE, wxSizeEventHandler( wx3DWnd::OnSize ) );

			Connect( -1, wxEVT_LEFT_DOWN, wxMouseEventHandler(wx3DWnd::OnMouseEvent) );
			Connect( -1, wxEVT_LEFT_UP, wxMouseEventHandler(wx3DWnd::OnMouseEvent) );
			Connect( -1, wxEVT_RIGHT_DOWN, wxMouseEventHandler(wx3DWnd::OnMouseEvent) );
			Connect( -1, wxEVT_RIGHT_UP, wxMouseEventHandler(wx3DWnd::OnMouseEvent) );
			Connect( -1, wxEVT_MOTION, wxMouseEventHandler(wx3DWnd::OnMouseEvent) );
			Connect( -1, wxEVT_MOUSEWHEEL, wxMouseEventHandler(wx3DWnd::OnMouseEvent) );

			Connect( wxID_ANY, wxEVT_TIMER, wxTimerEventHandler( wx3DWnd::OnTimer) );

			Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(wx3DWnd::OnKeyEvent));
			this->SetFocus();
		}
	}
	virtual ~wx3DWnd()
	{
	}

	void OnKeyEvent(wxKeyEvent& event)
	{
		m_pViewport->control().OnKeyDown( event.GetUnicodeKey() );
		Refresh();
	}

	void OnMouseEvent(wxMouseEvent& event)
	{
		if( event.LeftDown() || event.RightDown() )
		{
			m_pViewport->control().OnMouseDown( Controller::LBUTTON, event.GetX(), event.GetY());
			::SetCapture((HWND)this->GetHWND());
			Refresh();
		}
		else if( event.LeftUp() || event.RightDown() )
		{
			if ( ::GetCapture() == (HWND)this->GetHWND() )
			{
				m_pViewport->control().OnMouseUp( Controller::LBUTTON, event.GetX(), event.GetY());
				ReleaseCapture();
				Refresh();
			}
		}
		else if(event.GetWheelRotation())
		{
			m_pViewport->control().OnMouseWheel(0, event.GetWheelRotation()*event.GetWheelDelta(), event.GetX(), event.GetY());
			Refresh();
		}
		else if( event.Dragging() )
		{
			if( ::GetCapture() == (HWND)this->GetHWND() )
			{
				eh::Controller::Flags flags = 0;

				if(event.LeftIsDown())
					flags |= Controller::LBUTTON;

				if(event.RightIsDown())
					flags |= Controller::RBUTTON;

				m_pViewport->control().OnMouseMove( flags, event.GetX(), event.GetY());
				
				Refresh();

				//if(m_pViewport->getModeFlag(Camera::MODE_DRAW_NOT_SIMPLE) == false)
				//	SetTimer(0, 300,0); 
			}
			else
				m_pViewport->control().OnMouseMove( 0, event.GetX(), event.GetY());

			if(!m_pViewport->isValid())
				Refresh();
		}

	}

	void OnDraw(wxEraseEvent& event)
	{
		m_pViewport->drawScene();
	}

	void OnSize(wxSizeEvent& event)
	{
		m_pViewport->setDisplayRect(GetClientRect().x,GetClientRect().y, GetClientRect().width, GetClientRect().height);
		Refresh();
	}

	void ResetView()
	{
		m_pViewport->control().reset();
		Refresh();

		struct animated
		{
			bool test(const SceneNode::vec& nodes)
			{
				for(SceneNode::vec::const_iterator it = nodes.begin(); it != nodes.end(); it++)
				{
					if(GroupNode* pGroup = dynamic_cast<GroupNode*>((*it).get()))
					{
						if(pGroup->isAnimated())
							return true;

						if( test( pGroup->getChildNodes() ) )
							return true;
					}
				}

				return false;
			}
		};

		if( animated().test( m_pViewport->getScene()->getNodes() ) )
			m_aTimer.Start(20, true);
		else
			m_aTimer.Stop();
	}

	Viewport& GetViewport() { return *m_pViewport; }

	void OnTimer(wxTimerEvent& event )
	{
		m_pViewport->control().Animate();

		if(!m_pViewport->isValid())
			this->Refresh();

		m_aTimer.Start(20, true);
	}
private:
	wxTimer m_aTimer;
	std::auto_ptr<Viewport> m_pViewport;
};

