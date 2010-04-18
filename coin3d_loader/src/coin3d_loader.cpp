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

#include <Inventor/SoDB.h>
#include <Inventor/SoInteraction.h>
#include <Inventor/nodekits/SoNodeKit.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShape.h>

#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/fields/SoFieldData.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>

#include <Inventor/VRMLnodes/SoVRMLShape.h>
#include <Inventor/VRMLnodes/SoVRMLGeometry.h>
#include <Inventor/VRMLnodes/SoVRMLTransform.h>
#include <Inventor/VRMLnodes/SoVRMLIndexedFaceSet.h>
#include <Inventor/VRMLnodes/SoVRMLPositionInterpolator.h>
#include <Inventor/VRMLnodes/SoVRMLOrientationInterpolator.h>

#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoCallbackAction.h>

#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/nodes/SoVertexShape.h>
#include <Inventor/nodes/SoLight.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoSpotLight.h>
#include <Inventor/nodes/SoPointLight.h>
#include <Inventor/nodes/SoRotor.h>
#include <Inventor/nodes/SoPendulum.h>
#include <Inventor/nodes/SoShuttle.h>
#include <Inventor/nodes/SoLOD.h>
#include <Inventor/misc/SoChildList.h>

#include <iostream>
#include <string.h>
#include <math.h>
#include <map>

#include <boost/static_assert.hpp>


BOOST_STATIC_ASSERT(sizeof(SbMatrix) == sizeof(Matrix));

class Coin3DLoader: public SceneIO::IPlugIn
{
    struct GeoetryStruct
    {
        IVertexBuffer::ptr m_pVB;
        std::map<int, Material::ptr> m_mapMaterials;
        std::map<int, Uint_vec> m_mapMaterialTriangles;
        std::map<int, Uint_vec> m_mapMaterialLines;
        std::map<int, Uint_vec> m_mapMaterialPoints;
    };

public:

    Coin3DLoader()
    {
    }
    virtual ~Coin3DLoader()
    {

    }

    static void addTriangleCB(void* data, SoCallbackAction* action, const SoPrimitiveVertex *v0, const SoPrimitiveVertex *v1, const SoPrimitiveVertex *v2)
    {
        GeoetryStruct* _this = (GeoetryStruct*)(data);
        int material_index = v0->getMaterialIndex();

        if (_this->m_mapMaterials[material_index] == NULL)
        {
            SbColor ambient, diffuse, specular, emission;
            float transparency, shininess;
            action->getMaterial(ambient, diffuse, specular, emission, shininess, transparency, material_index);

            Material::ptr m = Material::create( RGBA(diffuse[0], diffuse[1], diffuse[2], transparency) );
            m->setAmbient( RGBA(ambient[0], ambient[1], ambient[2]) );
            m->setSpecular( RGBA(specular[0], specular[1], specular[2]) );
            m->setSpecularFactor( shininess );
            m->setEmission( RGBA(emission[0], emission[1], emission[2]) );
            _this->m_mapMaterials[material_index] = m;
        }

        _this->m_mapMaterialTriangles[material_index].push_back( _this->m_pVB->addVertex( (Vec3&)v0->getPoint(), (Vec3&)v0->getNormal()) );
        _this->m_mapMaterialTriangles[material_index].push_back( _this->m_pVB->addVertex( (Vec3&)v1->getPoint(), (Vec3&)v1->getNormal()) );
        _this->m_mapMaterialTriangles[material_index].push_back( _this->m_pVB->addVertex( (Vec3&)v2->getPoint(), (Vec3&)v2->getNormal()) );

    }
    static void addLineSegmentCB(void* data, SoCallbackAction* action, const SoPrimitiveVertex *v0, const SoPrimitiveVertex *v1)
    {
        GeoetryStruct* _this = (GeoetryStruct*)(data);
        int material_index = v0->getMaterialIndex();

        _this->m_mapMaterialLines[material_index].push_back( _this->m_pVB->addVertex( (Vec3&)v0->getPoint(), (Vec3&)v0->getNormal()) );
        _this->m_mapMaterialLines[material_index].push_back( _this->m_pVB->addVertex( (Vec3&)v1->getPoint(), (Vec3&)v1->getNormal()) );
    }
    static void addPointCB(void* data, SoCallbackAction* action,  const SoPrimitiveVertex *v0)
    {
        GeoetryStruct* _this = (GeoetryStruct*)(data);
        int material_index = v0->getMaterialIndex();

        _this->m_mapMaterialPoints[material_index].push_back( _this->m_pVB->addVertex( (Vec3&)v0->getPoint(), (Vec3&)v0->getNormal()) );
    }


