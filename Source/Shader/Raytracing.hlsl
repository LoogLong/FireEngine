//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#ifndef RAYTRACING_HLSL
#define RAYTRACING_HLSL

#define HLSL
#include "RayTracingDefine.h"
#include "RaytracingHlslCompat.h"

RWTexture2D<float4>						g_RenderTarget  : register(u0);

RaytracingAccelerationStructure			g_asScene       : register(t0, space0);

StructuredBuffer<IndexType>				g_Indices       : register(t1, space0);
StructuredBuffer<SVertexInstance>		g_Vertices      : register(t2, space0);

StructuredBuffer<SGeometryDesc>			g_geometry_descs: register(t3);
StructuredBuffer<SMaterial>			    g_materials     : register(t4);

SamplerState							g_sampler       : register(s0);

ConstantBuffer<SSceneConstantBuffer>	g_stSceneCB     : register(b0);
ConstantBuffer<SInstanceConstantBuffer> l_stModuleCB    : register(b1);


struct RayPayload
{
    float4 color;
    uint  recursion_depth;
};

// Retrieve hit world normal.
float3 HitWorldPosition()
{
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

float nrand(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}


// Generate a ray in world space for a camera pixel corresponding to an index from the dispatched 2D grid.
inline void GenerateCameraRay(float2 v2PixelSize,uint2 index, out float3 origin, out float3 direction)
{
     float x_scale = v2PixelSize.x; // tan(fov/2) * aspect_ratio
     float y_scale = v2PixelSize.y;// tan(fov/2)
     float x = (-(index.x + 0.5f) / DispatchRaysDimensions().x * 2.f + 1.0f) * x_scale;
     float y = (-(index.y + 0.5f) / DispatchRaysDimensions().y * 2.f + 1.0f) * y_scale;
	float3 pixel_dir = float3(x, y, 1.f);

    origin = g_stSceneCB.m_vCameraPos.xyz;
	direction = normalize(pixel_dir);
}

float4 TraceRadianceRay(in RayDesc rayDesc, in uint currentRayRecursionDepth)
{
    if (currentRayRecursionDepth >= MAX_RAY_RECURSION_DEPTH)
    {
        return float4(0, 0, 0, 0);
    }

    RayPayload rayPayload = { float4(0, 0, 0, 0), currentRayRecursionDepth + 1 };
    TraceRay(g_asScene,
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
        TraceRayParameters::InstanceMask,
        TraceRayParameters::HitGroup::Offset[RayType::Radiance],
        0,
        //TraceRayParameters::HitGroup::GeometryStride,
        TraceRayParameters::MissShader::Offset[RayType::Radiance],
        rayDesc, rayPayload);

    return rayPayload.color;
}

[shader("raygeneration")]
void MyRaygenShader()
{
    float3 rayDir;
    float3 origin;
    
    // Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
    GenerateCameraRay(g_stSceneCB.m_v2PixelSize,DispatchRaysIndex().xy, origin, rayDir);

    // Trace the ray.
    // Set the ray's extents.
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = rayDir;
    // Set TMin to a non-zero small value to avoid aliasing issues due to floating - point errors.
    // TMin should be kept small to prevent missing geometry at close contact areas.
    ray.TMin = 0.001;
    ray.TMax = 10000.0;
    
    

    // Write the raytraced color to the output texture.
    g_RenderTarget[DispatchRaysIndex().xy] = TraceRadianceRay(ray, 0);
}

[shader("closesthit")]
void MyClosestHitShader(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    float3 hitPosition = HitWorldPosition();

    uint indicesPerTriangle = 3;
    uint baseIndex = g_geometry_descs[GeometryIndex()].index_offset+ PrimitiveIndex() * indicesPerTriangle;

    uint vertex_offset = g_geometry_descs[GeometryIndex()].vertex_offset;

    const uint3 indices = uint3(g_Indices[baseIndex] + vertex_offset, g_Indices[baseIndex+1] + vertex_offset, g_Indices[baseIndex+2] + vertex_offset);

	float3 hit_normal = g_Vertices[indices[0]].normal +
		attr.barycentrics.x * (g_Vertices[indices[1]].normal - g_Vertices[indices[0]].normal) +
		attr.barycentrics.y * (g_Vertices[indices[2]].normal - g_Vertices[indices[0]].normal);

    float3 rayDir = g_stSceneCB.m_vLightPos.xyz - hitPosition;
    rayDir = normalize(rayDir);
    RayDesc reflectionRay;
    reflectionRay.Origin = hitPosition;
    reflectionRay.Direction = reflect(WorldRayDirection(), hit_normal);

    reflectionRay.TMin = 0.001;
    reflectionRay.TMax = 10000.0;

    float4 reflectionColor = TraceRadianceRay(reflectionRay, payload.recursion_depth);

    RayDesc random_ray;
    random_ray.Origin = hitPosition;
    float3 rand_direction = float3(nrand(g_Vertices[indices[0]].uv * RayTCurrent()), nrand(g_Vertices[indices[1]].uv * RayTCurrent()), nrand(g_Vertices[indices[2]].uv * RayTCurrent()));
    rand_direction = normalize(rand_direction);
    if (dot(rand_direction, hit_normal) < 0.f) {
        rand_direction = -rand_direction;
    }
    random_ray.Direction = rand_direction;

    random_ray.TMin = 0.001;
    random_ray.TMax = 10000.0;
    float4 amb_color = TraceRadianceRay(random_ray, payload.recursion_depth);

    float4 emission = g_materials[g_geometry_descs[GeometryIndex()].material_index].emission;
    
	payload.color = emission + reflectionColor*0.01f + amb_color * 0.5f;
}

[shader("miss")]
void MyMissShader(inout RayPayload payload)
{
    float4 background = float4(0.0f, 0.0f, 0.0f, 1.0f);
    payload.color = background;
}
[shader("miss")]

void MyMissShader_ShadowRay(inout ShadowRayPayload rayPayload)
{
    rayPayload.hit = false;
}
#endif // RAYTRACING_HLSL