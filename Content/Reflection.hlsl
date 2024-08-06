
struct ReflectionHitInfo
{
    float4 color; 
};

struct Attributes
{
    float2 uv;
};

[shader("closesthit")]
void ReflectionClosestHit(inout ReflectionHitInfo hit, Attributes attrib)
{
    hit.color = float4(0.0f,1.0f,0.0f,1.0f);
}

[shader("miss")]
void ReflectionMiss(inout ReflectionHitInfo hit : SV_RayPayload)
{
    //hit.color = float4(0.0f,0.0f,1.0f,1.0f);
}