
#include "Render/RenderingSystem.h"

#include <DirectXMath.h>

#include "Classes/texture.h"
#include "Core/file_system.h"
#include "Global/global_context.h"
#include "RHI/D3D12RHI.h"
#include "Core/ReadData.h"
#include "atlconv.h"
#include "Classes/mesh.h"

namespace FireEngine {
	using namespace DirectX;
	//全局光源信息变量
	XMFLOAT4 g_v4LightPosition = XMFLOAT4(0.0f, 200.8f, -30.0f, 0.0f);
	XMFLOAT4 g_v4LightAmbientColor = XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
	XMFLOAT4 g_v4LightDiffuseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	//全局摄像机信息变量278, 273, -800
	XMVECTOR g_vEye = { 278.0f, 273.0f, -800.0f, 0.0f };
	XMVECTOR g_vLookAt = { 0.0f, 0.f, 0.0f };
	XMVECTOR g_vUp = { 0.0f, 1.0f, 0.0f, 0.0f };
	XMVECTOR forward = g_vLookAt - g_vEye;

	CRenderingSystem::CRenderingSystem(CWindowSystem* window_system)
	{
		// init rendering system
		auto hwnd = window_system->GetWindowHwnd();
		m_rhi = new D3D12RHI(hwnd);

		auto width_height = window_system->GetWindowSize();
		m_rhi->CreateDevice();
	
		m_rhi->CreateCommandQueue();
		m_rhi->CreateSwapChain(width_height.first, width_height.second);
		m_rhi->CreateRayTracingRootSignature();

		auto shader_path = g_global_singleton_context->m_file_system->GetFullPath("Resource/Shader/Raytracing.cso");
		WCHAR* conv;
		{
			USES_CONVERSION;
			conv = A2W(shader_path.data());
		}
		std::vector<uint8_t> shader_byte_code = ReadData(conv);
		m_rhi->CreateRayTracingPipelineStateObject(shader_byte_code);
		m_rhi->CreateRenderEndFence();
		m_rhi->InitializeSampler();
		m_rhi->CreateRayTracingRenderTargetUAV();
		m_rhi->CreateShaderTable();
		m_rhi->CreateSceneConstantBuffer();
		m_rhi->CreateMaterials();

		std::vector<std::string> mesh_file_names;
		{
			mesh_file_names.reserve(6);
			mesh_file_names.emplace_back("Resource/models/floor.obj");//white
			mesh_file_names.emplace_back("Resource/models/shortbox.obj");//white
			mesh_file_names.emplace_back("Resource/models/tallbox.obj");//white
			mesh_file_names.emplace_back("Resource/models/left.obj");//red
			mesh_file_names.emplace_back("Resource/models/right.obj");//green
			mesh_file_names.emplace_back("Resource/models/light.obj");//light
			std::array<DirectX::XMFLOAT4, 6> color;
			color[0] = { 0.63f, 0.065f, 0.05f, 1.0f };
			color[1] = { 0.63f, 0.45f, 0.05f, 1.0f };
			color[2] = { 0.63f, 0.065f, 0.65f, 1.0f };
			color[3] = { 0.14f, 0.45f, 0.091f, 1.0f };
			color[4] = { 0.725f, 0.71f, 0.68f, 1.0f };
			color[5] = { 0.65f, 0.65f, 0.65f, 1.0f };
			int32_t color_idx = 0;
			std::array<uint32_t, 6> material_instance;
			material_instance[0] = 2;
			material_instance[1] = 2;
			material_instance[2] = 2;
			material_instance[3] = 0;
			material_instance[4] = 1;
			material_instance[5] = 3;
#define Combine 1
#if Combine
			std::vector<CMesh*> meshes;
			for (auto& path : mesh_file_names)
			{
				std::string mesh_path = g_global_singleton_context->m_file_system->GetFullPath(path);
				std::vector<SVertexInstance> vretices;
				std::vector<IndexType> indices;
				LoadMeshVertexObject(mesh_path, vretices, indices);
				std::unique_ptr<CMesh> mesh = std::make_unique<CMesh>();
				for (auto& vertex : vretices)
				{
					vertex.color[0] = color[color_idx].x;
					vertex.color[1] = color[color_idx].y;
					vertex.color[2] = color[color_idx].z;
					vertex.color[3] = color[color_idx].w;
				}
				mesh->material = material_instance[color_idx];
				color_idx++;
				mesh->m_vretices = std::move(vretices);
				mesh->m_indices = std::move(indices);
				meshes.emplace_back(mesh.get());
				uint64_t asset_id = g_global_singleton_context->m_asset_system->RetainAsset(std::move(mesh));
			}
			m_rhi->CreatePrimitives(meshes);
			{
				float x = 0.f;
				float y = 0.f;
				float z = 0.f;
				int32_t count = 0;
				for (auto && vertex_instance : meshes[5]->m_vretices)
				{
					x += vertex_instance.position[0];
					y += vertex_instance.position[1];
					z += vertex_instance.position[2];
					count++;
				}
				g_v4LightPosition.x = x / static_cast<float>(count);
				g_v4LightPosition.y = y / static_cast<float>(count);
				g_v4LightPosition.z = z / static_cast<float>(count);
				g_v4LightPosition.w = 1.0f;
			}
#else
			std::unique_ptr<CMesh>          mesh = std::make_unique<CMesh>();
			
			for (auto& path : mesh_file_names)
			{
				std::string mesh_path = g_global_singleton_context->m_file_system->GetFullPath(path);
				std::vector<SVertexInstance> vretices;
				std::vector<IndexType> indices;
				LoadMeshVertexObject(mesh_path, vretices, indices);

				IndexType idx_offset = static_cast<IndexType>(mesh->m_vretices.size());
				mesh->m_vretices.reserve(mesh->m_vretices.size() + vretices.size());
				for (auto& vertex : vretices)
				{
					vertex.color[0] = color[color_idx].x;
					vertex.color[1] = color[color_idx].y;
					vertex.color[2] = color[color_idx].z;
					vertex.color[3] = color[color_idx].w;
					mesh->m_vretices.emplace_back(vertex);
				}
				color_idx++;
				mesh->m_indices.reserve(mesh->m_indices.size() + indices.size());
				for (IndexType index : indices)
				{
					mesh->m_indices.emplace_back(idx_offset + index);
				}
			}
			m_rhi->CreatePrimitives(mesh->m_vretices, mesh->m_indices);
			printf("\n");
			for (auto& vert : mesh->m_vretices)
			{
				printf("normal[%f, %f, %f]\n", vert.normal[0], vert.normal[1], vert.normal[2]);
			}
			uint64_t asset_id = g_global_singleton_context->m_asset_system->RetainAsset(std::move(mesh));
#endif
		}

		{
			std::string texture_path = g_global_singleton_context->m_file_system->GetFullPath("Resource/texture/Earth4kTexture_4K.png");
			auto texture = std::make_unique<CTexture>();
			texture->LoadTextureFromFile(texture_path);
			m_rhi->CreateTexture(texture->m_data, texture->m_width, texture->m_height);
			uint64_t asset_id0 = g_global_singleton_context->m_asset_system->RetainAsset(std::move(texture));
		}
		{
			std::string texture_path = g_global_singleton_context->m_file_system->GetFullPath("Resource/texture/Earth4kNormal_4K.png");
			auto texture = std::make_unique<CTexture>();
			texture->LoadTextureFromFile(texture_path);
			m_rhi->CreateTexture(texture->m_data, texture->m_width, texture->m_height);
			uint64_t asset_id0 = g_global_singleton_context->m_asset_system->RetainAsset(std::move(texture));
		}
		m_rhi->CreateBottomLevelAccelerationStructure();
		m_rhi->CreateTopLevelInstanceResource();
		m_rhi->CreateTopLevelAccelerationStructure();

		m_rhi->BuildDescHeap();
	}

