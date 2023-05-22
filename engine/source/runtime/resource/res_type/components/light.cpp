#include "runtime/resource/res_type/components/light.h"

#include "runtime/core/base/macro.h"

namespace MoYu
{
    LightComponentRes::LightComponentRes(const LightComponentRes& res)
    {
        const std::string& camera_type_name = res.m_parameter.getTypeName();
        if (camera_type_name == "DirectionalLightParameter")
        {
            m_parameter = PILOT_REFLECTION_NEW(DirectionalLightParameter);
            PILOT_REFLECTION_DEEP_COPY(DirectionalLightParameter, m_parameter, res.m_parameter);
        }
        else if (camera_type_name == "PointLightParameter")
        {
            m_parameter = PILOT_REFLECTION_NEW(PointLightParameter);
            PILOT_REFLECTION_DEEP_COPY(PointLightParameter, m_parameter, res.m_parameter);
        }
        else if (camera_type_name == "SpotLightParameter")
        {
            m_parameter = PILOT_REFLECTION_NEW(SpotLightParameter);
            PILOT_REFLECTION_DEEP_COPY(SpotLightParameter, m_parameter, res.m_parameter);
        }
        else
        {
            LOG_ERROR("invalid light type");
        }
    }

    LightComponentRes::~LightComponentRes() { PILOT_REFLECTION_DELETE(m_parameter); }
} // namespace MoYu