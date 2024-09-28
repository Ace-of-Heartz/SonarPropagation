#include"SonarCommon.hlsl"

[shader("miss")]
void ObjectMiss(inout SoundHitInfo hit : SV_RayPayload)
{
    hit.colorAndDistance = float4(0.0f,0.0f,1.0f,-1.0f);
}


[shader("miss")]
void BoundaryMiss(inout SoundHitInfo hit : SV_RayPayload)
{
    hit.colorAndDistance = float4(0.0f, 1.0f, 1.0f, -1.0f);
    
}