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

#include <wx/wx.h>
#include <wx/aui/aui.h>
#include <wx/sysopt.h>

#include "wx/treectrl.h"

#if defined(_MSC_VER)
#include "Direct3DWnd.h"
#else
#include "OpenGLWnd.h"
#endif

#include <boost/bind.hpp>

#include <Scene.h>
#include <SceneIO.h>
using namespace eh;

class MainFrame: public wxFrame
{
public:
	enum wxID
	{
		wxID_WIREFRAME = wxID_HIGHEST,
		wxID_LIGHTING,
		wxID_SHADOW,
		wxID_BACKGROUND,
		wxID_BOUNDINGS,
		wxID_AABBTREE,
		wxID_FULLSCREEN,
		wxID_CAMERA_RESET,
		wxID_PERSPECTIVE,
		wxID_ORTHOGONAL,
		wxID_CAMERA0,
		wxID_CAMERA1,
		wxID_CAMERA2,
		wxID_CAMERA3,
		wxID_CAMERA4,
		wxID_CAMERA5,
		wxID_CAMERA7,
		wxID_CAMERA8,
		wxID_CAMERA9,
	};

	MainFrame():
		wxFrame(NULL, wxID_ANY, wxT("Teapot Viewer"), wxDefaultPosition, wxSize(640, 480)),
		m_p3DWnd(NULL), m_pToolbar(NULL), m_pTree(NULL), m_pGauge(NULL),
		m_pSceneIO(NULL)
	{
		this->SetIcon( wxIcon(wxT("BASE_ICO")) );

		m_mgr.SetManagedWindow(this);

		wxSystemOptions::SetOption(wxT("msw.remap"), 2);

#if 0
		m_mgr.AddPane( this->CreateTreeCtrl(), wxAuiPaneInfo().
			Name(wxT("tree")).Caption(wxT("SceneTree")).LeftDockable()
			      .PaneBorder(true));
#endif
		////////////


		//Menu

		wxMenu* pFileMenu = new wxMenu;
		pFileMenu->Append(wxID_OPEN, _T("&Open...\tCtrl-O"));
		Connect( wxID_OPEN, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(MainFrame::OnOpenFile));

		pFileMenu->Append(wxID_SAVE, _T("&Export..\tCtrl-S"));
		Connect( wxID_SAVE, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(MainFrame::OnSaveFile));

		pFileMenu->AppendSeparator();
		pFileMenu->Append(wxID_EXIT, _T("&Exit\tAlt-F4"));
		Connect( wxID_EXIT, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(MainFrame::OnExit));

		wxMenu* pViewMenu = new wxMenu;
		pViewMenu->AppendCheckItem(wxID_WIREFRAME, _T("&Wireframe\tW"));
		pViewMenu->AppendCheckItem(wxID_LIGHTING, _T("&Lighting\tL"))->Check();
		pViewMenu->AppendCheckItem(wxID_SHADOW, _T("&Shadow\tS"))->Check();
		pViewMenu->AppendCheckItem(wxID_BACKGROUND, _T("Back&ground\tG"))->Check();
		pViewMenu->AppendSeparator();
		pViewMenu->AppendCheckItem(wxID_BOUNDINGS, _T("&BoundingBoxes\tB"))->Check(false);
		pViewMenu->AppendCheckItem(wxID_AABBTREE, _T("Sce&ne-AABB-Tree\tN"))->Check(false);
		pViewMenu->AppendSeparator();
		Connect( wxID_WIREFRAME, wxID_AABBTREE, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(MainFrame::OnView));

		wxMenu* pCameraMenu = new wxMenu;
		pCameraMenu->AppendRadioItem(wxID_PERSPECTIVE, _T("&Perspective Projection\tP"));
		pCameraMenu->AppendRadioItem(wxID_ORTHOGONAL, _T("&Orthogonal Projection\tO"));
		Connect( wxID_CAMERA_RESET, wxID_CAMERA9, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(MainFrame::OnCamera));

		pViewMenu->AppendCheckItem(wxID_FULLSCREEN, _T("Fullscreen\tF11"))->Check(false);
		Connect( wxID_FULLSCREEN, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(MainFrame::OnFullscreen));


		wxMenu* pHelpMenu = new wxMenu;
		pHelpMenu->Append(wxID_ABOUT, _T("&About.."), _T("About.."));

		Connect( wxID_ABOUT, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(MainFrame::OnAbout));

		wxMenuBar* menuBar = new wxMenuBar( wxMB_DOCKABLE );
		menuBar->Append(pFileMenu, _T("&File"));
		menuBar->Append(pViewMenu, _T("&View"));
		menuBar->Append(pCameraMenu, _T("&Camera"));
		menuBar->Append(pHelpMenu, _T("&Help"));
		SetMenuBar(menuBar);

		// Toolbar
#if 0
		m_pToolbar = new wxToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_FLAT|wxTB_DOCKABLE|::wxTB_NODIVIDER); //this->CreateToolBar();

		//m_pToolbar->SetBackgroundColour( wxColour(255,255,255) );

