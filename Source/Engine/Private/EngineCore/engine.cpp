
#include "EngineCore/engine.h"

#include <chrono>

#include "Core/file_system.h"
#include "Window/window_system.h"
#include "Window/GenericWindow.h"
#include "Level/level_manager.h"
#include "Render/RenderingSystem.h"
#include "Global/global_context.h"

using namespace std::chrono;

namespace FireEngine
{
	CGlobalSingletonContext* g_global_singleton_context;
	steady_clock::time_point g_last_tick_time_point;
	void InitEngine(const std::string& resource_path)
	{
		g_global_singleton_context = new CGlobalSingletonContext();


		g_global_singleton_context->m_file_system = std::make_shared<CFileSystem>(resource_path);
		g_global_singleton_context->m_asset_system = std::make_shared<CAssetSystem>();
		g_global_singleton_context->m_window_system = std::make_shared<CWindowSystem>();
		g_global_singleton_context->m_level_manager = std::make_shared<CLevelManager>();
		g_global_singleton_context->m_rendering_system = std::make_shared<CRenderingSystem>(g_global_singleton_context->m_window_system.get());
		g_last_tick_time_point = steady_clock::now();
	}

	float CalculateDeltaTime()
	{
		float delta_time;
		steady_clock::time_point tick_time_point = steady_clock::now();
		duration<float> time_span = duration_cast<duration<float>>(tick_time_point - g_last_tick_time_point);
		delta_time = time_span.count();

		g_last_tick_time_point = tick_time_point;
		return delta_time;
	}

	void LogicTick(float dt)
	{
		g_global_singleton_context->m_level_manager->TickLevels(dt);
		
	}

	void RenderTick(float dt)
	{
		g_global_singleton_context->m_rendering_system->TickRendering(dt);
	}

	void TickEngine()
	{
		bool quit = false;
		while (!quit)
		{
			if (g_global_singleton_context->m_window_system->WindowShouldClose())
			{
				return;
			}
			float dt = CalculateDeltaTime();
			g_global_singleton_context->m_window_system->PollEvents();

			//single thread
			LogicTick(dt);
			RenderTick(dt);
		}
		


	}

	void ShutDownEngine()
	{
		delete g_global_singleton_context;
	}
}
