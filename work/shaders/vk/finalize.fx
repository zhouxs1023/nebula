//------------------------------------------------------------------------------
//  finalize.fx
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/techniques.fxh"
#include "lib/shared.fxh"

shared varblock FinalizeBlock
{
	textureHandle ColorTexture;
	textureHandle LuminanceTexture;
	textureHandle BloomTexture;
};


samplerstate UpscaleSampler
{
	//Samplers = { BloomTexture, GodrayTexture, LuminanceTexture };
	AddressU = Border;
	AddressV = Border;
	BorderColor = {0, 0, 0, 0};
};

samplerstate DefaultSampler
{
	//Samplers = { ColorTexture, DepthTexture, ShapeTexture};
	Filter = Point;
	AddressU = Border;
	AddressV = Border;
	BorderColor = {0, 0, 0, 0};
};

state FinalizeState
{
	CullMode = Back;
	DepthEnabled = false;
	DepthWrite = false;
};

// depth of field samples
#define MAXDOFSAMPLES 23

const vec2 DofSamples[MAXDOFSAMPLES] = {
        vec2( 0.0, 0.0 ),
        vec2( -0.326212, -0.40581  ),
        vec2( -0.840144, -0.07358  ),
        vec2( -0.695914,  0.457137 ),
        vec2( -0.203345,  0.620716 ),
        vec2(  0.96234,  -0.194983 ),
        vec2(  0.473434, -0.480026 ),
        vec2(  0.519456,  0.767022 ),
        vec2(  0.185461, -0.893124 ),
        vec2(  0.507431,  0.064425 ),
        vec2(  0.89642,   0.412458 ),
        vec2( -0.32194,   0.93261f ),
        vec2(  0.326212,  0.40581  ),
        vec2(  0.840144,  0.07358  ),
        vec2(  0.695914, -0.457137 ),
        vec2(  0.203345, -0.620716 ),
        vec2( -0.96234,   0.194983 ),
        vec2( -0.473434,  0.480026 ),
        vec2( -0.519456, -0.767022 ),
        vec2( -0.185461,  0.893124 ),
        vec2( -0.507431, -0.064425 ),
        vec2( -0.89642,  -0.412458 ),
        vec2(  0.32194,  -0.93261f )
};   

//------------------------------------------------------------------------------
/**
*/
shader
void
vsMain(
	[slot=0] in vec3 position,
	[slot=2] in vec2 uv,
	out vec2 UV) 
{
	gl_Position = vec4(position, 1);
	UV = uv;
}

//------------------------------------------------------------------------------
/**
	Get a depth-of-field blurred sample. Set all values to 0 in order to disable DoF
*/
vec4 
psDepthOfField(float depth, vec2 uv)
{
    // compute focus-blur param (0 -> no blur, 1 -> max blur)
    float focusDist = DoFDistances.x;
    float focusLength = DoFDistances.y;
    float filterRadius = DoFDistances.z;    
    float focus = saturate(abs(depth - focusDist) / focusLength);
    
    // perform a gaussian blur around uv
    vec4 sampleColor = vec4(0.0f);
    float dofWeight = 1.0f / MAXDOFSAMPLES;
	vec2 pixelSize = RenderTargetDimensions[0].zw;
    vec2 uvMul = focus * filterRadius * pixelSize.xy;
    int i;
    for (i = 0; i < MAXDOFSAMPLES; i++)
    {
        sampleColor += sample2DLod(ColorTexture, DefaultSampler, uv + (DofSamples[i] * uvMul), 0);
    }
    sampleColor *= dofWeight;
    return sampleColor;
} 

//------------------------------------------------------------------------------
/**
*/
shader
void
psMain(in vec2 UV,
	[color0] out vec4 color) 
{
    // get an averaged depth value        
    float depth = subpassLoad(InputAttachments[0]).r;
	
	vec4 c;
	c = psDepthOfField(depth, UV);	
	
	// Get the calculated average luminance 
	float fLumAvg = sample2DLod(LuminanceTexture, UpscaleSampler, vec2(0.5f, 0.5f), 0).r;
	
	c = ToneMap(c, vec4(fLumAvg), MaxLuminance);
	vec4 bloom = DecodeHDR(sample2DLod(BloomTexture, UpscaleSampler, UV, 0));
	vec4 godray = subpassLoad(InputAttachments[1]);
	vec4 shape = subpassLoad(InputAttachments[2]);
	c += bloom;   
	c.rgb += godray.rgb;
	//c.rgb += godray.rgb;
	vec4 grey = vec4(dot(c.xyz, Luminance.xyz));
	c = Balance * lerp(grey, c, Saturation);
	c.rgb *= FadeValue;
	c.rgb = lerp(c.rgb, shape.rgb, shape.a);
	color = c;	
}

//------------------------------------------------------------------------------
/**
*/
PostEffect(vsMain(), psMain(), FinalizeState);
