#include "RenderGraphCommon.h"

namespace RHI
{

	RgResourceHandle _DefaultRgResourceHandle = {RgResourceType::Unknown, RG_RESOURCE_FLAG_NONE, 0, UINT_MAX};
    RgResourceHandleExt _DefaultRgResourceHandleExt = {_DefaultRgResourceHandle, RgResourceSubType::NoneType, RgBarrierFlag::AutoBarrier};

}
