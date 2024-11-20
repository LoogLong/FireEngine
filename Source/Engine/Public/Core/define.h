#pragma once
#include <DirectXMath.h>

namespace FireEngine
{
	struct SVertexInstance
	{
		float position[4];
		float normal[3];
		float uv[2];
		float color[4];
	};

	struct SGeometryDesc
	{
		uint32_t vertex_offset;
		uint32_t vertex_count;
		uint32_t index_offset;
		uint32_t index_count;
		uint32_t material_index;
	};

	struct SMaterial {
		float emission[4]; // 3emission + 1 ior
		float kd[3];
		float ks[3];
		float specular_exponent;
	};
	struct SInstanceConstantBuffer
	{
		float m_vAlbedo[4];
	};

	struct SSceneConstantBuffer
	{
		DirectX::XMMATRIX m_view_matrix;
		DirectX::XMVECTOR m_camera_pos;

		DirectX::XMVECTOR m_light_pos;
		DirectX::XMVECTOR m_light_ambient_color;
		DirectX::XMVECTOR m_light_diffuse_color;


		DirectX::XMFLOAT2 m_scale;
		
	};

	typedef uint32_t IndexType;
}
namespace Config
{
	constexpr float mass = 1.f;
	constexpr float ks = 80.f;
	constexpr float gravity_magnitude = 9.8f;
	constexpr uint32_t steps_per_frame = 64;
	constexpr float DUMP_FACTOR = 0.01f;
	constexpr uint32_t default_window_size[2] = { 784, 784 };
};
