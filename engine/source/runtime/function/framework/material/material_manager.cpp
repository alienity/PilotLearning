#include "runtime/function/framework/material/material_manager.h"
#include "runtime/core/base/macro.h"
#include "runtime/resource/asset_manager/asset_manager.h"

namespace MoYu
{
    MaterialManager::~MaterialManager() { clear(); }

    void MaterialManager::initialize(std::shared_ptr<AssetManager> asset_manager)
    {
        MaterialRes material_res;
        asset_manager->loadAsset(_default_gold_material_path, material_res);
        m_matpath_matinfo_map[_default_gold_material_path] = {false, material_res};
    }

    void MaterialManager::clear()
    {
        m_matpath_matinfo_map.clear();
    }

    void MaterialManager::tick(float delta_time)
    {
        auto iter = m_matpath_matinfo_map.begin();
        for (; iter != m_matpath_matinfo_map.end(); iter++)
        {
            iter->second.m_is_dirty = false;
        }
    }

    void MaterialManager::flushMaterial()
    {
        auto iter = m_matpath_matinfo_map.begin();
        for (; iter != m_matpath_matinfo_map.end(); iter++)
        {
            if (iter->second.m_is_dirty)
            {
                asset_manager->saveAsset(iter->second.m_mat_res, iter->first);
            }
        }
    }

    MaterialRes& MaterialManager::loadMaterialRes(std::string material_path)
    {
        auto iter = m_matpath_matinfo_map.find(material_path);
        if (iter == m_matpath_matinfo_map.end())
        {
            MaterialRes material_res = {};
            asset_manager->loadAsset(material_path, material_res);
            m_matpath_matinfo_map[material_path] = {true, material_res};
        }
        return m_matpath_matinfo_map[material_path].m_mat_res;
    }

    void MaterialManager::saveMaterialRes(std::string material_path, MaterialRes& matres)
    {
        auto iter = m_matpath_matinfo_map.find(material_path);
        if (iter != m_matpath_matinfo_map.end())
        {
            m_matpath_matinfo_map[material_path] = {true, matres};
            asset_manager->saveAsset(matres, material_path);
        }
    }

    bool MaterialManager::isMaterialDirty(std::string material_path)
    {
        auto iter = m_matpath_matinfo_map.find(material_path);
        if (iter != m_matpath_matinfo_map.end())
        {
            return m_matpath_matinfo_map[material_path].m_is_dirty;
        }
        return false;
    }

    void MaterialManager::setMaterialDirty(std::string material_path, bool is_dirty)
    {
        auto iter = m_matpath_matinfo_map.find(material_path);
        if (iter != m_matpath_matinfo_map.end())
        {
            m_matpath_matinfo_map[material_path].m_is_dirty = is_dirty;
        }
    }


} // namespace MoYu
