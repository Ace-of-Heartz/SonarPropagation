#include "SonarUtils.hlsl"

//-------------------------------------------------------
// Mackenzie Formulas:
float mackenzie_formula(env_data data)
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

float mf_partial_of_r(env_data data)
{
    return 0.0;
}

float mf_partial_of_z(env_data data)
{
    return 0.016 +
            3.350 * pow(10, -7) * data.depth - 
            7.139 * pow(10, -13) * data.temperature * 3.0 * pow(data.depth, 2);
}

//-------------------------------------------------------

//-------------------------------------------------------
// Compact Mackenzie Formulas:
float compact_mackenzie_formula(env_data data)
{
    float t2 = data.temperature * data.temperature;
    
    return  1448.96 +
            4.591 * data.temperature -
            0.05304 * t2 +
            0.0002374 * t2 * data.temperature +
            0.016 * data.depth;
}

float cmf_partial_of_r(env_data data)
{
    return 0.0;
}

float cmf_partial_of_z(env_data data)
{
    return 0.016;
}
//-------------------------------------------------------

//-------------------------------------------------------
// Coppens Formulas:
//-------------------------------------------------------

ray_data init_ray(float r, float z, float theta)
{
    env_data envData = { 2.0, 34.7, z };
    
    float c = mackenzie_formula(envData);
    
    float xi = cos(theta) / c;
    float zeta = sin(theta) / c;
    
    ray_data ray;
    ray.r = r;
    ray.z = z;
    ray.xi = xi;
    ray.zeta = zeta;
    return ray;
}

// Differential Equation used for computing the path of a ray
ray_data sonar_diff_eq(ray_data ray)
{
    env_data envData;
    envData.depth = ray.z;
    envData.temperature = 34.7;
	envData.salinity = 2.0;

    
    float c = mackenzie_formula(envData);
    float c_r = mf_partial_of_r(envData);
    float c_z = mf_partial_of_z(envData);
    
    float x = -1.0 / pow(c, 2);
    
    float newR = ray.r * ray.xi;
    float newZ = ray.z * ray.zeta;
    float newXi = x * c_r;
    float newZeta = x * c_z;
    
    ray_data newRay;
    newRay.r = newR;
    newRay.z = newZ;
    newRay.xi = newXi;
    newRay.zeta = newZeta;

    return newRay;
}

ray_data input_to_data(ray_march_input coord)
{
    float3 p = coord.rayOrigin + coord.rayDirection * coord.distance;
    
    float r = sqrt(pow(p.x, 2) + pow(p.y, 2));
    float z = p.z;
    
    float theta = atan2(p.y, p.x);

    return init_ray(r, z, theta);
}

ray_march_output data_to_output(ray_data coord, float rayDirectionZ,float distance)
{
    float3 ro = (
        cos(coord.r),
        sin(coord.r),
        coord.z
    );
	float3 rd = normalize(float3(coord.xi, coord.zeta, rayDirectionZ));

    ray_march_output res;
	res.rayOrigin = ro;
    res.rayDirection = rd;
	res.distance = distance;

	return res;

}