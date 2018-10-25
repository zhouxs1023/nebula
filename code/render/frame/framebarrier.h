#pragma once
//------------------------------------------------------------------------------
/**
	Implements a barrier between frame operations.
	
	(C) 2016-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "coregraphics/barrier.h"
namespace Frame
{
class FrameBarrier : public FrameOp
{
public:
	/// constructor
	FrameBarrier();
	/// destructor
	virtual ~FrameBarrier();

	/// run operation
	void Run(const IndexT frameIndex);

	CoreGraphics::BarrierId barrier;
	CoreGraphicsQueueType queue;
};

} // namespace Frame2