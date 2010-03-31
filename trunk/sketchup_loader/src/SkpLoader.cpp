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

#include <atlbase.h>

#include <SketchUp_i.h>
#include <map>

class SkpLoader: public SceneIO::IPlugIn
{
private:
	float m_nCount;
	float m_iCount;
	SceneIO::progress_callback progress;
	Scene::ptr m_pScene;
	
	long GetEntityId(IUnknown* pUnk)
	{
		long entityId = 0;
		CComPtr<ISkpEntity> pEntity = NULL;
		if( SUCCEEDED(pUnk->QueryInterface(&pEntity)) )
		{
			pEntity->get_Id(&entityId);
			return entityId;
		}
		return -1;
	}
	void countEntities(CComPtr<ISkpEntityProvider> pEntProvider)
	{	
		m_nCount += 1;

		HRESULT hr;
		long nElements, i;

		//Recurse all the instances
		CComPtr<ISkpComponentInstances> pInstances = NULL;
		hr = pEntProvider->get_ComponentInstances(&pInstances);
		hr = pInstances->get_Count(&nElements);

		for(i=0; i<nElements; i++)
		{
			CComPtr<ISkpComponentInstance> pInstance;
			hr = pInstances->get_Item(i, &pInstance);

			CComPtr<ISkpComponentDefinition> pDef;
			hr = pInstance->get_ComponentDefinition(&pDef);

			CComPtr<ISkpEntityProvider> pEntProvider;
			hr = pDef->QueryInterface(&pEntProvider);

			countEntities(pEntProvider);
		}

		//Recurse all the groups
		CComPtr<ISkpGroups> pGroups = NULL;
		hr = pEntProvider->get_Groups(&pGroups);
		hr = pGroups->get_Count(&nElements);

		for(i=0; i<nElements; i++)
		{
			CComPtr<ISkpGroup> pGroup;
			hr = pGroups->get_Item(i, &pGroup);

			CComPtr<ISkpEntityProvider> pEntProvider;
			hr = pGroup->QueryInterface(&pEntProvider);

			countEntities(pEntProvider);
		}

		//Recurse all the images
		CComPtr<ISkpImages> pImages = NULL;
		hr = pEntProvider->get_Images(&pImages);
		hr = pImages->get_Count(&nElements);

		for(i=0; i<nElements; i++)
		{
			CComPtr<ISkpImage> pImage;
			hr = pImages->get_Item(i, &pImage);

			CComPtr<ISkpEntityProvider> pEntProvider;
			hr = pImage->QueryInterface(&pEntProvider);

			countEntities(pEntProvider);
		}
	}