		m_pToolbar->AddTool(wxID_OPEN, wxT("Open File"), wxBitmap(wxT("OPEN_BMP")), wxT("Open File.."));
		m_pToolbar->AddSeparator();

		m_pToolbar->AddTool(wxID_PERSPECTIVE, wxT("Perspective"), wxBitmap(wxT("PERSPECTIVE_BMP")), wxT("Perspective Projection"), ::wxITEM_RADIO);
		m_pToolbar->AddTool(wxID_ORTHOGONAL, wxT("Orthogonal"), wxBitmap(wxT("ORTHOGONAL_BMP")), wxT("Orthogonal Projection"), ::wxITEM_RADIO);
		m_pToolbar->Realize();

		m_pToolbar->AddSeparator();

		m_mgr.AddPane(m_pToolbar, wxAuiPaneInfo().Name(wxT("tb1")).Caption(wxT("Big Toolbar")).
			      ToolbarPane().Top().LeftDockable(false).RightDockable(false));
#endif
		//Statusbar

		this->CreateStatusBar();

		// Finish

		if( m_p3DWnd == NULL )
			m_p3DWnd = new wx3DWnd(this);

		m_mgr.AddPane(m_p3DWnd, wxAuiPaneInfo().
			      Name(wxT("3d")).Caption(wxT("wx3DWnd")).
			      CenterPane().PaneBorder(false));

		m_mgr.Update();
	}

	virtual ~MainFrame()
	{
		m_mgr.UnInit();
	}

	void OnOpenFile(wxCommandEvent& WXUNUSED(event))
	{
	    if(m_pSceneIO == NULL)
            m_pSceneIO = new SceneIO();

		wxFileDialog dlgOpenFile( this, wxT("Open.."), wxEmptyString, wxEmptyString, m_pSceneIO->getFileWildcards().c_str());

		if( dlgOpenFile.ShowModal() == wxID_OK )
		{
			this->UpdateWindowUI();

			LoadModel( dlgOpenFile.GetPath().c_str() );
		}
	}

	void OnSaveFile(wxCommandEvent& WXUNUSED(event))
	{
        if(m_pSceneIO == NULL)
            m_pSceneIO = new SceneIO();

		wxFileDialog dlgSaveFile( this, wxT("Export.."), wxEmptyString, wxEmptyString, m_pSceneIO->getFileWildcards(false).c_str(), wxFD_SAVE );

		if( dlgSaveFile.ShowModal() == wxID_OK)
		{
			m_pSceneIO->write( dlgSaveFile.GetPath().c_str(),
					m_p3DWnd->GetViewport().getScene(),
					boost::bind(&MainFrame::OnProgress, this, _1));
		}
	}

	void OnFullscreen(wxCommandEvent& event )
	{
		this->ShowFullScreen( event.IsChecked() );
	}

	void OnExit(wxCommandEvent& WXUNUSED(event))
	{
		this->Close();
	}

	void OnAbout(wxCommandEvent& WXUNUSED(event))
	{
	    if(m_pSceneIO == NULL)
            m_pSceneIO = new SceneIO();

		std::string glinfo = this->m_p3DWnd->GetViewport().getDriver()->getDriverInformation();

		std::wstring about = L"http://teapot-viewer.sourceforge.net\n\n";
		about += m_pSceneIO->getAboutString();
		about += std::wstring(glinfo.begin(), glinfo.end());
		wxMessageBox( about.c_str() );
	}

	bool LoadModel(const std::wstring& sFile, bool bAsThread = false )
	{
        if(m_pSceneIO == NULL)
            m_pSceneIO = new SceneIO();

		GetStatusBar()->SetStatusText( sFile );

		Scene::ptr pScene = Scene::create();

		bool ret = m_pSceneIO->read( sFile, pScene, boost::bind(&MainFrame::OnProgress, this, _1) );

		wxMenu* pCameraMenu = new wxMenu;
		pCameraMenu->AppendRadioItem(wxID_PERSPECTIVE, _T("&Perspective Projection\tP"));
		pCameraMenu->AppendRadioItem(wxID_ORTHOGONAL, _T("&Orthogonal Projection\tO"));
		pCameraMenu->AppendSeparator();
		pCameraMenu->Append(wxID_CAMERA_RESET, _T("&Default\t1"));

		for(size_t i = 0; i < pScene->getCameras().size(); i++)
		{
			Camera::ptr pCam = pScene->getCameras()[i];
			pCameraMenu->Append(wxID_CAMERA0+i, wxString::From8BitData(pCam->getName().c_str()) + wxString::Format(wxT("\t%d"),i+2) );
		}

		wxMenu* old = GetMenuBar()->Replace(2, pCameraMenu, _T("&Camera"));
		delete old;

		m_p3DWnd->GetViewport().setScene(pScene);
		m_p3DWnd->ResetView();
		return ret;
	}

