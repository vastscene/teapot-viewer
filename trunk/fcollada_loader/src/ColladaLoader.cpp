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
#include <boost/filesystem/path.hpp>
#include <boost/unordered_map.hpp>
#include <boost/static_assert.hpp>

#include <FCollada.h>

#include <FCDocument/FCDocument.h>
#include <FCDocument/FCDLibrary.h>
#include <FCDocument/FCDGeometry.h>
#include <FCDocument/FCDGeometryMesh.h>
#include <FCDocument/FCDGeometryPolygonsTools.h>
#include <FCDocument/FCDGeometryPolygons.h>
#include <FCDocument/FCDGeometryPolygonsInput.h>
#include <FCDocument/FCDGeometrySource.h>
#include <FCDocument/FCDMaterial.h>
#include <FCDocument/FCDEffect.h>
#include <FCDocument/FCDEffectProfile.h>
#include <FCDocument/FCDEffectStandard.h>
#include <FCDocument/FCDSceneNode.h>
#include <FCDocument/FCDTransform.h>
#include <FCDocument/FCDGeometryInstance.h>
#include <FCDocument/FCDMaterialInstance.h>
#include <FCDocument/FCDTexture.h>
#include <FCDocument/FCDImage.h>
#include <FCDocument/FCDCamera.h>
#include <FCDocument/FCDAnimated.h>
#include <FCDocument/FCDAnimationCurve.h>

#include <map>


class COLLADALoader: public SceneIO::IPlugIn
{
private:
	typedef boost::unordered_map<std::string, std::pair<FCDCamera*, Matrix> > CAMERAS;
	CAMERAS m_cams;
	
	boost::filesystem::wpath m_path;
	IVertexBuffer::ptr m_pVB;
	boost::unordered_map< std::string, Material::ptr > m_materials;

	typedef boost::unordered_map< Geometry::TYPE, Uint_vec > PRIM_INDICES;
	typedef boost::unordered_map< std::wstring, PRIM_INDICES > MAT_PRIM_INDICES;
	boost::unordered_map< std::string, MAT_PRIM_INDICES > m_geometry;

	void set_path( const boost::filesystem::wpath& _path )
	{
		m_path = _path;
		m_path.remove_filename();
	}

	std::wstring abs_path(const boost::filesystem::wpath& file)
	{
		if(!file.is_complete())
			return m_path.directory_string() + file.file_string();
		else
			return file.string();
	}