	void traverseEntities(CComPtr<ISkpEntityProvider> pEntProvider, long mat_idx = -1, Matrix tra = Matrix::Identity())
	{
		progress(++m_iCount/m_nCount);

		HRESULT hr;
		long nElements, i;

		//Recurse all the instances
		CComPtr<ISkpComponentInstances> pInstances = NULL;
		hr = pEntProvider->get_ComponentInstances(&pInstances);
		hr = pInstances->get_Count(&nElements);

		for(i=0; i<nElements; i++)
		{
			CComPtr<ISkpComponentInstance> pInstance;
			hr = pInstances->get_Item(i, &pInstance);

			CComPtr<ISkpComponentDefinition> pDef;
			hr = pInstance->get_ComponentDefinition(&pDef);

			CComPtr<ISkpEntityProvider> pEntProvider;
			hr = pDef->QueryInterface(&pEntProvider);

			CComPtr<ISkpTransform> pTransform;
			hr = pInstance->get_Transform(&pTransform);
			double tMatrix[16];
			hr = pTransform->GetData(tMatrix);

			math3D::Matrix mat;
			for(int m = 0; m < 16; m++)
				mat[m] = (Float)tMatrix[m];

			traverseEntities(pEntProvider, mat_idx, mat * tra);
		}

		//Recurse all the groups
		CComPtr<ISkpGroups> pGroups = NULL;
		hr = pEntProvider->get_Groups(&pGroups);
		hr = pGroups->get_Count(&nElements);

		for(i=0; i<nElements; i++)
		{
			CComPtr<ISkpGroup> pGroup;
			hr = pGroups->get_Item(i, &pGroup);

			CComPtr<ISkpEntityProvider> pEntProvider;
			hr = pGroup->QueryInterface(&pEntProvider);

			//Push Transform, Material and Layer
			CComPtr<ISkpTransform> pTransform;
			hr = pGroup->get_Transform(&pTransform);
			double tMatrix[16];
			hr = pTransform->GetData(tMatrix);

			math3D::Matrix mat;
			for(int m = 0; m < 16; m++)
				mat[m] = (Float)tMatrix[m];

			traverseEntities(pEntProvider, mat_idx, mat * tra);
		}

		//Recurse all the images
		CComPtr<ISkpImages> pImages = NULL;
		hr = pEntProvider->get_Images(&pImages);
		hr = pImages->get_Count(&nElements);

		for(i=0; i<nElements; i++)
		{
			CComPtr<ISkpImage> pImage;
			hr = pImages->get_Item(i, &pImage);

			CComPtr<ISkpEntityProvider> pEntProvider;
			hr = pImage->QueryInterface(&pEntProvider);

			//Push Transform, Material and Layer
			CComPtr<ISkpTransform> pTransform;
			hr = pImage->get_Transform(&pTransform);
			double tMatrix[16];
			hr = pTransform->GetData(tMatrix);

			math3D::Matrix mat;
			for(int m = 0; m < 16; m++)
				mat[m] = (Float)tMatrix[m];

			traverseEntities(pEntProvider, mat_idx, mat * tra);
		}

		//Write all the faces
		CComPtr<ISkpFaces> pFaces = NULL;
		hr = pEntProvider->get_Faces(&pFaces);
		hr = pFaces->get_Count(&nElements);

		if(nElements>0)
		{
			IVertexBuffer::ptr pVB = createVertexBuffer( sizeof(Vec3)*2 + sizeof(Float)*2);
			std::map<long, Uint_vec> faces;
			Uint_vec edges;


			BSTR sName = 0;
			CComPtr<ISkpComponentDefinition> pDef;
			if(SUCCEEDED(pEntProvider->QueryInterface(&pDef)) && pDef)
			{
				pDef->get_Name(&sName);
			}

			long objId = GetEntityId(pEntProvider);

			if(SceneNode::ptr pObject = m_objects[objId])
			{
				m_pScene->insertNode( GroupNode::create(pObject, tra) );
				return;
			}

			for(i=0; i<nElements; i++)
			{
				CComPtr<ISkpFace> pFace = NULL;
				if (SUCCEEDED(pFaces->get_Item(i, &pFace)) && pFace)
					makeFace(pFace, mat_idx, pVB, faces);
			}

			//Write all the edges

			if( m_bImportEdges )
			{
				CComPtr<ISkpEdges> pEdges = NULL;
				hr = pEntProvider->get_Edges(&pEdges);
				hr = pEdges->get_Count(&nElements);

				for(int i=0; i<nElements; i++)
				{
					CComPtr<ISkpEdge> pEdge = NULL;
					if (SUCCEEDED(pEdges->get_Item(i, &pEdge)) && pEdge)
					{

						BOOL bSmooth = FALSE;
						pEdge->get_IsSmooth(&bSmooth);
						if(bSmooth)
							continue;

						double a[3], b[3];
						pEdge->_GetStartPoint(a);
						pEdge->_GetEndPoint(b);

						edges.push_back( pVB->addVertex(Vec3( a[0], a[1], a[2] )) );
						edges.push_back( pVB->addVertex(Vec3( b[0], b[1], b[2] )) );

					}
				}
			}

			if(pVB->getVertexCount() > 0)
			{	
				ShapeNode::ptr shape = ShapeNode::create();

				for(std::map<long, Uint_vec>::const_iterator it = faces.begin(); it != faces.end(); it++)
					shape->addGeometry( m_materials[it->first], Geometry::create(Geometry::TRIANGLES, pVB, it->second) );


				if(edges.size() > 0)
					shape->addGeometry( Material::Black(), Geometry::create(Geometry::LINES, pVB, edges) );

				m_objects[objId] = shape;
				m_pScene->insertNode( GroupNode::create(shape, tra) );
			}
		}
	}

