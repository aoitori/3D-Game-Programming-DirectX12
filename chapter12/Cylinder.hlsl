#ifndef NUM_DIR_LIGHTS
	#define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
	#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
	#define NUM_SPOT_LIGHTS 0
#endif

//control clyinder
#ifndef NUM_SLICE_COUNT
	#define NUM_SLICE_COUNT 10
	#define ARRAY_AMOUNTS NUM_SLICE_COUNT * 2 + 2
#endif


// Include structures and functions for lighting.
#include "LightingUtil.hlsl"

Texture2DArray gTreeMapArray : register(t0);


SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld;
	float4x4 gTexTransform;
};

// Constant data that varies per material.
cbuffer cbPass : register(b1)
{
	float4x4 gView;
	float4x4 gInvView;
	float4x4 gProj;
	float4x4 gInvProj;
	float4x4 gViewProj;
	float4x4 gInvViewProj;
	float3 gEyePosW;
	float PI2;
	float2 gRenderTargetSize;
	float2 gInvRenderTargetSize;
	float gNearZ;
	float gFarZ;
	float gTotalTime;
	float gDeltaTime;
	float4 gAmbientLight;

	float4 gFogColor;
	float gFogStart;
	float gFogRange;
	float cbPerObjectPad2;

	// Indices [0, NUM_DIR_LIGHTS) are directional lights;
	// indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
	// indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
	// are spot lights for a maximum of MaxLights per object.
	Light gLights[MaxLights];
};

cbuffer cbMaterial : register(b2)
{
	float4   gDiffuseAlbedo;
	float3   gFresnelR0;
	float    gRoughness;
	float4x4 gMatTransform;
};

struct VertexIn
{
	float3 posL		: POSITION;
	float3 normalL	: NORMAL;
};

struct VertexOut {
	float3 posW		: POSITION;
	float3 normalW	: NORMAL;
};


struct GeoOut {
	float4 posH		: SV_POSITION;
	float3 posW		: POSITION;
	float3 normalW : NORMAL;

};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	vout.posW = vin.posL;
	vout.normalW = vin.normalL;

	return vout;
}


void subDivide(VertexOut vin[2], out VertexOut vout[ARRAY_AMOUNTS])
{
	float dTheta = PI2 / NUM_SLICE_COUNT;
	float r = distance(vin[1].posW, vin[0].posW) / 2;

	[unroll]
	for (int i = 0, j = 0; j < NUM_SLICE_COUNT; ++i, ++j) {
		float c = r * cos(j*dTheta);
		float s = r * sin(j*dTheta);
		vout[i].posW = float3(c, 0, s);
		vout[i + 1].normalW = vout[i].normalW = float3(0.0f, +1.0f, 0.0f);
		vout[i + 1].posW = float3(c, 1.0f, s);
		++i;
	}
	
	vout[ARRAY_AMOUNTS - 2] = vout[0];
	vout[ARRAY_AMOUNTS - 1] = vout[1];

}

void OutputSubDivision(VertexOut v[ARRAY_AMOUNTS], inout TriangleStream<GeoOut> triStream)
{
	GeoOut gout;
	[unroll]
	for (int i = 0; i < ARRAY_AMOUNTS; ++i) {
		gout.posW = mul(float4(v[i].posW, 1.0f), gWorld).xyz;
		gout.posH = mul(float4(gout.posW, 1.0f), gViewProj);
		gout.normalW = mul(v[i].normalW, (float3x3)gWorld);
		triStream.Append(gout);
	}
}

[maxvertexcount(ARRAY_AMOUNTS)]
void GS(line VertexOut vin[2], 
	inout TriangleStream<GeoOut> triStream) {
	
	VertexOut v[ARRAY_AMOUNTS];
	subDivide(vin, v);
	OutputSubDivision(v, triStream);
}

float4 PS(GeoOut pin) : SV_Target
{
	pin.normalW = normalize(pin.normalW);

	float3 toEyeW = gEyePosW - pin.posW;
	float distToEye = length(toEyeW);
	toEyeW /= distToEye;

	float4 ambient = gAmbientLight * gDiffuseAlbedo;

	const float shininess = 1.0f - gRoughness;
	Material mat = { gDiffuseAlbedo, gFresnelR0, shininess };
	float3 shadowFactor = 1.0f;
	float4 directLight = ComputeLighting(gLights, mat, pin.posW,
		pin.normalW, toEyeW, shadowFactor);

	float4 litColor = ambient + directLight;

	litColor.a = gDiffuseAlbedo.a;

	return litColor;
}
