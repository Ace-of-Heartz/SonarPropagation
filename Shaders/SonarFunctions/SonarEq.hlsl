#include "SonarUtils.hlsl"

//-------------------------------------------------------
// Mackenzie Formulas:
float MackenzieFormula(EnvData data)
{
    float t2 = pow(data.temperature,2);
    
    return  1449.05 +
            4.6 * data.temperature - 
            0.055 * t2 +
            0.00029 * t2 * data.temperature +
            0.016 * data.depth + 
            (1.34 - 0.01025 * data.temperature) * (data.salinity - 35) + 
            1.675 * pow(10,-7) * data.depth - 
            7.139 * pow(10,-13) * data.temperature * data.depth;
}

float MFPartialOfR(EnvData data)
{
    return 0.0;
}

float MFPartialOfZ(EnvData data)
{
    return 0.016 +
            3.350 * pow(10, -7) * data.depth - 
            7.139 * pow(10, -13) * data.temperature * 3.0 * pow(data.depth, 2);
}

//-------------------------------------------------------

//-------------------------------------------------------
// Compact Mackenzie Formulas:
float CompactMackenzieFormula(EnvData data)
{
    float t2 = data.temperature * data.temperature;
    
    return  1448.96 +
            4.591 * data.temperature -
            0.05304 * t2 +
            0.0002374 * t2 * data.temperature +
            0.016 * data.depth;
}

float CMFPartialOfR(EnvData data)
{
    return 0.0;
}

float CMFPartialOfZ(EnvData data)
{
    return 0.016;
}
//-------------------------------------------------------

//-------------------------------------------------------
// Coppens Formulas:
//-------------------------------------------------------

RayData InitRay(float r, float z, float theta)
{
    EnvData envData = { 2.0, 34.7, z };
    
    float c = MackenzieFormula(envData);
    
    float xi = cos(theta) / c;
    float zeta = sin(theta) / c;
    
    RayData ray = { r, z, xi, zeta };
    return ray;
}

// Differential Equation used for computing the path of a ray
RayData SonarDiffEq(RayData ray)
{
    EnvData envData = { 2.0, 34.7, ray.z };
    
    float c = MackenzieFormula(envData);
    float c_r = MFPartialOfR(envData);
    float c_z = MFPartialOfZ(envData);
    
    float x = -1.0 / pow(c, 2);
    
    float newR = ray.r * ray.xi;
    float newZ = ray.z * ray.zeta;
    float newXi = x * c_r;
    float newZeta = x * c_z;
    
    RayData newRay = { newR, newZ, newXi, newZeta };
    
    return newRay;
}

RayData CartesianToCylinder(RayMarchInput coord)
{
    float3 p = coord.rayOrigin + coord.rayDirection * coord.distance;
    
    float r = sqrt(pow(p.x, 2) + pow(p.y, 2));
    float z = p.z;
    
    float theta = atan2(p.y, p.x);

    return InitRay(r, z, theta);
}

float3 CylinderToCartesian(RayData coord)
{
    return float3(
        cos(coord.r),
        sin(coord.r),
        coord.z
    );
}