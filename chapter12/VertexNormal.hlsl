struct VertexIn
{
	float3 PosL    : POSITION;
	float3 NormalL : NORMAL;
	float2 TexC    : TEXCOORD;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
	float3 PosW    : POSITION;
	float3 NormalW : NORMAL;
	float2 TexC    : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	// Transform to world space.
	float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
	vout.PosW = posW.xyz;

	// Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
	vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);

	// Transform to homogeneous clip space.
	vout.PosH = mul(posW, gViewProj);

	// Output vertex attributes for interpolation across triangle.
	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
	vout.TexC = mul(texC, gMatTransform).xy;

	return vout;
}

[maxvertexcount(10)]
void GS(point VertexOut v[1], inout LineStream<VertexOut> lstream)
{
	VertexOut vout;

	vout.PosW = v[0].PosW + v[0].NormalW * 2;
	vout.PosH = mul(float4(vout.PosW, 1.0f), gViewProj);
	vout.NormalW = v[0].NormalW;
	vout.TexC = v[0].TexC;
	lstream.Append(v[0]);
	lstream.Append(vout);
}

float4 PS(VertexOut pin) :SV_Target
{
	return float4(1.0f, 0.0f, 0.0f, 1.0f);
}
