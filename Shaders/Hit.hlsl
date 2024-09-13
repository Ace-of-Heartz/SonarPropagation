#include "Common.hlsl"

Texture2D <float4> texture0 : register(t0);


// VertexBuffer struct
struct STriVertex
{
    float3 vertex;
    float3 normal;
    float2 uv;
}; 


struct ShadowHitInfo
{
    bool isHit;
};

struct ReflectionHitInfo
{
    float4 color;
};

// Constant buffer for individual instance colors
cbuffer Colors : register(b0)
{
    float3 R;
    float3 G;
    float3 B;
}

tbuffer Textures : register(t0)
{
    Texture2D <float4> texture0;
};

StructuredBuffer<STriVertex> BTriVertex : register(t0); // Vertex buffer
StructuredBuffer<int> indices : register(t1); //Index buffer
RaytracingAccelerationStructure SceneBVH : register(t2);


[shader("closesthit")]
void ClosestHit(inout HitInfo payload, Attributes attrib)
{
    float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

    float3 hitColor = R * barycentrics.x + G * barycentrics.y + B * barycentrics.z;

    payload.colorAndDistance = float4(hitColor, RayTCurrent());
}

[shader("closesthit")]
void QuadClosestHit(inout HitInfo payload, Attributes attrib)
{
    float3 lightPos = float3(2, 6, -2); // An arbitrary point light in space

    float3 worldOrigin = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();

    float3 lightDir = normalize(lightPos - worldOrigin);

    RayDesc shadowRay;
    shadowRay.Origin = worldOrigin;
    shadowRay.Direction = lightDir;
    shadowRay.TMin = 0.01;
    shadowRay.TMax = 10000;

    ShadowHitInfo shadowPayload;

    shadowPayload.isHit = false;

    TraceRay(
    SceneBVH,
    RAY_FLAG_NONE,
    0xFF, // No mask
    1,
    0,
    1,
    shadowRay,
    shadowPayload
  );

    float factor = shadowPayload.isHit ? 0.3 : 1.0;
  
    float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

    float3 hitColor = R * barycentrics.x + G * barycentrics.y + B * barycentrics.z;

    payload.colorAndDistance = float4(hitColor * factor, RayTCurrent());
}

[shader("closesthit")]
void QuadReflectionClosestHit(inout HitInfo payload, Attributes attrib)
{
    float3 normal = float3(0.f, 1.0f, 0.f);

    float3 reflectionDir = normalize(reflect(WorldRayDirection().xyz, normal));

    float3 worldOrigin = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();

    RayDesc reflectionRay;
    reflectionRay.Origin = worldOrigin;
    reflectionRay.Direction = reflectionDir;
    reflectionRay.TMin = 0.01;
    reflectionRay.TMax = 10000;

    ReflectionHitInfo reflectionPayload;

    float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);
    float3 hitColor = R * barycentrics.x + G * barycentrics.y + B * barycentrics.z;

    reflectionPayload.color = float4(hitColor, 1.0f);

    TraceRay(
  SceneBVH,
  RAY_FLAG_NONE,
  0xFF,
  0,
  0,
  1,
  reflectionRay,
  reflectionPayload
  );

    float3 refl = reflectionPayload.color.xyz;

    payload.colorAndDistance = float4(refl, RayTCurrent());
}