	bool makeFace(CComPtr<ISkpFace> pFace, long mat_idx, IVertexBuffer::ptr pVB, std::map<long, Uint_vec>& faces)
	{
		HRESULT hr;

		BOOL bHasFrontTexture = false;
		BOOL bHasBackTexture = false;
		BOOL bHasBackColor = false;

		long frontId=-1;
		long backId=-1;

		CComPtr<ISkpMaterial> pFrontMaterial = NULL;
		if (SUCCEEDED(pFace->get_FrontMaterial(&pFrontMaterial)) && pFrontMaterial)
		{
			frontId = GetEntityId(pFrontMaterial);
			hr = pFrontMaterial->get_IsTexture(&bHasFrontTexture);

		}

		CComPtr<ISkpMaterial> pBackMaterial = NULL;
		if (SUCCEEDED(pFace->get_BackMaterial(&pBackMaterial)) && pBackMaterial)
		{
			backId = GetEntityId(pBackMaterial);

			hr = pBackMaterial->get_IsColor(&bHasBackColor);
			hr = pBackMaterial->get_IsTexture(&bHasBackTexture);
		}

		BOOL bHasTexture = bHasFrontTexture | bHasBackTexture;
		CComPtr<ISkpUVHelper> pUVHelper = NULL;

		//If the face has a texture(s) applied to it, then create a UVHelper class so we can output the uv
		//coordinance at each vertex.
		if (bHasTexture)
		{
			CComPtr<ISkpCorrectPerspective> pCorrectPerspective;
			hr = m_pTextureWriter->QueryInterface(&pCorrectPerspective); 
			hr = pFace->GetUVHelper(bHasFrontTexture, bHasBackTexture, pCorrectPerspective, &pUVHelper);
		}

		CComPtr<ISkpLoops> pLoops;
		hr = pFace->get_Loops(&pLoops);

		long nLoops;
		hr = pLoops->get_Count(&nLoops);

		for(long i = 0; i < nLoops; i++)
		{
			CComPtr<ISkpLoop> pLoop;
			pLoops->get_Item(i, &pLoop);
		}

		CComPtr<ISkpPolygonMesh> pMesh;

		if (bHasTexture)
			hr = pFace->CreateMeshWithUVHelper(PolygonMeshPoints | PolygonMeshUVQFront | PolygonMeshUVQBack | PolygonMeshNormals, pUVHelper, &pMesh);
		else
		{
			CComPtr<ISkpCorrectPerspective> pCorrectPerspective;
			hr = m_pTextureWriter->QueryInterface(&pCorrectPerspective); 
			hr = pFace->CreateMesh(PolygonMeshPoints | PolygonMeshNormals, pCorrectPerspective, &pMesh);
		}

		assert(SUCCEEDED(hr));

		long nPolys;
		hr = pMesh->get_NumPolygons(&nPolys);

		for (long i=0;i<nPolys;i++)
		{
			long nPoints;
			HRESULT hr = pMesh->CountPolygonPoints(i+1, &nPoints); //The mesh is 1 based

			for (long j=0; j<nPoints; j++)
			{
				double v[3]; 
				double n[3];
				double t[3];

				long idx = 0;
				pMesh->GetPolygonPointIndex(i+1, j+1, &idx);

				pMesh->_GetPoint(idx, v); 
				pMesh->_GetVertexNormal(idx, n);

				if (bHasFrontTexture)
					pMesh->_GetFrontUVPoint(idx, t);

				faces[frontId].push_back( pVB->addVertex(Vec3( v[0], v[1], v[2] ),Vec3( n[0], n[1], n[2] ), Vec3(t[0],t[1],0) ));
			}

			//BACKFACE
			//for (long j = nPoints; j>0; j--)
			//{
			//	double v[3]; 
			//	double n[3];
			//	double t[3];

			//	long idx = 0;
			//	pMesh->GetPolygonPointIndex(i+1, j, &idx);

			//	pMesh->_GetPoint(idx, v); 
			//	pMesh->_GetVertexNormal(idx, n);
			//	pMesh->_GetBackUVPoint(idx, t);

			//	VNT vnt;

			//	vnt.v = Vec3( v[0], v[1], v[2] );
			//	vnt.n = Vec3( n[0], n[1], n[2] );
			//	vnt.tu = t[0];
			//	vnt.tv = t[1];
			//	
			//	if(backId == -1)
			//		backId = frontId;

			//	faces[backId].push_back( vertexBuff.insert( vnt ) );
			//}
		}

		return true;
	}


