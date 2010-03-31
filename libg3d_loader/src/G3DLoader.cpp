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

#include <SceneIO.h>
using namespace eh;

#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>

#include <vector>
#include <string>

#include <g3d/g3d.h>
#include <g3d/plugins.h>
#include <g3d/stream.h>

class G3DLoader: public SceneIO::IPlugIn
{
	std::vector< std::pair<std::wstring,std::wstring> > m_types;
	G3DContext* m_context;

	static gboolean plugin_load_image_from_stream(G3DContext *context, G3DStream *stream, G3DImage *image, gpointer user_data)
	{
		image->name = g_strdup(stream->uri);
		std::cout << "G3D Texture: " << image->name << std::endl;

		return TRUE;
	}

	static gchar *plugin_description(G3DContext *context)
	{
		return g_strdup("Dummy Image Plugin");
	}

	static gchar **plugin_extentions(G3DContext *context)
	{
		return g_strsplit("bmp:jpg", ":", 0);
	}

	void addDummyImagePlugIn()
	{
		G3DPlugin *plugin = g_new0(G3DPlugin, 1);

		plugin->module = NULL;
		plugin->name = g_strdup("dummy");
		plugin->path = g_strdup("dummy");
		plugin->type = G3D_PLUGIN_IMAGE;
		plugin->ext_func = plugin_extentions;
		plugin->desc_func = plugin_description;
		plugin->init_func = NULL;
		plugin->cleanup_func = NULL;
		plugin->loadmodel_func = NULL;
		plugin->loadmodelstream_func = NULL;

		plugin->loadimage_func = NULL;
		plugin->loadimagestream_func = plugin_load_image_from_stream;

		m_context->plugins = g_slist_append(m_context->plugins, plugin);	//add PlugIn

		if(plugin->ext_func)
		{
			gchar **ext, **exts;
			ext = exts = plugin->ext_func(m_context);
			while(*ext != NULL)
			{
				g_hash_table_insert(m_context->exts_image, *ext, plugin);
				ext ++;
			}
			plugin->extensions = exts;
		}
	}

	void loadG3DPlugIns()
	{
		boost::filesystem::path path = boost::filesystem::initial_path().string() + "/libg3d_plugins";
		g_setenv("LEOCAD_LIB", path.string().c_str(), true);

		std::cout << "loading G3DPlugins in " << path.string() << std::endl;

		for (boost::filesystem::directory_iterator it(path), end; it != end; ++it)
		{
			if ( boost::filesystem::is_directory(it->status()) )
				continue;

			boost::filesystem::path file = it->leaf();

#ifdef G_OS_WIN32
			if( !boost::iequals( file.extension(), ".dll" ) )
				continue;
#else
			if( !boost::iequals( file.extension(), ".la" ) )
				continue;
#endif
			bool found = false;
			for(GSList *pl = m_context->plugins; pl != NULL && !found; pl = pl->next)
				if(it->string() == ((G3DPlugin*)pl->data)->name)
					found = true;
			if(found)
				continue;

			if(GModule* module = g_module_open(it->string().c_str(), (GModuleFlags)0))
			{
				G3DPlugin *plugin = g_new0(G3DPlugin, 1);
				memset(plugin, sizeof(*plugin), 0);

				plugin->module = module;
				plugin->name = g_strdup(it->leaf().c_str());
				plugin->path = g_strdup(path.string().c_str());
				plugin->type = G3D_PLUGIN_IMPORT;

				g_module_symbol(module, "plugin_extensions", (gpointer *)&plugin->ext_func);
				g_module_symbol(module, "plugin_description", (gpointer *)&plugin->desc_func);
				g_module_symbol(module, "plugin_init", (gpointer *)&plugin->init_func);
				g_module_symbol(module, "plugin_cleanup", (gpointer *)&plugin->cleanup_func);
				g_module_symbol(module, "plugin_load_model", (gpointer *)&plugin->loadmodel_func);
				g_module_symbol(module, "plugin_load_model_from_stream", (gpointer *)&plugin->loadmodelstream_func);
				g_module_symbol(module, "plugin_load_image", (gpointer *)&plugin->loadimage_func);
				g_module_symbol(module, "plugin_load_image_from_stream", (gpointer *)&plugin->loadimagestream_func);

				/* append plugin to list */
				m_context->plugins = g_slist_append(m_context->plugins, plugin);

				/* handle managed extensions */
				if(plugin->ext_func)
				{
					gchar **ext, **exts;
					ext = exts = plugin->ext_func(m_context);
					while(*ext != NULL)
					{
						if(plugin->type == G3D_PLUGIN_IMAGE)
							g_hash_table_insert(m_context->exts_image, *ext, plugin);
						else if(plugin->type == G3D_PLUGIN_IMPORT)
							g_hash_table_insert(m_context->exts_import, *ext, plugin);

						ext ++;
					}
					plugin->extensions = exts;
				}

				if(plugin->init_func)
					plugin->user_data = plugin->init_func(m_context);
			}
			else
			{
				g_warning("libg3d: plugins: failed to load %s: %s\n", path.string().c_str(), g_module_error());
			}

		}
	}
public:

