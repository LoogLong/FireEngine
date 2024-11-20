#pragma once
#include <memory>
#include <vector>

#include "Core/define.h"
#include "Core/Asset.h"

namespace FireEngine {
	class CMesh : public CAssetBase
	{
	public:
		CMesh() = default;
		std::vector<SVertexInstance> m_vretices;
		std::vector<IndexType> m_indices;
		uint32_t material;
	};
}