	void addPolygons(FCDGeometryPolygons* pPolys, MAT_PRIM_INDICES& matprims )
	{
		
		// indices to vertex
		FCDGeometryPolygonsInput* pi = pPolys->FindInput(FUDaeGeometryInput::POSITION);
		FCDGeometryPolygonsInput* ni = pPolys->FindInput(FUDaeGeometryInput::NORMAL);
		FCDGeometryPolygonsInput* ti = pPolys->FindInput(FUDaeGeometryInput::TEXCOORD);

		// source of vertex
		FCDGeometrySource* positions = pPolys->GetParent()->FindSourceByType(FUDaeGeometryInput::POSITION);
		FCDGeometrySource* normals   = pPolys->GetParent()->FindSourceByType(FUDaeGeometryInput::NORMAL);
		FCDGeometrySource* texcoords  = pPolys->GetParent()->FindSourceByType(FUDaeGeometryInput::TEXCOORD);

		
		Geometry::TYPE PrimMode = Geometry::TRIANGLES;
		switch(pPolys->GetPrimitiveType())
		{
		case FCDGeometryPolygons::/*PrimitiveType::*/LINES:
			PrimMode = Geometry::LINES;
			break;
		case FCDGeometryPolygons::LINE_STRIPS:
			PrimMode = Geometry::LINE_STRIP;
			break;
		case FCDGeometryPolygons::POLYGONS:
			PrimMode = Geometry::TRIANGLES;
			break;
		case FCDGeometryPolygons::TRIANGLE_FANS:
			PrimMode = Geometry::TRIANGLE_FAN;
			break;
		case FCDGeometryPolygons::TRIANGLE_STRIPS:
			PrimMode = Geometry::TRIANGLE_STRIP;
			break;
		case FCDGeometryPolygons::POINTS:
			PrimMode = Geometry::POINTS;
			break;
		}

		Uint_vec& idx = matprims[pPolys->GetMaterialSemantic().c_str()][PrimMode];

		for (size_t i = 0; i < pi->GetIndexCount(); i++) 
		{
			uint32 _pi = (uint32)pi->GetIndices()[i];

			const Vec3* v = NULL;
			const Vec3* n = NULL;

			v = reinterpret_cast<math3D::Vec3*>(&positions->GetData()[_pi*3]);
			if(normals)
			{
				uint32 _ni = (uint32)ni->GetIndices()[i];
				n = reinterpret_cast<math3D::Vec3*>(&normals->GetData()[_ni*3]);
			}
			else
				n = &math3D::Vec3::Null();

			if(texcoords)
			{
				uint32 _ti = (uint32)ti->GetIndices()[i];
				Vec3 t;
				t.x = texcoords->GetData()[_ti*texcoords->GetStride()];
				t.y = texcoords->GetData()[_ti*texcoords->GetStride()+1];

				idx.push_back( m_pVB->addVertex( *v, *n, t ) );
			}
			else
			{
				idx.push_back( m_pVB->addVertex( *v, *n, Vec3::Null() ) );
			}


		}

		//BACKFACE
		//size_t n = idx.size();
		//idx.reserve(idx.size()*2);
		//for (size_t i = n; i > 0; i--) 
		//	idx.push_back( idx[i-1] );
	
	}
	void addMesh(FCDGeometryMesh* pMesh)
	{
		if (!pMesh->IsTriangles()) 
			FCDGeometryPolygonsTools::Triangulate(pMesh);

		for (size_t j = 0; j < pMesh->GetPolygonsCount(); j++) 
		{
			if(FCDGeometryPolygons* pPolys = pMesh->GetPolygons(j))
				addPolygons( pPolys, m_geometry[ pMesh->GetDaeId().c_str()] );
		}
	}
	
	const Matrix& getTransform(FCDSceneNode* pNode, std::vector<Matrix>& transforms)
	{
		if(pNode != NULL)
			for (size_t i=0; i< pNode->GetTransformCount(); i++)
			{
				FCDTransform* t = pNode->GetTransform(i);
				if( t->IsAnimated())
				{
					float start_time = t->GetDocument()->GetStartTime();
					float end_time = t->GetDocument()->GetEndTime();

					transforms.resize( (size_t)((end_time-start_time)*30.f + 1), transforms[0]);
					for(size_t i = 0; i < transforms.size(); i++)
					{
						FCDAnimated* a = t->GetAnimated();
						a->Evaluate(start_time + (1.f/30.f)*i);
						transforms[i] = reinterpret_cast<const Matrix&>(t->ToMatrix().m[0][0]) * transforms[i];
					}
				}
				else
					for(size_t i = 0; i < transforms.size(); i++)
						transforms[i] = reinterpret_cast<const Matrix&>(t->ToMatrix().m[0][0]) * transforms[i];
			}

		return transforms[0];;
	}

