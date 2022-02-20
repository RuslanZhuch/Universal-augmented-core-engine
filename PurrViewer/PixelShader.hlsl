
struct PSIn
{
    float4 posH : SV_POSITION;
    float4 color : COLOR;
};

float4 main(in PSIn pIn) : SV_TARGET
{
    const float4 tex = pIn.color;
    return tex;
}