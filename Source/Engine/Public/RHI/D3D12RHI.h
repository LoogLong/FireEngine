#pragma once
#include <array>
#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <vector>
#include <cassert>
#include <d3dcompiler.h>
#include <memory>
#include <string>
#include <strsafe.h>
#include <tchar.h>

#include "Classes/mesh.h"
#include "Core/define.h"

using namespace Microsoft::WRL;

namespace RHID3D12
{
	constexpr DXGI_FORMAT RENDER_TARGET_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
	constexpr float CLEAR_COLOR[] = { 0.1f, 0.1f, 0.1f, 1.0f };

	constexpr UINT DXGI_FACTORY_FLAGS = 0U;
	constexpr UINT FRAME_BACK_BUF_COUNT = 3u;

	const std::string SHADER_PATH = "Resource\\Shader\\shaders.hlsl";
}

#define CHECK_RESULT(hr) if(!SUCCEEDED(hr)){ printf("[ERROR]:line:%d\n", __LINE__);}

namespace FireEngine
{
	struct SGeometryResource
	{
		SGeometryResource() = default;
		~SGeometryResource() = default;
		SGeometryResource(SGeometryResource&) = default;
		SGeometryResource(SGeometryResource&&) = default;
		
		ComPtr<ID3D12Resource2> m_vertex_buffer;
		ComPtr<ID3D12Resource2> m_index_buffer;
		uint32_t m_vertex_count;
		uint32_t m_index_count;
		uint32_t m_vertex_stride;
		uint32_t m_index_stride;
		ComPtr<ID3D12Resource2> m_geometry_descs;
		std::vector<SGeometryDesc> m_geometry_descs_cpu;
	};

	struct SConstantBuffer
	{
		SConstantBuffer() = default;
		~SConstantBuffer() = default;
		SConstantBuffer(SConstantBuffer&) = default;
		SConstantBuffer(SConstantBuffer&&) = default;

		ComPtr<ID3D12Resource2> rhi_buffer;
		uint32_t size_in_bytes;
	};

	struct EventFence
	{
		ComPtr<ID3D12Fence1> m_fence;
		UINT64 m_fence_value{ 0ui64 };
		HANDLE m_fence_event{ nullptr };

		EventFence() = default;
		~EventFence() = default;
		EventFence(EventFence&) = default;
		EventFence(EventFence&&) = default;
	};


	class D3D12RHI
	{
	public:
		D3D12RHI(const HWND& hwnd);

		// 枚举适配器，并选择合适的适配器来创建3D设备对象
		void CreateDevice();

		void CreateCommandQueue();

		void CreateSwapChain(const uint32_t width, const uint32_t height);


		size_t CreateRootSignature();
		size_t CreateRayTracingRootSignature();

		void CreatePipelineStateObject();
		void CreateRayTracingPipelineStateObject(const std::vector<uint8_t>& shader_byte_code);

		void CreateRenderEndFence();
		void InitializeSampler();
		void CreateRayTracingRenderTargetUAV();

		void CreateShaderTable();

		void CreateTexture(std::vector<uint8_t>& texture_data, uint32_t width, uint32_t height);


		void WaitForFence() const;


		void DoRayTracing();

		TCHAR* GetCurrentAdapterName();

		void CreatePrimitives(const std::vector<SVertexInstance>& vertex_vector, const std::vector<IndexType>& vertex_indices);
		void CreatePrimitives(const std::vector<CMesh*>& meshes);
		void CreateMaterials();
		void CreateSceneConstantBuffer();
		void UpdateSceneConstantBuffer(SSceneConstantBuffer* data, uint64_t size);
		void CreateInstanceConstantBuffer();

		void CreateBottomLevelAccelerationStructure();
		void CreateTopLevelInstanceResource();
		void CreateTopLevelAccelerationStructure();
		void BuildDescHeap();

