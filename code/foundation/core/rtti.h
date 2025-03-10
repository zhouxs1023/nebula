#pragma once
//------------------------------------------------------------------------------
/**
    @class Core::Rtti

    Nebula's runtime type information for one class. Every class derived
    from Core::RefCounted should use the macros __DeclareClass and __ImplementClass
    to properly initialize the runtime type information for the class. This
    will also automatically register the class with the Core::Factory object
    to implement object construction from class name string or fourcc code.

    (C) 2006 RadonLabs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "core/factory.h"
#include "core/sysfunc.h"
#include "util/string.h"
#include "util/fourcc.h"

//------------------------------------------------------------------------------
namespace Core
{
class RefCounted;
class Rtti
{
public:
    /// define a creation callback function prototype
    typedef void* (*Creator)();
	typedef void* (*ArrayCreator)(SizeT);

    /// constructor
    Rtti(const char* className, Util::FourCC fcc, Creator creatorFunc, ArrayCreator arrayCreatorFunc, const Core::Rtti* parentClass, SizeT instSize);
    /// legacy constructor without FourCC for Mangalore compatibility
    Rtti(const char* className, Creator creatorFunc, ArrayCreator arrayCreatorFunc, const Core::Rtti* parentClass, SizeT instSize);
    /// equality operator
    bool operator==(const Rtti& rhs) const;
    /// inequality operator
    bool operator!=(const Rtti& rhs) const;
    /// get class name
    const Util::String& GetName() const;
    /// get four character code of class
    Util::FourCC GetFourCC() const;
    /// get pointer to parent class
    const Core::Rtti* GetParent() const;
    /// get instance size in bytes
    SizeT GetInstanceSize() const;
    /// create an object of this class
	void* Create() const;
	/// create an array of objects of this class
	void* CreateArray(SizeT num) const;
    /// return true if this rtti is equal or derived from to other rtti
    bool IsDerivedFrom(const Rtti& other) const;
    /// return true if this rtti is equal or derived from to other rtti, by string
    bool IsDerivedFrom(const Util::String& otherClassName) const;
    /// return true if this rtti is equal or derived from to other rtti, by fourcc
    bool IsDerivedFrom(const Util::FourCC& otherClassFourCC) const;
    /// allocate instance memory block (called by class new operator)
    void* AllocInstanceMemory();
	/// allocate instance memory array block (called by class new operator)
	void* AllocInstanceMemoryArray(size_t num);
    /// free instance memory block (called by class delete operator)
    void FreeInstanceMemory(void* ptr);

private:
    /// constructor method, called from the various constructors
    void Construct(const char* className, Util::FourCC fcc, Creator creatorFunc, ArrayCreator arrayCreatorFunc, const Core::Rtti* parentClass, SizeT instSize);

    Util::String name;
    const Core::Rtti* parent;
    Util::FourCC fourCC;
    Creator creator;
	ArrayCreator arrayCreator;
    SizeT instanceSize;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
Rtti::operator==(const Rtti& rhs) const
{
    return this == &rhs;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Rtti::operator!=(const Rtti& rhs) const
{
    return this != &rhs;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
Rtti::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline Util::FourCC
Rtti::GetFourCC() const
{
    return this->fourCC;
}

//------------------------------------------------------------------------------
/**
*/
inline const Core::Rtti*
Rtti::GetParent() const
{
    return this->parent;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
Rtti::GetInstanceSize() const
{
    return this->instanceSize;
}

#include "core/rttimacros.h"

}  // namespace Core
//------------------------------------------------------------------------------
