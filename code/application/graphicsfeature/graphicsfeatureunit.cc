//------------------------------------------------------------------------------
//  graphicsfeature/graphicsfeatureunit.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "graphicsfeature/graphicsfeatureunit.h"
#include "lighting/lightcontext.h"
#include "models/modelcontext.h"
#include "graphics/cameracontext.h"
#include "visibility/visibilitycontext.h"
#include "dynui/imguicontext.h"
#include "characters/charactercontext.h"
#include "dynui/im3d/im3dcontext.h"
#include "appgame/gameapplication.h"

using namespace Graphics;
using namespace Visibility;
using namespace Models;

namespace GraphicsFeature
{
__ImplementClass(GraphicsFeature::GraphicsFeatureUnit, 'FXFU' , Game::FeatureUnit);
__ImplementSingleton(GraphicsFeatureUnit);

//------------------------------------------------------------------------------
/**
*/
GraphicsFeatureUnit::GraphicsFeatureUnit()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
GraphicsFeatureUnit::~GraphicsFeatureUnit()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsFeatureUnit::OnActivate()
{
	FeatureUnit::OnActivate();

    this->gfxServer = Graphics::GraphicsServer::Create();
    this->inputServer = Input::InputServer::Create();
    this->gfxServer->Open();
    this->inputServer->Open();

    SizeT width = this->GetCmdLineArgs().GetInt("-w", 1280);
    SizeT height = this->GetCmdLineArgs().GetInt("-h", 960);

    //FIXME
    CoreGraphics::WindowCreateInfo wndInfo =
    {
        CoreGraphics::DisplayMode{ 100, 100, width, height },
        "GraphicsFeature",
        "",
        CoreGraphics::AntiAliasQuality::None,
        true,
        true,
        false
    };
    this->wnd = CreateWindow(wndInfo);


    CameraContext::Create();
    ModelContext::Create();
    ObserverContext::Create();
    ObservableContext::Create();
    Lighting::LightContext::Create();
    Characters::CharacterContext::Create();
    Dynui::ImguiContext::Create();
    Im3d::Im3dContext::Create();

    this->defaultView = gfxServer->CreateView("mainview", "frame:vkdebug.json");
    this->defaultStage = gfxServer->CreateStage("defaultStage", true);
    this->defaultView->SetStage(this->defaultStage);

    this->globalLight = Graphics::CreateEntity();
    Lighting::LightContext::RegisterEntity(this->globalLight);
    Lighting::LightContext::SetupGlobalLight(this->globalLight, Math::float4(1, 1, 1, 0), 1.0f, Math::float4(0, 0, 0, 0), Math::float4(0, 0, 0, 0), 0.0f, Math::vector(1, 1, 1), false);

    ObserverContext::CreateBruteforceSystem({});

	GraphicsComponent::Create();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsFeatureUnit::OnDeactivate()
{
    FeatureUnit::OnDeactivate();

	GraphicsComponent::Discard();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsFeatureUnit::OnBeginFrame()
{
    this->inputServer->BeginFrame();
    this->gfxServer->BeginFrame();
    //FIXME
    this->gfxServer->BeforeViews();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsFeatureUnit::OnFrame()
{
    this->gfxServer->RenderViews();
    this->gfxServer->EndViews();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsFeatureUnit::OnEndFrame()
{
    this->gfxServer->EndFrame();
    CoreGraphics::WindowPresent(this->wnd, App::GameApplication::FrameIndex);
    this->inputServer->EndFrame();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsFeatureUnit::OnRenderDebug()
{
}

} // namespace Game