	G3DLoader():
		m_context(g3d_context_new())
	{
		g3d_plugins_init(m_context);
		loadG3DPlugIns();
		addDummyImagePlugIn();

		for(GSList *plugins = m_context->plugins; plugins != NULL; plugins = plugins->next)
		{
			G3DPlugin* plugin = (G3DPlugin *)plugins->data;

			if(plugin->type != G3D_PLUGIN_IMPORT)
				continue;

			std::string tmp;

			gchar **ext = plugin->extensions;
			while(ext && *ext)
			{
				tmp += std::string("*.") + *ext + std::string(";");
				ext ++;
			}

			std::wstring exts( tmp.begin(), tmp.end() );

			if(plugin->desc_func)
			{
				tmp = plugin->desc_func(m_context);
				tmp = tmp.substr(0, tmp.find('\n',0));
				boost::algorithm::replace_all(tmp, "import ", "");
				boost::algorithm::replace_all(tmp, "Import ", "");
				boost::algorithm::replace_all(tmp, "plugin", "");
				boost::algorithm::replace_all(tmp, "for ", "");
				boost::algorithm::replace_all(tmp, "to ", "");
				boost::algorithm::replace_all(tmp, "load ", "");
				boost::algorithm::replace_all(tmp, "loading ", "");
				boost::algorithm::trim(tmp);
			}

			m_types.push_back( std::make_pair( std::wstring(tmp.begin(), tmp.end()), exts) );
		}
	}

	virtual ~G3DLoader()
	{
		g3d_context_free(m_context);
	}

	SceneNode::ptr doObject(G3DObject* object, eh::IVertexBuffer::ptr pVB)
	{
		if(object->hide)
			return NULL;

		Poly obj_normals;

		typedef boost::unordered_map<G3DMaterial*, Uint_vec> FaceMap;

		FaceMap faces;

		Vec3 v, n(0,0,0), t;
#if 0
		for(GSList* fit = object->faces; fit != NULL; fit = fit->next)
		{
			G3DFace* face = (G3DFace *)fit->data;

			if( !(face->flags & G3D_FLAG_FAC_NORMALS) )
			{
				if(obj_normals.size()==0)
					obj_normals.resize(object->vertex_count);

				if(!g3d_face_get_normal(face, object, &n.x, &n.y, &n.z))
				{
					fit = fit->next;
				}

				g3d_vector_unify(&n.x, &n.y, &n.z);
			}

			for(guint32 i = 0; i < face->vertex_count; i ++)
			{
				Uint idx = face->vertex_indices[i];
				if(obj_normals.size() < idx)
					obj_normals.resize(idx+1);

				obj_normals[ idx ] = (obj_normals[ idx ]+ n).normalized();
			}
		}
#endif
		for(GSList* fit = object->faces; fit != NULL; fit = fit->next)
		{
			G3DFace* face = (G3DFace *)fit->data;

			if(!(face->flags & G3D_FLAG_FAC_NORMALS))
			{
				if(!g3d_face_get_normal(face, object, &n.x, &n.y, &n.z))
					fit = fit->next;

				g3d_vector_unify(&n.x, &n.y, &n.z);
			}

			for(guint32 i = 0; i < (face->vertex_count - 2); i ++)
			{
				for(guint32 j = 0; j < 3; j ++)
				{
					/* vertex stuff */
					guint32 idx = 0;

					if(j == 0)
						idx = face->vertex_indices[0];
					else
						idx = face->vertex_indices[i+j];

					v.x = object->vertex_data[idx*3+0];
					v.y = object->vertex_data[idx*3+1];
					v.z = object->vertex_data[idx*3+2];

					/* normal stuff */
					if(face->flags & G3D_FLAG_FAC_NORMALS)
					{
						n.x = face->normals[(i + j) * 3 + 0];
						n.y = face->normals[(i + j) * 3 + 1];
						n.z = face->normals[(i + j) * 3 + 2];
					}
					else
					{
						//n = obj_normals[idx];
					}

					/* texture stuff */
					if(0 && face->flags & G3D_FLAG_FAC_TEXMAP)
					{
						/* u */
						t.x = face->tex_image->tex_scale_u * ((j == 0) ?
								face->tex_vertex_data[0] : face->tex_vertex_data[(i + j) * 2 + 0]);
						/* v */
						t.y = face->tex_image->tex_scale_v * ((j == 0) ?
								face->tex_vertex_data[1] : face->tex_vertex_data[(i + j) * 2 + 1]);
					}
					else
						t = Vec3::Null();

					faces[face->material].push_back( pVB->addVertex(v, n, t) );
				}
			}
		}

		GroupNode::ptr pObject = GroupNode::create();

		for(GSList* oit = object->objects; oit != NULL; oit = oit->next)
			pObject->addChildNodes( doObject( (G3DObject*) oit->data, pVB ) );

		if( faces.size() > 0 )
		{
			ShapeNode::ptr pShape = ShapeNode::create();

			for(FaceMap::iterator it = faces.begin(); it != faces.end(); ++it)
			{
				eh::Material::ptr pMat = NULL;

				if(it->first)
				{
					pMat = Material::create(RGBA(it->first->r, it->first->g, it->first->b, it->first->a));
					if(it->first->tex_image)
						pMat->setTexture( Texture::createFromFile(it->first->tex_image->name) );
				}

				eh::Geometry::ptr pGeo = eh::Geometry::create(eh::Geometry::TRIANGLES, pVB, it->second);

				pShape->addGeometry(pMat, pGeo);
			}

			pObject->addChildNodes( pShape );

		}

		if(object->transformation)
			pObject->setTransform( reinterpret_cast<const Matrix&>(*object->transformation) );
		
		return pObject;

	}

