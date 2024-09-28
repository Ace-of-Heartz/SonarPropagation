#include "../Common.hlsl"
#include "SonarCommon.hlsl"

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