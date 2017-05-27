float4 VS(float4 inPos : POSITION) : SV_POSITION
{
  return inPos;
}

float4 PS() : SV_TARGET
{
  return float4(0.9f, 0.4f, 0.1f, 1.0f);
}