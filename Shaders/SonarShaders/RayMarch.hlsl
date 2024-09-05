#include "SonarEq.hlsl"

void RK45()
{

}

struct RayMarchData
{
    float3 rayOrigin;
    float3 rayDirection;
    float distance;
};

void RayMarch(RayMarchData data)
{
    for (int i = 0; i < 100000; ++i)
    {
           
    }
}

void ComputeNextRay(RayData data)
{
    int m = 5;
    int n = 4;
    
    
    
    SonarDiffEq(data);
    
    RayData ks[];
    
    for (int i = 2; i <= m; ++i) 
    {
        ks[i] = data; //TODO: Change from struct
        
        for (int j = 1; j <= i-1; ++j)
        {
            ks[i] = ks[j] * coMatrix[i][j]; // TODO: Declare comatrix
        }
    }

    
    
}