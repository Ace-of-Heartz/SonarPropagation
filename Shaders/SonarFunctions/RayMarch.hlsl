#include "SonarEq.hlsl"

cbuffer RKData : register(b1)
{
    float4 row_col_size_TEMP;
    float4 data;
}

RayData ComputeNextRay(RayData data)
{
    int m = 4;
    int n = 3;
    
    float mtx[3 * 4] = 
    {
        0.5,0.0,0.0,
        0.0,0.5,0.0,
        0.0,0.0,0.1,
    };
    
    float w[4] =
    {
        1/6, 1/3, 1/3, 1/6  
    };
    
    SonarDiffEq(data);
    
    RayData ks[4];
    
    for (int i = 1; i < m; ++i) 
    {
        ks[i] = data; 
        
        for (int j = 0; j < i-1; ++j)
        {
            RayData res = { ks[j].r * mtx[i * n + j],
                ks[j].z * mtx[i * n + j],
                ks[j].xi * mtx[i * n + j],
                ks[j].zeta * mtx[i * n + j]
            };
            ks[i] = res;
            //ks[i] = ks[j] * mtx[i * n + j]; // TODO: Declare comatrix
        }
    }
    
    RayData newU = data;
    
    for (int k = 0; k < m; ++k)
    {
        RayData newU =
        {
            data.r    + ks[k].r    * w[k],
            data.z    + ks[k].z    * w[k],
            data.xi   + ks[k].xi   * w[k],
            data.zeta + ks[k].zeta * w[k]
        };
    }
    return newU;
}

RayMarchOutput RayMarch(RayMarchInput data)
{
    float ARBITRARY_DEPTH = 6800.0;
    float ARBITRARY_OFFSET = 10.0;
    
    RayData u = CartesianToCylinder(data);
    
    for (int i = 0; i < 100000; ++i)
    {
        RayData u2 = ComputeNextRay(u);
        
        float3 p = CylinderToCartesian(u);
        // TODO: Check for proximity to other objects
        // Check for boundaries, 
        // if distance is smaller than a certain threshold, break
        if (p.z < 0.0 || p.z < ARBITRARY_DEPTH)
        {
            break;
        }
        
        u = u2;
        
        if (p.z <= ARBITRARY_OFFSET || p.z < ARBITRARY_DEPTH - ARBITRARY_OFFSET)
        {
            break;
        }
    }
    float3 ro = CylinderToCartesian(u);
    float3 rd = normalize(float3(u.xi, u.zeta, data.rayDirection.z));
     
    RayMarchOutput res = { ro, rd };
    
    return res;
}