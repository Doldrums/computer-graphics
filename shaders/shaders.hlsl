cbuffer ConstantBuffer: register(b0)
{
    float4x4 mwpMatrix;
}
struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 world_position : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(float4 position : POSITION, float4 normal: NORMAL, float4 ambient : COLOR0, float4 diffuse : COLOR1,  float4 emissive : COLOR2)
{
    PSInput result;
    result.position = mul(mwpMatrix, position);
    result.world_position = position;
    result.color = ambient;
    
    return result;
}

float GetLambertIntensity(PSInput input)
{
    float3 to_light = normalize(float3(1.f, 1.f, 1.f) - input.world_position);
    return saturate(dot(input.normal, to_light));
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color * GetLambertIntensity(input);
}