protected:

	void OnCamera(wxCommandEvent& event)
	{
		switch( event.GetId() )
		{
		case wxID_CAMERA_RESET:
			m_p3DWnd->GetViewport().setScene( m_p3DWnd->GetViewport().getScene(), m_p3DWnd->GetViewport().getScene()->createOrbitalCamera() );
			m_p3DWnd->ResetView();
			break;
		case wxID_PERSPECTIVE:
		case wxID_ORTHOGONAL:
			m_p3DWnd->GetViewport().setModeFlag( Viewport::MODE_ORTHO, event.GetId() != wxID_PERSPECTIVE );
			this->GetMenuBar()->Check( wxID_PERSPECTIVE, event.GetId() == wxID_PERSPECTIVE );
			this->GetMenuBar()->Check( wxID_ORTHOGONAL, event.GetId() == wxID_ORTHOGONAL );
			if( m_pToolbar )
			{
				this->m_pToolbar->ToggleTool( wxID_PERSPECTIVE, event.GetId() == wxID_PERSPECTIVE );
				this->m_pToolbar->ToggleTool( wxID_ORTHOGONAL, event.GetId() == wxID_ORTHOGONAL );
			}
			break;
		default:
			{
			Uint iCamera = event.GetId() - wxID_CAMERA0;
			m_p3DWnd->GetViewport().setScene( m_p3DWnd->GetViewport().getScene(), m_p3DWnd->GetViewport().getScene()->getCameras()[iCamera] );
			}
			break;
		}
		m_p3DWnd->Refresh();
	}
	void OnView(wxCommandEvent& event)
	{
		switch( event.GetId() )
		{
		case wxID_WIREFRAME:
			m_p3DWnd->GetViewport().setModeFlag( Viewport::MODE_WIREFRAME, event.IsChecked() );
			break;
		case wxID_LIGHTING:
			m_p3DWnd->GetViewport().setModeFlag( Viewport::MODE_LIGHTING, event.IsChecked() );
			break;
		case wxID_SHADOW:
			m_p3DWnd->GetViewport().setModeFlag( Viewport::MODE_SHADOW, event.IsChecked() );
			break;
		case wxID_BACKGROUND:
			m_p3DWnd->GetViewport().setModeFlag( Viewport::MODE_BACKGROUND, event.IsChecked() );
			break;
		case wxID_BOUNDINGS:
			m_p3DWnd->GetViewport().setModeFlag( Viewport::MODE_DRAWPRIMBOUNDS, event.IsChecked() );
			break;
		case wxID_AABBTREE:
			m_p3DWnd->GetViewport().setModeFlag( Viewport::MODE_DRAWAABBTREE, event.IsChecked() );
			break;
		}

		m_p3DWnd->Refresh();
	}

	void OnProgress(float p)
	{
		if(p == 1.f)
		{
			if(m_pGauge)
			{
				m_pGauge->Hide();
				delete m_pGauge;
				m_pGauge = NULL;
				this->SetTitle( GetStatusBar()->GetStatusText() );
				GetStatusBar()->SetFieldsCount(1);
				GetStatusBar()->SetStatusText(wxT("Ready"));
			}
			return;
		}

		if(m_pGauge == NULL)
		{
			int widths[] = { 100, -1 };
			GetStatusBar()->SetFieldsCount(2, widths);
			wxRect rect;
			this->GetStatusBar()->GetFieldRect(0, rect);
			m_pGauge = new wxGauge( this->GetStatusBar(), ::wxID_ANY, 100,
						rect.GetLeftTop(), rect.GetSize(),
						wxNO_BORDER|wxHORIZONTAL|wxGA_SMOOTH);
		}

		if(m_pGauge)
		{
			wxRect rect;
			this->GetStatusBar()->GetFieldRect(0, rect);

			m_pGauge->SetSize( rect.GetSize() );
			m_pGauge->Move( rect.GetLeftTop() );

			m_pGauge->SetValue( (int)(p*100.f));

			m_pGauge->Show();
		}

		const wxString& sFile = GetStatusBar()->GetStatusText();
		GetStatusBar()->SetStatusText( wxString::Format(wxT("Processing %s ... %d%%"), sFile.c_str(), (int)(p*100) ), 1 );
	}

	wxTreeCtrl* CreateTreeCtrl()
	{

	    m_pTree = new wxTreeCtrl(this, wxID_ANY,
					      wxPoint(0,0), wxSize(160,250),
					      wxTR_DEFAULT_STYLE | wxNO_BORDER);

	    //wxImageList* imglist = new wxImageList(16, 16, true, 2);
	    //imglist->Add(wxArtProvider::GetBitmap(wxART_FOLDER, wxART_OTHER, wxSize(16,16)));
	    //imglist->Add(wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_OTHER, wxSize(16,16)));
	    //m_pTree->AssignImageList(imglist);

	    return m_pTree;
	}

	wx3DWnd* m_p3DWnd;
	wxToolBar* m_pToolbar;
	wxAuiManager m_mgr;
	wxTreeCtrl* m_pTree;
	wxGauge* m_pGauge;
	SceneIO* m_pSceneIO;

};

class MyApp : public wxApp
{
public:
	virtual bool OnInit()
	{
		MainFrame* pFrame = new MainFrame();

		pFrame->Show(true);

		if( this->argc > 1)
			pFrame->LoadModel( this->argv[1] );


		return true;
	}
};

//IMPLEMENT_APP(MyApp)
IMPLEMENT_APP_CONSOLE(MyApp);

