#include "hybrid_common.hlsl"

cbuffer cbPerObject : register(b0, space0) {
  float4x4 WVP;
  float4x4 World;
};

ConstantBuffer<PerMaterialConstBuffer> g_per_material : register(b0, space1);

//ConstantBuffer<PerRenderConstBuffer> g_per_render : register(b0, space2);

struct VertexData {
  float4 pos : POSITION;
  float4 color : COLOR;
  float3 normal : NORMAL;
};

struct RasterAttr {
  float4 pos : SV_POSITION;
  float3 pos_w : POSITION1;
  float4 color : COLOR;
  float3 w_normal : NORMAL;
};

RasterAttr VS(VertexData vert) {
  RasterAttr output;

  output.pos= mul(WVP, vert.pos);
  output.color = vert.color;
  output.w_normal = mul(World, float4(vert.normal.xyz, 0)).xyz;
  float4 pos_w = mul(World, vert.pos);
  output.pos_w = pos_w.xyz / pos_w.w;

  return output;
}

GBufferPixel PS(RasterAttr input) {
  return encode_gbuffer(g_per_material, input.w_normal);
}
