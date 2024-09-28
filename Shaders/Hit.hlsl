#include "Common.hlsl"


// VertexBuffer struct
//struct STriVertex
//{
//    float4 vertex;
//    float4 normal;
//    //float2 uv;
//}; 

StructuredBuffer<STriVertex> BTriVertex : register(t0); // Vertex buffer
StructuredBuffer<int> indices : register(t1); //Index buffer

Texture2D<float4> photonMap : register(t2); // Output texture
SamplerState samLinear : register(s0); // Sampler state

[shader("closesthit")]
void MeshClosestHit(inout HitInfo payload, Attributes attrib)
{
    float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);
    
    uint vertId = 3 * PrimitiveIndex();
    STriVertex a = BTriVertex[indices[vertId + 0]];
    STriVertex b = BTriVertex[indices[vertId + 1]];
    STriVertex c = BTriVertex[indices[vertId + 2]];

    float2 uv = GetUV(barycentrics, a, b, c);

    payload.colorAndDistance = float4(uv,0.0, RayTCurrent());
}
 

