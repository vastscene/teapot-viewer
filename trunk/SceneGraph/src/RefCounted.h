// Copyright (c) 2007,2010, Eduard Heidt

#pragma once

#include <boost/intrusive_ptr.hpp>
#include <boost/utility.hpp>

namespace eh
{
    class RefCounted: public boost::noncopyable
    {
    private:
        friend void intrusive_ptr_add_ref(RefCounted* p);
        friend void intrusive_ptr_release(RefCounted* p);
        size_t refcount;
    protected:

        RefCounted():refcount(0){}
        virtual ~RefCounted(){}

    public:
        size_t count() const
        {
            return refcount;
        }
    };

    inline void intrusive_ptr_add_ref(RefCounted* p)
    {
        ++(p->refcount);
    }
    inline void intrusive_ptr_release(RefCounted* p)
    {
        if ( (--p->refcount) == 0 )
        {
            delete p;
            p = NULL;
        }
    }

	template<class T>
	class Ptr: public boost::intrusive_ptr< T >
    {
    public:
        Ptr()
        {
        }
		Ptr(T* p):boost::intrusive_ptr<T>(p)
		{
		}
        template<class U>
		Ptr(boost::intrusive_ptr<U> const & p):boost::intrusive_ptr<T>( dynamic_cast<T*>(p.get()) )
		{
		}
    };

} // namespace boost

