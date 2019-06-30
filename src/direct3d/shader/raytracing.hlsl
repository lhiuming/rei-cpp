struct RayGenConstantBuffer {
  float4 viewport; // top, left, bottom, right
};

RaytracingAccelerationStructure scene: register(t0, space0);
RWTexture2D<float4> render_target: register(u0);
ConstantBuffer<RayGenConstantBuffer> raygen_cb: register(b0);

typedef BuiltInTriangleIntersectionAttributes HitAttributes;

struct RayPayload {
  float4 color;
};

[shader("raygeneration")] 
void raygen_shader() {
  float2 lerpValues = (float2)DispatchRaysIndex() / (float2)DispatchRaysDimensions();
  float3 ray_dir = float3(0, 0, 1);
  float4 vp = raygen_cb.viewport;
  float3 origin = float3(lerp(vp.y, vp.w, lerpValues.x), lerp(vp.x, vp.z, lerpValues.y), 0.0f);

  RayDesc ray;
  ray.Origin = origin;
  ray.Direction = ray_dir;
  ray.TMin = 0.001;
  ray.TMax = 10000.0;
  RayPayload payload = {{0, 0, 0, 0}};
  TraceRay(scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, payload);

  render_target[DispatchRaysIndex().xy] = payload.color;
}

[shader("closesthit")] 
void closest_hit_shader(inout RayPayload payload, in HitAttributes attr) {
  float3 barycentrics = float3(
    1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
  float3 c0 = {1.4, 1.4, 1.4}; // overflow
  float3 c1 = {.6, .4, .4};
  float3 c2 = {.4, .4, .4};
  float3x3 color_matrix = {c0, c1, c2};
  float3 mixed = mul(barycentrics, color_matrix);
  payload.color = float4(mixed, 1);
}

[shader("miss")] 
void miss_shader(inout RayPayload payload) {
  float3 ayanami_blue = {129.0 / 255, 187.0 / 255, 235.0 / 255};
  float3 asuka_red = {156.0 / 255, 0, 0};
  float3 origin = WorldRayOrigin();
  float mixing = clamp(origin.y + 1, 0, 2) / 2;
  mixing = pow(mixing, 3);
  payload.color = float4(lerp(ayanami_blue, asuka_red, mixing), 1.0);
}
