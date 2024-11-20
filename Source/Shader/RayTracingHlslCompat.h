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

#ifndef RAYTRACINGHLSLCOMPAT_H
#define RAYTRACINGHLSLCOMPAT_H

#ifndef HLSL
#define uint uint32_t

#endif

// Ray types traced in this sample.
namespace RayType {
    enum Enum {
        Radiance = 0,   // ~ Primary, reflected camera/view rays calculating color for each hit.
        Shadow,         // ~ Shadow/visibility rays, only testing for occlusion
        Count
    };
}

namespace TraceRayParameters
{
    static const uint32_t InstanceMask = ~0;   // Everything is visible.
    namespace HitGroup {
        static const uint32_t Offset[RayType::Count] =
        {
            0, // Radiance ray
            1  // Shadow ray
        };
        static const uint32_t GeometryStride = RayType::Count;
    }
    namespace MissShader {
        static const uint32_t Offset[RayType::Count] =
        {
            0, // Radiance ray
            1  // Shadow ray
        };
    }
}

#endif // RAYTRACINGHLSLCOMPAT_H