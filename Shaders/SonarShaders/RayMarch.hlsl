#include "SonarEq.hlsl"


ray_data ComputeNextRay(ray_data data, float h)
{
    int m = 4;
    int n = 3;
    
    float mtx[3 * 4] = 
    {
		0.0,0.0,0.0,
        0.5,0.0,0.0,
        0.0,0.5,0.0,
        0.0,0.0,1.0,
    };
    
    float w[4] =
    {
        1/6, 1/3, 1/3, 1/6  
    };
    
    ray_data du = sonar_diff_eq(data);
    
    ray_data ks[4];

	ks[0] = du;

    for (int i = 1; i < m; ++i) 
    {
        ks[i] = data; 
        
        for (int j = 0; j < i-1; ++j)
        {
            ray_data res;
            res.r = ks[j].r * mtx[i * n + j] * h;
            res.z = ks[j].z * mtx[i * n + j] * h;
            res.xi = ks[j].xi * mtx[i * n + j] * h;
            res.zeta = ks[j].zeta * mtx[i * n + j] * h;

            ks[i] = sum_ray_data(ks[i],res);
            //ks[i] += ks[j] * mtx[i * n + j];
        }
    }
    
    ray_data newU = data;
    
    for (int k = 0; k < m; ++k)
    {
        ray_data res;
		res.r = data.r + ks[k].r * w[k];
        res.z = data.z + ks[k].z * w[k];
        res.xi = data.xi + ks[k].xi * w[k];
        res.zeta = data.zeta + ks[k].zeta * w[k];

        newU = sum_ray_data(newU, res);
    }
    return newU;
}

ray_march_output RayMarch(ray_march_input data)
{
    float ARBITRARY_DEPTH = 6800.0;
    float ARBITRARY_OFFSET = 10.0;
	float ARBITRARY_H = 1.0;

    ray_data u = input_to_data(data);
    
    for (int i = 0; i < 100000; ++i)
    {
        ray_data u2 = ComputeNextRay(u,ARBITRARY_H);
        
        float3 p = data_to_output(u,data.rayDirection.z,data.distance).rayOrigin;
        // TODO: Check for proximity to other objects
        // Check for boundaries, if distance is smaller than a certain threshold, break
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
	ray_march_output res = data_to_output(u, data.rayDirection.z,data.distance);
    
    return res;
}