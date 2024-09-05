float2 CartesianToCylinder(float3 coord)
{
    return float2(
    0.0,
    coord.z
    );
}

float3 CylinderToCartesian(float2 coord)
{
    return float3(
    cos(coord.r),
    sin(coord.r),
    coord.g
    );
}