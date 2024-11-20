#include "Core/ReadData.h"

#include "Core/OBJ_Loader.hpp"
#include "Windows.h"
namespace FireEngine
{
	std::vector<uint8_t> ReadData(const wchar_t* name)
	{
		std::ifstream inFile(name, std::ios::in | std::ios::binary | std::ios::ate);

#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
		if (!inFile)
		{
			wchar_t moduleName[_MAX_PATH] = {};
			if (!GetModuleFileNameW(nullptr, moduleName, _MAX_PATH)) throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "GetModuleFileNameW");

			wchar_t drive[_MAX_DRIVE];
			wchar_t path[_MAX_PATH];

			if (_wsplitpath_s(moduleName, drive, _MAX_DRIVE, path, _MAX_PATH, nullptr, 0, nullptr, 0)) throw std::runtime_error("_wsplitpath_s");

			wchar_t filename[_MAX_PATH];
			if (_wmakepath_s(filename, _MAX_PATH, drive, path, name, nullptr)) throw std::runtime_error("_wmakepath_s");

			inFile.open(filename, std::ios::in | std::ios::binary | std::ios::ate);
		}
#endif

		if (!inFile) throw std::runtime_error("ReadData");

		const std::streampos len = inFile.tellg();
		if (!inFile) throw std::runtime_error("ReadData");

		std::vector<uint8_t> blob;
		blob.resize(size_t(len));

		inFile.seekg(0, std::ios::beg);
		if (!inFile) throw std::runtime_error("ReadData");

		inFile.read(reinterpret_cast<char*>(blob.data()), len);
		if (!inFile) throw std::runtime_error("ReadData");

		inFile.close();

		return blob;
	}

	bool LoadMeshVertex(const std::string& mesh_file_name, std::vector<FireEngine::SVertexInstance>& out_vertex_instances, std::vector<IndexType>& out_indices)
	{
		std::ifstream fin;
		char          input;

		fin.open(mesh_file_name.c_str());
		if (fin.fail())
		{
			return false;
		}
		fin.get(input);
		while (input != ':')
		{
			fin.get(input);
		}

		uint32_t vertex_cnt;
		fin >> vertex_cnt;

		fin.get(input);
		while (input != ':')
		{
			fin.get(input);
		}
		fin.get(input);
		fin.get(input);

		out_vertex_instances.reserve(vertex_cnt);
		out_indices.reserve(vertex_cnt);

		for (uint32_t i = 0; i < vertex_cnt; i++)
		{
			auto& vertex = out_vertex_instances.emplace_back();
			fin >> vertex.position[0] >> vertex.position[1] >> vertex.position[2];
			vertex.position[3] = 1.0f;
			fin >> vertex.uv[0] >> vertex.uv[1];
			fin >> vertex.normal[0] >> vertex.normal[1] >> vertex.normal[2];

			out_indices.emplace_back(i);
		}
		return true;
	}

	void LoadMeshVertexObject(const std::string& mesh_file_name, std::vector<FireEngine::SVertexInstance>& out_vertex_instances, std::vector<IndexType>& out_indices)
	{
		objl::Loader loader;
		loader.LoadFile(mesh_file_name);
	
		assert(loader.LoadedMeshes.size() == 1);
		auto mesh = loader.LoadedMeshes[0];

		out_vertex_instances.reserve(mesh.Vertices.size());
		for (int i = 0; i < mesh.Vertices.size(); i++) {
			FireEngine::SVertexInstance& vert     = out_vertex_instances.emplace_back();
			const objl::Vertex&          src_vert = mesh.Vertices[i];
			vert.position[0]                      = src_vert.Position.X;
			vert.position[1]                      = src_vert.Position.Y;
			vert.position[2]                      = src_vert.Position.Z;
			vert.position[3]                      = 1.0f;

			// normalize
			float size_qure = src_vert.Normal.X* src_vert.Normal.X + src_vert.Normal.Y * src_vert.Normal.Y + src_vert.Normal.Z * src_vert.Normal.Z;
			float length = std::sqrt(size_qure);
			float inv_length = 1.0f / length;
			vert.normal[0] = src_vert.Normal.X * inv_length;
			vert.normal[1] = src_vert.Normal.Y * inv_length;
			vert.normal[2] = src_vert.Normal.Z * inv_length;

			vert.uv[0] = src_vert.TextureCoordinate.X;
			vert.uv[1] = src_vert.TextureCoordinate.Y;
		}
		out_indices.reserve(mesh.Indices.size());
		for (uint64_t i = 0; i < mesh.Indices.size(); i++)
		{
			out_indices.emplace_back(mesh.Indices[i]);
		}
	}
}
