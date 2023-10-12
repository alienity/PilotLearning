#include "RenderGraphCommon.h"

namespace RHI
{

	RgResourceHandle _DefaultRgResourceHandle = {RgResourceType::Unknown, RG_RESOURCE_FLAG_NONE, 0, UINT_MAX};
    RgResourceHandleExt _DefaultRgResourceHandleExt = {_DefaultRgResourceHandle,
                                                       RgResourceState::RHI_RESOURCE_STATE_COMMON,
                                                       RgResourceState::RHI_RESOURCE_STATE_COMMON,
                                                       RgBarrierFlag::AutoBarrier};

}
