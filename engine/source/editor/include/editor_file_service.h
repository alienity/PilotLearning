#pragma once

#include <memory>
#include <string>
#include <vector>

namespace MoYu
{
    class EditorFileNode;
    using EditorFileNodeArray = std::vector<std::shared_ptr<EditorFileNode>>;

    struct EditorFileNode
    {
        std::string         m_file_name;
        std::string         m_file_type;
        std::string         m_file_path;
        std::string         m_relative_path;
        int                 m_node_depth;
        EditorFileNodeArray m_child_nodes;
        EditorFileNode() = default;
        EditorFileNode(const std::string& name, const std::string& type , const std::string& relative_path, const std::string& path, int depth) :
            m_file_name(name), m_file_type(type), m_file_path(path), m_relative_path(relative_path), m_node_depth(depth)
        {}
    };

    inline bool operator==(const EditorFileNode& lhs, const EditorFileNode& rhs)
    {
        return lhs.m_file_name == rhs.m_file_name && lhs.m_file_type == rhs.m_file_type &&
               lhs.m_file_path == rhs.m_file_path && lhs.m_relative_path == rhs.m_relative_path &&
               lhs.m_node_depth == rhs.m_node_depth;
    }
    inline bool operator!=(const EditorFileNode& lhs, const EditorFileNode& rhs) { return !(lhs == rhs); }

    class EditorFileService
    {
        EditorFileNodeArray m_file_node_array;
        EditorFileNode      m_root_node {"asset", "Folder", "asset", "asset", -1};

    private:
        std::shared_ptr<EditorFileNode> getParentNodePtr(std::shared_ptr<EditorFileNode> file_node);
        bool                            checkFileArray(std::shared_ptr<EditorFileNode> file_node);

    public:
        std::shared_ptr<EditorFileNode> getEditorRootNode();

        void buildEngineFileTree();

        std::shared_ptr<EditorFileNode> getSelectedEditorNode();

        void setSelectedEditorNode(std::shared_ptr<EditorFileNode> selected_node);

    private:
        std::shared_ptr<EditorFileNode> m_editor_node_ptr;
    };
} // namespace MoYu