	private:
		/*constance value*/
		D3D_FEATURE_LEVEL m_feature_level = D3D_FEATURE_LEVEL_12_1;
		UINT m_rtv_descriptor_size{ 0u };
		UINT m_dxgi_factory_flags{ 0u };
		TCHAR m_adapter_name[MAX_PATH]{};
		DXGI_FORMAT m_render_target_format = DXGI_FORMAT_R8G8B8A8_UNORM;

		D3D12_VIEWPORT m_view_port{};
		D3D12_RECT m_scissor_rect{};
		std::array<uint32_t, 2> m_swap_chain_size;
		UINT m_frame_index{ 0 };

		HWND m_hwnd{};

		UINT64 m_srv_cbv_uav_descriptor_size{ 0 };

		ComPtr<IDXGIFactory7> m_dxgi_factory;
		ComPtr<IDXGIAdapter1> m_dxgi_adapter;
		ComPtr<ID3D12Device8> m_d3d12_device;
		ComPtr<ID3D12CommandQueue> m_cmd_queue;
		ComPtr<ID3D12CommandAllocator> m_cmd_allocator;
		ComPtr<ID3D12GraphicsCommandList6> m_cmd_list;

		ComPtr<IDXGISwapChain1> m_swap_chain1;
		ComPtr<IDXGISwapChain4> m_swap_chain4;
		ComPtr<ID3D12DescriptorHeap> m_rtv_heap;
		ComPtr<ID3D12DescriptorHeap> m_srv_cbv_uav_heap;
		ComPtr<ID3D12DescriptorHeap> m_sampler_heap;
		ComPtr<ID3D12Resource2> m_render_targets[RHID3D12::FRAME_BACK_BUF_COUNT];

		// ray tracing
		ComPtr<ID3D12Resource2> m_raytracing_render_targets;
		ComPtr<ID3D12Heap1> m_shader_table_heap;

		ComPtr<ID3D12Resource> m_miss_shader_table;
		ComPtr<ID3D12Resource> m_hit_group_shader_table;
		ComPtr<ID3D12Resource> m_ray_gen_shader_table;

		// 加速结构
		ComPtr<ID3D12Resource> m_bottom_level_acceleration_structure;
		ComPtr<ID3D12Resource> m_bottom_level_scratch_resource;
		ComPtr<ID3D12Resource> m_top_level_acceleration_structure;
		ComPtr<ID3D12Resource> m_top_level_scratch_resource;
		ComPtr<ID3D12Resource> m_top_level_instance_resource;
		//textures
		std::vector<ComPtr<ID3D12Resource>> m_textures;

		//constant buffer
		SConstantBuffer m_scene_constant_buffer;
		std::vector<SConstantBuffer> m_instance_buffer_local;

		/*用于同步信息的fence*/
		EventFence m_render_end_fence;
		// std::vector<EventFence> m_event_fences;


		/*缓存shader和pso*/
		std::vector<ComPtr<ID3DBlob>> m_shader_cache;
		std::vector<ComPtr<ID3D12PipelineState>> m_pipeline_states;
		ComPtr<ID3D12StateObject> m_raytracing_state_object;

		std::vector<ComPtr<ID3D12RootSignature>> m_root_signatures;

		/*缓存本帧用到的资源*/
		std::vector<SGeometryResource> m_render_primitives;
		ComPtr<ID3D12Resource> m_materials;
		std::vector<SMaterial> m_materials_cpu;

		// desc heap 常量 与root signature相关
		const UINT64                m_ray_tracing_rt_uav = 0; // uav 光追的输出
		const UINT64                c_nDSHIndxIBView = 1;
		const UINT64                c_nDSHIndxVBView = 2;
		const UINT64                c_nDSHIndxCBScene = 3;
		const UINT64                c_nDSHIndxCBModule = 4;
		const UINT64                c_nDSHIndxASBottom1 = 5;
		const UINT64                c_nDSHIndxASBottom2 = 6;
		const UINT64                c_nDSNIndxTexture = 7;
		const UINT64                c_nDSNIndxNormal = 8;

	};

}