	void CRenderingSystem::TickRendering(float dt)
	{
		

		XMVECTOR z_axis = XMVectorZero();
		z_axis = XMVectorSetZ(z_axis, 1.0f);
		{
			//swig
			forward = XMVector3Normalize(forward);
			auto rotation_axis = XMVector3Cross(z_axis, forward);
			XMVECTOR swig;
			if (!XMVector3NearEqual(rotation_axis, XMVectorZero(), XMVectorZero()))
			{
				XMVECTOR cos = XMVector3Dot(z_axis, forward);
				XMVECTOR angle = XMVectorACos(cos);
				swig = XMQuaternionRotationAxis(rotation_axis, XMVectorGetX(angle) * 0.5f);
			}
			else
			{
				swig = XMQuaternionIdentity();
			}
			
			//twist
			XMVECTOR y_axis = XMVectorZero();
			y_axis = XMVectorSetY(y_axis, 1.0f);
			auto swig_y = XMVector3Rotate(y_axis, swig);

			auto rotation_y = XMVector3Cross(swig_y, g_vUp);
			XMVECTOR twist;
			if (!XMVector3NearEqual(rotation_y, XMVectorZero(), XMVectorZero()))
			{
				XMVECTOR cos_y = XMVector3Dot(swig_y, g_vUp);
				XMVECTOR angle_y = XMVectorACos(cos_y);
				twist = XMQuaternionRotationAxis(rotation_y, XMVectorGetX(angle_y) * 0.5f);
			}
			else
			{
				twist = XMQuaternionIdentity();
			}
			
			auto rotation = XMQuaternionDot(twist, swig_y);
			rotation = XMQuaternionInverse(rotation);
			auto view = XMMatrixRotationQuaternion(rotation);
			
			//屏幕像素点尺寸
			m_scene_constant_buffer.m_view_matrix = view;
			m_scene_constant_buffer.m_camera_pos = g_vEye;
		}
		m_scene_constant_buffer.m_light_pos = XMLoadFloat4(&g_v4LightPosition);
		m_scene_constant_buffer.m_light_ambient_color = XMLoadFloat4(&g_v4LightAmbientColor);
		m_scene_constant_buffer.m_light_diffuse_color = XMLoadFloat4(&g_v4LightDiffuseColor);

		//设置屏幕像素尺寸

		//设置屏幕像素尺寸
		float fov = 40;
		float scale = std::tan(DirectX::XMConvertToRadians(fov * 0.5f));
		auto window_size = g_global_singleton_context->m_window_system->GetWindowSize();
		float image_aspect_ratio = window_size.first / (float)window_size.second;
		m_scene_constant_buffer.m_scale.x = scale * image_aspect_ratio;
		m_scene_constant_buffer.m_scale.y = scale;
		m_rhi->UpdateSceneConstantBuffer(&m_scene_constant_buffer, sizeof(m_scene_constant_buffer));
		m_rhi->DoRayTracing();
		m_rhi->WaitForFence();
	}
}