	int traverseSG(FCDSceneNode* pNode, GroupNode::ptr pParentGroup, const Matrix& _tra = Matrix::Identity())
	{
		int geo_count = 0;

		std::vector< Matrix > transforms(1, Matrix::Identity());
		Matrix tra = _tra * getTransform(pNode, transforms);

		GroupNode::ptr pGroup = GroupNode::createAnimated( SceneNode::vec(), transforms );

		ShapeNode::ptr pShape = ShapeNode::create();

		for(size_t i = 0; i< pNode->GetInstanceCount(); i++) 
		{
			FCDEntityInstance* pInst = pNode->GetInstance(i);
			FCDEntity* pEntity = pInst->GetEntity();
			std::string id = pEntity->GetDaeId().c_str();

			if(m_cams.find(id.c_str()) != m_cams.end())
				m_cams[id.c_str()].second = tra;

			if(pInst->GetType() != FCDEntityInstance::GEOMETRY)
				continue;

			FCDGeometryInstance* pGeoInst = dynamic_cast<FCDGeometryInstance*>(pInst);

			//assert(m_geometry.find(id) != m_geometry.end());

			for(MAT_PRIM_INDICES::const_iterator it = m_geometry[id].begin(),
				end = m_geometry[id].end(); it != end; it++)
			{
				Material::ptr pMat = NULL;

				for( size_t k = 0; k < pGeoInst->GetMaterialInstanceCount(); k++) 
				{
					// look for this material in my material lib, so I store a pointer
					FCDMaterialInstance* pMatInst = pGeoInst->GetMaterialInstance(k);
					std::string matid = pMatInst->GetMaterial()->GetDaeId().c_str();

					if (std::wstring(pMatInst->GetSemantic().c_str()) == it->first) 
					{
						pMat = m_materials[ matid ];
						break;
					}
				}

				for(PRIM_INDICES::const_iterator piit = it->second.begin(); piit != it->second.end(); piit++)
				{
					pShape->addGeometry( pMat, Geometry::create(piit->first, m_pVB, piit->second) );
					geo_count++;
				}
			}
		}

		if(pShape->GeometryBegin() != pShape->GeometryEnd())
			pGroup->addChildNodes(pShape);
		
		for (size_t i = 0; i < pNode->GetChildrenCount(); i++) 
			geo_count += traverseSG( pNode->GetChild(i), pGroup, tra);

		if(pGroup->getChildNodes().size()>0)
			pParentGroup->addChildNodes( pGroup );

		return geo_count;
	}
public:
	COLLADALoader():
		m_pVB(NULL)
	{
		FCollada::Initialize();
	}
	virtual ~COLLADALoader()
	{
		FCollada::Release();
	}

