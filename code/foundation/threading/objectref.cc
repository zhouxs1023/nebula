//------------------------------------------------------------------------------
//  objectref.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "threading/objectref.h"

namespace Threading
{
__ImplementClass(Threading::ObjectRef, 'OBJR', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
ObjectRef::ObjectRef() :
    ref(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ObjectRef::~ObjectRef()
{
    if (this->IsValid())
    {
        this->Invalidate();
    }
}        

} // namespace Threading