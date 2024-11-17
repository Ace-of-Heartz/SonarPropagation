#include "Common.hlsl"

StructuredBuffer<STriVertex> BTriVertex : register(t0); // Vertex buffer
StructuredBuffer<int> indices : register(t1); //Index buffer

[shader("closesthit")]
void MeshClosestHit(inout HitInfo payload, Attributes attrib)
{
    float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);
    
    uint vertId = 3 * PrimitiveIndex();
    STriVertex a = BTriVertex[indices[vertId + 0]];
    STriVertex b = BTriVertex[indices[vertId + 1]];
    STriVertex c = BTriVertex[indices[vertId + 2]];

    float2 uv = GetUV(barycentrics, a, b, c);

    //payload.colorAndDistance = float4(hitColor, RayTCurrent());
    payload.colorAndDistance = float4(uv,0.0, RayTCurrent());
}
 

