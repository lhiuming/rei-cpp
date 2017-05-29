// Yes, you actually need to define this truct 
struct VS_OUTPUT
{
    float4 Pos : SV_POSITION; // TODO: what re these two modifier?
    float4 Color : COLOR;
};

// constant buffer to hold the projection transforming matrix  
cbuffer cbPerObject
{
    float4x4 WVP;
};

VS_OUTPUT VS(float4 inPos : POSITION, float4 inColor : COLOR)
{
  VS_OUTPUT output;

  output.Pos = mul(inPos, WVP);
  output.Color = inColor;

  return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{
  return input.Color;
}