//------------------------------------------------------------------------------
//  jsonreader.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include "pjson/pjson.h"
#include "io/jsonreader.h"
#include "util/variant.h"

namespace IO
{
__ImplementClass(IO::JsonReader, 'JSLR', IO::StreamReader);


using namespace Util;
using namespace Math;
using namespace pjson;
    
//------------------------------------------------------------------------------
/**
*/
JsonReader::JsonReader() :
    document(0),
    curNode(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
JsonReader::~JsonReader()
{
    if (this->IsOpen())
    {
        this->Close();
    }
}

//------------------------------------------------------------------------------
/**
    Opens the stream and reads the content of the stream into
    pjson.
*/
bool
JsonReader::Open()
{
    n_assert(0 == this->document);
    
    if (StreamReader::Open())
    {
        this->document = n_new(pjson::document);

        // create an document object        
        // we need to 0 terminate the input buffer
        SizeT fileSize = this->stream->GetSize();
        this->buffer = (char*)Memory::Alloc(Memory::StreamDataHeap, fileSize + 1);
        this->stream->Read(buffer, fileSize);
        this->buffer[fileSize] = '\0';
        if (!this->document->deserialize_in_place(buffer))
        {
            const URI& uri = this->stream->GetURI();
            const pjson::error_info & error = this->document->get_error();
            Util::String position;
            if(error.m_ofs<(size_t)this->stream->GetSize())
            {
                position.Set(((const char *)this->buffer) + error.m_ofs, 40);
            }            
            n_error("JsonReader::Open(): failed to parse json file: %s\n%s\nat: %s\n", uri.AsString().AsCharPtr(), error.m_pError_message, position.AsCharPtr());
            
            return false;
        }        

        // set the current node to the root node
        this->curNode = this->document;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
JsonReader::Close()
{
    n_assert(0 != this->document);
    this->document->clear();    
    n_delete(this->document);
    this->curNode = 0;    
    Memory::Free(Memory::StreamDataHeap, this->buffer);
    this->buffer = nullptr;
    StreamReader::Close();
}

//------------------------------------------------------------------------------
/**
    This method returns true if the node identified by path exists. Path
    follows the normal filesystem path conventions, "/" is the separator,    
    starts with a "/", a relative path doesn't.
*/
bool
JsonReader::HasNode(const String& path)
{
    n_assert(0 != this->document);
    bool absPath = (path[0] == '/');
    Array<String> tokens = path.Tokenize("/");

    // get starting node (either root or current node)
    const value_variant* node;
    if (absPath)
    {
        node = this->document;       
    }
    else
    {
        n_assert(0 != this->curNode);
        node = this->curNode;
    }

    // iterate through path components
    int i;
    int num = tokens.Size();
    for (i = 0; i < num; i++)
    {       
        const String& cur = tokens[i];
        if (!node->is_object())
        {
            return false;
        }
        node = node->find_value_variant(cur.AsCharPtr());
        if (0 == node)
        {
            return false;
        }
    }
    return true;
}

//------------------------------------------------------------------------------
/**
	Resets the json reader to the root node
*/
void 
JsonReader::SetToRoot()
{
	// set the current node to the root node
    this->curNode = this->document;
    this->parents.Clear();
    this->parentIdx.Clear();
	n_assert(this->curNode);
}

//------------------------------------------------------------------------------
/**
    Set the node pointed to by the path string as current node. The path
    may be absolute or relative, following the usual filesystem path 
    conventions. Separator is a slash. 
    Arrays or objects with children can be indexed with []
*/
bool
JsonReader::SetToNode(const String& path)
{
    n_assert(this->IsOpen());
    n_assert(path.IsValid());

    bool absPath = (path[0] == '/');
    Array<String> tokens = path.Tokenize("/");

    // get starting node (either root or current node)    
    if (absPath)
    {
        this->SetToRoot();        
    }    
    n_assert(0 != this->curNode);    

    // iterate through path components
    int i;
    int num = tokens.Size();
    for (i = 0; i < num; i++)
    {        
        const String& cur = tokens[i];

        // check if token is array access
        if (String::MatchPattern(cur, "\\[[0-9]*\\]"))
        {
            Util::String num = cur;
            num.Trim("[]");
            unsigned int idx = num.AsInt();
            if (!this->curNode->is_object_or_array()) goto fail;
            if (!(idx < this->curNode->size())) goto fail;

            const value_variant* node = &this->curNode->get_value_at_index(idx);
            this->parents.Push(this->curNode);
            this->parentIdx.Push(this->childIdx);
            this->curNode = node;
            this->childIdx = idx;
        }
        else
        {
            if (!this->SetToFirstChild(cur)) goto fail;            
        }
    }        
    return true;
fail:
    this->parents.Clear();
    this->parentIdx.Clear();
    return false;
}

//------------------------------------------------------------------------------
/**
    Sets the current node to the first child node. If no child node exists,
    the current node will remain unchanged and the method will return false.
    If name is a valid string, only child element matching the name will 
    be returned. If name is empty, all child nodes will be considered.
*/
bool
JsonReader::SetToFirstChild(const Util::String& name)
{
    n_assert(this->IsOpen());
    n_assert(0 != this->curNode);
    const value_variant* child = 0;    
    if (name.IsEmpty())
    {
        child = &this->curNode->get_value_at_index(0);
    }
    else
    {
        child = this->curNode->find_value_variant(name.AsCharPtr());
    }
    if (child)
    {
        this->parents.Push(this->curNode);
        this->parentIdx.Push(this->childIdx);
        this->curNode = child;
        this->childIdx = 0;
        return true;
    }
    else
    {                
        return false;
    }
}

//------------------------------------------------------------------------------
/**
    Sets the current node to the next sibling. If no more children exist,
    the current node will be reset to the parent node and the method will 
    return false. If name is a valid string, only child element matching the 
    name will be returned. If name is empty, all child nodes will be considered.
*/
bool
JsonReader::SetToNextChild()
{
    n_assert(this->IsOpen());
    n_assert(0 != this->curNode);
    n_assert(0 <= this->childIdx);

    this->childIdx++;

    if (this->childIdx < (IndexT)this->parents.Peek()->size())
    {
        const value_variant* child = &this->parents.Peek()->get_value_at_index(this->childIdx);
        if (child)
        {
            this->curNode = child;
            return true;
        }
    }
	this->SetToParent();
    return false;    
}

//------------------------------------------------------------------------------
/**
    Sets the current node to its parent. If no parent exists, the
    current node will remain unchanged and the method will return false.
*/
bool
JsonReader::SetToParent()
{
    n_assert(this->IsOpen());
    n_assert(0 != this->curNode);
    if (!this->parents.IsEmpty())
    {
        this->curNode = this->parents.Pop();
        this->childIdx = this->parentIdx.Pop();
        return true;
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------
/**
 
*/
bool
JsonReader::IsArray() const
{
    n_assert(this->IsOpen());
    n_assert(0 != this->curNode);
    return this->curNode->is_array();
}


//------------------------------------------------------------------------------
/**

*/
bool
JsonReader::IsObject() const
{
    n_assert(this->IsOpen());
    n_assert(0 != this->curNode);
    return this->curNode->is_object();
}
//------------------------------------------------------------------------------
/**

*/
bool
JsonReader::HasChildren() const
{
    n_assert(this->IsOpen());
    n_assert(0 != this->curNode);
    return this->curNode->has_children();
}

//------------------------------------------------------------------------------
/**

*/
SizeT
JsonReader::CurrentSize() const
{
    n_assert(this->IsOpen());
    n_assert(0 != this->curNode);
    if (this->curNode->is_object_or_array())
    {
        return this->curNode->size();
    }
    return 0;
}
 
//------------------------------------------------------------------------------
/**
    Return true if an attribute of the given name exists on the current node.
*/
bool
JsonReader::HasAttr(const char* name) const
{
    n_assert(this->IsOpen());
    n_assert(0 != this->curNode);
    n_assert(0 != name);
    n_assert(this->curNode->is_object());
    return (0 != this->curNode->has_key(name));
}

//------------------------------------------------------------------------------
/**
    Return array with names of all attrs on current node
*/
Array<String>
JsonReader::GetAttrs() const
{
    n_assert(this->IsOpen());
    n_assert(0 != this->curNode);
    n_assert(this->curNode->is_object());
    Array<String> res;
    for (unsigned int i = 0; i < this->curNode->size(); ++i)
    {
        res.Append(this->curNode->get_key_name_at_index(i));
    }
    return res;
}

//------------------------------------------------------------------------------
/**
*/
Util::String
JsonReader::GetCurrentNodeName() const
{
	//  get_key_name_at_index(this->childIdx)
	auto parent = this->parents.Peek();
	// auto parentIdx = this->parentIdx.Peek();
	return parent->get_key_name_at_index(this->childIdx);
}


//------------------------------------------------------------------------------
/**
    
*/
const pjson::value_variant* 
JsonReader::GetChild(const char * name) const
{
    n_assert(this->IsOpen());
    n_assert(0 != this->curNode);

    if (0 != name)
    {
        n_assert(this->curNode->is_object());        
        return this->curNode->find_value_variant(name);
    }
    else
    {
        return this->curNode;
    }
}

//------------------------------------------------------------------------------
/**
    Return the provided attribute as string. If the attribute does not exist
    the method will fail hard (use HasAttr() to check for its existance).
*/
String
JsonReader::GetString(const char* name) const
{
    const value_variant * node = this->GetChild(name);

    n_assert(node);
    n_assert(node->is_string());
    return node->as_string_ptr();    
}

//------------------------------------------------------------------------------
/**
    Return the provided attribute as stringatom. If the attribute does not exist
    the method will fail hard (use HasAttr() to check for its existance).
*/
StringAtom
JsonReader::GetStringAtom(const char* name) const
{
    const value_variant * node = this->GetChild(name);

    n_assert(node);
    n_assert(node->is_string());
    return node->as_string_ptr();
}

//------------------------------------------------------------------------------
/**
    Return the provided attribute as a bool. If the attribute does not exist
    the method will fail hard (use HasAttr() to check for its existance).
*/
bool
JsonReader::GetBool(const char* name) const
{
    const value_variant * node = this->GetChild(name);

    n_assert(node);
    n_assert(node->is_bool());
    return node->as_bool();
}

//------------------------------------------------------------------------------
/**
    Return the provided attribute as int. If the attribute does not exist
    the method will fail hard (use HasAttr() to check for its existance).
*/
int
JsonReader::GetInt(const char* name) const
{
    const value_variant * node = this->GetChild(name);

    n_assert(node);
    n_assert(node->is_int());
    return node->as_int32();
}

//------------------------------------------------------------------------------
/**
	Return the provided attribute as float2. If the attribute does not exist
	the method will fail hard (use HasAttr() to check for its existance).
*/
uint
JsonReader::GetUInt(const char * attr) const
{
	const value_variant * node = this->GetChild(attr);
	
	n_assert(node);
	n_assert(node->is_int());
	return (uint)node->as_int32();
}

//------------------------------------------------------------------------------
/**
    Return the provided attribute as float. If the attribute does not exist
    the method will fail hard (use HasAttr() to check for its existance).
*/
float
JsonReader::GetFloat(const char* name) const
{
    const value_variant * node = this->GetChild(name);

    n_assert(node);
	// Floats can be either double or integer if it has no fraction
    n_assert(node->is_double() || node->is_int());
    return node->as_float();
}

//------------------------------------------------------------------------------
/**
    Return the provided attribute as float2. If the attribute does not exist
    the method will fail hard (use HasAttr() to check for its existance).
*/
float2
JsonReader::GetFloat2(const char* name) const
{
    const value_variant * node = this->GetChild(name);

    n_assert(node->is_array());
    n_assert(node->size() == 2);
    return float2(node->get_value_at_index(0).as_float(), node->get_value_at_index(1).as_float());
}

//------------------------------------------------------------------------------
/**
    Return the provided attribute as float4. If the attribute does not exist
    the method will fail hard (use HasAttr() to check for its existance).
*/
float4
JsonReader::GetFloat4(const char* name) const
{
    const value_variant * node = this->GetChild(name);

    n_assert(node->is_array());
    n_assert(node->size() == 4);
    NEBULA_ALIGN16 float v[4];
    for (int i = 0; i < 4; i++)
    {
        v[i] = node->get_value_at_index(i).as_float();
    }
    float4 f;
    f.load(v);
    return f;
}

//------------------------------------------------------------------------------
/**
    Return the provided attribute as matrix44. If the attribute does not exist
    the method will fail hard (use HasAttr() to check for its existance).
*/
matrix44
JsonReader::GetMatrix44(const char* name) const
{
    const value_variant * node = this->GetChild(name);

    n_assert(node->is_array());
    n_assert(node->size() == 16);
    NEBULA_ALIGN16 float v[16];
    for (int i = 0; i < 16; i++)
    {
        v[i] = node->get_value_at_index(i).as_float();
    }
    matrix44 m;
    m.load(v);
    return m;
}

//------------------------------------------------------------------------------
/**
Return the provided attribute as transform44. If the attribute does not exist
the method will fail hard (use HasAttr() to check for its existance).
*/
transform44
JsonReader::GetTransform44(const char* name) const
{
    const value_variant * node = this->GetChild(name);

    n_assert(node->is_array());
    n_assert(node->size() == 36);

    float v[36];
    for (int i = 0; i < 36; i++)
    {
        v[i] = node->get_value_at_index(i).as_float();
    }
    transform44 m;
    m.loadu(v);
    return m;
}
    
//------------------------------------------------------------------------------
/**
    Return the provided optional attribute as string. If the attribute doesn't
    exist, the default value will be returned.
*/
String
JsonReader::GetOptString(const char* name, const String& defaultValue) const
{
    if (this->HasAttr(name))
    {
        return this->GetString(name);
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
    Return the provided optional attribute as bool. If the attribute doesn't
    exist, the default value will be returned.
*/
bool
JsonReader::GetOptBool(const char* name, bool defaultValue) const
{
    if (this->HasAttr(name))
    {
        return this->GetBool(name);
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
    Return the provided optional attribute as int. If the attribute doesn't
    exist, the default value will be returned.
*/
int
JsonReader::GetOptInt(const char* name, int defaultValue) const
{
    if (this->HasAttr(name))
    {
        return this->GetInt(name);
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
    Return the provided optional attribute as float. If the attribute doesn't
    exist, the default value will be returned.
*/
float
JsonReader::GetOptFloat(const char* name, float defaultValue) const
{
    if (this->HasAttr(name))
    {
        return this->GetFloat(name);
    }
    else
    {
        return defaultValue;
    }
}
 
//------------------------------------------------------------------------------
/**
    Return the provided optional attribute as float2. If the attribute doesn't
    exist, the default value will be returned.
*/
float2
JsonReader::GetOptFloat2(const char* name, const float2& defaultValue) const
{
    if (this->HasAttr(name))
    {
        return this->GetFloat2(name);
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
    Return the provided optional attribute as float4. If the attribute doesn't
    exist, the default value will be returned.
*/
float4
JsonReader::GetOptFloat4(const char* name, const float4& defaultValue) const
{
    if (this->HasAttr(name))
    {
        return this->GetFloat4(name);
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
    Return the provided optional attribute as matrix44. If the attribute doesn't
    exist, the default value will be returned.
*/
matrix44
JsonReader::GetOptMatrix44(const char* name, const matrix44& defaultValue) const
{
    if (this->HasAttr(name))
    {
        return this->GetMatrix44(name);
    }
    else
    {
        return defaultValue;
    }
}

//------------------------------------------------------------------------------
/**
Return the provided optional attribute as transform44. If the attribute doesn't
exist, the default value will be returned.
*/
transform44
JsonReader::GetOptTransform44(const char* name, const transform44& defaultValue) const
{
	if (this->HasAttr(name))
	{
		return this->GetTransform44(name);
	}
	else
	{
		return defaultValue;
	}
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonReader::Get<Util::Array<uint32_t>>(Util::Array<uint32_t> & ret, const char* attr)
{
    const value_variant * node = this->GetChild(attr);

    n_assert(node->is_array());
    unsigned int count = node->size();    
    ret.Reserve(count);
    for (unsigned int i = 0; i < count; i++)
    {
        ret.Append(node->get_value_at_index(i).as_int32());
    }    
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonReader::Get<Util::Array<int>>(Util::Array<int> & ret, const char* attr)
{
    const value_variant * node = this->GetChild(attr);

    n_assert(node->is_array());
    unsigned int count = node->size();
    ret.Reserve(count);
    for (unsigned int i = 0; i < count; i++)
    {
        ret.Append(node->get_value_at_index(i).as_int32());
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonReader::Get<bool>(bool & ret, const char* attr)
{
    const value_variant * node = this->GetChild(attr);

    n_assert(node->is_bool());
    ret = node->as_bool();
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonReader::Get<int>(int & ret, const char* attr)
{
    const value_variant * node = this->GetChild(attr);

    n_assert(node->is_int());
    ret = node->as_int32();
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonReader::Get<Math::matrix44>(Math::matrix44 & ret, const char* attr)
{        
    ret = this->GetMatrix44(attr);
}


//------------------------------------------------------------------------------
/**
*/
template<> void JsonReader::Get<Math::vector>(Math::vector & ret, const char* attr)
{
    //FIXME this searches twice
    const value_variant * node = this->GetChild(attr);
    NEBULA_ALIGN16 float v[4];
    for (int i = 0; i < 3; i++)
    {
        v[i] = node->get_value_at_index(i).as_float();
    }
    ret.load(v);    
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonReader::Get<uint32_t>(uint32_t & ret, const char* attr)
{
    const value_variant * node = this->GetChild(attr);

    n_assert(node->is_int());
    ret = node->as_int32();
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonReader::Get<uint16_t>(uint16_t & ret, const char* attr)
{
    const value_variant * node = this->GetChild(attr);

    n_assert(node->is_int());
    ret = (uint16_t)node->as_int32();
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonReader::Get<float>(float & ret, const char* attr)
{
    const value_variant * node = this->GetChild(attr);

    n_assert(node->is_numeric());
    ret = node->as_float();
}

//------------------------------------------------------------------------------
/**
	Returns the attribute as variant type.
	This checks what type the variant is and returns the value type that it indicates.
	If incoming variant type is void, the reader will automatically detect type from the attribute.
	Will most likely assert if type is incorrect, so use with caution!
*/
template<> void JsonReader::Get<Util::Variant>(Util::Variant & ret, const char* attr)
{
	const value_variant * node = this->GetChild(attr);

	switch (ret.GetType())
	{
	case Util::Variant::Type::Void:
	{
		// Special case: No type has been assigned, let the parser decide the type.
		const value_variant * node = this->GetChild(attr);

		if (node->is_bool())
		{
			ret.SetType(Util::Variant::Type::Bool);
			ret.SetBool(node->as_bool());
		}
		if (node->is_int())
		{
			ret.SetType(Util::Variant::Type::Int);
			ret.SetInt(node->as_int32());
		}
		else if (node->is_double())
		{
			ret.SetType(Util::Variant::Type::Double);
			ret.SetDouble(node->as_double());
		}
		else if (node->is_string())
		{
			ret.SetType(Util::Variant::Type::String);
			ret.SetString(node->as_string_ptr());
		}
		else
		{
			n_error("Could not resolve variant type!");
		}
		break;
	}
	case Util::Variant::Type::Bool:
		ret.SetBool(this->GetBool(attr));
		break;
	case Util::Variant::Type::Float:
		ret.SetFloat(this->GetFloat(attr));
		break;
	case Util::Variant::Type::Float4:
		ret.SetFloat4(this->GetFloat4(attr));
		break;
	case Util::Variant::Type::Matrix44:
		ret.SetMatrix44(this->GetMatrix44(attr));
		break;
	case Util::Variant::Type::UInt:
		ret.SetUInt(this->GetUInt(attr));
		break;
	case Util::Variant::Type::String:
		ret.SetString(this->GetString(attr));
		break;
	case Util::Variant::Type::Int:
		ret.SetInt(this->GetInt(attr));
		break;
	case Util::Variant::Type::Guid:
		ret.SetGuid(Util::Guid::FromString(this->GetString(attr)));
		break;
	default:
		n_error("Could not resolve variant type!");
		break;
	}
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonReader::Get<Util::String>(Util::String & ret, const char* attr)
{
    const value_variant * node = this->GetChild(attr);

    n_assert(node->is_string());
    ret = node->as_string_ptr();
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonReader::Get<Util::StringAtom>(Util::StringAtom & ret, const char* attr)
{
    const value_variant * node = this->GetChild(attr);

    n_assert(node->is_string());
    ret = node->as_string_ptr();
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonReader::Get<Util::Array<float>>(Util::Array<float> &ret, const char* attr)
{
    const value_variant * node = this->GetChild(attr);

    n_assert(node->is_array());
    unsigned int count = node->size();
    ret.Reserve(count);
    for (unsigned int i = 0; i < count; i++)
    {
        ret.Append(node->get_value_at_index(i).as_float());
    }    
}

//------------------------------------------------------------------------------
/**
*/
template<> void JsonReader::Get<Util::Array<Util::String>>(Util::Array<Util::String> &ret, const char* attr)
{
    const value_variant * node = this->GetChild(attr);

    n_assert(node->is_array());
    unsigned int count = node->size();
    ret.Reserve(count);
    for (unsigned int i = 0; i < count; i++)
    {
        ret.Append(node->get_value_at_index(i).as_string_ptr());
    }    
}


//------------------------------------------------------------------------------
/**
*/
template<> bool JsonReader::GetOpt<bool>(bool & ret, const char* attr)
{
    const value_variant * node = this->GetChild(attr);
    if (node)
    {
        n_assert(node->is_bool());
        ret = node->as_bool();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
template<> bool JsonReader::GetOpt<int>(int & ret, const char* attr)
{
    const value_variant * node = this->GetChild(attr);
    if (node)
    {
        n_assert(node->is_int());
        ret = node->as_int32();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
template<> bool JsonReader::GetOpt<uint16_t>(uint16_t & ret, const char* attr)
{
    const value_variant * node = this->GetChild(attr);
    if (node)
    {
        n_assert(node->is_int());
        ret = static_cast<uint16_t>(node->as_int32());
        return true;
    }
    return false;
}


//------------------------------------------------------------------------------
/**
*/
template<> bool JsonReader::GetOpt<Math::point>(Math::point & ret, const char* attr)
{
    //FIXME this searches twice
    const value_variant * node = this->GetChild(attr);
    if (node)
    {
        NEBULA_ALIGN16 float v[4];
        for (int i = 0; i < 3; i++)
        {
            v[i] = node->get_value_at_index(i).as_float();
        }
        ret.load(v);
        return true;
    }
    return false;
}
//------------------------------------------------------------------------------
/**
*/
template<> bool JsonReader::GetOpt<uint32_t>(uint32_t & ret, const char* attr)
{
    const value_variant * node = this->GetChild(attr);
    if (node)
    {
        n_assert(node->is_int());
        ret = node->as_int32();        
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
template<> bool JsonReader::GetOpt<float>(float & ret, const char* attr)
{
    const value_variant * node = this->GetChild(attr);
    if (node)
    {
        n_assert(node->is_numeric());
        ret = node->as_float();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
template<> bool JsonReader::GetOpt<Math::float4>(Math::float4 & ret, const char* attr)
{    
    //FIXME this searches twice
    const value_variant * node = this->GetChild(attr);
    if (node)
    {
        ret = this->GetFloat4(attr);        
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
template<> bool JsonReader::GetOpt<Math::vector>(Math::vector & ret, const char* attr)
{
    //FIXME this searches twice
    const value_variant * node = this->GetChild(attr);
    if (node)
    {
        NEBULA_ALIGN16 float v[4];
        for (int i = 0; i < 3; i++)
        {
            v[i] = node->get_value_at_index(i).as_float();
        }
        ret.load(v);
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
template<> bool JsonReader::GetOpt<Math::quaternion>(Math::quaternion & ret, const char* attr)
{
    //FIXME this searches twice
    const value_variant * node = this->GetChild(attr);
    if (node)
    {
        ret = this->GetFloat4(attr);
        return true;
    }
    return false;
}


//------------------------------------------------------------------------------
/**
*/
template<> bool JsonReader::GetOpt<Math::matrix44>(Math::matrix44 & ret, const char* attr)
{
    //FIXME this searches twice
    const value_variant * node = this->GetChild(attr);
    if (node)
    {
        ret = this->GetMatrix44(attr);
        return true;
    }
    return false;
}
//------------------------------------------------------------------------------
/**
*/
template<> bool JsonReader::GetOpt<Util::String>(Util::String & ret, const char* attr)
{
    const value_variant * node = this->GetChild(attr);
    if (node)
    {
        n_assert(node->is_string());
        ret = node->as_string_ptr();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
template<> bool JsonReader::GetOpt<Util::Array<int>>(Util::Array<int> & target, const char* attr)
{
    const value_variant * node = this->GetChild(attr);

    if (node)
    {
        n_assert(node->is_array());
        unsigned int count = node->size();
        target.Reserve(count);
        for (unsigned int i = 0; i < count; i++)
        {
            target.Append(node->get_value_at_index(i).as_int32());
        }
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
template<> bool JsonReader::GetOpt<Util::Array<uint32_t>>(Util::Array<uint32_t> & target, const char* attr)
{
    const value_variant * node = this->GetChild(attr);

    if (node)
    {
        n_assert(node->is_array());
        unsigned int count = node->size();
        target.Reserve(count);
        for (unsigned int i = 0; i < count; i++)
        {
            target.Append(node->get_value_at_index(i).as_int32());
        }
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
template<> bool JsonReader::GetOpt<Util::Array<float>>(Util::Array<float> & target, const char* attr)
{
    const value_variant * node = this->GetChild(attr);
    
    if (node)
    {
        n_assert(node->is_array());
        unsigned int count = node->size();
        target.Reserve(count);
        for (unsigned int i = 0; i < count; i++)
        {
            target.Append(node->get_value_at_index(i).as_float());
        }
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
template<> bool JsonReader::GetOpt<Util::Array<Util::String>>(Util::Array<Util::String> & target, const char* attr)
{
    const value_variant * node = this->GetChild(attr);

    if (node)
    {
        n_assert(node->is_array());
        unsigned int count = node->size();
        target.Reserve(count);
        for (unsigned int i = 0; i < count; i++)
        {
            target.Append(node->get_value_at_index(i).as_string_ptr());
        }
        return true;
    }
    return false;
}

} // namespace IO
