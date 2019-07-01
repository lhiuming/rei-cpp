// Light struct
struct Light {
  float3 dir;
  float4 ambient;
  float4 diffuse;
};

// constant buffer to hold the projection transforming matrix
cbuffer cbPerObject : register(b0) {
  float4x4 WVP;
  float4x4 World;
};
cbuffer cbPerFrame : register(b1) {
  Light light;
  float4x4 camera_world_trans;
  float3 camera_pos;
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
  output.Normal = mul(World, normal);
  float4 pos_w = mul(World, inPos);
  output.pos_w = pos_w.xyz / pos_w.w;

  return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET {
  float3 light_v = float3(0, 1, -0.3); // light.dir
  float3 light_color = float3(0.3, 0.3, 0.3);

  float3 ambient_color = float3(0.1, 0.1, 0.1);

  float3 surface_n = normalize(input.Normal);

  // diffusive radiance
  float diff = dot(light_v, surface_n);
  if (diff > 0.5)
    diff = 0.7;
  else if (diff > 0.2)
    diff = 0.5;
  else
    diff = 0.0;
  float3 diff_irr = diff * light_color;

  // specular radiance
  float3 view_v = normalize(camera_pos - input.pos_w);
  float3 H = normalize(view_v + light_v);
  float spec = clamp(dot(H, surface_n), 0.0, 1.0);
  spec = pow(spec, 8);
  spec = step(0.5, spec);
  float3 spec_irr = spec * light_color;

  float3 main_light_irr = diff_irr + spec_irr;
  //float3 main_light_irr = diff_irr;
  float3 finalColor = input.Color * (main_light_irr+ ambient_color);

  return float4(finalColor, input.Color.a); // take the alpha channel of object
}

// Fake VS entrypoint to supress IDE error
float4 main() : SV_POSITION {
  return float4(1, 1, 0, 1);
}