	virtual bool read(const std::wstring& sFile, Scene::ptr pScene, SceneIO::progress_callback& progress)
	{
		if(m_context == NULL)
			return false;
	
		struct Functor
		{
			SceneIO::progress_callback progress;
			static Functor& Instance()
			{
				static Functor instance;
				return instance;
			}
			static gboolean progress_callback(G3DFloat percentage, gboolean show, gpointer user_data)
			{
				Instance().progress(percentage*0.5f);
				return true;
			}
		};

		Functor::Instance().progress = progress;
		m_context->update_progress_bar_func = Functor::Instance().progress_callback;

		std::string file(sFile.begin(), sFile.end());
		G3DModel *model = g3d_model_load_full(m_context, file.c_str(), 0);

		if(model)
		{
			eh::IVertexBuffer::ptr pVB = eh::createVertexBuffer(sizeof(Vec3)*2 + sizeof(Float)*2);

			Uint n = 0;
			for(GSList* oit = model->objects; oit != NULL; oit = oit->next )
				n++;

			Uint i = 0;
			for(GSList* oit = model->objects; oit != NULL; oit = oit->next )
			{
				progress(0.5f + (((Float)i++)/n)*0.5f);
				pScene->insertNode( doObject( (G3DObject *)oit->data, pVB ) );
			}

			std::cout << "VertexCount = " << pVB->getVertexCount() << std::endl;

			g3d_model_free(model);

			return true;
		}
		else
			return false;
	}

	virtual std::wstring about() const
	{
		return L"libg3d_loader";
	}
	virtual Uint file_type_count() const
	{
		return (Uint)m_types.size();
	}
	virtual std::wstring file_type(Uint i) const
	{
		return m_types[i].first;
	}
	virtual std::wstring file_exts(Uint i) const
	{
		return m_types[i].second;
	}
	virtual std::wstring rpath() const
	{
		return L"";
	}
	virtual bool canWrite(Uint i) const
	{
		return false;
	}

	virtual bool canRead(Uint i) const
	{
		return true;
	}

	virtual bool write(const std::wstring& sFile, Scene::ptr pScene, SceneIO::progress_callback& progress)
	{
		return false;
	}
};

extern "C"
#if defined(_MSC_VER)
__declspec(dllexport)
#endif
SceneIO::IPlugIn* XcreatePlugIn()
{
	return new G3DLoader();
}