    std::map<uint32_t, SceneNode::ptr>* m_pMapNodes;

    SceneNode::ptr makeShape(SoNode* node)
    {
		std::map<uint32_t, SceneNode::ptr>::iterator it = m_pMapNodes->find(node->getNodeId());
        if (it != m_pMapNodes->end())
            return it->second;

        SoCallbackAction cbAction;

        GeoetryStruct tmp;
        tmp.m_pVB = createVertexBuffer( sizeof(Float)*8 );

        cbAction.addTriangleCallback(SoVRMLGeometry::getClassTypeId(), addTriangleCB, &tmp);
        cbAction.addLineSegmentCallback(SoVRMLGeometry::getClassTypeId(), addLineSegmentCB, &tmp);
        cbAction.addPointCallback(SoVRMLGeometry::getClassTypeId(), addPointCB, &tmp);

        cbAction.apply(node);

        ShapeNode::ptr s = ShapeNode::create();
		(*m_pMapNodes)[node->getNodeId()] = s;

        for (std::map<int, Uint_vec>::const_iterator it = tmp.m_mapMaterialTriangles.begin();  it != tmp.m_mapMaterialTriangles.end(); ++it)
            s->addGeometry( tmp.m_mapMaterials[it->first], Geometry::create(Geometry::TRIANGLES, tmp.m_pVB, it->second) );

        for (std::map<int, Uint_vec>::const_iterator it = tmp.m_mapMaterialLines.begin();  it != tmp.m_mapMaterialLines.end(); ++it)
            s->addGeometry( tmp.m_mapMaterials[it->first], Geometry::create(Geometry::LINES, tmp.m_pVB, it->second) );

        for (std::map<int, Uint_vec>::const_iterator it = tmp.m_mapMaterialPoints.begin();  it != tmp.m_mapMaterialPoints.end(); ++it)
            s->addGeometry( tmp.m_mapMaterials[it->first], Geometry::create(Geometry::POINTS, tmp.m_pVB, it->second) );


        if(s->GeometryBegin() == s->GeometryEnd())
            std::cout << "makeShape, no triangulation with: " << typeid(*node).name() << std::endl;

        return s;

    }

	SceneNode::ptr makeShape(SoCoordinate3* pCoordNode, SoMaterial* pMaterials, SoIndexedFaceSet* pIFSNode)
	{
		if(!pCoordNode || !pIFSNode )
			return NULL;

		std::map<uint32_t, SceneNode::ptr>::iterator it = m_pMapNodes->find(pIFSNode->getNodeId());
        if (it != m_pMapNodes->end())
            return it->second;

		ShapeNode::ptr s = ShapeNode::create();
		(*m_pMapNodes)[pIFSNode->getNodeId()] = s;

		std::map<int32_t, Uint_vec> indices;
		IVertexBuffer::ptr pVB = createVertexBuffer( sizeof(Float)*8 );

		const SbColor* diffuse = pMaterials->diffuseColor.getValues(0);
		const int32_t* idx = pIFSNode->coordIndex.getValues(0);
		const int32_t* midx = pIFSNode->materialIndex.getValues(0);
		const SbVec3f* coords = pCoordNode->point.getValues(0);
		for(int i = 0; i < pIFSNode->coordIndex.getNum();)
		{
			int32_t mid = 0;
			const Vec3 p1 = reinterpret_cast<const Vec3&>(coords[ idx[i++] ]) *10.f;
			const Vec3 p2 = reinterpret_cast<const Vec3&>(coords[ idx[i++] ]) *10.f;
			const Vec3 p3 = reinterpret_cast<const Vec3&>(coords[ idx[i++] ]) *10.f;
			const Vec3& n = math3D::calcNormal(p1, p2, p3);

			indices[mid].push_back( pVB->addVertex(p1, n) ); 
			indices[mid].push_back( pVB->addVertex(p2, n) ); 
			indices[mid].push_back( pVB->addVertex(p3, n) ); 

			if(idx[i] == -1)
				i++;
		}

		for(std::map<int32_t, Uint_vec>::const_iterator it = indices.begin(); it != indices.end(); ++it)
		{
			Material::ptr mat = Material::create( RGBA( diffuse[it->first][0],diffuse[it->first][1],diffuse[it->first][2]) );
			s->addGeometry( mat, Geometry::create( Geometry::TRIANGLES, pVB, it->second ));
		}

		return s;
	}

