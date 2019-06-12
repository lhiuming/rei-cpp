// Light struct 
struct Light
{
    float3 dir;
    float4 ambient;
    float4 diffuse;
};

// constant buffer to hold the projection transforming matrix  
cbuffer cbPerObject
{
    float4x4 WVP;
    float4x4 World;
};
cbuffer cbPerFrame
{
    Light light;
};

// Yes, you actually need to define this truct 
struct VS_OUTPUT
{
    float4 Pos : SV_POSITION; // TODO: what re these two modifier?
    float4 Color : COLOR;
    float3 Normal : NORMAL;
};

VS_OUTPUT VS(float4 inPos : POSITION, float4 inColor : COLOR, float3 normal : NORMAL)
{
    VS_OUTPUT output;

    output.Pos = mul(inPos, WVP);
    output.Color = inColor;
    output.Normal = mul(normal, World);

    return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{
    float4 diffuse = input.Color;

    float tint = dot(normalize(light.dir), normalize(input.Normal));

    if (tint > 0.5)
        tint = 1.0;
    else if (tint < 0.2)
        tint = 0.0;
    else
        tint = 0.4;
    
    float3 finalColor = diffuse * (tint * light.diffuse + light.ambient);

    return float4(finalColor, diffuse.a); // take the alpha channel of object 
}