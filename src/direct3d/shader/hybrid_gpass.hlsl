#include "hybrid_common.hlsl"

// constant buffer to hold the projection transforming matrix
cbuffer cbPerObject : register(b0, space0) {
  float4x4 WVP;
  float4x4 World;
};

ConstantBuffer<PerRenderConstBuffer> g_per_render : register(b0, space1);

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

struct GBufferPixel {
  float4 normal : SV_TARGET0;
  float4 albedo_smoothness : SV_TARGET1;
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
  GBufferPixel rt;
  rt.normal.xyz = normalize(input.w_normal);
  rt.albedo_smoothness.xyz = input.color.xyz;
  rt.albedo_smoothness.w = 0.5f;
  return rt;
}
