#include "Common.hlsl"


// VertexBuffer struct
struct STriVertex
{
    float3 vertex;
    float3 normal;
    float2 uv;
}; 

StructuredBuffer<STriVertex> BTriVertex : register(t0); // Vertex buffer
StructuredBuffer<int> indices : register(t1); //Index buffer

Texture2D<float4> photonMap : register(t2); // Output texture
SamplerState samLinear : register(s0); // Sampler state

[shader("closesthit")]
void MeshClosestHit(inout HitInfo payload, Attributes attrib)
{
    float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

    uint vertId = 3 * PrimitiveIndex();
    //float3 hitColor = BTriVertex[indices[vertId + 0]].normal * barycentrics.x +
    //                BTriVertex[indices[vertId + 1]].normal * barycentrics.y +
    //                BTriVertex[indices[vertId + 2]].normal * barycentrics.z;

    
    float2 uv = BTriVertex[indices[vertId + 0]].uv * barycentrics.x +
                    BTriVertex[indices[vertId + 1]].uv * barycentrics.y +
                    BTriVertex[indices[vertId + 2]].uv * barycentrics.z;
    
    //float3 hitColor = photonMap.Sample(samLinear, uv).xyz;
    
    payload.colorAndDistance = float4(uv,0, RayTCurrent());
}
 

