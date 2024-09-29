#include "../Common.hlsl"
#include "SonarCommon.hlsl"

StructuredBuffer<STriVertex> BTriVertex : register(t0); // Vertex buffer
StructuredBuffer<int> indices : register(t1); //Index buffer

RWTexture2D<float4> photonMap : register(t2); // Output texture
SamplerState samLinear : register(s0); // Sampler state

[shader("closesthit")]
void ObjectClosestHit(inout SoundHitInfo hit,Attributes attrib)
{


}

[shader("closesthit")]
void BoundaryClosestHit(inout SoundHitInfo hit, Attributes attrib)
{
    //TODO: Reflect
}

void GetBoundaryNormal(inout SoundHitInfo hit)
{
       
}