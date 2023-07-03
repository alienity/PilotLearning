#pragma once

#include "runtime/function/framework/object/object_id_allocator.h"
#include "runtime/core/base/robin_hood.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <optional>

namespace MoYu
{
    class Character;
    class GObject;
    class ObjectInstanceRes;
    //class PhysicsScene;

    using LevelObjectsMap = robin_hood::unordered_map<GObjectID, std::shared_ptr<GObject>>;

    /// The main class to manage all game objects
    class Level : public std::enable_shared_from_this<Level>
    {
    public:
        Level();

        virtual ~Level();

        bool load(const std::string& level_res_url);
        void unload();

        bool save();

        void tick(float delta_time);
        
        const std::string& getLevelResUrl() const { return m_level_res_url; }

        const LevelObjectsMap& getAllGObjects() const { return m_gobjects; }

        std::shared_ptr<GObject> getGObjectByID(GObjectID go_id) const;
        
        std::shared_ptr<GObject> createGObject(std::string nodeName, GObjectID parentID = K_Root_Object_Id);
        std::shared_ptr<GObject> createGObject(std::string nodeName, GObjectID objectID, GObjectID parentID);

        std::shared_ptr<GObject> instantiateGObject(ObjectInstanceRes& object_instance_res);

        void deleteGObject(GObjectID go_id);
        void changeParent(GObjectID from_id, GObjectID to_parent_id, std::optional<std::uint32_t> sibling_index = std::nullopt);

        std::shared_ptr<GObject> getRootNode() { return m_root_object; }

    protected:
        void init();
        void clear();
        void resortChildrenSiblingIndex(GObjectID objectID);
        void removeGObject(GObjectID go_id);

        bool        m_is_loaded {false};
        std::string m_level_res_url;

        // all game objects in this level, key: object id, value: object instance
        LevelObjectsMap m_gobjects;

        std::shared_ptr<GObject> m_root_object;
    };
} // namespace MoYu
