#include "runtime/function/framework/level/level.h"

#include "runtime/core/base/macro.h"

#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/res_type/common/level.h"

#include "runtime/engine.h"
#include "runtime/function/character/character.h"
#include "runtime/function/framework/object/object.h"

#include "runtime/function/framework/component/transform/transform_component.h"

#include <limits>
#include <algorithm>

namespace Pilot
{
    Level::Level() { init(); }

    Level::~Level() { clear(); }

    void Level::init() {}

    void Level::clear()
    {
        m_current_active_character.reset();
        m_root_object.reset();
        m_gobjects.clear();
    }

    void Level::resortChildrenSiblingIndex(GObjectID objectID)
    {
        std::shared_ptr<GObject> gObjectPtr = m_gobjects[objectID];
        std::vector<GObjectID> mChildrenId = gObjectPtr->m_chilren_id;
        for (size_t i = 0; i < mChildrenId.size(); i++)
            m_gobjects[mChildrenId[i]]->m_sibling_index = i;
    }

    std::weak_ptr<GObject> Level::createGObject(std::string nodeName, GObjectID parentID)
    {
        GObjectID objectID = ObjectIDAllocator::alloc();
        while (m_gobjects.find(objectID) != m_gobjects.end())
        {
            objectID = ObjectIDAllocator::alloc();
        }
        return createGObject(nodeName, objectID, parentID);
    }

    std::weak_ptr<GObject> Level::createGObject(std::string nodeName, GObjectID objectID, GObjectID parentID)
    {
        auto objectInterator = m_gobjects.find(objectID);
        if (objectInterator != m_gobjects.end())
        {
            LOG_INFO("{} has been created!!!", nodeName);
            std::shared_ptr<GObject> objectPtr = objectInterator->second;
            return std::weak_ptr<GObject>(objectPtr);
        }

        ASSERT(((objectID == k_root_gobject_id) && (parentID == k_invalid_gobject_id)) ||
               (parentID != k_invalid_gobject_id));

        std::shared_ptr<GObject> gobject = std::make_shared<GObject>(objectID, shared_from_this());

        gobject->m_name      = nodeName;
        gobject->m_id        = objectID;
        gobject->m_parent_id = parentID;

        if (parentID != k_invalid_gobject_id)
        {
            auto parentIter = m_gobjects.find(parentID);
            ASSERT(parentIter != m_gobjects.end());
            std::shared_ptr<GObject> parentObject = parentIter->second;
            parentObject->m_chilren_id.push_back(objectID);
            gobject->m_sibling_index = parentObject->m_chilren_id.size() - 1;
        }

        auto new_transform_component = PILOT_REFLECTION_NEW(TransformComponent);
        gobject->tryAddComponent(new_transform_component);

        m_gobjects.emplace(objectID, gobject);

        if (gobject->m_id == k_root_gobject_id)
            m_root_object = gobject;

        return std::weak_ptr<GObject>(gobject);
    }

    std::weak_ptr<GObject> Level::instantiateGObject(ObjectInstanceRes& object_instance_res)
    {
        ASSERT(m_gobjects.find(object_instance_res.m_id) == m_gobjects.end());

        std::shared_ptr<GObject> gobject = std::make_shared<GObject>(shared_from_this());
        gobject->load(object_instance_res);
        
        m_gobjects.emplace(object_instance_res.m_id, gobject);

        return std::weak_ptr<GObject>(gobject);
    }

    bool Level::load(const std::string& level_res_url)
    {
        LOG_INFO("loading level: {}", level_res_url);

        m_level_res_url = level_res_url;

        LevelRes   level_res;
        const bool is_load_success = g_runtime_global_context.m_asset_manager->loadAsset(level_res_url, level_res);
        if (is_load_success == false)
        {
            return false;
        }

        for (ObjectInstanceRes& object_instance_res : level_res.m_objects)
        {
            std::weak_ptr<GObject> object_weak_ptr = instantiateGObject(object_instance_res);
            if (object_weak_ptr.lock()->isRootNode())
                m_root_object = object_weak_ptr;
        }

        if (m_root_object.expired())
        {
            m_root_object = createGObject("Root Node", k_root_gobject_id, k_invalid_gobject_id);
        }

        // create active character
        for (const auto& object_pair : m_gobjects)
        {
            std::shared_ptr<GObject> object = object_pair.second;
            if (object == nullptr)
                continue;

            if (level_res.m_character_name == object->getName())
            {
                m_current_active_character = std::make_shared<Character>(object);
                break;
            }
        }

        m_is_loaded = true;

        LOG_INFO("level load succeed");

        return true;
    }

    void Level::unload()
    {
        clear();
        LOG_INFO("unload level: {}", m_level_res_url);
    }

