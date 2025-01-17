#pragma once

#include "runtime/core/base/macro.h"
#include "runtime/function/global/global_context.h"
#include "runtime/core/meta/json.h"

#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>

namespace MoYu
{
    class AssetManager
    {
    public:
        template<typename AssetType>
        bool loadAsset(const std::string& asset_url, AssetType& out_asset) const
        {
            // read json file to string
            std::filesystem::path asset_path = getFullPath(asset_url);
            std::ifstream asset_json_file(asset_path);
            if (!asset_json_file)
            {
                LOG_ERROR("open file: {} failed!", asset_path.generic_string());
                return false;
            }

            std::stringstream buffer;
            buffer << asset_json_file.rdbuf();
            std::string asset_json_text(buffer.str());

            // parse to json object and read to runtime res object

            NJson j_asset = NJson::parse(asset_json_text);

            try
            {
                out_asset = j_asset.get<AssetType>();
            }
            catch (const std::exception&)
            {
                out_asset = {};
            }

            return true;
        }

        template<typename AssetType>
        bool saveAsset(const AssetType& out_asset, const std::string& asset_url) const
        {
            std::ofstream asset_json_file(getFullPath(asset_url));
            if (!asset_json_file)
            {
                LOG_ERROR("open file {} failed!", asset_url);
                return false;
            }

            // write to json object and dump to string
            NJson asset_json = out_asset;
            std::string asset_json_text = asset_json.dump(4);

            // write to file
            asset_json_file << asset_json_text;
            asset_json_file.flush();

            return true;
        }

        std::filesystem::path getFullPath(const std::string& relative_path) const;

        template<typename AssetType>
        static AssetType loadJson(const std::string& json_str)
        {
            std::string asset_json_text(json_str);

            // parse to json object and read to runtime res object

            NJson j_asset = NJson::parse(asset_json_text);

            AssetType out_asset {};
            try
            {
                out_asset = j_asset.get<AssetType>();
            }
            catch (const std::exception&)
            {
                out_asset = {};
            }

            return out_asset;
        }

        template<typename AssetType>
        static const std::string saveJson(AssetType& out_asset)
        {
            // write to json object and dump to string
            NJson asset_json = out_asset;
            std::string asset_json_text = asset_json.dump(4);

            return asset_json_text;
        }

    };
} // namespace MoYu
