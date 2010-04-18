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

#include <map>
#include <vector>
#include <string>
#include <boost/function.hpp>
#include <boost/filesystem/path.hpp>

#include "Scene.h"


namespace eh
{

    class API_3D SceneIO
    {
    public:
        typedef boost::function<void (float)> progress_callback;
        typedef boost::function<void (const std::wstring&)> status_callback;

    class API_3D File : public boost::noncopyable
        {
            std::wstring m_path;
        public:
            File(const std::wstring& file);
			File(const std::string& file);
            ~File();

            const std::wstring  getName() const;
            const std::wstring& getPath() const
            {
                return m_path;
            }

            size_t getContent(std::auto_ptr<char>& data) const;
        };

    class IPlugIn : public boost::noncopyable
        {
        public:
            virtual ~IPlugIn(){};
            virtual std::wstring about() const = 0;
            virtual Uint file_type_count() const = 0;
            virtual std::wstring file_type(Uint i) const = 0;
            virtual std::wstring file_exts(Uint i) const = 0;
            virtual std::wstring rpath() const = 0;
            virtual bool canWrite(Uint i) const = 0;
            virtual bool canRead(Uint i) const = 0;

            virtual bool read(const std::wstring& aFile, Scene::ptr pScene, SceneIO::progress_callback& progress) = 0;
            virtual bool write(const std::wstring& sFile, Scene::ptr pScene, SceneIO::progress_callback& progress) = 0;
        };

        SceneIO();
        ~SceneIO();

        static void setSetStatusTextCallback(status_callback);
        static void setStatusText(const std::wstring& text);

		static Texture::ptr createTexture(const std::wstring& text);
		static Texture::ptr createTexture(const std::string& text);

        std::wstring getAboutString() const;
        std::wstring getFileWildcards(bool bLoading = true) const;

        bool read(const std::wstring& sFile, Scene::ptr pScene, progress_callback progress = NULL) const;
        bool write(const std::wstring& sFile, Scene::ptr pScene, progress_callback progress = NULL) const;
    private:
        bool execute(const std::wstring& file, Scene::ptr pScene, progress_callback progress, bool bLoading ) const;

        struct Impl;
        Impl* m_pImpl;
    };

} //end namespace
