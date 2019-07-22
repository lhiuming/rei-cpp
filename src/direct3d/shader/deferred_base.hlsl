// constant buffer to hold the projection transforming matrix
cbuffer cbPerObject : register(b0, space0) {
  float4x4 WVP;
  float4x4 World;
};
cbuffer cbPerFrame : register(b0, space1) {
  //Light light;
  float4x4 camera_world_trans;
  float4 camera_pos;
};

// Yes, you actually need to define this truct
struct VS_OUTPUT {
  float4 Pos : SV_POSITION; // TODO: what re these two modifier?
  float3 pos_w : POSITION1;
  float4 Color : COLOR;
  float3 Normal : NORMAL;
};

VS_OUTPUT VS(float4 inPos : POSITION, float4 inColor : COLOR, float3 normal : NORMAL) {
  VS_OUTPUT output;

  output.Pos = mul(WVP, inPos);
  output.Color = inColor;
  output.Normal = mul(World, float4(normal.xyz, 0)).xyz;
  float4 pos_w = mul(World, inPos);
  output.pos_w = pos_w.xyz / pos_w.w;

  return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET {
  return float4(0, 0, 0, 0);
}
