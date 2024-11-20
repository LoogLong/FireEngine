#pragma once

#include "memory"
#include "Core/define.h"
#include "Window/window_system.h"


namespace FireEngine
{
	class D3D12RHI;
	class CRenderingSystem
	{
	public:
		CRenderingSystem(CWindowSystem* window_system);
		~CRenderingSystem() = default;

		void TickRendering(float dt);

		
	private:

		D3D12RHI* m_rhi;

		SSceneConstantBuffer m_scene_constant_buffer;
	};
}
