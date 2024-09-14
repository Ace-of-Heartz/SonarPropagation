#include "Common.hlsl"


// VertexBuffer struct
struct STriVertex
{
    float3 vertex;
    float3 normal;
    float2 uv;
}; 

//tbuffer Textures : register(t0)
//{
//    Texture2D <float4> texture0;
//};

StructuredBuffer<STriVertex> BTriVertex : register(t0); // Vertex buffer
StructuredBuffer<int> indices : register(t1); //Index buffer
//RaytracingAccelerationStructure SceneBVH : register(t2);


[shader("closesthit")]
void MeshClosestHit(inout HitInfo payload, Attributes attrib)
{
    float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

    payload.colorAndDistance = float4(barycentrics, RayTCurrent());
}



