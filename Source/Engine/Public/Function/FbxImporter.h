#pragma once
#include <memory>

#include "fbxsdk.h"
#include "Classes/mesh.h"

namespace FireEngine
{
	class CFbxImporter
	{
	public:
		CFbxImporter();
		~CFbxImporter();
		static CFbxImporter* GetInstance()
		{
			static std::unique_ptr<CFbxImporter> fbx_importer;
			if (!fbx_importer)
			{
				fbx_importer = std::make_unique<CFbxImporter>();
			}
			return fbx_importer.get();
		}

		bool LoadScene(const std::string& file_name);
		void LoadStaticMesh(CMesh* out_mesh);
		void ImportSkeletalMesh(CMesh* out_mesh);
		void ReleaseScene();

	private:
		void Initialize();

		FbxManager* m_fbx_manager{ nullptr };
		FbxScene* m_fbx_scene{ nullptr };
		FbxGeometryConverter* m_geometry_converter{ nullptr };
	};





}
