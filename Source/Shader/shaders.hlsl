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

struct PSInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};
cbuffer cbGlobal : register(b0)
{
	row_major matrix    g_mViewProjection;
}

cbuffer cbPerObject : register(b1)
{
	row_major matrix    g_mWorld;
}

//static float4x4 g_scale = { {0.003f,0,0,0},{0,0.003f,0,0}, {0,0,0.003f,0}, {0,0,0,1} };
PSInput VSMain(float4 position : POSITION, float4 color : COLOR)
{
	PSInput result;

	result.position = mul(position, g_mWorld);
	result.position = mul(result.position, g_mViewProjection);
	result.color = color;
	

	return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	return input.color;
}