	virtual bool read(const std::wstring& sFile, Scene::ptr pScene, SceneIO::progress_callback& progress)
	{
		set_path(sFile);
		m_pVB = createVertexBuffer( sizeof(Vec3)*2 + sizeof(Float)*2 );

		FCDocument doc; 
		if(FCollada::LoadDocumentFromFile(&doc, sFile.c_str() ) == false)
		{
			std::cerr << "FCollada::LoadDocumentFromFile(" << sFile.c_str() << ") failed" << std::endl;
			return false;
		}

		//const Vec3& up = reinterpret_cast<const Vec3&>(doc.GetAsset()->GetUpAxis());
		//std::cout << "UpAxis:" << up;

		if(FCDCameraLibrary* camlib = doc.GetCameraLibrary())
		{
			for(size_t i = 0; i < camlib->GetEntityCount(); i++)
			{
				FCDCamera* pFCam = camlib->GetEntity(i);
				m_cams[pFCam->GetDaeId().c_str()].first = pFCam;
			}
		}

		// Materials

		if(FCDMaterialLibrary* materiallib = doc.GetMaterialLibrary())
		{
			for (size_t i = 0; i < materiallib->GetEntityCount(); i++) 
			{
				FCDMaterial* pFMat = materiallib->GetEntity(i);

				FCDEffect* pFx = pFMat->GetEffect();
				
				FCDEffectStandard* pProfile = dynamic_cast<FCDEffectStandard*>(pFx->FindProfile(FUDaeProfileType::COMMON));

				if(pProfile)
				{
					Material::ptr pMat = m_materials[pFMat->GetDaeId().c_str()] = Material::create();
					
					RGBA diffuse = reinterpret_cast<const RGBA&>(pProfile->GetDiffuseColor());
					if( pProfile->GetTransparencyMode() != FCDEffectStandard::/*TransparencyMode::*/A_ONE)
						diffuse.a = diffuse.a * 0.5f * pProfile->GetTranslucencyFactor();
					else
						diffuse.a = 1.f - pProfile->GetTranslucencyFactor();

					//if(diffuse.A() < 0.4)
					//	diffuse.A() = 0.2;

					pMat->setDiffuse(diffuse);
					pMat->setAmbient(reinterpret_cast<const RGBA&>(pProfile->GetAmbientColor()));
					pMat->setSpecular(reinterpret_cast<const RGBA&>(pProfile->GetSpecularColor()));
					pMat->setEmission(reinterpret_cast<const RGBA&>(pProfile->GetEmissionColor()));
					pMat->setSpecularFactor(pProfile->GetSpecularFactor());

					for(size_t j = 0; j < pProfile->GetTextureCount(FUDaeTextureChannel::DIFFUSE); j++)
					{
						if( FCDTexture* pTexture = pProfile->GetTexture(FUDaeTextureChannel::DIFFUSE, j) ) 
						{
							if ( FCDImage* pImage = pTexture->GetImage() ) 
								pMat->setTexture( Texture::createFromFile( abs_path(pImage->GetFilename().c_str())));
						}
					}
					for(size_t j = 0; j < pProfile->GetTextureCount(FUDaeTextureChannel::REFLECTION); j++)
					{
						if( FCDTexture* pTexture = pProfile->GetTexture(FUDaeTextureChannel::REFLECTION, j) ) 
						{
							if ( FCDImage* pImage = pTexture->GetImage() ) 
								pMat->setReflTexture( Texture::createFromFile( abs_path(pImage->GetFilename().c_str()) ));
						}
					}
				}
			}
		}

		//Meshes

		if(FCDGeometryLibrary* geolib = doc.GetGeometryLibrary())
		{
			for (size_t i = 0; i < geolib->GetEntityCount(); i++) 
			{
				progress(((float)i)/geolib->GetEntityCount());

				FCDGeometry* pGeo = geolib->GetEntity(i);

				if (pGeo->IsMesh()) 
					addMesh(pGeo->GetMesh());

				/*
				// nurbs
				if (geo->IsPSurface()) {
					m_ptr_geometry=dynamic_cast<CCLGeometry*>(new CCLSurface(geo->GetPSurface()));
					// add and look for the next geometry
					m_ptrs_geometries.push_back(m_ptr_geometry);
					continue;
				}	

				// splines
				if (geo->IsSpline()) {
					m_ptr_geometry=dynamic_cast<CCLGeometry*>(new CCLSpline(geo->GetSpline()));
					// add and look for the next geometry
					m_ptrs_geometries.push_back(m_ptr_geometry);
					continue;
				}
				*/
			}
		}

		FCDSceneNode* root = doc.GetVisualSceneRoot();

		GroupNode::ptr g = GroupNode::create();
		if( traverseSG(root, g) > 0)
		{
			for(size_t i = 0; i < g->getChildNodes().size(); i++)
				pScene->insertNode( g->getChildNodes()[i] );

			for(CAMERAS::const_iterator it = m_cams.begin(); it != m_cams.end(); ++it)
			{
				FCDCamera* cam = m_cams[it->first].first;
				Float aspect = cam->GetAspectRatio();
				Float farZ = cam->GetFarZ();
				Float nearZ = cam->GetNearZ();
				Matrix transform = m_cams[it->first].second;

				Float fovY = 0;

				if (cam->HasVerticalFov())
					fovY = cam->GetFovY();
				else
					fovY = aspect * cam->GetFovX();

				if(fequal(aspect,0))
					aspect = 1.f;

				if(fequal(fovY,0))
					fovY = 45.f;

				Float h = 2.f* (tan(DEG2RAD(fovY/2.f)) * nearZ);
				Float w = aspect * h;

				pScene->addCamera( Camera::create(it->first, w, h, nearZ, farZ, transform.getInverted() ));
			}
		}
		else
		{
			for(boost::unordered_map< std::string, MAT_PRIM_INDICES >::const_iterator id = m_geometry.begin(); id != m_geometry.end(); id++)
			for(MAT_PRIM_INDICES::const_iterator it = m_geometry[id->first].begin(),
				end = m_geometry[id->first].end(); it != end; it++)
			{
				for(PRIM_INDICES::const_iterator piit = it->second.begin(); piit != it->second.end(); piit++)
				{
					Material::ptr pMat = Material::Blue();
					pScene->insertNode( ShapeNode::create( pMat, Geometry::create(piit->first, m_pVB, piit->second) ) );
				}
			}
		}

		m_cams.clear();
		m_geometry.clear();
		m_materials.clear();
		m_pVB = NULL;

		return true;
	}

	virtual std::wstring about() const
	{
		return L"Collada Loader";
	}
	virtual Uint file_type_count() const
	{
		return 1;
	}
	virtual std::wstring file_type(Uint i) const
	{
		return L"Collada Models";
	}
	virtual std::wstring file_exts(Uint i) const
	{
		return L"*.dae;*.xml";
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
{
#if defined(_MSC_VER)
__declspec(dllexport)
#endif
SceneIO::IPlugIn* XcreatePlugIn()
{
	return new COLLADALoader();
}
}
