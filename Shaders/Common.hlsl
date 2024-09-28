// Hit information, aka ray payload
// This sample only carries a shading color and hit distance.
// Note that the payload should be kept as small as possible,
// and that its size must be declared in the corresponding
// D3D12_RAYTRACING_SHADER_CONFIG pipeline subobjet.
struct HitInfo
{
  float4 colorAndDistance;
};


// Attributes output by the raytracing when hitting a surface,
// here the barycentric coordinates
struct Attributes
{
  float2 bary;
};

struct STriVertex
{
    float4 vertex;
    float4 normal;
    //float2 uv;
};

float2 GetUV(float3 bary, STriVertex a, STriVertex b, STriVertex c)
{
    float u = a.vertex.w * bary.x +
              b.vertex.w * bary.y +
              c.vertex.w * bary.z;
    
    float v = a.normal.w * bary.x +
              b.normal.w * bary.y +
              c.normal.w * bary.z;
    
    return float2(u, v);
}

float3 GetNormal(float3 bary, STriVertex a, STriVertex b, STriVertex c)
{
    float3 normal = a.normal.xyz * bary.x +
                    b.normal.xyz * bary.y +
                    c.normal.xyz * bary.z;
    
    return normal;
}

float3 GetPosition(float3 bary, STriVertex a, STriVertex b, STriVertex c)
{
    float3 position = a.vertex.xyz * bary.x +
                      b.vertex.xyz * bary.y +
                      c.vertex.xyz * bary.z;
    
    return position;
}