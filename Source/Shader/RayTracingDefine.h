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

#ifndef RAYTRACINGDEFINE_H
#define  RAYTRACINGDEFINE_H

#define MAX_RAY_RECURSION_DEPTH 10    // ~ primary rays + reflections + shadow rays from reflected geometry.


struct SSceneConstantBuffer
{
    float4x4 m_mxView;
    float4 m_vCameraPos;

    float4 m_vLightPos;
    float4 m_vLightAmbientColor;
    float4 m_vLightDiffuseColor;

	float2 m_v2PixelSize;
};

struct SGeometryDesc
{
    uint vertex_offset;
    uint vertex_count;
    uint index_offset;
    uint index_count;
    uint material_index;
};

struct SMaterial {
    float4 emission; // 3emission + 1 ior
    float3 kd;
    float3 ks;
    float specular_exponent;
};

struct SInstanceConstantBuffer
{
    float4 m_vAlbedo;
};

typedef uint IndexType;

// 顶点结构
struct SVertexInstance
{
    float4 position;		//Position
	float3 normal;		//Normal
    float2 uv;		//Texcoord
    float4 color;		//Texcoord
};
struct ShadowRayPayload
{
    bool hit;
};


#endif //  RAYTRACINGDEFINE_H