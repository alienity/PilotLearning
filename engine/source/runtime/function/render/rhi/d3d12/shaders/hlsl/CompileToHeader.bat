@echo off
echo "start compiling ..."

set PATH=..\..\..\..\..\..\..\..\3rdparty\dxc_2023_08_14\bin\x64;

dxc -Zi -E"main" -Vn"g_pScreenQuadCommonVS" -D"_GAMING_DESKTOP=1" -T"vs_6_6" -Fh"..\CompiledShaders\ScreenQuadCommonVS.h" -Qembed_debug  -nologo ".\ScreenQuadCommonVS.hlsl"

dxc -Zi -E"main" -Vn"g_pDownsampleDepthPS" -D"_GAMING_DESKTOP=1" -T"ps_6_6" -Fh"..\CompiledShaders\DownsampleDepthPS.h" -Qembed_debug  -nologo ".\DownsampleDepthPS.hlsl"

dxc -Zi -E"main" -Vn"g_pGenerateMipsGammaCS" -D"_GAMING_DESKTOP=1" -T"cs_6_6" -Fh"..\CompiledShaders\GenerateMipsGammaCS.h" -Qembed_debug  -nologo ".\GenerateMipsGammaCS.hlsl"
dxc -Zi -E"main" -Vn"g_pGenerateMipsGammaOddCS" -D"_GAMING_DESKTOP=1" -T"cs_6_6" -Fh"..\CompiledShaders\GenerateMipsGammaOddCS.h" -Qembed_debug  -nologo ".\GenerateMipsGammaOddCS.hlsl"
dxc -Zi -E"main" -Vn"g_pGenerateMipsGammaOddXCS" -D"_GAMING_DESKTOP=1" -T"cs_6_6" -Fh"..\CompiledShaders\GenerateMipsGammaOddXCS.h" -Qembed_debug  -nologo ".\GenerateMipsGammaOddXCS.hlsl"
dxc -Zi -E"main" -Vn"g_pGenerateMipsGammaOddYCS" -D"_GAMING_DESKTOP=1" -T"cs_6_6" -Fh"..\CompiledShaders\GenerateMipsGammaOddYCS.h" -Qembed_debug  -nologo ".\GenerateMipsGammaOddYCS.hlsl"

dxc -Zi -E"main" -Vn"g_pGenerateMipsLinearCS" -D"_GAMING_DESKTOP=1" -T"cs_6_6" -Fh"..\CompiledShaders\GenerateMipsLinearCS.h" -Qembed_debug  -nologo ".\GenerateMipsLinearCS.hlsl"
dxc -Zi -E"main" -Vn"g_pGenerateMipsLinearOddCS" -D"_GAMING_DESKTOP=1" -T"cs_6_6" -Fh"..\CompiledShaders\GenerateMipsLinearOddCS.h" -Qembed_debug  -nologo ".\GenerateMipsLinearOddCS.hlsl"
dxc -Zi -E"main" -Vn"g_pGenerateMipsLinearOddXCS" -D"_GAMING_DESKTOP=1" -T"cs_6_6" -Fh"..\CompiledShaders\GenerateMipsLinearOddXCS.h" -Qembed_debug  -nologo ".\GenerateMipsLinearOddXCS.hlsl"
dxc -Zi -E"main" -Vn"g_pGenerateMipsLinearOddYCS" -D"_GAMING_DESKTOP=1" -T"cs_6_6" -Fh"..\CompiledShaders\GenerateMipsLinearOddYCS.h" -Qembed_debug  -nologo ".\GenerateMipsLinearOddYCS.hlsl"

dxc -Zi -E"main" -Vn"g_pGenerateMipsCS" -T"cs_6_6" -Fh"..\CompiledShaders\GenerateMipsCS.h" -Qembed_debug  -nologo ".\GenerateMips.hlsl"

echo "finish compiling ..."

