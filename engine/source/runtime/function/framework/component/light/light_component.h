#pragma once

#include "runtime/core/math/moyu_math.h"
#include "runtime/resource/res_type/components/light.h"
#include "runtime/function/framework/component/component.h"

namespace MoYu
{
    class RenderCamera;

    class LightComponent : public Component
    {
    public:
        LightComponent() = default;

        void reset();

        void postLoadResource(std::weak_ptr<GObject> object, void* data) override;

        void tick(float delta_time) override;

    //private:
        // Editor直接编辑这个，作为当前帧的修改对象，修改后一定要设置SetDirtyFlag
        LightComponentRes m_light_res;

        // 假设1是当前帧，2是下一帧
        // 1是空，2是有Light的，添加光源
        // 1是有Light的，2是空，删除光源
        // 1和2都有Light，参数不同，更新光源
        LightComponentRes m_light_res_buffer[2];
        uint32_t m_current_index {0};
        uint32_t m_next_index {1};
    };
} // namespace MoYu
