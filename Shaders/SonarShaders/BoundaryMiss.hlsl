#include "SonarCommon.hlsl"

[shader("miss")]
void BoundaryMiss(inout SonarHitInfo hit : SV_RayPayload)
{
    hit.colorAndDistance = float4(0.0f, 1.0f, 1.0f, -1.0f);
}