    bool Level::save()
    {
        LOG_INFO("saving level: {}", m_level_res_url);
        LevelRes output_level_res;

        const size_t                    object_cout    = m_gobjects.size();
        std::vector<ObjectInstanceRes>& output_objects = output_level_res.m_objects;
        output_objects.resize(object_cout);

        size_t object_index = 0;
        for (const auto& id_object_pair : m_gobjects)
        {
            if (id_object_pair.second)
            {
                id_object_pair.second->save(output_objects[object_index]);
                ++object_index;
            }
        }

        const bool is_save_success =
            g_runtime_global_context.m_asset_manager->saveAsset(output_level_res, m_level_res_url);

        if (is_save_success == false)
        {
            LOG_ERROR("failed to save {}", m_level_res_url);
        }
        else
        {
            LOG_INFO("level save succeed");
        }

        return is_save_success;
    }

    void Level::tick(float delta_time)
    {
        if (!m_is_loaded)
        {
            return;
        }

        for (const auto& id_object_pair : m_gobjects)
        {
            assert(id_object_pair.second);
            if (id_object_pair.second)
            {
                id_object_pair.second->tick(delta_time);
            }
        }
        if (m_current_active_character && g_is_editor_mode == false)
        {
            m_current_active_character->tick(delta_time);
        }

    }

    std::weak_ptr<GObject> Level::getGObjectByID(GObjectID go_id) const
    {
        auto iter = m_gobjects.find(go_id);
        if (iter != m_gobjects.end())
        {
            return iter->second;
        }

        return std::weak_ptr<GObject>();
    }

    void Level::deleteGObject(GObjectID go_id)
    {
        if (go_id == k_root_gobject_id || go_id == k_invalid_gobject_id)
            return;

        //auto iter = m_gobjects.find(go_id);
        //ASSERT(iter != m_gobjects.end());
        //std::shared_ptr<GObject> current_object = iter->second;
        std::shared_ptr<GObject> current_object = m_gobjects[go_id];

        // resursive delete child
        std::vector<GObjectID> cur_obj_children_id = current_object->m_chilren_id;
        for (size_t i = 0; i < cur_obj_children_id.size(); i++)
            deleteGObject(cur_obj_children_id[i]);

        //auto parentIter = m_gobjects.find(current_object->m_parent_id);
        //ASSERT(parentIter != m_gobjects.end());
        //std::shared_ptr<GObject> parentObject = parentIter->second;
        std::shared_ptr<GObject> parentObject = m_gobjects[current_object->m_parent_id];

        std::vector<std::uint32_t>& parent_children_id = parentObject->m_chilren_id;
        
        auto cur_obj_in_parent_iner = std::find(parent_children_id.begin(), parent_children_id.end(), go_id);
        ASSERT(cur_obj_in_parent_iner != parent_children_id.end());
        parent_children_id.erase(cur_obj_in_parent_iner);

        resortChildrenSiblingIndex(parentObject->m_id);
    }

    void Level::changeParent(GObjectID from_id, GObjectID to_parent_id, std::optional<std::uint32_t> sibling_index)
    {
        ASSERT((from_id != k_root_gobject_id) && (from_id != k_invalid_gobject_id) &&
               (to_parent_id != k_invalid_gobject_id));

        std::shared_ptr<GObject> mObjectPtr  = m_gobjects[from_id];
        std::shared_ptr<GObject> mParentPtr  = m_gobjects[mObjectPtr->m_parent_id];
        std::shared_ptr<GObject> toParentPtr = m_gobjects[to_parent_id];

        if (mObjectPtr->m_parent_id != to_parent_id)
        {
            std::vector<GObjectID>& mParentChildrenID = mParentPtr->m_chilren_id;
            auto from_obj_in_parent_iter = std::find(mParentChildrenID.begin(), mParentChildrenID.end(), from_id);
            mParentChildrenID.erase(from_obj_in_parent_iter);
            resortChildrenSiblingIndex(mParentPtr->m_id);

            std::uint32_t new_sibling_index = sibling_index.value_or(toParentPtr->m_chilren_id.size());
            toParentPtr->m_chilren_id.insert(toParentPtr->m_chilren_id.begin() + new_sibling_index, from_id);
            resortChildrenSiblingIndex(toParentPtr->m_id);
        }
        else
        {
            std::vector<GObjectID>& mParentChildrenID = mParentPtr->m_chilren_id;

            std::uint32_t from_sibling_index = mObjectPtr->m_sibling_index;
            std::uint32_t to_sibling_index   = sibling_index.value_or(toParentPtr->m_chilren_id.size() - 1);

            GObjectID _tmp                        = mParentChildrenID[from_sibling_index];
            mParentChildrenID[from_sibling_index] = mParentChildrenID[to_sibling_index];
            mParentChildrenID[to_sibling_index]   = _tmp;

            resortChildrenSiblingIndex(toParentPtr->m_id);
        }
    }

} // namespace Pilot