    SceneNode::ptr traverseGraph(SoNode* node)
    {
		std::map<uint32_t, SceneNode::ptr>::iterator it = m_pMapNodes->find(node->getNodeId());
        if (it != m_pMapNodes->end())
            return it->second;

        if (SoGroup* group = dynamic_cast<SoGroup*>(node))
        {
            std::cout << "SoGroup: " << typeid(*node).name() << std::endl;

            GroupNode::ptr g = GroupNode::create();
            (*m_pMapNodes)[node->getNodeId()] = g;

            SbViewportRegion vpr;
            SoGetMatrixAction gma(vpr);
            gma.apply(node); //tra->getMatrix(&gma);

            g->setTransform( reinterpret_cast<const Matrix&>(gma.getMatrix()) );

			SoMaterial* materials = NULL;
			SoCoordinate3* coords = NULL;
			SoIndexedFaceSet* indices = NULL;

			for (int i = 0; i < group->getNumChildren(); i++)
			{
				if(coords == NULL)
				{
					if(coords = dynamic_cast<SoCoordinate3*>(group->getChild(i) ))
						continue;
				}

				if(indices == NULL)
				{
					if(indices = dynamic_cast<SoIndexedFaceSet*>(group->getChild(i) ))
						continue;
				}

				if(materials == NULL)
				{
					if(materials = dynamic_cast<SoMaterial*>(group->getChild(i) ))
						continue;
				}

				g->addChildNodes( traverseGraph( group->getChild(i) ) );
			}

			g->addChildNodes( makeShape(coords, materials, indices) );
		
            return g;
        }
        else
        {
            return makeShape(node);
        }
    }

	static SbBool ProgressCallback(const SbName & itemid, float fraction, SbBool interruptible, void * userdata)
	{
		SceneIO::progress_callback progress_callback = *((SceneIO::progress_callback*)userdata);
		progress_callback(fraction);
		return true;
	}

    virtual bool read(const std::wstring& sFile, Scene::ptr pScene, SceneIO::progress_callback& progress)
    {
        std::map<uint32_t, SceneNode::ptr> NodesMap;
        m_pMapNodes = &NodesMap;
        
        //std::auto_ptr<char> data;
        //size_t size = SceneIO::File(sFile).getContent(data);
        //if ( size == 0 )
        //{
        //    std::wcerr << L"aFile.getContent(data.get()) != size" << std::endl;
        //    return false;
        //}
        
        std::string sFileA(sFile.begin(), sFile.end());

        SoDB::init();
        SoNodeKit::init();
		SoInteraction::init();

		SoDB::addProgressCallback(ProgressCallback, &progress);

        SoInput input;
		//if (!input.readBinaryArray((unsigned char)data.get(), size))
		if (!input.openFile(sFileA.c_str()))
        {
            return false;
        }

		SoSeparator* rootWRLNode = SoDB::readAll(&input);

        input.closeFile();

        if (rootWRLNode)
        {
            rootWRLNode->ref();

            pScene->insertNode( traverseGraph(rootWRLNode) );

            rootWRLNode->unref();
            return true;
        }

        return false;
    }

    virtual std::wstring about() const
    {
        return L"coin3d_loader";
    }
    virtual Uint file_type_count() const
    {
        return 1;
    }
    virtual std::wstring file_type(Uint i) const
    {
        return L"Vrml/Inventor Files";
    }
    virtual std::wstring file_exts(Uint i) const
    {
        return L"*.wrl;*.iv";
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
    return new Coin3DLoader();
}
