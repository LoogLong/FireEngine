#pragma once

#include <memory>
#include <vector>
#include "Level/level.h"

namespace FireEngine
{

	class CLevelManager
	{
	public:
		CLevelManager();
		~CLevelManager() = default;

		void TickLevels(float dt);

	private:
		std::vector<std::unique_ptr<CLevel>> m_levels;
		CLevel* m_active_level{ nullptr };
	};
}
