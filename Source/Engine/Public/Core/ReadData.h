#pragma once

#include <cstdint>
#include <fstream>
#include <vector>
#include "define.h"

namespace FireEngine
{
	std::vector<uint8_t> ReadData(_In_z_ const wchar_t* name);

	bool LoadMeshVertex(const std::string& mesh_file_name, std::vector<FireEngine::SVertexInstance>& out_vertex_instances, std::vector<IndexType>& out_indices);

	void LoadTexture(const std::string& tex_file_name);

	void LoadMeshVertexObject(const std::string& mesh_file_name, std::vector<FireEngine::SVertexInstance>& out_vertex_instances, std::vector<IndexType>& out_indices);
}
