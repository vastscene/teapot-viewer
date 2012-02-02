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
#include "Base3DWnd.h"

class Direct3D9Wnd: public Base3DWnd<wxWindow>
{
	typedef IDriver* (*CreateDriverFunc)(int* pWindow);
	CreateDriverFunc CreateDirec3D9Driver;
public:

	Direct3D9Wnd(wxWindow* parent): Base3DWnd(parent)
	{
		HMODULE hModule = LoadLibraryA("Direct3D9Driver.dll");
		assert(hModule);
		CreateDirec3D9Driver = (CreateDriverFunc)GetProcAddress(hModule, "CreateDirect3D9Driver");
		if( Ptr<IDriver> pDriver = CreateDirec3D9Driver( (int*)this->GetHWND() ) )
		{
			GetViewport().setDriver( pDriver );
			GetViewport().setDisplayRect(GetClientRect().x,GetClientRect().y, GetClientRect().width, GetClientRect().height);		
		
			Connect( -1, wxEVT_ERASE_BACKGROUND, wxEraseEventHandler( Direct3D9Wnd::OnDraw ) );
			Connect( -1, wxEVT_SIZE, wxSizeEventHandler( Direct3D9Wnd::OnSize ) );

			this->SetFocus();
		}
	}
	virtual ~Direct3D9Wnd()
	{
	}

	void OnDraw(wxEraseEvent& event)
	{
		GetViewport().drawScene();
	}

	void OnSize(wxSizeEvent& event)
	{
		GetViewport().setDisplayRect(GetClientRect().x,GetClientRect().y, GetClientRect().width, GetClientRect().height);
		Refresh();
	}
};

