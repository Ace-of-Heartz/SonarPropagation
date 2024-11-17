#include "../Common.hlsl"
#include "SonarCommon.hlsl"
#include "RayMarch.hlsl"

StructuredBuffer<STriVertex> BTriVertex : register(t0); // Vertex buffer
StructuredBuffer<int> indices : register(t1); //Index buffer

//RWTexture2D<float4> photonMap : register(u0); // Output texture
//SamplerState samLinear : register(s0); // Sampler state
RaytracingAccelerationStructure SceneBVH : register(t2);

[shader("closesthit")]
void ObjectClosestHit(inout SoundHitInfo hit,Attributes attrib)
{
    float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);
        
    uint vertId = 3 * PrimitiveIndex();
    STriVertex a = BTriVertex[indices[vertId + 0]];
    STriVertex b = BTriVertex[indices[vertId + 1]];
    STriVertex c = BTriVertex[indices[vertId + 2]];
    
    //TODO: UV mapping
	float2 uv = GetUV(barycentrics, a, b, c);

    hit.isObjectHit = true;
    hit.uv = uv;
}

[shader("closesthit")]
void BoundaryClosestHit(inout SoundHitInfo hit, Attributes attrib)
{
    float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);
    
    uint vertId = 3 * PrimitiveIndex();
    STriVertex a = BTriVertex[indices[vertId + 0]];
    STriVertex b = BTriVertex[indices[vertId + 1]];
    STriVertex c = BTriVertex[indices[vertId + 2]];

    float3 normal = GetNormal(barycentrics, a, b, c);

    float3 rayDirection = WorldRayDirection();

	float3 nextRayDirection = reflect(rayDirection, normal);

    ray_march_input inputRay; 
    inputRay.rayDirection = float4(nextRayDirection,0.);
    inputRay.rayOrigin = float4(WorldRayOrigin(), 0.);
    inputRay.distance = 0.0;
    
    ray_march_output outputRay = RayMarch(inputRay);
    
    RayDesc nextRay;
    nextRay.Origin = outputRay.rayOrigin.xyz;
    nextRay.Direction = outputRay.rayDirection.xyz;
	nextRay.TMin = 0.001f;
    nextRay.TMax = 1000000.0f;

    TraceRay(
		SceneBVH,
		RAY_FLAG_NONE,
		0xFF,
		0,
		0,
		0,
		nextRay,
		hit
    );
}

