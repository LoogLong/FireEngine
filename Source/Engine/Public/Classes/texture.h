#pragma once
#include <memory>
#include <string>
#include <vector>

#include "Core/define.h"
#include "Core/Asset.h"

namespace FireEngine {
	class CTexture : public CAssetBase
	{
	public:
		CTexture() = default;

		void LoadTextureFromFile(const std::string& tex_file_name);

	public:
		int32_t      m_width;
		int32_t      m_height;
		int32_t      m_channels;
		std::vector<uint8_t> m_data;
	};
}
