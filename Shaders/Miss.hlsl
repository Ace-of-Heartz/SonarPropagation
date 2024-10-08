#include "Common.hlsl"

[shader("miss")]
void MeshMiss(inout HitInfo payload : SV_RayPayload)
{
    uint2 launchIndex = DispatchRaysIndex().xy;
    float2 dims = float2(DispatchRaysDimensions().xy);

    float ramp = launchIndex.y / dims.y;
    payload.colorAndDistance = float4(0.0f, 0.8f*ramp, 0.7f - 0.3f*ramp, -1.0f);
}