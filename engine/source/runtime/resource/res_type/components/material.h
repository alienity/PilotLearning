#pragma once
#include "runtime/resource/res_type/common_serializer.h"
#include "runtime/function/render/render_common.h"

namespace MoYu
{
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SceneImage, m_is_srgb, m_auto_mips, m_mip_levels, m_image_file)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MaterialImage, m_tilling, m_image)

    struct MaterialRes
    {
        std::string _ShaderName {""};

        glm::float4 _BaseColor{ 1, 1, 1, 1 };
        MaterialImage _BaseColorMap{};

        float _Metallic{ 0 };
        float _Smoothness{ 0.5f };
        float _MetallicRemapMin{ 0.0f };
        float _MetallicRemapMax{ 1.0f };
        float _SmoothnessRemapMin{ 0.0f };
        float _SmoothnessRemapMax{ 1.0f };
        float _AlphaRemapMin{ 0.0f };
        float _AlphaRemapMax{ 1.0f };
        float _AORemapMin{ 0.0f };
        float _AORemapMax{ 1.0f };

        MaterialImage _NormalMap{}; // Tangent space normal map
        MaterialImage _NormalMapOS{}; // Object space normal map - no good default value
        float _NormalScale{ 1.0f };

        MaterialImage _BentNormalMap{};
        MaterialImage _BentNormalMapOS{};

        MaterialImage _HeightMap{};
        float _HeightAmplitude;
        float _HeightCenter;

        MaterialImage _DetailMap{};
        float _DetailAlbedoScale{ 1.0f };
        float _DetailNormalScale{ 1.0f };
        float _DetailSmoothnessScale{ 1.0f };

        MaterialImage _TangentMap{};
        MaterialImage _TangentMapOS{};
        float _Anisotropy{ 0 };
        MaterialImage _AnisotropyMap{};

        float _SubsurfaceMasks{ 1.0f };
        MaterialImage _SubsurfaceMaskMap{};
        float _TransmissionMask{ 1.0f };
        MaterialImage _TransmissionMaskMap{};
        float _Thickness{ 1.0f };
        MaterialImage _ThicknessMap{};
        glm::float4 _ThicknessRemap{ 0,1,0,0 };

        float _IridescenceThickness{ 1.0f };
        MaterialImage _IridescenceThicknessMap{};
        glm::float4 _IridescenceThicknessRemap{ 0, 1, 0, 0 };
        float _IridescenceMask{ 1.0f };
        MaterialImage _IridescenceMaskMap{};

        float _CoatMask{ 0.0f };
        MaterialImage _CoatMaskMap{};

        float _EnergyConservingSpecularColor{ 1.0f };
        glm::float4 _SpecularColor{ 1,1,1,1 };
        MaterialImage _SpecularColorMap{};

        int _SpecularOcclusionMode{ 1 };

        glm::float3 _EmissiveColor{ 0,0,0 };
        float _AlbedoAffectEmissive{ 0.0f };
        float _EmissiveExposureWeight{ 1.0f };

        float _UseShadowThreshold{ 0.0f };
        float _AlphaCutoff{ 0.5f };
        float _AlphaCutoffShadow{ 0.5f };
        float _AlphaCutoffPrepass{ 0.5f };
        float _AlphaCutoffPostpass{ 0.5f };

        // Transparency
        float _Ior{ 1.5f };
        glm::float3 _TransmittanceColor{ 1.0, 1.0, 1.0 };
        MaterialImage _TransmittanceColorMap{};
        float _ATDistance{ 1.0f };

        // Blending state
        float _SurfaceType{ 0.0f };
        float _BlendMode{ 0.0f };

        float _EnableBlendModePreserveSpecularLighting{ 1.0 };

        float _DoubleSidedEnable{ 0.0f };

        float _ObjectSpaceUVMapping{ 0.0f };
        float _InvTilingScale{ 1.0f };
        float _TexWorldScale{ 1.0f };
        glm::float4 _UVMappingMask{ 1,0,0,0 };

        float _PPDMinSamples{ 5 };
        float _PPDMaxSamples{ 15 };
        int _PPDLodThreshold{ 5 };
    };

    inline void to_json(json& j, const MaterialRes& P)
    {
#define MaterialResJsonPair(Name) {#Name, P.Name}
        j = {
            MaterialResJsonPair(_ShaderName),
            MaterialResJsonPair(_BaseColor),
            MaterialResJsonPair(_BaseColorMap),
            MaterialResJsonPair(_Metallic),
            MaterialResJsonPair(_Smoothness),
            MaterialResJsonPair(_MetallicRemapMin),
            MaterialResJsonPair(_MetallicRemapMax),
            MaterialResJsonPair(_SmoothnessRemapMin),
            MaterialResJsonPair(_SmoothnessRemapMax),
            MaterialResJsonPair(_AlphaRemapMin),
            MaterialResJsonPair(_AlphaRemapMax),
            MaterialResJsonPair(_AORemapMin),
            MaterialResJsonPair(_AORemapMax),
            MaterialResJsonPair(_NormalMap),
            MaterialResJsonPair(_NormalMapOS),
            MaterialResJsonPair(_NormalScale),
            MaterialResJsonPair(_BentNormalMap),
            MaterialResJsonPair(_BentNormalMapOS),
            MaterialResJsonPair(_HeightAmplitude),
            MaterialResJsonPair(_HeightCenter),
            MaterialResJsonPair(_DetailMap),
            MaterialResJsonPair(_DetailAlbedoScale),
            MaterialResJsonPair(_DetailNormalScale),
            MaterialResJsonPair(_DetailSmoothnessScale),
            MaterialResJsonPair(_TangentMap),
            MaterialResJsonPair(_TangentMapOS),
            MaterialResJsonPair(_Anisotropy),
            MaterialResJsonPair(_AnisotropyMap),
            MaterialResJsonPair(_SubsurfaceMasks),
            MaterialResJsonPair(_SubsurfaceMaskMap),
            MaterialResJsonPair(_TransmissionMask),
            MaterialResJsonPair(_TransmissionMaskMap),
            MaterialResJsonPair(_Thickness),
            MaterialResJsonPair(_ThicknessMap),
            MaterialResJsonPair(_ThicknessRemap),
            MaterialResJsonPair(_IridescenceThickness),
            MaterialResJsonPair(_IridescenceThicknessMap),
            MaterialResJsonPair(_IridescenceThicknessRemap),
            MaterialResJsonPair(_IridescenceMask),
            MaterialResJsonPair(_IridescenceMaskMap),
            MaterialResJsonPair(_CoatMask),
            MaterialResJsonPair(_CoatMaskMap),
            MaterialResJsonPair(_EnergyConservingSpecularColor),
            MaterialResJsonPair(_SpecularColor),
            MaterialResJsonPair(_SpecularColorMap),
            MaterialResJsonPair(_SpecularOcclusionMode),
            MaterialResJsonPair(_EmissiveColor),
            MaterialResJsonPair(_AlbedoAffectEmissive),
            MaterialResJsonPair(_EmissiveExposureWeight),
            MaterialResJsonPair(_UseShadowThreshold),
            MaterialResJsonPair(_AlphaCutoff),
            MaterialResJsonPair(_AlphaCutoffShadow),
            MaterialResJsonPair(_AlphaCutoffPrepass),
            MaterialResJsonPair(_AlphaCutoffPostpass),
            MaterialResJsonPair(_Ior),
            MaterialResJsonPair(_TransmittanceColor),
            MaterialResJsonPair(_TransmittanceColorMap),
            MaterialResJsonPair(_ATDistance),
            MaterialResJsonPair(_SurfaceType),
            MaterialResJsonPair(_BlendMode),
            MaterialResJsonPair(_EnableBlendModePreserveSpecularLighting),
            MaterialResJsonPair(_DoubleSidedEnable),
            MaterialResJsonPair(_ObjectSpaceUVMapping),
            MaterialResJsonPair(_InvTilingScale),
            MaterialResJsonPair(_TexWorldScale),
            MaterialResJsonPair(_UVMappingMask),
            MaterialResJsonPair(_PPDMinSamples),
            MaterialResJsonPair(_PPDMaxSamples),
            MaterialResJsonPair(_PPDLodThreshold)
        };
    }

    inline void from_json(const json& j, MaterialRes& P)
    {
#define MaterialResJsonPair2(Name, Type) P.Name = j.at(#Name).get<Type>();

        MaterialResJsonPair2(_ShaderName, std::string);
        MaterialResJsonPair2(_BaseColor, glm::float4);
        MaterialResJsonPair2(_BaseColorMap, MaterialImage);
        MaterialResJsonPair2(_Metallic, float);
        MaterialResJsonPair2(_Smoothness, float);
        MaterialResJsonPair2(_MetallicRemapMin, float);
        MaterialResJsonPair2(_MetallicRemapMax, float);
        MaterialResJsonPair2(_SmoothnessRemapMin, float);
        MaterialResJsonPair2(_SmoothnessRemapMax, float);
        MaterialResJsonPair2(_AlphaRemapMin, float);
        MaterialResJsonPair2(_AlphaRemapMax, float);
        MaterialResJsonPair2(_AORemapMin, float);
        MaterialResJsonPair2(_AORemapMax, float);
        MaterialResJsonPair2(_NormalMap, MaterialImage);
        MaterialResJsonPair2(_NormalMapOS, MaterialImage);
        MaterialResJsonPair2(_NormalScale, float);
        MaterialResJsonPair2(_BentNormalMap, MaterialImage);
        MaterialResJsonPair2(_BentNormalMapOS, MaterialImage);
        MaterialResJsonPair2(_HeightAmplitude, float);
        MaterialResJsonPair2(_HeightCenter, float);
        MaterialResJsonPair2(_DetailMap, MaterialImage);
        MaterialResJsonPair2(_DetailAlbedoScale, float);
        MaterialResJsonPair2(_DetailNormalScale, float);
        MaterialResJsonPair2(_DetailSmoothnessScale, float);
        MaterialResJsonPair2(_TangentMap, MaterialImage);
        MaterialResJsonPair2(_TangentMapOS, MaterialImage);
        MaterialResJsonPair2(_Anisotropy, float);
        MaterialResJsonPair2(_AnisotropyMap, MaterialImage);
        MaterialResJsonPair2(_SubsurfaceMasks, float);
        MaterialResJsonPair2(_SubsurfaceMaskMap, MaterialImage);
        MaterialResJsonPair2(_TransmissionMask, float);
        MaterialResJsonPair2(_TransmissionMaskMap, MaterialImage);
        MaterialResJsonPair2(_Thickness, float);
        MaterialResJsonPair2(_ThicknessMap, MaterialImage);
        MaterialResJsonPair2(_ThicknessRemap, glm::float4);
        MaterialResJsonPair2(_IridescenceThickness, float);
        MaterialResJsonPair2(_IridescenceThicknessMap, MaterialImage);
        MaterialResJsonPair2(_IridescenceThicknessRemap, glm::float4);
        MaterialResJsonPair2(_IridescenceMask, float);
        MaterialResJsonPair2(_IridescenceMaskMap, MaterialImage);
        MaterialResJsonPair2(_CoatMask, float);
        MaterialResJsonPair2(_CoatMaskMap, MaterialImage);
        MaterialResJsonPair2(_EnergyConservingSpecularColor, float);
        MaterialResJsonPair2(_SpecularColor, glm::float4);
        MaterialResJsonPair2(_SpecularColorMap, MaterialImage);
        MaterialResJsonPair2(_SpecularOcclusionMode, int);
        MaterialResJsonPair2(_EmissiveColor, glm::float3);
        MaterialResJsonPair2(_AlbedoAffectEmissive, float);
        MaterialResJsonPair2(_EmissiveExposureWeight, float);
        MaterialResJsonPair2(_UseShadowThreshold, float);
        MaterialResJsonPair2(_AlphaCutoff, float);
        MaterialResJsonPair2(_AlphaCutoffShadow, float);
        MaterialResJsonPair2(_AlphaCutoffPrepass, float);
        MaterialResJsonPair2(_AlphaCutoffPostpass, float);
        MaterialResJsonPair2(_Ior, float);
        MaterialResJsonPair2(_TransmittanceColor, glm::float3);
        MaterialResJsonPair2(_TransmittanceColorMap, MaterialImage);
        MaterialResJsonPair2(_ATDistance, float);
        MaterialResJsonPair2(_SurfaceType, float);
        MaterialResJsonPair2(_BlendMode, float);
        MaterialResJsonPair2(_EnableBlendModePreserveSpecularLighting, float);
        MaterialResJsonPair2(_DoubleSidedEnable, float);
        MaterialResJsonPair2(_ObjectSpaceUVMapping, float);
        MaterialResJsonPair2(_InvTilingScale, float);
        MaterialResJsonPair2(_TexWorldScale, float);
        MaterialResJsonPair2(_UVMappingMask, glm::float4);
        MaterialResJsonPair2(_PPDMinSamples, float);
        MaterialResJsonPair2(_PPDMaxSamples, float);
        MaterialResJsonPair2(_PPDLodThreshold, int);
    }

} // namespace MoYu