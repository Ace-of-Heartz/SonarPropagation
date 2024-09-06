struct EnvData
{
    float depth;
    float temperature;
    float salinity;
};

struct RayMarchInput
{
    float3 rayOrigin;
    float3 rayDirection;
    float distance;
};

struct RayMarchOutput
{
    float3 rayOrigin;
    float3 rayDirection;
};

struct RayData
{
    float r;
    float z;
    float xi;
    float zeta;
};