	bool m_bImportEdges;
	std::map<long, Material::ptr> m_materials;
	std::map<long, SceneNode::ptr> m_objects;
	ISkpTextureWriter2* m_pTextureWriter;
public:
	SkpLoader():
		m_nCount(0), 
		m_iCount(0), 
		m_pTextureWriter(NULL),
		m_bImportEdges(false)
	{
	}
	virtual ~SkpLoader()
	{}

	virtual bool read(const std::wstring& sFile, Scene::ptr pScene, SceneIO::progress_callback& progress)
	{
		m_pScene = pScene;
		this->progress = progress;

		if(MessageBox(NULL, _T("Import Edges?"), _T("SkpLoader"), MB_YESNO) == IDYES)
			m_bImportEdges = true;

		HMODULE hModule = LoadLibrary(_T("SketchUpReader.dll")); 

		if(hModule == NULL)
			return false;		

		try
		{
			HRESULT hr = 0;

			typedef HRESULT (*GET_SKP_APP)(ISkpApplication** ppApplication) ; 
			GET_SKP_APP getApplication = (GET_SKP_APP)GetProcAddress(hModule, "GetSketchUpSkpApplication"); 
			if(getApplication == NULL) 
				throw -1;

			CComPtr<ISkpApplication> pSkpApp = NULL; 
			CComPtr<ISkpFileReader> pFileReader = NULL; 

			if (SUCCEEDED(getApplication(&pSkpApp)) && pSkpApp != NULL) 
			{ 
				if(FAILED(pSkpApp->QueryInterface(&pFileReader)))
					throw -1;
			}


			CComPtr<ISkpDocument> doc = NULL; 

			if(FAILED(pFileReader->OpenFile(SysAllocString(sFile.c_str()),&doc)))
				throw -1;

			CComPtr<ISkpMaterials> materials = NULL; 
			doc->get_Materials(&materials); 

			long count = 0; 
			materials->get_Count(&count); 

			for(long i =0; i < count; i++) 
			{ 
				CComPtr<ISkpMaterial> material = NULL; 
				materials->get_Item(i, &material); 

				BSTR name = 0; 
				material->get_Name(&name); 


				Material::ptr pMat = m_materials[ GetEntityId(material) ] = Material::create();


				double a = 0;
				BOOL bUsesAlpha = FALSE;
				if(SUCCEEDED(material->get_UsesAlpha(&bUsesAlpha)) && bUsesAlpha)
					material->get_Alpha(&a);

				OLE_COLOR rgba = 0;
				BOOL bIsColor = FALSE;
				if(SUCCEEDED(material->get_IsColor(&bIsColor)) && bIsColor)
				{
					material->get_Color(&rgba);

					Float r = ((rgba&0x000000FF)>>0)/255.f;
					Float g = ((rgba&0x0000FF00)>>8)/255.f;
					Float b = ((rgba&0x00FF0000)>>16)/255.f;

					pMat->setDiffuse( RGBA(r, g, b, a) );
				}
			}

			CComPtr<ISkpTextureWriter> pTW;
			hr = pSkpApp->CreateTextureWriter(&pTW);
			hr = pTW->QueryInterface(&m_pTextureWriter);

			CComPtr<ISkpEntityProvider> pEntProvider = NULL;
			if(SUCCEEDED(doc->QueryInterface(&pEntProvider)) && pEntProvider)
			{
				countEntities(pEntProvider);
				traverseEntities(pEntProvider);
			}
			else 
				throw -1;

		}
		catch(int)
		{

		}

		if(m_pTextureWriter)
			m_pTextureWriter->Release();

		FreeLibrary(hModule); 

		return true;
	}

	virtual std::wstring about() const
	{
		return L"sketchup_loader";
	}
	virtual Uint file_type_count() const
	{
		return 1;
	}
	virtual std::wstring file_type(Uint i) const
	{
		return L"SketchUp Document";
	}
	virtual std::wstring file_exts(Uint i) const
	{
		return L"*.skp";
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
	return new SkpLoader();
}