#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif

// Include structures and functions for lighting.
#include "LightingUtil.hlsl"

Texture2D    gDiffuseMap : register(t0);


SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

// Constant data that varies per frame.
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
	float cbPerObjectPad1;
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
	float2 cbPerObjectPad2;

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
	float3 PosL    : POSITION;
	float3 NormalL : NORMAL;
	float2 TexC    : TEXCOORD;
};

struct VertexOut
{
	float3 PosL    : POSITION;
	float3 NormalL : NORMAL;
	float2 TexC    : TEXCOORD;
};

struct Geo
{
	float4 PosH    : SV_POSITION;
	float3 PosW    : POSITION;
	float3 NormalW : NORMAL;
	float2 TexC    : TEXCOORD;
};

void Subdivide(VertexOut vin[3], out VertexOut vout[6])
{
	VertexOut m[3];
	m[0].PosL = (vin[0].PosL + vin[1].PosL) * 0.5f;
	m[1].PosL = (vin[1].PosL + vin[2].PosL) * 0.5f;
	m[2].PosL = (vin[0].PosL + vin[2].PosL) * 0.5f;

	m[0].NormalL = normalize(m[0].PosL);
	m[1].NormalL = normalize(m[1].PosL);
	m[2].NormalL = normalize(m[2].PosL);

	m[0].TexC = (vin[0].TexC + vin[1].TexC) * 0.5f;
	m[1].TexC = (vin[1].TexC + vin[2].TexC) * 0.5f;
	m[2].TexC = (vin[0].TexC + vin[2].TexC) * 0.5f;

	vout[0] = vin[0];
	vout[1] = m[0];
	vout[2] = m[2];
	vout[3] = m[1];
	vout[4] = vin[2];
	vout[5] = vin[1];
}

void OutputSubdivide(VertexOut v[6], inout TriangleStream<Geo> triStream)
{
	Geo g[6];

	[unroll]
	for (int i = 0; i < 6; ++i)
	{
		g[i].PosW = mul(float4(v[i].PosL, 1.0f), gWorld).xyz;
		g[i].PosH = mul(float4(g[i].PosW, 1.0f), gViewProj);
		g[i].NormalW = mul(float4(v[i].NormalL, 1.0f), gWorld).xyz;
		g[i].TexC = v[i].TexC;
	}

	[unroll]
	for (int j = 0; j < 5; ++j)
	{
		triStream.Append(g[j]);
	}

	triStream.RestartStrip();
	triStream.Append(g[1]);
	triStream.Append(g[5]);
	triStream.Append(g[3]);
}

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	vout.PosL = vin.PosL;
	vout.NormalL = vin.NormalL;

	// Output vertex attributes for interpolation across triangle.
	vout.TexC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform).xy;

	return vout;
}

[maxvertexcount(32)]
void GS(triangle VertexOut gin[3], inout TriangleStream<Geo> triStream)
{
	float d = distance(gEyePosW, float3(0.0f, 0.0f, 0.0f));
	if (d < 50.0f) {
		VertexOut v[6];
		Subdivide(gin, v);
		
		VertexOut tri0[3] = { v[0], v[1], v[2] };
		VertexOut tri1[3] = { v[1], v[3], v[2] };
		VertexOut tri2[3] = { v[2], v[3], v[4] };
		VertexOut tri3[3] = { v[1], v[5], v[3] };

		Subdivide(tri0, v);
		OutputSubdivide(v, triStream);
		triStream.RestartStrip();

		Subdivide(tri1, v);
		OutputSubdivide(v, triStream);
		triStream.RestartStrip();

		Subdivide(tri2, v);
		OutputSubdivide(v, triStream);
		triStream.RestartStrip();

		Subdivide(tri3, v);
		OutputSubdivide(v, triStream);
	}
	else if (d < 80.0f){
		VertexOut v[6];
		Subdivide(gin, v);
		OutputSubdivide(v, triStream);
	}
	else {
		Geo gout[3];
		for (int i = 0; i < 3; ++i) {
			gout[i].PosW = mul(float4(gin[i].PosL, 1.0f), gWorld).xyz;
			gout[i].PosH = mul(float4(gout[i].PosW, 1.0f), gViewProj);
			gout[i].NormalW = mul(gin[i].NormalL, (float3x3)gWorld);
			gout[i].TexC = gin[i].TexC;

			triStream.Append(gout[i]);
		}
	}
}

float4 PS(Geo pin) : SV_Target
{
	float4 diffuseAlbedo = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC) * gDiffuseAlbedo;

#ifdef ALPHA_TEST
	// Discard pixel if texture alpha < 0.1.  We do this test as soon 
	// as possible in the shader so that we can potentially exit the
	// shader early, thereby skipping the rest of the shader code.
	clip(diffuseAlbedo.a - 0.1f);
#endif

	// Interpolating normal can unnormalize it, so renormalize it.
	pin.NormalW = normalize(pin.NormalW);

	// Vector from point being lit to eye. 
	float3 toEyeW = gEyePosW - pin.PosW;
	float distToEye = length(toEyeW);
	toEyeW /= distToEye; // normalize

						 // Light terms.
	float4 ambient = gAmbientLight*diffuseAlbedo;

	const float shininess = 1.0f - gRoughness;
	Material mat = { diffuseAlbedo, gFresnelR0, shininess };
	float3 shadowFactor = 1.0f;
	float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
		pin.NormalW, toEyeW, shadowFactor);

	float4 litColor = ambient + directLight;

#ifdef FOG
	float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
	litColor = lerp(litColor, gFogColor, fogAmount);
#endif

	// Common convention to take alpha from diffuse albedo.
	litColor.a = diffuseAlbedo.a;

	return litColor;
}
