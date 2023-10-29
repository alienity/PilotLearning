#pragma once

#include "runtime/function/render/render_camera.h"
#include "runtime/function/render/render_common.h"
#include "runtime/resource/res_type/global/global_rendering.h"

#include <cstdint>
#include <deque>
#include <optional>
#include <string>

namespace MoYu
{
    struct CameraSwapData
    {
        RenderCameraType m_camera_type;
        bool m_is_perspective;
        glm::float4x4 m_view_matrix;
        int   m_width;
        int   m_height;
        float m_nearZ;
        float m_farZ;
        float m_fov_y;
    };

    struct GameObjectResourceDesc
    {
        std::deque<GameObjectDesc> m_game_object_descs;

        void           add(GameObjectDesc desc);
        bool           isEmpty() const;
        GameObjectDesc getNextProcessObject();
        void           popProcessObject();
    };

    struct RenderSwapData
    {
        std::optional<GameObjectResourceDesc> m_game_object_resource_desc;
        std::optional<GameObjectResourceDesc> m_game_object_to_delete;
        std::optional<CameraSwapData>         m_camera_swap_data;

        void addDirtyGameObject(GameObjectDesc desc);
        void addDeleteGameObject(GameObjectDesc desc);
    };

    enum SwapDataType : uint8_t
    {
        LogicSwapDataType = 0,
        RenderSwapDataType,
        SwapDataTypeCount
    };

    class RenderSwapContext
    {
    public:
        RenderSwapData& getLogicSwapData();
        RenderSwapData& getRenderSwapData();
        void            swapLogicRenderData();
        void            resetGameObjectResourceSwapData();
        void            resetGameObjectToDelete();
        void            resetCameraSwapData();

    private:
        uint8_t        m_logic_swap_data_index {LogicSwapDataType};
        uint8_t        m_render_swap_data_index {RenderSwapDataType};
        RenderSwapData m_swap_data[SwapDataTypeCount];

        bool isReadyToSwap() const;
        void swap();
    };
} // namespace MoYu
