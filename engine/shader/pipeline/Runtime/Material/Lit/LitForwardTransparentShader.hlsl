//-------------------------------------------------------------------------------------
// Define
//-------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------
// Include
//-------------------------------------------------------------------------------------

#include "../../ShaderLibrary/Common.hlsl"
#include "../../ShaderLibrary/ShaderVariables.hlsl"
#include "../../RenderPipeline/ShaderPass/FragInputs.hlsl"
#include "../../RenderPipeline/ShaderPass/ShaderPass.hlsl"

//-------------------------------------------------------------------------------------
// variable declaration
//-------------------------------------------------------------------------------------

#include "../../Material/Lit/LitProperties.hlsl"

//-------------------------------------------------------------------------------------
// Forward Pass
//-------------------------------------------------------------------------------------

#define PROBE_VOLUMES_OFF
#define SCREEN_SPACE_SHADOWS_OFF
#define SHADERPASS SHADERPASS_FORWARD
#define _SURFACE_TYPE_TRANSPARENT
#define _NORMALMAP
#define _NORMALMAP_TANGENT_SPACE
#define _MASKMAP
#define SHADOW_LOW
#define UNITY_NO_DXT5nm

#include "../../Material/Material.hlsl"
#include "../../Lighting/Lighting.hlsl"

#define HAS_LIGHTLOOP
#include "../../Lighting/LightLoop/LightLoopDef.hlsl"
#include "../../Material/Lit/Lit.hlsl"
#include "../../Lighting/LightLoop/LightLoop.hlsl"

#include "../../Material/Lit/ShaderPass/LitSharePass.hlsl"
#include "../../Material/Lit/LitData.hlsl"

#include "../../CustomInput/CustomInput.hlsl"

#include "../../RenderPipeline/ShaderPass/ShaderPassForward.hlsl"

//#pragma vertex Vert
//#pragma fragment Frag
