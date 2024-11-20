#pragma once
#include <memory>
#include <string>
#include <vector>

#include "Asset.h"

namespace FireEngine
{
	class CFileSystem
	{
	public:
		CFileSystem(const std::string& resource_path) : m_resource_root_path(resource_path){
			m_resource_root_path = m_resource_root_path + "/";
		}

		std::string GetFullPath(std::string_view relative_path) const
		{
			return m_resource_root_path + relative_path.data();
		}
	private:
		std::string m_resource_root_path;


	};


	class CAssetSystem
	{
	public:

		uint64_t RetainAsset(std::unique_ptr<CAssetBase>&& asset)
		{
			uint64_t asset_id =m_assets.size();
			m_assets.emplace_back(std::move(asset));
			return asset_id;
		}

		CAssetBase* GetAsset(uint64_t asset_id)
		{
			if (asset_id < m_assets.size())
			{
				return m_assets[asset_id].get();
			}
			return nullptr;
		}
	private:
		std::vector<std::unique_ptr<CAssetBase>> m_assets;
	};
};
