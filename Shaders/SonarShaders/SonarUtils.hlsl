struct env_data
{
    float depth;
    float temperature;
    float salinity;
};

struct ray_march_input
{
    float4 rayOrigin;
    float4 rayDirection;
    float distance;
};

struct ray_march_output
{
    float3 rayOrigin;
    float3 rayDirection;
	float distance;
};

struct ray_data
{
    float r;
    float z;
    float xi;
    float zeta;
};

ray_data sum_ray_data(ray_data r1, ray_data r2)
{
    ray_data res;
    res.r = r1.r + r2.r;
    res.z = r1.z + r2.z;
    res.xi = r1.xi + r2.xi;
    res.zeta = r1.zeta + r2.zeta;

    return res;
}