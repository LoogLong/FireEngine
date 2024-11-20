
#include "Classes/texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


namespace FireEngine
{
	void CTexture::LoadTextureFromFile(const std::string& tex_file_name)
	{
		stbi_uc* pixels = stbi_load(tex_file_name.c_str(), &m_width, &m_height, &m_channels, STBI_rgb_alpha);
		if (!pixels)
		{
			printf("[error]图片读取失败!\n");
			return;
		}
		uint64_t data_size = static_cast<uint64_t>(m_width) * m_height * 4 * sizeof(uint8_t);
		m_data.resize(data_size);
		memcpy(m_data.data(), pixels, data_size);
		stbi_image_free(pixels);
	}
}
