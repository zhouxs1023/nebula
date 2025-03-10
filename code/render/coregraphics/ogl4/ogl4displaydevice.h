#pragma once
//------------------------------------------------------------------------------
/**
    @class OpenGL4::OGL4DisplayDevice
    
    OpenGL4 implementation of DisplayDevice class. Manages the application
    window. Uses Win32DisplayDevice for creating the base window and the event loop, 
	will not work on other platforms obviously
    
    (C) 2012-2018 Individual contributors, see AUTHORS file
*/
#include "coregraphics/win32/win32displaydevice.h"
#include "util/array.h"



//------------------------------------------------------------------------------
namespace OpenGL4
{
class OGL4DisplayDevice : public Win32::Win32DisplayDevice
{
    __DeclareClass(OGL4DisplayDevice);
    __DeclareSingleton(OGL4DisplayDevice);
public:
    /// constructor
    OGL4DisplayDevice();
    /// destructor
    virtual ~OGL4DisplayDevice();
    /// check if the adapter actually exists
    bool AdapterExists(CoreGraphics::Adapter::Code adapter);
    /// get available display modes on given adapter
    Util::Array<CoreGraphics::DisplayMode> GetAvailableDisplayModes(CoreGraphics::Adapter::Code adapter, CoreGraphics::PixelFormat::Code pixelFormat);
    /// return true if a given display mode is supported
    bool SupportsDisplayMode(CoreGraphics::Adapter::Code adapter, const CoreGraphics::DisplayMode& requestedMode);
    /// get current adapter display mode (i.e. the desktop display mode)
    CoreGraphics::DisplayMode GetCurrentAdapterDisplayMode(CoreGraphics::Adapter::Code adapter);
    /// get general info about display adapter
    CoreGraphics::AdapterInfo GetAdapterInfo(CoreGraphics::Adapter::Code adapter);

private:
    /// adjust window size taking client area into account
    virtual CoreGraphics::DisplayMode ComputeAdjustedWindowRect();

		
};

} // namespace OpenGL4
//------------------------------------------------------------------------------
    