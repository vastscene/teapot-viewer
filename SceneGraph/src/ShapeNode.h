// Copyright (c) 2007,2010, Eduard Heidt

#pragma once

#include "SceneNode.h"
#include "Geometry.h"
#include "Material.h"
#include <boost/unordered_map.hpp>

namespace eh
{

    typedef boost::unordered_map< std::pair< Ptr<Material>, Geometry::TYPE >, Ptr<Geometry> > MATERIAL_GEOMETRY_CONTAINER;

    class GeometryIterator: public MATERIAL_GEOMETRY_CONTAINER::const_iterator
    {
    public:
        explicit GeometryIterator(MATERIAL_GEOMETRY_CONTAINER::const_iterator it):
                MATERIAL_GEOMETRY_CONTAINER::const_iterator(it){}
//	GeometryIterator(GeometryIterator& git):m_it(git.m_it){}
        const Ptr<Material> getMaterial() const
        {
            return (*this)->first.first;
        }
        Geometry::TYPE getType() const
        {
            return (*this)->first.second;
        }
        const Ptr<Geometry>& getGeometry() const
        {
            return (*this)->second;
        }
    };

    class API_3D ShapeNode: public SceneNode
    {
    private:
        ShapeNode();
        MATERIAL_GEOMETRY_CONTAINER m_geometry;
    public:

        static Ptr<ShapeNode> create();
        static Ptr<ShapeNode> create(Ptr<Material> pMat, Ptr<Geometry> pGeo)
        {
            Ptr<ShapeNode> s = create();
            s->addGeometry(pMat, pGeo);
            return s;
        }

        virtual ~ShapeNode();

        void addGeometry(Ptr<Material> pMat, Ptr<Geometry> pGeo);

        GeometryIterator GeometryBegin() const
        {
            return GeometryIterator(m_geometry.begin());
        }
        GeometryIterator GeometryEnd() const
        {
            return  GeometryIterator(m_geometry.end());
        }

        virtual void accept(IVisitor &visitor)
        {
            visitor.visit(*this);
        }
        virtual const AABBox& calcBounding();
    };

}//end namespace
