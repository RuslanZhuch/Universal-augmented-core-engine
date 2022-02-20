cbuffer cbPerObject : register(b0)
{
    float4x4 worldViewProj;
};

struct VertexIn
{
    float3 posLoc : POSITION;
    float4 color : COLOR;
};

struct VertexOut
{
    float4 posH : SV_POSITION;
    float4 color : COLOR;
};

VertexOut main(in VertexIn vIn)
{
	
    VertexOut oData;

    oData.posH = mul(float4(vIn.posLoc, 1.f), worldViewProj);
    oData.color = vIn.color;

    return oData;

}