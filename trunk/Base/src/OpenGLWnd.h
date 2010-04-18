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
#include "Base3DWnd.h"

#if defined(_MSC_VER)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

class OpenGLWnd: public Base3DWnd<wxGLCanvas>
{
    typedef IDriver* (*CreateDriverFunc)(int* pWindow);
    CreateDriverFunc CreateOpenGL1Driver;
public:

    OpenGLWnd(wxWindow* parent):Base3DWnd<wxGLCanvas>(parent)
    {

#if defined(_MSC_VER)
        HMODULE hModule = LoadLibraryA("OpenGLDriver.dll");
        assert(hModule);
        CreateOpenGL1Driver = (CreateDriverFunc)GetProcAddress(hModule, "CreateOpenGL1Driver");
#else
        void* hModule = dlopen("OpenGLDriver.so", RTLD_NOW);
        if (!hModule)
            std::cerr << dlerror() <<  std::endl;
        assert(hModule);
        CreateOpenGL1Driver = (CreateDriverFunc) dlsym(hModule, "CreateOpenGL1Driver");
#endif

#if defined(_MSC_VER)
        Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler( OpenGLWnd::OnEraseBG ));
#else
        Connect(wxEVT_PAINT, wxPaintEventHandler( OpenGLWnd::OnPaint ));
#endif
        Connect(wxEVT_SIZE, wxSizeEventHandler( OpenGLWnd::OnSize ));

        this->SetFocus();
    }


    void OnEraseBG(wxEraseEvent& event)
    {
        //Flicker on Windows? event.Skip();
        wxPaintDC dc(this);

        if (!GetContext())
            return;

        SetCurrent();

        if (GetViewport().getDriver() == NULL)
        {
            if (CreateOpenGL1Driver != NULL)
                if (IDriver::ptr pDriver = CreateOpenGL1Driver( NULL ) )
                {
                    GetViewport().setDriver( pDriver );
                    GetViewport().setDisplayRect(
                        GetClientRect().x,
                        GetClientRect().y,
                        GetClientRect().width,
                        GetClientRect().height);
                }
        }

        if (GetViewport().getDriver())
            GetViewport().drawScene();

        SwapBuffers();
    }
    void OnPaint(wxPaintEvent& event)
    {
        wxPaintDC dc(this);

        if (!GetContext())
            return;

        SetCurrent();

        if (GetViewport().getDriver() == NULL)
        {
            if (CreateOpenGL1Driver != NULL)
                if (IDriver::ptr pDriver = CreateOpenGL1Driver( NULL ) )
                {
                    GetViewport().setDriver( pDriver );
                    GetViewport().setDisplayRect(
                        GetClientRect().x,
                        GetClientRect().y,
                        GetClientRect().width,
                        GetClientRect().height);
                }
        }

        if (GetViewport().getDriver())
            GetViewport().drawScene();

        SwapBuffers();

        event.Skip();

    }

    void OnSize(wxSizeEvent& event)
    {
        wxGLCanvas::OnSize(event);

        if (GetViewport().getDriver() != NULL)
        {
            if (!GetContext())
                return;

            SetCurrent();
            GetViewport().setDisplayRect(GetClientRect().x,
                                         GetClientRect().y,
                                         GetClientRect().width,
                                         GetClientRect().height);
            Refresh();
        }

    }
};
