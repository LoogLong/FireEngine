#pragma once
#include <memory>
#include <vector>
#include "component.h"
namespace FireEngine {
	class CActor
	{

	private:
		std::vector<std::unique_ptr<CComponent>> m_components;
	};
}
