//------------------------------------------------------------------------------
//  unit.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include <lib/geometrybase.fxh>
#include <lib/techniques.fxh>
#include <lib/colorblending.fxh>
#include <lib/shared.fxh>

group(BATCH_GROUP) shared varblock UnitBlock
{
	textureHandle TeamColorMask;
	vec4 TeamColor;
};

samplerstate TeamSampler
{
	//Samplers = { TeamColorMask };
	Filter = MinMagMipLinear;
	AddressU = Wrap;
	AddressV = Wrap;
};

//------------------------------------------------------------------------------
/**
	Not a ubershader
*/
[earlydepth]
shader
void
psNotaUnit(in vec3 ViewSpacePos,
	in vec3 Tangent,
	in vec3 Normal,
	in vec3 Binormal,
	in vec2 UV,
	in vec3 WorldViewVec,
	[color0] out vec4 Albedo,
	[color1] out vec4 Normals,
	[color2] out float Depth,	
	[color3] out vec4 Specular,
	[color4] out vec4 Emissive) 
{	
	vec4 diffColor = sample2D(AlbedoMap, TeamSampler, UV) * vec4(MatAlbedoIntensity.rgb, 1);
	float roughness = sample2D(RoughnessMap, TeamSampler, UV).r * MatRoughnessIntensity;
	vec4 specColor = sample2D(SpecularMap, TeamSampler, UV) * MatSpecularIntensity;
	float cavity = sample2D(CavityMap, TeamSampler, UV).r;
	float teamMask = sample2D(TeamColorMask, TeamSampler, UV).r;
	vec4 maskColor = TeamColor * teamMask;
	
	vec4 normals = sample2D(NormalMap, NormalSampler, UV);
	vec3 bumpNormal = normalize(calcBump(Tangent, Binormal, Normal, normals));

	mat4x4 invView = InvView;
	mat2x3 env = calcEnv(specColor, bumpNormal, ViewSpacePos, WorldViewVec, invView, roughness);
	vec4 spec = calcSpec(specColor.rgb, roughness);
	vec4 albedo = calcColor(diffColor + vec4(Overlay(diffColor.rgb, maskColor.rgb), 0), vec4(1), spec);	
	vec4 emissive = vec4((env[0] * albedo.rgb + env[1]) * cavity, 1);

	Specular = spec;
	Albedo = albedo;
	Emissive = emissive;
	Depth = calcDepth(ViewSpacePos);
	Normals = PackViewSpaceNormal(bumpNormal);
}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(
	Static, 
	"Static", 
	vsStatic(), 
	psNotaUnit(calcColor = SimpleColor,
		calcBump = NormalMapFunctor,
		calcSpec = NonReflectiveSpecularFunctor,
		calcDepth = ViewSpaceDepthFunctor,
		calcEnv = PBR),
	StandardState);
	
//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(
	Skinned, 
	"Skinned", 
	vsSkinned(), 
	psNotaUnit(calcColor = SimpleColor,
		calcBump = NormalMapFunctor,
		calcSpec = NonReflectiveSpecularFunctor,
		calcDepth = ViewSpaceDepthFunctor,
		calcEnv = PBR),
	StandardState);