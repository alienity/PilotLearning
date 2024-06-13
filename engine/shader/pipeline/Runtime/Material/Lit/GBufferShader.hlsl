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
#define SHADERPASS SHADERPASS_GBUFFER

#include "../../Material/Material.hlsl"
#include "../../Material/Lit/Lit.hlsl"
#include "../../Material/Lit/ShaderPass/LitSharePass.hlsl"
#include "../../Material/Lit/LitData.hlsl"

#include "../../CustomInput/CustomInput.hlsl"

#include "../../RenderPipeline/ShaderPass/ShaderPassGBuffer.hlsl"

//#pragma vertex Vert
//#pragma fragment Frag
