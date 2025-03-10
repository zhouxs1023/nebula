//------------------------------------------------------------------------------
// lightprobecontext.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "lightprobecontext.h"
#include "graphics/graphicsserver.h"

namespace Graphics
{

LightProbeContext::LightProbeAllocator LightProbeContext::lightProbeAllocator;
_ImplementContext(LightProbeContext, LightProbeContext::lightProbeAllocator);

//------------------------------------------------------------------------------
/**
*/
LightProbeContext::LightProbeContext()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
LightProbeContext::~LightProbeContext()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
const Math::matrix44&
LightProbeContext::GetTransform(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);
	return lightProbeAllocator.Get<0>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
ContextEntityId
LightProbeContext::Alloc()
{
	return ContextEntityId();
}

//------------------------------------------------------------------------------
/**
*/
void
LightProbeContext::Dealloc(ContextEntityId id)
{
}

} // namespace Graphics