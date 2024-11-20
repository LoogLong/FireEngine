#pragma once
#include <memory>
#include <string>


namespace FireEngine
{
	class CFileSystem;
	class CWindowSystem;
	class CLevelManager;
	class CRenderingSystem;
	class CAssetSystem;

	class CGlobalSingletonContext
	{
	public:
		CGlobalSingletonContext();




		std::shared_ptr<CFileSystem> m_file_system;
		std::shared_ptr<CAssetSystem> m_asset_system;
		std::shared_ptr<CWindowSystem> m_window_system;
		std::shared_ptr<CLevelManager> m_level_manager;
		std::shared_ptr<CRenderingSystem> m_rendering_system;

	};
	extern CGlobalSingletonContext* g_global_singleton_context;

};
