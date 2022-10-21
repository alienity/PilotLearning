#pragma once

#include "runtime/resource/res_type/common/world.h"
#include "runtime/core/base/robin_hood.h"

#include "runtime/resource/res_type/components/material.h"

#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>

namespace Pilot
{
    class AssetManager;

    static std::string _default_gold_material_path = "asset/objects/environment/_material/gold.material.json";

    static std::string _default_color_texture_path              = "asset/texture/default/albedo.jpg";
    static std::string _default_metallic_roughness_texture_path = "asset/texture/default/mr.jpg";
    static std::string _default_normal_texture_path             = "asset/texture/default/normal.jpg";

    class MaterialManager
    {
    public:
        virtual ~MaterialManager();

        void initialize(std::shared_ptr<AssetManager> asset_manager);
        void clear();

        void tick(float delta_time);

        void flushMaterial();

        MaterialRes& loadMaterialRes(std::string material_path);
        void saveMaterialRes(std::string material_path, MaterialRes& matres);

        bool isMaterialDirty(std::string material_path);
        void setMaterialDirty(std::string material_path, bool is_dirty = true);

    private:
        struct MaterialInfomation
        {
            bool        m_is_dirty;
            MaterialRes m_mat_res;
        };
        robin_hood::unordered_map<std::string, MaterialInfomation> m_matpath_matinfo_map;

        std::shared_ptr<AssetManager> asset_manager;
    };
} // namespace Pilot
