#include "RHI/D3D12RHI.h"

#include <DirectXMath.h>

#include "RayTracingHlslCompat.h"
#include "Core/define.h"
#include "Core/basic_math.h"
namespace FireEngine
{
	const wchar_t* c_hitGroupNames_TriangleGeometry[] =
	{
		L"MyHitGroup_Triangle", L"MyHitGroup_Triangle_ShadowRay"
	};
	const wchar_t* c_pszRaygenShaderName     = L"MyRaygenShader";
	const wchar_t* c_pszClosestHitShaderName = L"MyClosestHitShader"; // 只有三角形，这里就只有一种最近命中函数了
	const wchar_t* c_missShaderNames[] =
	{
		L"MyMissShader", L"MyMissShader_ShadowRay"
	};
	D3D12RHI::D3D12RHI(const HWND& hwnd)
	{
		// 打开显示子系统的调试支持
		{
#if defined(_DEBUG)
			ComPtr<ID3D12Debug> debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			{
				debugController->EnableDebugLayer();
				// 打开附加的调试支持
				m_dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
			}
#endif
		}

		// 创建DXGI Factory对象
		{
			CHECK_RESULT(CreateDXGIFactory2(m_dxgi_factory_flags, IID_PPV_ARGS(&m_dxgi_factory)));
			// 关闭ALT+ENTER键切换全屏的功能，因为我们没有实现OnSize处理，所以先关闭
			CHECK_RESULT(m_dxgi_factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
		}

		m_hwnd = hwnd;
	}

	void D3D12RHI::CreateDevice()
	{
		DXGI_ADAPTER_DESC1 stAdapterDesc = {};

		// 枚举高性能显卡
		for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != m_dxgi_factory->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&m_dxgi_adapter)); ++adapterIndex)
		{
			m_dxgi_adapter->GetDesc1(&stAdapterDesc);
			// 创建D3D12.1的设备
			if (SUCCEEDED(D3D12CreateDevice(m_dxgi_adapter.Get(), m_feature_level, IID_PPV_ARGS(&m_d3d12_device))))
			{
				break;
			}
		}
		// 把显卡名字复制出来
		StringCchPrintf(m_adapter_name, MAX_PATH, _T("%s"), stAdapterDesc.Description);
		{
			//检查ray tracing
			D3D12_FEATURE_DATA_D3D12_OPTIONS5 stFeatureSupportData = {};
			HRESULT                           hr                   = m_d3d12_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &stFeatureSupportData, sizeof(stFeatureSupportData));

			//检测硬件是否是直接支持DXR
			bool b_is_raytracing_support = SUCCEEDED(hr) && (stFeatureSupportData.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED);
			if (!b_is_raytracing_support)
			{
				printf("[error],ray tracing not support!\n");
				return;
			}
		}

		//缓存一些常量
		m_srv_cbv_uav_descriptor_size = m_d3d12_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	void D3D12RHI::CreateCommandQueue()
	{
		D3D12_COMMAND_QUEUE_DESC queue_desc = {};
		queue_desc.Type                     = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queue_desc.Priority                 = 0;
		CHECK_RESULT(m_d3d12_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&m_cmd_queue)));


		CHECK_RESULT(m_d3d12_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmd_allocator)));

		// 创建图形命令列表
		CHECK_RESULT(m_d3d12_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmd_allocator.Get(), nullptr, IID_PPV_ARGS(&m_cmd_list)));
		CHECK_RESULT(m_cmd_list->Close());
	}

	void D3D12RHI::CreateSwapChain(const uint32_t width, const uint32_t height)
	{
		m_view_port    = {0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH};
		m_scissor_rect = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
		m_swap_chain_size = { width,height };
		DXGI_SWAP_CHAIN_DESC1 stSwapChainDesc = {};
		stSwapChainDesc.BufferCount           = RHID3D12::FRAME_BACK_BUF_COUNT;
		stSwapChainDesc.Width                 = width;
		stSwapChainDesc.Height                = height;
		stSwapChainDesc.Format                = m_render_target_format;
		stSwapChainDesc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		stSwapChainDesc.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		stSwapChainDesc.SampleDesc.Count      = 1;


		CHECK_RESULT(m_dxgi_factory->CreateSwapChainForHwnd(m_cmd_queue.Get(), m_hwnd, &stSwapChainDesc, nullptr, nullptr, & m_swap_chain1));

		CHECK_RESULT(m_swap_chain1.As(&m_swap_chain4));

		m_frame_index = m_swap_chain4->GetCurrentBackBufferIndex();

		//创建RTV(渲染目标视图)描述符堆
		D3D12_DESCRIPTOR_HEAP_DESC stRTVHeapDesc = {};
		stRTVHeapDesc.NumDescriptors             = RHID3D12::FRAME_BACK_BUF_COUNT;
		stRTVHeapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		stRTVHeapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		CHECK_RESULT(m_d3d12_device->CreateDescriptorHeap(&stRTVHeapDesc, IID_PPV_ARGS(&m_rtv_heap)));
		//得到每个描述符元素的大小
		m_rtv_descriptor_size = m_d3d12_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		//创建RTV的描述符
		D3D12_CPU_DESCRIPTOR_HANDLE stRTVHandle = m_rtv_heap->GetCPUDescriptorHandleForHeapStart();
		for (UINT i = 0; i < RHID3D12::FRAME_BACK_BUF_COUNT; ++i)
		{
			CHECK_RESULT(m_swap_chain4->GetBuffer(i, IID_PPV_ARGS(&m_render_targets[i])));
			m_d3d12_device->CreateRenderTargetView(m_render_targets[i].Get(), nullptr, stRTVHandle);
			stRTVHandle.ptr += m_rtv_descriptor_size;
		}
	}

	size_t D3D12RHI::CreateRootSignature()
	{
		constexpr UINT root_signature_count = 2;
		// 第一个是全局const buffer
		D3D12_ROOT_PARAMETER root_parameter[root_signature_count];
		root_parameter[0].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
		root_parameter[0].Descriptor.ShaderRegister = 0;
		root_parameter[0].Descriptor.RegisterSpace  = 0;
		root_parameter[0].ShaderVisibility          = D3D12_SHADER_VISIBILITY_VERTEX;

		//第二个是各自的
		root_parameter[1].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
		root_parameter[1].Descriptor.ShaderRegister = 1;
		root_parameter[1].Descriptor.RegisterSpace  = 0;
		root_parameter[1].ShaderVisibility          = D3D12_SHADER_VISIBILITY_VERTEX;


		D3D12_ROOT_SIGNATURE_DESC root_signature_desc;
		root_signature_desc.NumParameters     = root_signature_count;
		root_signature_desc.pParameters       = root_parameter;
		root_signature_desc.NumStaticSamplers = 0;
		root_signature_desc.pStaticSamplers   = nullptr;
		root_signature_desc.Flags             = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;


		ComPtr<ID3DBlob> signature_blob;
		ComPtr<ID3DBlob> error_blob;

		if (!SUCCEEDED(D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature_blob, &error_blob )))
		{
			CHAR psz_err_msg[MAX_PATH] = {};
			StringCchPrintfA(psz_err_msg, MAX_PATH, "\n%s\n", static_cast<CHAR*>(error_blob->GetBufferPointer()));
			::OutputDebugStringA(psz_err_msg);
		}
		ComPtr<ID3D12RootSignature> root_signature;
		CHECK_RESULT(m_d3d12_device->CreateRootSignature(0, signature_blob->GetBufferPointer(), signature_blob->GetBufferSize(), IID_PPV_ARGS(&root_signature)));
		m_root_signatures.emplace_back(root_signature);

		return m_root_signatures.size() - 1;
	}

	size_t D3D12RHI::CreateRayTracingRootSignature()
	{
		ComPtr<ID3D12RootSignature> global_rs;
		ComPtr<ID3D12RootSignature> local_rs;

		D3D12_DESCRIPTOR_RANGE1 stRanges[4]           = {}; // Perfomance TIP: Order from most frequent to least frequent.
		stRanges[0].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		stRanges[0].NumDescriptors                    = 1;
		stRanges[0].BaseShaderRegister                = 0;
		stRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		stRanges[0].RegisterSpace                     = 0;
		stRanges[0].Flags                             = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

		stRanges[1].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		stRanges[1].NumDescriptors                    = 2;
		stRanges[1].BaseShaderRegister                = 1;
		stRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		stRanges[1].RegisterSpace                     = 0;
		stRanges[1].Flags                             = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

		stRanges[2].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		stRanges[2].NumDescriptors                    = 2;
		stRanges[2].BaseShaderRegister                = 3;
		stRanges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		stRanges[2].RegisterSpace                     = 0;
		stRanges[2].Flags                             = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

		stRanges[3].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		stRanges[3].NumDescriptors                    = 1;
		stRanges[3].BaseShaderRegister                = 0;
		stRanges[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		stRanges[3].RegisterSpace                     = 0;
		stRanges[3].Flags                             = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

		D3D12_ROOT_PARAMETER1 stGlobalRootParams[6] = {};

		stGlobalRootParams[0].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		stGlobalRootParams[0].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;
		stGlobalRootParams[0].DescriptorTable.NumDescriptorRanges = 1;
		stGlobalRootParams[0].DescriptorTable.pDescriptorRanges   = &stRanges[0];

		stGlobalRootParams[1].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_SRV;
		stGlobalRootParams[1].ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;
		stGlobalRootParams[1].Descriptor.RegisterSpace  = 0;
		stGlobalRootParams[1].Descriptor.ShaderRegister = 0;

		stGlobalRootParams[2].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
		stGlobalRootParams[2].ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;
		stGlobalRootParams[2].Descriptor.RegisterSpace  = 0;
		stGlobalRootParams[2].Descriptor.ShaderRegister = 0;

		stGlobalRootParams[3].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		stGlobalRootParams[3].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;
		stGlobalRootParams[3].DescriptorTable.NumDescriptorRanges = 1;
		stGlobalRootParams[3].DescriptorTable.pDescriptorRanges   = &stRanges[1];

		stGlobalRootParams[4].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		stGlobalRootParams[4].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;
		stGlobalRootParams[4].DescriptorTable.NumDescriptorRanges = 1;
		stGlobalRootParams[4].DescriptorTable.pDescriptorRanges   = &stRanges[2];

		stGlobalRootParams[5].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		stGlobalRootParams[5].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;
		stGlobalRootParams[5].DescriptorTable.NumDescriptorRanges = 1;
		stGlobalRootParams[5].DescriptorTable.pDescriptorRanges   = &stRanges[3];

		//D3D12_ROOT_SIGNATURE_DESC stGlobalRootSignatureDesc = {};
		//stGlobalRootSignatureDesc.NumParameters = ARRAYSIZE(stGlobalRootParams);
		//stGlobalRootSignatureDesc.pParameters = stGlobalRootParams;

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC st_global_root_signature_desc;
		st_global_root_signature_desc.Version                             = D3D_ROOT_SIGNATURE_VERSION_1_1;
		st_global_root_signature_desc.Desc_1_1.Flags                      = D3D12_ROOT_SIGNATURE_FLAG_NONE;
		st_global_root_signature_desc.Desc_1_1.NumParameters              = ARRAYSIZE(stGlobalRootParams);
		st_global_root_signature_desc.Desc_1_1.pParameters                = stGlobalRootParams;
		st_global_root_signature_desc.Desc_1_1.NumStaticSamplers          = 0;
		st_global_root_signature_desc.Desc_1_1.pStaticSamplers            = nullptr;

		ComPtr<ID3DBlob> pIRSBlob;
		ComPtr<ID3DBlob> pIRSErrMsg;

		HRESULT hrRet = ::D3D12SerializeVersionedRootSignature(&st_global_root_signature_desc, &pIRSBlob, &pIRSErrMsg);

		if (FAILED(hrRet))
		{
			if (pIRSErrMsg)
			{
				printf("编译全局根签名出错：%s\n", (char*)pIRSErrMsg->GetBufferPointer());
				printf("编译全局根签名出错：%s\n", (char*)pIRSErrMsg->GetBufferPointer());
			}
			CHECK_RESULT(hrRet)
		}

		CHECK_RESULT(m_d3d12_device->CreateRootSignature(1 , pIRSBlob->GetBufferPointer() , pIRSBlob->GetBufferSize() , IID_PPV_ARGS(&global_rs)))

		pIRSBlob.Reset();
		pIRSErrMsg.Reset();

		D3D12_ROOT_PARAMETER1 stLocalRootParams[1]    = {};
		stLocalRootParams[0].ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		stLocalRootParams[0].ShaderVisibility         = D3D12_SHADER_VISIBILITY_ALL;
		stLocalRootParams[0].Constants.Num32BitValues = (UINT)Math::Upper(sizeof(SInstanceConstantBuffer), sizeof(uint32_t));
		stLocalRootParams[0].Constants.ShaderRegister = 1;
		stLocalRootParams[0].Constants.RegisterSpace  = 0;

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC st_local_root_signature_desc;
		st_local_root_signature_desc.Version                             = D3D_ROOT_SIGNATURE_VERSION_1_1;
		st_local_root_signature_desc.Desc_1_1.Flags                      = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
		st_local_root_signature_desc.Desc_1_1.NumParameters              = ARRAYSIZE(stLocalRootParams);
		st_local_root_signature_desc.Desc_1_1.pParameters                = stLocalRootParams;
		st_local_root_signature_desc.Desc_1_1.NumStaticSamplers          = 0;
		st_local_root_signature_desc.Desc_1_1.pStaticSamplers            = nullptr;

		hrRet = D3D12SerializeVersionedRootSignature(&st_local_root_signature_desc, &pIRSBlob, &pIRSErrMsg);

		if (FAILED(hrRet))
		{
			if (pIRSErrMsg)
			{
				printf("编译根签名出错：%s\n", (char*)pIRSErrMsg->GetBufferPointer());
				printf("编译根签名出错：%s\n", (char*)pIRSErrMsg->GetBufferPointer());
			}
			CHECK_RESULT(hrRet)
		}

		CHECK_RESULT(m_d3d12_device->CreateRootSignature(1 , pIRSBlob->GetBufferPointer() , pIRSBlob->GetBufferSize() , IID_PPV_ARGS(&local_rs)))
		m_root_signatures.emplace_back(global_rs);
		m_root_signatures.emplace_back(local_rs);

		return m_root_signatures.size() - 1;
	}

	void D3D12RHI::CreatePipelineStateObject()
	{
		ComPtr<ID3DBlob> blob_vertex_shader;
		ComPtr<ID3DBlob> blob_pixel_shader;
		ComPtr<ID3DBlob> error_blob;
#if defined(_DEBUG)
		//调试状态下，打开Shader编译的调试标志，不优化
		UINT nCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT nCompileFlags = 0;
#endif
		WCHAR psz_shader_file_name[MAX_PATH] = {};
		// StringCchPrintf(psz_shader_file_name, MAX_PATH, _T("%s")/*, pszAppPath*/, RHID3D12::SHADER_PATH);

		if (!SUCCEEDED(D3DCompileFromFile(psz_shader_file_name, nullptr, nullptr, "VSMain", "vs_5_0", nCompileFlags, 0, & blob_vertex_shader, &error_blob)))
		{
			CHAR psz_err_msg[MAX_PATH] = {};
			StringCchPrintfA(psz_err_msg, MAX_PATH, "\n%s\n", static_cast<CHAR*>(error_blob->GetBufferPointer()));
			::OutputDebugStringA(psz_err_msg);
		}
		if (!SUCCEEDED(D3DCompileFromFile(psz_shader_file_name, nullptr, nullptr, "PSMain", "ps_5_0", nCompileFlags, 0, & blob_pixel_shader, &error_blob)))
		{
			CHAR psz_err_msg[MAX_PATH] = {};
			StringCchPrintfA(psz_err_msg, MAX_PATH, "\n%s\n", static_cast<CHAR*>(error_blob->GetBufferPointer()));
			::OutputDebugStringA(psz_err_msg);
		}

		m_shader_cache.emplace_back(blob_vertex_shader);
		m_shader_cache.emplace_back(blob_pixel_shader);


		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC input_element_descs[] = {{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}, {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC st_pso_desc = {};
		st_pso_desc.InputLayout                        = {input_element_descs, _countof(input_element_descs)};
		st_pso_desc.pRootSignature                     = m_root_signatures[0].Get();
		st_pso_desc.VS.pShaderBytecode                 = blob_vertex_shader->GetBufferPointer();
		st_pso_desc.VS.BytecodeLength                  = blob_vertex_shader->GetBufferSize();
		st_pso_desc.PS.pShaderBytecode                 = blob_pixel_shader->GetBufferPointer();
		st_pso_desc.PS.BytecodeLength                  = blob_pixel_shader->GetBufferSize();

		st_pso_desc.RasterizerState.FillMode          = D3D12_FILL_MODE_WIREFRAME;
		st_pso_desc.RasterizerState.CullMode          = D3D12_CULL_MODE_NONE;
		st_pso_desc.RasterizerState.ForcedSampleCount = 16;

		st_pso_desc.BlendState.AlphaToCoverageEnable                 = FALSE;
		st_pso_desc.BlendState.IndependentBlendEnable                = FALSE;
		st_pso_desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		st_pso_desc.DepthStencilState.DepthEnable   = FALSE;
		st_pso_desc.DepthStencilState.StencilEnable = FALSE;

		st_pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

		st_pso_desc.NumRenderTargets = 1;
		st_pso_desc.RTVFormats[0]    = RHID3D12::RENDER_TARGET_FORMAT;

		st_pso_desc.SampleMask       = UINT_MAX;
		st_pso_desc.SampleDesc.Count = 1;

		ComPtr<ID3D12PipelineState> pipeline_state_point;
		CHECK_RESULT(m_d3d12_device->CreateGraphicsPipelineState(&st_pso_desc, IID_PPV_ARGS(&pipeline_state_point)));
		m_pipeline_states.emplace_back(pipeline_state_point);

		ComPtr<ID3D12PipelineState> pipeline_state_line;
		st_pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		CHECK_RESULT(m_d3d12_device->CreateGraphicsPipelineState(&st_pso_desc, IID_PPV_ARGS(&pipeline_state_line)));
		m_pipeline_states.emplace_back(pipeline_state_line);
	}

	void D3D12RHI::CreateRayTracingPipelineStateObject(const std::vector<uint8_t>& shader_byte_code)
	{
		std::vector<D3D12_STATE_SUBOBJECT> state_subobjects;
		state_subobjects.resize(8);
		size_t szIndex = 0;
		// Global Root Signature
		D3D12_STATE_SUBOBJECT st_sub_obj_global_rs;
		st_sub_obj_global_rs.Type                  = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
		st_sub_obj_global_rs.pDesc                 = m_root_signatures[0].GetAddressOf(); //GLOBAL_ROOT_SIGNATURE

		state_subobjects[szIndex++] = st_sub_obj_global_rs;

		// Raytracing Pipeline Config (主要设定递归深度，这里只有主光线/一次光线，设定为1)
		D3D12_RAYTRACING_PIPELINE_CONFIG st_pipeline_cfg;
		st_pipeline_cfg.MaxTraceRecursionDepth = 10;// MAX_RAY_RECURSION_DEPTH;

		D3D12_STATE_SUBOBJECT stSubObjPipelineCfg = {};
		stSubObjPipelineCfg.pDesc                 = &st_pipeline_cfg;
		stSubObjPipelineCfg.Type                  = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;

		//arSubObjects.Add(stSubObjPipelineCfg);

		state_subobjects[szIndex++] = stSubObjPipelineCfg;

		// DXIL Library 
		D3D12_EXPORT_DESC       stRayGenExportDesc[4] = {};
		D3D12_DXIL_LIBRARY_DESC stRayGenDxilLibDesc;

		stRayGenExportDesc[0].Name  = c_pszRaygenShaderName;
		stRayGenExportDesc[0].Flags = D3D12_EXPORT_FLAG_NONE;

		stRayGenExportDesc[1].Name  = c_pszClosestHitShaderName;
		stRayGenExportDesc[1].Flags = D3D12_EXPORT_FLAG_NONE;

		stRayGenExportDesc[2].Name  = c_missShaderNames[RayType::Radiance];
		stRayGenExportDesc[2].Flags = D3D12_EXPORT_FLAG_NONE;

		stRayGenExportDesc[3].Name = c_missShaderNames[RayType::Shadow];
		stRayGenExportDesc[3].Flags = D3D12_EXPORT_FLAG_NONE;

		stRayGenDxilLibDesc.NumExports                  = _countof(stRayGenExportDesc);
		stRayGenDxilLibDesc.pExports                    = stRayGenExportDesc;
		stRayGenDxilLibDesc.DXILLibrary.pShaderBytecode = shader_byte_code.data();
		stRayGenDxilLibDesc.DXILLibrary.BytecodeLength  = shader_byte_code.size();

		D3D12_STATE_SUBOBJECT stSubObjDXILLib = {};
		stSubObjDXILLib.Type                  = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
		stSubObjDXILLib.pDesc                 = &stRayGenDxilLibDesc;

		state_subobjects[szIndex++] = stSubObjDXILLib;

		// Hit Shader Table Group 
		{
			// radiance 
			D3D12_HIT_GROUP_DESC stHitGroupDesc = {};
			stHitGroupDesc.ClosestHitShaderImport = c_pszClosestHitShaderName;
			stHitGroupDesc.HitGroupExport = c_hitGroupNames_TriangleGeometry[RayType::Radiance];
			D3D12_STATE_SUBOBJECT hitGroupSubobject;
			hitGroupSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
			hitGroupSubobject.pDesc = &stHitGroupDesc;

			state_subobjects[szIndex++] = hitGroupSubobject;
		}
		{
			//shadows
			D3D12_HIT_GROUP_DESC stHitGroupDesc = {};
			stHitGroupDesc.ClosestHitShaderImport = c_pszClosestHitShaderName;
			stHitGroupDesc.HitGroupExport = c_hitGroupNames_TriangleGeometry[RayType::Shadow];
			D3D12_STATE_SUBOBJECT hitGroupSubobject;
			hitGroupSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
			hitGroupSubobject.pDesc = &stHitGroupDesc;

			state_subobjects[szIndex++] = hitGroupSubobject;
		}

		// Raytracing Shader Config (主要设定质心坐标结构体字节大小、TraceRay负载字节大小)
		D3D12_RAYTRACING_SHADER_CONFIG st_shader_cfg;
		st_shader_cfg.MaxAttributeSizeInBytes        = sizeof(float) * 2;
		st_shader_cfg.MaxPayloadSizeInBytes          = sizeof(float) * 8;

		D3D12_STATE_SUBOBJECT stShaderCfgStateObject;
		stShaderCfgStateObject.Type                  = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
		stShaderCfgStateObject.pDesc                 = &st_shader_cfg;

		state_subobjects[szIndex++] = stShaderCfgStateObject;

		// Local Root Signature
		D3D12_STATE_SUBOBJECT st_sub_obj_local_rs;
		st_sub_obj_local_rs.Type                  = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
		st_sub_obj_local_rs.pDesc                 = m_root_signatures[1].GetAddressOf();

		size_t szLocalObjIdx        = szIndex;
		state_subobjects[szIndex++] = st_sub_obj_local_rs;

		// Local Root Signature to Exports Association
		// (主要设定局部根签名与命中Shader函数组的关系,即命中Shader函数组可访问的Shader全局变量)
		D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION stObj2ExportsAssoc = {};
		stObj2ExportsAssoc.pSubobjectToAssociate                  = &state_subobjects[szLocalObjIdx];
		stObj2ExportsAssoc.NumExports                             = _countof(c_hitGroupNames_TriangleGeometry);
		stObj2ExportsAssoc.pExports                               = c_hitGroupNames_TriangleGeometry;

		D3D12_STATE_SUBOBJECT stSubObjAssoc;
		stSubObjAssoc.Type                  = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		stSubObjAssoc.pDesc                 = &stObj2ExportsAssoc;

		state_subobjects[szIndex++] = stSubObjAssoc;

		// 最后填充State Object的结构体 并创建Raytracing PSO
		D3D12_STATE_OBJECT_DESC stRaytracingPSOdesc;
		stRaytracingPSOdesc.Type                    = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
		stRaytracingPSOdesc.NumSubobjects           = (UINT)state_subobjects.size();
		stRaytracingPSOdesc.pSubobjects             = state_subobjects.data();

		// 创建管线状态对象
		CHECK_RESULT(m_d3d12_device->CreateStateObject( &stRaytracingPSOdesc , IID_PPV_ARGS(&m_raytracing_state_object)))
	}

	void D3D12RHI::CreateRenderEndFence()
	{
		CHECK_RESULT(m_d3d12_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_render_end_fence.m_fence)));
		m_render_end_fence.m_fence_value = 1u;

		// 创建一个Event同步对象，用于等待围栏事件通知
		m_render_end_fence.m_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_render_end_fence.m_fence_event == nullptr)
		{
			CHECK_RESULT(HRESULT_FROM_WIN32(GetLastError()));
		}
	}

	void D3D12RHI::InitializeSampler()
	{
		//创建采样器描述符堆
		D3D12_DESCRIPTOR_HEAP_DESC stSamplerHeapDesc = {};
		stSamplerHeapDesc.NumDescriptors             = 6;
		stSamplerHeapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		stSamplerHeapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		CHECK_RESULT(m_d3d12_device->CreateDescriptorHeap( &stSamplerHeapDesc , IID_PPV_ARGS(&m_sampler_heap)));

		//创建采样器
		D3D12_SAMPLER_DESC stSamplerDesc = {};
		stSamplerDesc.Filter             = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		stSamplerDesc.MinLOD             = 0;
		stSamplerDesc.MaxLOD             = D3D12_FLOAT32_MAX;
		stSamplerDesc.MipLODBias         = 0.0f;
		stSamplerDesc.MaxAnisotropy      = 1;
		stSamplerDesc.ComparisonFunc     = D3D12_COMPARISON_FUNC_NEVER;
		stSamplerDesc.AddressU           = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		stSamplerDesc.AddressV           = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		stSamplerDesc.AddressW           = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

		m_d3d12_device->CreateSampler(&stSamplerDesc, m_sampler_heap->GetCPUDescriptorHandleForHeapStart());
	}

	void D3D12RHI::CreateRayTracingRenderTargetUAV()
	{
		D3D12_RESOURCE_DESC stTexUADesc = m_render_targets[0]->GetDesc();
		stTexUADesc.Flags               = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		D3D12_HEAP_PROPERTIES st_default_heap_props;
		st_default_heap_props.Type                  = D3D12_HEAP_TYPE_DEFAULT;
		st_default_heap_props.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		st_default_heap_props.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
		st_default_heap_props.CreationNodeMask      = 0;
		st_default_heap_props.VisibleNodeMask       = 0;

		CHECK_RESULT(m_d3d12_device->CreateCommittedResource( &st_default_heap_props , D3D12_HEAP_FLAG_NONE , &stTexUADesc , D3D12_RESOURCE_STATE_UNORDERED_ACCESS , nullptr , IID_PPV_ARGS(&m_raytracing_render_targets)));

	}

	void D3D12RHI::CreateShaderTable()
	{
		D3D12_RESOURCE_DESC stBufferResSesc = {};
		stBufferResSesc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
		stBufferResSesc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		stBufferResSesc.Flags               = D3D12_RESOURCE_FLAG_NONE;
		stBufferResSesc.Format              = DXGI_FORMAT_UNKNOWN;
		stBufferResSesc.Height              = 1;
		stBufferResSesc.DepthOrArraySize    = 1;
		stBufferResSesc.MipLevels           = 1;
		stBufferResSesc.SampleDesc.Count    = 1;
		stBufferResSesc.SampleDesc.Quality  = 0;

		// Get shader identifiers.
		ComPtr<ID3D12StateObjectProperties> pIDXRStateObjectProperties;
		CHECK_RESULT(m_raytracing_state_object.As(&pIDXRStateObjectProperties));

		const void* ray_gen_shader_identifier   = pIDXRStateObjectProperties->GetShaderIdentifier(c_pszRaygenShaderName);
		const void* radiance_miss_shader_identifier      = pIDXRStateObjectProperties->GetShaderIdentifier(c_missShaderNames[RayType::Radiance]);
		const void* shadow_miss_shader_identifier      = pIDXRStateObjectProperties->GetShaderIdentifier(c_missShaderNames[RayType::Shadow]);
		const void* radiance_hit_group_shader_identifier = pIDXRStateObjectProperties->GetShaderIdentifier(c_hitGroupNames_TriangleGeometry[RayType::Radiance]);
		const void* shadow_hit_group_shader_identifier = pIDXRStateObjectProperties->GetShaderIdentifier(c_hitGroupNames_TriangleGeometry[RayType::Shadow]);
		const UINT  shader_identifier_size      = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

		D3D12_HEAP_DESC upload_heap_desc = {};
		UINT64          n64_heap_size      = 1llu * 1024llu * 1024llu; //分配1M的堆 这里足够放三个Shader Table即可
		UINT64          n64_heap_offset    = 0;                        //堆上的偏移
		UINT64          n64AllocSize     = 0;
		UINT8*          pBufs            = nullptr;
		D3D12_RANGE     stReadRange      = {0, 0};

		upload_heap_desc.SizeInBytes = Math::Upper(n64_heap_size, (UINT64)D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT); //64K边界对齐大小
		//注意上传堆肯定是Buffer类型，可以不指定对齐方式，其默认是64k边界对齐
		upload_heap_desc.Alignment                       = 0;
		upload_heap_desc.Properties.Type                 = D3D12_HEAP_TYPE_UPLOAD; //上传堆类型
		upload_heap_desc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		upload_heap_desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		//上传堆就是缓冲，可以摆放任意数据
		upload_heap_desc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

		//创建用于缓冲Shader Table的Heap，这里使用的是自定义上传堆
		CHECK_RESULT(m_d3d12_device->CreateHeap(&upload_heap_desc , IID_PPV_ARGS(&m_shader_table_heap)));

		//注意分配尺寸对齐是32字节上对齐，否则纯DXR方式运行会报错
		UINT64 nAlignment     = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		UINT64 nSizeAlignment = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;

		// Ray gen shader table
		{
			UINT64 nNumShaderRecords = 1;
			UINT64 nShaderRecordSize = shader_identifier_size;

			n64AllocSize          = nNumShaderRecords * nShaderRecordSize;
			n64AllocSize          = Math::Upper(n64AllocSize, nSizeAlignment);
			stBufferResSesc.Width = n64AllocSize;

			CHECK_RESULT(m_d3d12_device->CreatePlacedResource( m_shader_table_heap.Get() , n64_heap_offset , &stBufferResSesc , D3D12_RESOURCE_STATE_GENERIC_READ , nullptr , IID_PPV_ARGS(&m_ray_gen_shader_table) ));
			m_ray_gen_shader_table->SetName(L"RayGenShaderTable");

			CHECK_RESULT(m_ray_gen_shader_table->Map( 0 , &stReadRange , reinterpret_cast<void**>(&pBufs)));

			::memcpy(pBufs, ray_gen_shader_identifier, shader_identifier_size);

			m_ray_gen_shader_table->Unmap(0, nullptr);
		}

		n64_heap_offset += Math::Upper(n64AllocSize, nAlignment); //向上64k边界对齐准备下一个分配
		CHECK_RESULT(n64_heap_offset < n64_heap_size);

		// Miss shader table
		{
			UINT64 nNumShaderRecords = 1;
			UINT64 nShaderRecordSize = shader_identifier_size;
			n64AllocSize             = nNumShaderRecords * nShaderRecordSize;
			n64AllocSize             = Math::Upper(n64AllocSize, nSizeAlignment);

			stBufferResSesc.Width = n64AllocSize;

			CHECK_RESULT(m_d3d12_device->CreatePlacedResource( m_shader_table_heap.Get() , n64_heap_offset , &stBufferResSesc , D3D12_RESOURCE_STATE_GENERIC_READ , nullptr , IID_PPV_ARGS(&m_miss_shader_table) ));
			m_miss_shader_table->SetName(L"MissShaderTable");
			pBufs = nullptr;

			CHECK_RESULT(m_miss_shader_table->Map( 0 , &stReadRange , reinterpret_cast<void**>(&pBufs)));

			::memcpy(pBufs, radiance_miss_shader_identifier, shader_identifier_size);

			pBufs += shader_identifier_size;
			::memcpy(pBufs, shadow_miss_shader_identifier, shader_identifier_size);

			m_miss_shader_table->Unmap(0, nullptr);
		}

		//D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT
		n64_heap_offset += Math::Upper(n64AllocSize, nAlignment); //向上64k边界对齐准备下一个分配
		CHECK_RESULT(n64_heap_offset < n64_heap_size);

		// Hit group shader table
		{
			UINT64                  nNumShaderRecords = 1;
			SInstanceConstantBuffer instance_constant_buffer;
			instance_constant_buffer.m_vAlbedo[0] = 1.0f;
			instance_constant_buffer.m_vAlbedo[1] = 1.0f;
			instance_constant_buffer.m_vAlbedo[2] = 1.0f;
			instance_constant_buffer.m_vAlbedo[3] = 1.0f;
			UINT64 nShaderRecordSize              = shader_identifier_size + sizeof(instance_constant_buffer);

			n64AllocSize = nNumShaderRecords * nShaderRecordSize * 2;
			n64AllocSize = Math::Upper(n64AllocSize, nSizeAlignment);

			stBufferResSesc.Width = n64AllocSize;

			CHECK_RESULT(m_d3d12_device->CreatePlacedResource( m_shader_table_heap.Get() , n64_heap_offset , &stBufferResSesc , D3D12_RESOURCE_STATE_GENERIC_READ , nullptr , IID_PPV_ARGS(&m_hit_group_shader_table) ));
			m_hit_group_shader_table->SetName(L"HitGroupShaderTable");
			pBufs = nullptr;

			CHECK_RESULT(m_hit_group_shader_table->Map( 0 , &stReadRange , reinterpret_cast<void**>(&pBufs)));

			//复制Shader Identifier
			::memcpy(pBufs, radiance_hit_group_shader_identifier, shader_identifier_size);

			pBufs = static_cast<BYTE*>(pBufs) + shader_identifier_size;
			//复制局部的参数，也就是Local Root Signature标识的局部参数
			::memcpy(pBufs, &instance_constant_buffer, sizeof(instance_constant_buffer));

			pBufs = static_cast<BYTE*>(pBufs) + sizeof(instance_constant_buffer);
			::memcpy(pBufs, shadow_hit_group_shader_identifier, shader_identifier_size);

			pBufs = static_cast<BYTE*>(pBufs) + shader_identifier_size;
			::memcpy(pBufs, &instance_constant_buffer, sizeof(instance_constant_buffer));

			m_hit_group_shader_table->Unmap(0, nullptr);
		}
	}

	void D3D12RHI::CreateTexture(std::vector<uint8_t>& texture_data, uint32_t width, uint32_t height)
	{
		ComPtr<ID3D12Resource> tex_upload_res;
		ComPtr<ID3D12Resource> tex_res;

		D3D12_HEAP_PROPERTIES stTextureHeapProp = {};
		stTextureHeapProp.Type                  = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_DESC stTextureDesc = {};

		stTextureDesc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		stTextureDesc.MipLevels          = 1;
		stTextureDesc.Format             = DXGI_FORMAT_B8G8R8A8_UNORM;
		stTextureDesc.Width              = width;
		stTextureDesc.Height             = height;
		stTextureDesc.Flags              = D3D12_RESOURCE_FLAG_NONE;
		stTextureDesc.DepthOrArraySize   = 1;
		stTextureDesc.SampleDesc.Count   = 1;
		stTextureDesc.SampleDesc.Quality = 0;

		CHECK_RESULT(m_d3d12_device->CreateCommittedResource( &stTextureHeapProp , D3D12_HEAP_FLAG_NONE , &stTextureDesc //可以使用CD3DX12_RESOURCE_DESC::Tex2D来简化结构体的初始化
			, D3D12_RESOURCE_STATE_COPY_DEST , nullptr , IID_PPV_ARGS(&tex_res)));

		//获取需要的上传堆资源缓冲的大小，这个尺寸通常大于实际图片的尺寸
		D3D12_RESOURCE_DESC stDestDesc          = tex_res->GetDesc();
		UINT64              n64UploadBufferSize = 0;
		m_d3d12_device->GetCopyableFootprints(&stDestDesc, 0, 1, 0, nullptr, nullptr, nullptr, &n64UploadBufferSize);

		stTextureHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC stUploadTextureDesc = {};

		stUploadTextureDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
		stUploadTextureDesc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		stUploadTextureDesc.Width              = n64UploadBufferSize;
		stUploadTextureDesc.Height             = 1;
		stUploadTextureDesc.DepthOrArraySize   = 1;
		stUploadTextureDesc.MipLevels          = 1;
		stUploadTextureDesc.Format             = DXGI_FORMAT_UNKNOWN;
		stUploadTextureDesc.SampleDesc.Count   = 1;
		stUploadTextureDesc.SampleDesc.Quality = 0;
		stUploadTextureDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		stUploadTextureDesc.Flags              = D3D12_RESOURCE_FLAG_NONE;

		CHECK_RESULT(m_d3d12_device->CreateCommittedResource( &stTextureHeapProp , D3D12_HEAP_FLAG_NONE , &stUploadTextureDesc , D3D12_RESOURCE_STATE_GENERIC_READ , nullptr , IID_PPV_ARGS(&tex_upload_res)));

		UINT                               nNumSubresources   = 1u; //我们只有一副图片，即子资源个数为1
		UINT                               nTextureRowNum     = 0u;
		UINT64                             n64TextureRowSizes = 0u;
		UINT64                             n64RequiredSize    = 0u;
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT stTxtLayouts       = {};

		m_d3d12_device->GetCopyableFootprints(&stDestDesc, 0, nNumSubresources, 0, &stTxtLayouts, &nTextureRowNum, &n64TextureRowSizes, &n64RequiredSize);

		BYTE* pData = nullptr;
		CHECK_RESULT(tex_upload_res->Map(0, NULL, reinterpret_cast<void**>(&pData)));

		BYTE*       dest_slice   = reinterpret_cast<BYTE*>(pData) + stTxtLayouts.Offset;
		auto        image_data  = texture_data.data();
		uint64_t    pic_row_pitch = (uint64_t)width * 4;
		const BYTE* src_slice    = reinterpret_cast<const BYTE*>(image_data);
		for (UINT y = 0; y < nTextureRowNum; ++y)
		{
			memcpy(dest_slice + static_cast<SIZE_T>(stTxtLayouts.Footprint.RowPitch) * y, src_slice + pic_row_pitch * y, pic_row_pitch);
		}
		tex_upload_res->Unmap(0, nullptr);

		D3D12_TEXTURE_COPY_LOCATION stDstCopyLocation = {};
		stDstCopyLocation.pResource                   = tex_res.Get();
		stDstCopyLocation.Type                        = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		stDstCopyLocation.SubresourceIndex            = 0;

		D3D12_TEXTURE_COPY_LOCATION stSrcCopyLocation = {};
		stSrcCopyLocation.pResource                   = tex_upload_res.Get();
		stSrcCopyLocation.Type                        = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		stSrcCopyLocation.PlacedFootprint             = stTxtLayouts;

		CHECK_RESULT(m_cmd_allocator->Reset())
		CHECK_RESULT(m_cmd_list->Reset(m_cmd_allocator.Get(), nullptr))
		m_cmd_list->CopyTextureRegion(&stDstCopyLocation, 0, 0, 0, &stSrcCopyLocation, nullptr);

		D3D12_RESOURCE_BARRIER stResBar = {};
		stResBar.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		stResBar.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		stResBar.Transition.pResource   = tex_res.Get();
		stResBar.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		stResBar.Transition.StateAfter  = D3D12_RESOURCE_STATE_COMMON;
		stResBar.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		m_cmd_list->ResourceBarrier(1, &stResBar);


		CHECK_RESULT(m_cmd_list->Close())


		//执行命令列表
		ID3D12CommandList* ppCommandLists[] = {m_cmd_list.Get()};
		m_cmd_queue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		//开始同步GPU与CPU的执行，先记录围栏标记值
		const UINT64 n64CurrentFenceValue = m_render_end_fence.m_fence_value;
		CHECK_RESULT(m_cmd_queue->Signal(m_render_end_fence.m_fence.Get(), n64CurrentFenceValue));
		m_render_end_fence.m_fence_value++;
		CHECK_RESULT(m_render_end_fence.m_fence->SetEventOnCompletion(n64CurrentFenceValue, m_render_end_fence.m_fence_event));

		WaitForSingleObject(m_render_end_fence.m_fence_event, INFINITE);
		m_textures.emplace_back(tex_upload_res);
		m_textures.emplace_back(tex_res);
	}

	void D3D12RHI::WaitForFence() const
	{
		WaitForSingleObject(m_render_end_fence.m_fence_event, INFINITE);
	}

	void D3D12RHI::DoRayTracing()
	{
		//获取新的后缓冲序号，因为Present真正完成时后缓冲的序号就更新了
		m_frame_index = m_swap_chain4->GetCurrentBackBufferIndex();

		//命令分配器先Reset一下
		HRESULT h = m_cmd_allocator->Reset();
		if (!SUCCEEDED(h))
		{
			printf("[ERROR] Failed Reset CMD Allocator!\n");
		}
		// CHECK_RESULT(h);
		//Reset命令列表，并重新指定命令分配器和PSO对象
		h = m_cmd_list->Reset(m_cmd_allocator.Get(), nullptr);
		if (!SUCCEEDED(h))
		{
			printf("[ERROR] Failed Reset CMD List!\n");
		}

		{// 记录绘制指令
			D3D12_DISPATCH_RAYS_DESC stDispatchRayDesc    = {};
			stDispatchRayDesc.HitGroupTable.StartAddress  = m_hit_group_shader_table->GetGPUVirtualAddress();
			stDispatchRayDesc.HitGroupTable.SizeInBytes   = m_hit_group_shader_table->GetDesc().Width;
			stDispatchRayDesc.HitGroupTable.StrideInBytes = stDispatchRayDesc.HitGroupTable.SizeInBytes;

			stDispatchRayDesc.MissShaderTable.StartAddress  = m_miss_shader_table->GetGPUVirtualAddress();
			stDispatchRayDesc.MissShaderTable.SizeInBytes   = m_miss_shader_table->GetDesc().Width;
			stDispatchRayDesc.MissShaderTable.StrideInBytes = stDispatchRayDesc.MissShaderTable.SizeInBytes;

			stDispatchRayDesc.RayGenerationShaderRecord.StartAddress = m_ray_gen_shader_table->GetGPUVirtualAddress();
			stDispatchRayDesc.RayGenerationShaderRecord.SizeInBytes  = m_ray_gen_shader_table->GetDesc().Width;

			stDispatchRayDesc.Width  = m_swap_chain_size[0];
			stDispatchRayDesc.Height = m_swap_chain_size[1];
			stDispatchRayDesc.Depth  = 1;

			auto& global_rs = m_root_signatures[0];
			m_cmd_list->SetComputeRootSignature(global_rs.Get());
			m_cmd_list->SetPipelineState1(m_raytracing_state_object.Get());

			ID3D12DescriptorHeap* ppDescriptorHeaps[] = {m_srv_cbv_uav_heap.Get(), m_sampler_heap.Get()};
			m_cmd_list->SetDescriptorHeaps(_countof(ppDescriptorHeaps), ppDescriptorHeaps);

			D3D12_GPU_DESCRIPTOR_HANDLE objUAVHandle = m_srv_cbv_uav_heap->GetGPUDescriptorHandleForHeapStart();
			objUAVHandle.ptr += (m_ray_tracing_rt_uav * m_srv_cbv_uav_descriptor_size);;

			m_cmd_list->SetComputeRootDescriptorTable(0, objUAVHandle);

			m_cmd_list->SetComputeRootShaderResourceView(1, m_top_level_acceleration_structure->GetGPUVirtualAddress());

			m_cmd_list->SetComputeRootConstantBufferView(2, m_scene_constant_buffer.rhi_buffer->GetGPUVirtualAddress());

			// 设置Index和Vertex的描述符堆句柄
			D3D12_GPU_DESCRIPTOR_HANDLE objIBHandle = m_srv_cbv_uav_heap->GetGPUDescriptorHandleForHeapStart();
			objIBHandle.ptr += (c_nDSHIndxIBView * m_srv_cbv_uav_descriptor_size);

			m_cmd_list->SetComputeRootDescriptorTable(3, objIBHandle);

			//设置两个纹理的描述符堆句柄
			D3D12_GPU_DESCRIPTOR_HANDLE objTxtureHandle = m_srv_cbv_uav_heap->GetGPUDescriptorHandleForHeapStart();
			objTxtureHandle.ptr += (c_nDSNIndxTexture * m_srv_cbv_uav_descriptor_size);

			m_cmd_list->SetComputeRootDescriptorTable(4, objTxtureHandle);

			m_cmd_list->SetComputeRootDescriptorTable(5, m_sampler_heap->GetGPUDescriptorHandleForHeapStart());

			m_cmd_list->DispatchRays(&stDispatchRayDesc);
		}
		{
			D3D12_RESOURCE_BARRIER preCopyBarriers[2] = {};

			preCopyBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			preCopyBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			preCopyBarriers[0].Transition.pResource = m_render_targets[m_frame_index].Get();
			preCopyBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			preCopyBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
			preCopyBarriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			preCopyBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			preCopyBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			preCopyBarriers[1].Transition.pResource = m_raytracing_render_targets.Get();
			preCopyBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			preCopyBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
			preCopyBarriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			m_cmd_list->ResourceBarrier(ARRAYSIZE(preCopyBarriers), preCopyBarriers);

			m_cmd_list->CopyResource(m_render_targets[m_frame_index].Get(), m_raytracing_render_targets.Get());

			D3D12_RESOURCE_BARRIER postCopyBarriers[2] = {};

			postCopyBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			postCopyBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			postCopyBarriers[0].Transition.pResource = m_render_targets[m_frame_index].Get();
			postCopyBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			postCopyBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
			postCopyBarriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			postCopyBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			postCopyBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			postCopyBarriers[1].Transition.pResource = m_raytracing_render_targets.Get();
			postCopyBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
			postCopyBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			postCopyBarriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			m_cmd_list->ResourceBarrier(ARRAYSIZE(postCopyBarriers), postCopyBarriers);

			//关闭命令列表，可以去执行了
			CHECK_RESULT(m_cmd_list->Close());

			//执行命令列表
			ID3D12CommandList* ppCommandLists[] = { m_cmd_list.Get() };
			m_cmd_queue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

			//提交画面
			CHECK_RESULT(m_swap_chain4->Present(1, 0));

			//---------------------------------------------------------------------------------------------
			//开始同步GPU与CPU的执行，先记录围栏标记值
			const UINT64 n64CurFenceValue = m_render_end_fence.m_fence_value;
			CHECK_RESULT(m_cmd_queue->Signal(m_render_end_fence.m_fence.Get(), n64CurFenceValue));
			m_render_end_fence.m_fence_value++;
			CHECK_RESULT(m_render_end_fence.m_fence->SetEventOnCompletion(n64CurFenceValue, m_render_end_fence.m_fence_event));
		}
	}


	TCHAR* D3D12RHI::GetCurrentAdapterName()
	{
		return m_adapter_name;
	}

	void D3D12RHI::CreatePrimitives(const std::vector<SVertexInstance>& vertex_vector, const std::vector<IndexType>& vertex_indices)
	{
		auto& primitive = m_render_primitives.emplace_back();
		{
			ComPtr<ID3D12Resource2> vertex_buffer;
			const UINT64            vertex_buffer_size = vertex_vector.size() * sizeof(SVertexInstance);
			D3D12_HEAP_PROPERTIES   heap_prop          = {D3D12_HEAP_TYPE_UPLOAD};
			D3D12_RESOURCE_DESC     resource_desc      = {};
			resource_desc.Dimension                    = D3D12_RESOURCE_DIMENSION_BUFFER;
			resource_desc.Layout                       = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			resource_desc.Flags                        = D3D12_RESOURCE_FLAG_NONE;
			resource_desc.Format                       = DXGI_FORMAT_UNKNOWN;
			resource_desc.Width                        = vertex_buffer_size;
			resource_desc.Height                       = 1;
			resource_desc.DepthOrArraySize             = 1;
			resource_desc.MipLevels                    = 1;
			resource_desc.SampleDesc.Count             = 1;
			resource_desc.SampleDesc.Quality           = 0;

			CHECK_RESULT(m_d3d12_device->CreateCommittedResource(&heap_prop, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertex_buffer)));

			UINT8*      data_begin = nullptr;
			D3D12_RANGE read_range = {0, 0};
			(vertex_buffer->Map(0, &read_range, reinterpret_cast<void**>(&data_begin)));

			memcpy(data_begin, vertex_vector.data(), vertex_buffer_size);

			vertex_buffer->Unmap(0, nullptr);
			primitive.m_vertex_buffer = vertex_buffer;
			primitive.m_vertex_count  = static_cast<uint32_t>(vertex_vector.size());
			primitive.m_vertex_stride = sizeof(SVertexInstance);
		}
		{
			ComPtr<ID3D12Resource2> index_buffer;
			const UINT64            index_buffer_size = vertex_indices.size() * sizeof(IndexType);
			D3D12_HEAP_PROPERTIES   heap_prop         = {D3D12_HEAP_TYPE_UPLOAD};
			D3D12_RESOURCE_DESC     resource_desc     = {};
			resource_desc.Dimension                   = D3D12_RESOURCE_DIMENSION_BUFFER;
			resource_desc.Layout                      = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			resource_desc.Flags                       = D3D12_RESOURCE_FLAG_NONE;
			resource_desc.Format                      = DXGI_FORMAT_UNKNOWN;
			resource_desc.Width                       = index_buffer_size;
			resource_desc.Height                      = 1;
			resource_desc.DepthOrArraySize            = 1;
			resource_desc.MipLevels                   = 1;
			resource_desc.SampleDesc.Count            = 1;
			resource_desc.SampleDesc.Quality          = 0;

			CHECK_RESULT(m_d3d12_device->CreateCommittedResource(&heap_prop, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&index_buffer)));

			UINT8*      data_begin = nullptr;
			D3D12_RANGE read_range = {0, 0};
			(index_buffer->Map(0, &read_range, reinterpret_cast<void**>(&data_begin)));

			memcpy(data_begin, vertex_indices.data(), index_buffer_size);

			index_buffer->Unmap(0, nullptr);
			primitive.m_index_buffer = index_buffer;
			primitive.m_index_count  = static_cast<uint32_t>(vertex_indices.size());
			primitive.m_index_stride = sizeof(IndexType);
		}
	}

	void D3D12RHI::CreatePrimitives(const std::vector<CMesh*>& meshes)
	{
		uint64_t total_vertex_count = 0;
		uint64_t total_index_count = 0;
		std::vector<SGeometryDesc> geometry_descs;
		geometry_descs.reserve(meshes.size());
		for (auto& mesh : meshes)
		{
			auto& geometry = geometry_descs.emplace_back();
			geometry.vertex_offset = total_vertex_count;
			geometry.index_offset = total_index_count;
			geometry.vertex_count = mesh->m_vretices.size();
			geometry.index_count = mesh->m_indices.size();
			geometry.material_index = mesh->material;
			total_vertex_count += mesh->m_vretices.size();
			total_index_count += mesh->m_indices.size();

		}

		auto& primitive = m_render_primitives.emplace_back();
		primitive.m_index_count = total_index_count;
		primitive.m_vertex_count = total_vertex_count;

		ComPtr<ID3D12Resource2> vertex_buffer;
		const UINT64            vertex_buffer_size = total_vertex_count * sizeof(SVertexInstance);
		D3D12_HEAP_PROPERTIES   heap_prop = { D3D12_HEAP_TYPE_UPLOAD };
		D3D12_RESOURCE_DESC     resource_desc = {};
		resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		resource_desc.Format = DXGI_FORMAT_UNKNOWN;
		resource_desc.Width = vertex_buffer_size;
		resource_desc.Height = 1;
		resource_desc.DepthOrArraySize = 1;
		resource_desc.MipLevels = 1;
		resource_desc.SampleDesc.Count = 1;
		resource_desc.SampleDesc.Quality = 0;

		CHECK_RESULT(m_d3d12_device->CreateCommittedResource(&heap_prop, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertex_buffer)));

		UINT8* vetex_data_begin = nullptr;
		D3D12_RANGE read_range = { 0, 0 };
		(vertex_buffer->Map(0, &read_range, reinterpret_cast<void**>(&vetex_data_begin)));

		
		primitive.m_vertex_buffer = vertex_buffer;
		primitive.m_vertex_stride = sizeof(SVertexInstance);
	
		ComPtr<ID3D12Resource2> index_buffer;
		const UINT64            index_buffer_size = total_index_count * sizeof(IndexType);
		resource_desc.Width = index_buffer_size;

		CHECK_RESULT(m_d3d12_device->CreateCommittedResource(&heap_prop, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&index_buffer)));

		UINT8* index_data_begin = nullptr;
		index_buffer->Map(0, &read_range, reinterpret_cast<void**>(&index_data_begin));


		primitive.m_index_buffer = index_buffer;
		primitive.m_index_stride = sizeof(IndexType);


		for (int64_t i = 0; i < meshes.size(); ++i)
		{
			auto mesh = meshes[i];
			auto& desc = geometry_descs[i];
			memcpy(vetex_data_begin + desc.vertex_offset * sizeof(SVertexInstance), mesh->m_vretices.data(), mesh->m_vretices.size() * sizeof(SVertexInstance));
			memcpy(index_data_begin + desc.index_offset * sizeof(IndexType), mesh->m_indices.data(), mesh->m_indices.size() * sizeof(IndexType));

		}

		vertex_buffer->Unmap(0, nullptr);
		index_buffer->Unmap(0, nullptr);


		{
			ComPtr<ID3D12Resource2> geometry_desc_resource;

			resource_desc.Width = geometry_descs.size() * sizeof(SGeometryDesc);
			CHECK_RESULT(m_d3d12_device->CreateCommittedResource(&heap_prop, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&geometry_desc_resource)));
			UINT8* desc_data_begin = nullptr;
			geometry_desc_resource->Map(0, &read_range, reinterpret_cast<void**>(&desc_data_begin));
			memcpy(desc_data_begin, geometry_descs.data(), resource_desc.Width);
			geometry_desc_resource->Unmap(0, nullptr);
			primitive.m_geometry_descs = geometry_desc_resource;
		}
		primitive.m_geometry_descs_cpu = std::move(geometry_descs);
	}

	void D3D12RHI::CreateMaterials()
	{
		std::vector<SMaterial> materials;

		materials.reserve(4);
		{//red
			auto& mat = materials.emplace_back();
			mat.emission[0] = 0.f;
			mat.emission[1] = 0.f;
			mat.emission[2] = 0.f;
			mat.emission[3] = 0.f;

			mat.kd[0] = 0.63f;
			mat.kd[1] = 0.065f;
			mat.kd[2] = 0.05f;

			mat.ks[0] = 0.1f;
			mat.ks[1] = 0.1f;
			mat.ks[2] = 0.1f;

			mat.specular_exponent = 50.f;
		}
		{//green
			auto& mat = materials.emplace_back();
			mat.emission[0] = 0.f;
			mat.emission[1] = 0.f;
			mat.emission[2] = 0.f;
			mat.emission[3] = 0.f;

			mat.kd[0] = 0.14f;
			mat.kd[1] = 0.45f;
			mat.kd[2] = 0.091f;

			mat.ks[0] = 0.1f;
			mat.ks[1] = 0.1f;
			mat.ks[2] = 0.1f;

			mat.specular_exponent = 50.f;
		}
		{//white
			auto& mat = materials.emplace_back();
			mat.emission[0] = 0.f;
			mat.emission[1] = 0.f;
			mat.emission[2] = 0.f;
			mat.emission[3] = 0.f;

			mat.kd[0] = 0.725f;
			mat.kd[1] = 0.71f;
			mat.kd[2] = 0.68f;

			mat.ks[0] = 0.1f;
			mat.ks[1] = 0.1f;
			mat.ks[2] = 0.1f;

			mat.specular_exponent = 50.f;
		}
		{//light
			auto& mat = materials.emplace_back();
			mat.emission[0] = 8.0f * (0.747f + 0.058f) + 15.6f * (0.740f + 0.287f) + 18.4f * (0.737f + 0.642f);
			mat.emission[1] = 8.0f * (0.747f + 0.258f) + 15.6f * (0.740f + 0.160f) + 18.4f * (0.737f + 0.159f);
			mat.emission[2] = 8.0f * (0.747f) + 15.6f * (0.740f) + 18.4f * (0.737f);
			mat.emission[3] = 1.f;

			mat.kd[0] = 0.65f;
			mat.kd[1] = 0.65f;
			mat.kd[2] = 0.65f;

			mat.ks[0] = 0.1f;
			mat.ks[1] = 0.1f;
			mat.ks[2] = 0.1f;

			mat.specular_exponent = 50.f;
		}
		D3D12_HEAP_PROPERTIES   heap_prop = { D3D12_HEAP_TYPE_UPLOAD };
		D3D12_RESOURCE_DESC     resource_desc = {};
		resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		resource_desc.Format = DXGI_FORMAT_UNKNOWN;
		resource_desc.Height = 1;
		resource_desc.DepthOrArraySize = 1;
		resource_desc.MipLevels = 1;
		resource_desc.SampleDesc.Count = 1;
		resource_desc.SampleDesc.Quality = 0;

		resource_desc.Width = materials.size() * sizeof(SMaterial);
		CHECK_RESULT(m_d3d12_device->CreateCommittedResource(&heap_prop, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_materials)));
		UINT8* desc_data_begin = nullptr;
		D3D12_RANGE read_range = { 0, 0 };
		m_materials->Map(0, &read_range, reinterpret_cast<void**>(&desc_data_begin));
		memcpy(desc_data_begin, materials.data(), resource_desc.Width);
		m_materials->Unmap(0, nullptr);

		m_materials_cpu = std::move(materials);
	}

	void D3D12RHI::CreateSceneConstantBuffer()
	{
		D3D12_RESOURCE_DESC stBufferResSesc = {};
		stBufferResSesc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
		stBufferResSesc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		stBufferResSesc.Flags               = D3D12_RESOURCE_FLAG_NONE;
		stBufferResSesc.Format              = DXGI_FORMAT_UNKNOWN;
		stBufferResSesc.Height              = 1;
		stBufferResSesc.DepthOrArraySize    = 1;
		stBufferResSesc.MipLevels           = 1;
		stBufferResSesc.SampleDesc.Count    = 1;
		stBufferResSesc.SampleDesc.Quality  = 0;

		D3D12_HEAP_PROPERTIES st_upload_heap_props;
		st_upload_heap_props.Type                  = D3D12_HEAP_TYPE_UPLOAD;
		st_upload_heap_props.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		st_upload_heap_props.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
		st_upload_heap_props.CreationNodeMask      = 0;
		st_upload_heap_props.VisibleNodeMask       = 0;

		size_t size_in_bytes = Math::Upper(sizeof(SSceneConstantBuffer), (uint64_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

		stBufferResSesc.Width = size_in_bytes;
		CHECK_RESULT(m_d3d12_device->CreateCommittedResource( &st_upload_heap_props , D3D12_HEAP_FLAG_NONE , &stBufferResSesc , D3D12_RESOURCE_STATE_GENERIC_READ , nullptr , IID_PPV_ARGS(&m_scene_constant_buffer.rhi_buffer)));
		m_scene_constant_buffer.size_in_bytes = size_in_bytes;
	}

	void D3D12RHI::UpdateSceneConstantBuffer(SSceneConstantBuffer* data, uint64_t size)
	{
		void* data_ptr = nullptr;
		m_scene_constant_buffer.rhi_buffer->Map(0, nullptr, &data_ptr);
		memcpy(data_ptr, data, size);
		m_scene_constant_buffer.rhi_buffer->Unmap(0, nullptr);
	}

	void D3D12RHI::CreateInstanceConstantBuffer()
	{
		const D3D12_HEAP_PROPERTIES heap_prop     = {D3D12_HEAP_TYPE_UPLOAD};
		D3D12_RESOURCE_DESC         resource_desc = {};
		const auto size_in_bytes = Math::Upper(sizeof(DirectX::XMFLOAT4X4), 256llu);
		resource_desc.Dimension                   = D3D12_RESOURCE_DIMENSION_BUFFER;
		resource_desc.Layout                      = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resource_desc.Flags                       = D3D12_RESOURCE_FLAG_NONE;
		resource_desc.Format                      = DXGI_FORMAT_UNKNOWN;
		resource_desc.Width                       = size_in_bytes;
		resource_desc.Height                      = 1;
		resource_desc.DepthOrArraySize            = 1;
		resource_desc.MipLevels                   = 1;
		resource_desc.SampleDesc.Count            = 1;
		resource_desc.SampleDesc.Quality          = 0;
		auto& constant_buffer =m_instance_buffer_local.emplace_back();
		CHECK_RESULT(m_d3d12_device->CreateCommittedResource( &heap_prop, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constant_buffer.rhi_buffer)));
	}


	void D3D12RHI::CreateBottomLevelAccelerationStructure()
	{
		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometry_descs;
		const auto& geometry_resource = m_render_primitives[0];
		uint64_t                                    geometry_count = geometry_resource.m_geometry_descs_cpu.size();
		geometry_descs.reserve(geometry_count);
	
		D3D12_GPU_VIRTUAL_ADDRESS index_address = geometry_resource.m_index_buffer->GetGPUVirtualAddress();
		D3D12_GPU_VIRTUAL_ADDRESS vertex_address = geometry_resource.m_vertex_buffer->GetGPUVirtualAddress();
		for (uint64_t geometry_index = 0; geometry_index < geometry_count; ++geometry_index)
		{
			auto&                      geometry_desc         = geometry_resource.m_geometry_descs_cpu[geometry_index];
			D3D12_RAYTRACING_GEOMETRY_DESC& stModuleGeometryDesc      = geometry_descs.emplace_back();
			stModuleGeometryDesc.Type                                 = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
			stModuleGeometryDesc.Triangles.IndexBuffer                = index_address + geometry_desc.index_offset * sizeof(IndexType);
			stModuleGeometryDesc.Triangles.IndexCount                 = geometry_desc.index_count;
			stModuleGeometryDesc.Triangles.IndexFormat                = DXGI_FORMAT_R32_UINT;
			stModuleGeometryDesc.Triangles.Transform3x4               = 0;
			stModuleGeometryDesc.Triangles.VertexFormat               = DXGI_FORMAT_R32G32B32_FLOAT;
			stModuleGeometryDesc.Triangles.VertexCount                = geometry_desc.vertex_count;
			stModuleGeometryDesc.Triangles.VertexBuffer.StartAddress  = vertex_address + geometry_desc.vertex_offset * sizeof(SVertexInstance);
			stModuleGeometryDesc.Triangles.VertexBuffer.StrideInBytes = geometry_resource.m_vertex_stride;

			// Mark the geometry as opaque. 
			// PERFORMANCE TIP: mark geometry as opaque whenever applicable as it can enable important ray processing optimizations.
			// Note: When rays encounter opaque geometry an any hit shader will not be executed whether it is present or not.
			stModuleGeometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
		}

		// Get required sizes for an acceleration structure.
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS emBuildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC    stBottomLevelBuildDesc = {};
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& stBottomLevelInputs    = stBottomLevelBuildDesc.Inputs;
		stBottomLevelInputs.DescsLayout                                              = D3D12_ELEMENTS_LAYOUT_ARRAY;
		stBottomLevelInputs.Flags                                                    = emBuildFlags;
		stBottomLevelInputs.NumDescs                                                 = static_cast<UINT>(geometry_descs.size());
		stBottomLevelInputs.Type                                                     = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		stBottomLevelInputs.pGeometryDescs                                           = geometry_descs.data();

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO stBottomLevelPrebuildInfo = {};
		m_d3d12_device->GetRaytracingAccelerationStructurePrebuildInfo(&stBottomLevelInputs, &stBottomLevelPrebuildInfo);

		CHECK_RESULT(stBottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

		D3D12_RESOURCE_DESC stBufferResSesc = {};
		stBufferResSesc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
		stBufferResSesc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		stBufferResSesc.Flags               = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		stBufferResSesc.Format              = DXGI_FORMAT_UNKNOWN;
		stBufferResSesc.Height              = 1;
		stBufferResSesc.DepthOrArraySize    = 1;
		stBufferResSesc.MipLevels           = 1;
		stBufferResSesc.SampleDesc.Count    = 1;
		stBufferResSesc.SampleDesc.Quality  = 0;
		stBufferResSesc.Width               = stBottomLevelPrebuildInfo.ScratchDataSizeInBytes;

		D3D12_HEAP_PROPERTIES stDefaultHeapProps;
		stDefaultHeapProps.Type                  = D3D12_HEAP_TYPE_DEFAULT;
		stDefaultHeapProps.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		stDefaultHeapProps.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
		stDefaultHeapProps.CreationNodeMask      = 0;
		stDefaultHeapProps.VisibleNodeMask       = 0;
		CHECK_RESULT(m_d3d12_device->CreateCommittedResource( &stDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &stBufferResSesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_bottom_level_scratch_resource)));

		D3D12_RESOURCE_STATES emInitialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

		stBufferResSesc.Width = stBottomLevelPrebuildInfo.ResultDataMaxSizeInBytes;
		CHECK_RESULT(m_d3d12_device->CreateCommittedResource( &stDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &stBufferResSesc, emInitialResourceState, nullptr, IID_PPV_ARGS(&m_bottom_level_acceleration_structure)));

		// Bottom Level Acceleration Structure desc
		stBottomLevelBuildDesc.ScratchAccelerationStructureData = m_bottom_level_scratch_resource->GetGPUVirtualAddress();
		stBottomLevelBuildDesc.DestAccelerationStructureData    = m_bottom_level_acceleration_structure->GetGPUVirtualAddress();

		D3D12_RESOURCE_BARRIER resource_barrier = {};
		resource_barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		resource_barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		resource_barrier.UAV.pResource          = m_bottom_level_acceleration_structure.Get();

		// Build acceleration structure.
		CHECK_RESULT(m_cmd_allocator->Reset())
		CHECK_RESULT(m_cmd_list->Reset(m_cmd_allocator.Get(), nullptr))
		m_cmd_list->BuildRaytracingAccelerationStructure(&stBottomLevelBuildDesc, 0, nullptr);
		m_cmd_list->ResourceBarrier(1, &resource_barrier);
	}

	void D3D12RHI::CreateTopLevelInstanceResource()
	{
		D3D12_RAYTRACING_INSTANCE_DESC stInstanceDesc = {};
		DirectX::XMMATRIX mxTrans = DirectX::XMMatrixIdentity();
		::memcpy(stInstanceDesc.Transform, &mxTrans, (size_t)3 * 4 * sizeof(float));

		stInstanceDesc.InstanceMask = 1;
		stInstanceDesc.AccelerationStructure = m_bottom_level_acceleration_structure->GetGPUVirtualAddress();

		D3D12_RESOURCE_DESC stBufferResSesc = {};
		stBufferResSesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		stBufferResSesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		stBufferResSesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		stBufferResSesc.Format = DXGI_FORMAT_UNKNOWN;
		stBufferResSesc.Height = 1;
		stBufferResSesc.DepthOrArraySize = 1;
		stBufferResSesc.MipLevels = 1;
		stBufferResSesc.SampleDesc.Count = 1;
		stBufferResSesc.SampleDesc.Quality = 0;
		stBufferResSesc.Width = sizeof(stInstanceDesc);

		D3D12_HEAP_PROPERTIES stUploadHeapProps;
		stUploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		stUploadHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		stUploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		stUploadHeapProps.CreationNodeMask = 0;
		stUploadHeapProps.VisibleNodeMask = 0;

		CHECK_RESULT(m_d3d12_device->CreateCommittedResource(&stUploadHeapProps, D3D12_HEAP_FLAG_NONE, &stBufferResSesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_top_level_instance_resource)));

		void* data_ptr = nullptr;
		m_top_level_instance_resource->Map(0, nullptr, &data_ptr);
		memcpy(data_ptr, &stInstanceDesc, sizeof(stInstanceDesc));
		m_top_level_instance_resource->Unmap(0, nullptr);
	}

	void D3D12RHI::CreateTopLevelAccelerationStructure()
	{
		D3D12_RESOURCE_DESC stBufferResSesc = {};
		stBufferResSesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		stBufferResSesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		stBufferResSesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		stBufferResSesc.Format = DXGI_FORMAT_UNKNOWN;
		stBufferResSesc.Height = 1;
		stBufferResSesc.DepthOrArraySize = 1;
		stBufferResSesc.MipLevels = 1;
		stBufferResSesc.SampleDesc.Count = 1;
		stBufferResSesc.SampleDesc.Quality = 0;

		D3D12_HEAP_PROPERTIES stDefaultHeapProps;
		stDefaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
		stDefaultHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		stDefaultHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		stDefaultHeapProps.CreationNodeMask = 0;
		stDefaultHeapProps.VisibleNodeMask = 0;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS emBuildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC    stTopLevelBuildDesc = {};
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& stTopLevelInputs = stTopLevelBuildDesc.Inputs;
		stTopLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		stTopLevelInputs.Flags = emBuildFlags;
		stTopLevelInputs.NumDescs = 1;
		stTopLevelInputs.pGeometryDescs = nullptr;
		stTopLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO stTopLevelPrebuildInfo = {};
		m_d3d12_device->GetRaytracingAccelerationStructurePrebuildInfo(&stTopLevelInputs, &stTopLevelPrebuildInfo);

		CHECK_RESULT(stTopLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);
		stBufferResSesc.Width = stTopLevelPrebuildInfo.ScratchDataSizeInBytes;
	
		CHECK_RESULT(m_d3d12_device->CreateCommittedResource(&stDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &stBufferResSesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_top_level_scratch_resource)));

		D3D12_RESOURCE_STATES emInitialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

		stBufferResSesc.Width = stTopLevelPrebuildInfo.ResultDataMaxSizeInBytes;
		CHECK_RESULT(m_d3d12_device->CreateCommittedResource(&stDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &stBufferResSesc, emInitialResourceState, nullptr, IID_PPV_ARGS(&m_top_level_acceleration_structure)));

		// Top Level Acceleration Structure desc
		stTopLevelBuildDesc.DestAccelerationStructureData = m_top_level_acceleration_structure->GetGPUVirtualAddress();
		stTopLevelBuildDesc.ScratchAccelerationStructureData = m_top_level_scratch_resource->GetGPUVirtualAddress();
		stTopLevelBuildDesc.Inputs.InstanceDescs = m_top_level_instance_resource->GetGPUVirtualAddress();
		m_cmd_list->BuildRaytracingAccelerationStructure(&stTopLevelBuildDesc, 0, nullptr);


		CHECK_RESULT(m_cmd_list->Close())
		//执行命令列表
		ID3D12CommandList* ppCommandLists[] = { m_cmd_list.Get() };
		m_cmd_queue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		//开始同步GPU与CPU的执行，先记录围栏标记值
		const UINT64 n64CurrentFenceValue = m_render_end_fence.m_fence_value;
		CHECK_RESULT(m_cmd_queue->Signal(m_render_end_fence.m_fence.Get(), n64CurrentFenceValue));
		m_render_end_fence.m_fence_value++;
		CHECK_RESULT(m_render_end_fence.m_fence->SetEventOnCompletion(n64CurrentFenceValue, m_render_end_fence.m_fence_event));


		WaitForSingleObject(m_render_end_fence.m_fence_event, INFINITE);
	}

	void D3D12RHI::BuildDescHeap()
	{
		D3D12_DESCRIPTOR_HEAP_DESC stDXRDescriptorHeapDesc = {};
		// 存放9个描述符:
		// 2 - 顶点与索引缓冲 SRVs
		// 1 - 光追渲染输出纹理 SRV
		// 2 - 加速结构的缓冲 UAVs
		// 2 - 加速结构体数据缓冲 UAVs 主要用于fallback层
		// 2 - 纹理和Normal Map的描述符
		stDXRDescriptorHeapDesc.NumDescriptors = 9;
		stDXRDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		stDXRDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		CHECK_RESULT(m_d3d12_device->CreateDescriptorHeap(&stDXRDescriptorHeapDesc, IID_PPV_ARGS(&m_srv_cbv_uav_heap)));

		{
			D3D12_CPU_DESCRIPTOR_HANDLE stUAVDescriptorHandle = m_srv_cbv_uav_heap->GetCPUDescriptorHandleForHeapStart();
			stUAVDescriptorHandle.ptr += (m_ray_tracing_rt_uav * m_srv_cbv_uav_descriptor_size);

			D3D12_UNORDERED_ACCESS_VIEW_DESC stUAVDesc = {};
			stUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			m_d3d12_device->CreateUnorderedAccessView(m_raytracing_render_targets.Get(), nullptr, &stUAVDesc, stUAVDescriptorHandle);

		}
		{
			// geometry desc
			auto&                           geometry_desc = m_render_primitives[0].m_geometry_descs;
			D3D12_SHADER_RESOURCE_VIEW_DESC stSRVDesc = {};
			stSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			stSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			stSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
			stSRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			stSRVDesc.Buffer.NumElements = m_render_primitives[0].m_geometry_descs_cpu.size();
			stSRVDesc.Buffer.StructureByteStride = sizeof(SGeometryDesc);

			D3D12_CPU_DESCRIPTOR_HANDLE stSrvHandleTexture = m_srv_cbv_uav_heap->GetCPUDescriptorHandleForHeapStart();
			stSrvHandleTexture.ptr += (c_nDSNIndxTexture * m_srv_cbv_uav_descriptor_size);

			m_d3d12_device->CreateShaderResourceView(geometry_desc.Get(), &stSRVDesc, stSrvHandleTexture);
		}
		{
			// materials
			D3D12_RESOURCE_DESC             stTXDesc         = m_materials->GetDesc();
			D3D12_SHADER_RESOURCE_VIEW_DESC stSRVDesc = {};
			stSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			stSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			stSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
			stSRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			stSRVDesc.Buffer.NumElements = m_materials_cpu.size();
			stSRVDesc.Buffer.StructureByteStride = sizeof(SMaterial);

			D3D12_CPU_DESCRIPTOR_HANDLE stSrvHandleNormalMap = m_srv_cbv_uav_heap->GetCPUDescriptorHandleForHeapStart();
			stSrvHandleNormalMap.ptr += (c_nDSNIndxNormal * m_srv_cbv_uav_descriptor_size);

			m_d3d12_device->CreateShaderResourceView(m_materials.Get(), &stSRVDesc, stSrvHandleNormalMap);
		}
		{
			// index
			auto&                           geometry  = m_render_primitives[0];
			D3D12_SHADER_RESOURCE_VIEW_DESC stSRVDesc = {};
			stSRVDesc.ViewDimension                   = D3D12_SRV_DIMENSION_BUFFER;
			stSRVDesc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			stSRVDesc.Format                          = DXGI_FORMAT_UNKNOWN;
			stSRVDesc.Buffer.Flags                    = D3D12_BUFFER_SRV_FLAG_NONE;
			stSRVDesc.Buffer.NumElements              = geometry.m_index_count;
			stSRVDesc.Buffer.StructureByteStride      = sizeof(IndexType);

			D3D12_CPU_DESCRIPTOR_HANDLE stIBHandle = m_srv_cbv_uav_heap->GetCPUDescriptorHandleForHeapStart();
			stIBHandle.ptr += (c_nDSHIndxIBView * m_srv_cbv_uav_descriptor_size);

			m_d3d12_device->CreateShaderResourceView(geometry.m_index_buffer.Get(), &stSRVDesc, stIBHandle);

			// Vertex SRV
			stSRVDesc.Format                     = DXGI_FORMAT_UNKNOWN;
			stSRVDesc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;
			stSRVDesc.Buffer.NumElements         = geometry.m_vertex_count;
			stSRVDesc.Buffer.StructureByteStride = sizeof(SVertexInstance);

			D3D12_CPU_DESCRIPTOR_HANDLE stVBHandle = m_srv_cbv_uav_heap->GetCPUDescriptorHandleForHeapStart();
			stVBHandle.ptr += (c_nDSHIndxVBView * m_srv_cbv_uav_descriptor_size);

			m_d3d12_device->CreateShaderResourceView(geometry.m_vertex_buffer.Get(), &stSRVDesc, stVBHandle);
		}
	}
}
