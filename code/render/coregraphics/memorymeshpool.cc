//------------------------------------------------------------------------------
//  memorymeshloader.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/memorymeshpool.h"
#include "coregraphics/mesh.h"
#include "coregraphics/legacy/nvx2streamreader.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/config.h"
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::MemoryMeshPool, 'DMMP', Resources::ResourceMemoryPool);

using namespace Resources;

//------------------------------------------------------------------------------
/**
*/
MemoryMeshPool::MemoryMeshPool()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
MemoryMeshPool::~MemoryMeshPool()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ResourcePool::LoadStatus
MemoryMeshPool::LoadFromMemory(const Resources::ResourceId id, const void* info)
{
	MeshCreateInfo* data = (MeshCreateInfo*)info;
	MeshCreateInfo& mesh = this->Get<0>(id.resourceId);
	mesh = *data;

	this->states[id.poolId] = Resource::Loaded;

	return ResourcePool::Success;
}

//------------------------------------------------------------------------------
/**
*/
void
MemoryMeshPool::Unload(const Resources::ResourceId id)
{
}

//------------------------------------------------------------------------------
/**
*/
void
MemoryMeshPool::BindMesh(const MeshId id, const IndexT prim)
{
#if _DEBUG
	n_assert(id.resourceType == MeshIdType);
#endif
	this->allocator.EnterGet();
	MeshCreateInfo& inf = this->allocator.Get<0>(id.resourceId);

	// setup pipeline (a bit ugly)
	CoreGraphics::SetVertexLayout(inf.primitiveGroups[prim].GetVertexLayout());
	CoreGraphics::SetPrimitiveTopology(inf.topology);

	// set input
	CoreGraphics::SetPrimitiveGroup(inf.primitiveGroups[prim]);

	// bind vertex buffers
	IndexT i;
	for (i = 0; i < inf.streams.Size(); i++)
		CoreGraphics::SetStreamVertexBuffer(inf.streams[i].index, inf.streams[i].vertexBuffer, inf.primitiveGroups[prim].GetBaseVertex());

	if (inf.indexBuffer != CoreGraphics::IndexBufferId::Invalid())
		CoreGraphics::SetIndexBuffer(inf.indexBuffer, inf.primitiveGroups[prim].GetBaseIndex());
	this->allocator.LeaveGet();
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<CoreGraphics::PrimitiveGroup>&
MemoryMeshPool::GetPrimitiveGroups(const MeshId id) const
{
	const MeshCreateInfo& inf = this->allocator.Get<0>(id.resourceId);
	return inf.primitiveGroups;
}

//------------------------------------------------------------------------------
/**
*/
const VertexBufferId
MemoryMeshPool::GetVertexBuffer(const MeshId id, const IndexT stream) const
{
	const MeshCreateInfo& inf = this->allocator.Get<0>(id.resourceId);
	return inf.streams[stream].vertexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
const IndexBufferId
MemoryMeshPool::GetIndexBuffer(const MeshId id) const
{
	const MeshCreateInfo& inf = this->allocator.Get<0>(id.resourceId);
	return inf.indexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::PrimitiveTopology::Code
MemoryMeshPool::GetPrimitiveTopology(const MeshId id) const
{
	const MeshCreateInfo& inf = this->allocator.Get<0>(id.resourceId);
	return inf.topology;
}

//------------------------------------------------------------------------------
/**
*/
void
MemoryMeshPool::BeginGet()
{
	this->allocator.EnterGet();
}

//------------------------------------------------------------------------------
/**
*/
void
MemoryMeshPool::EndGet()
{
	this->allocator.LeaveGet();
}

} // namespace CoreGraphics
