#pragma once
#include "runtime/resource/res_type/common_serializer.h"
#include "runtime/function/render/render_common.h"

namespace MoYu
{
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SceneImage, m_is_srgb, m_auto_mips, m_mip_levels, m_image_file)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MaterialImage, m_tilling, m_image)

    struct MaterialRes
    {
        std::string _ShaderName{ "" };
        MaterialImage _BaseColorMap{ DefaultMaterialImageWhite }; // white
        MaterialImage _MaskMap{ DefaultMaterialImageWhite }; // MaskMap is RGBA: Metallic, Ambient Occlusion (Optional, float) detail Mask (Optional, float) Smoothness
        MaterialImage _NormalMap{ DefaultMaterialImageBump }; // bump. Tangent space normal map
        MaterialImage _NormalMapOS{ DefaultMaterialImageWhite }; // white. Object space normal map - no good default value    
        MaterialImage _BentNormalMap{ DefaultMaterialImageBump }; // bump. Tangent space normal map
        MaterialImage _BentNormalMapOS{ DefaultMaterialImageWhite }; // white. Object space normal map - no good default value
        MaterialImage _HeightMap{ DefaultMaterialImageBlack }; // black
        MaterialImage _DetailMap{ DefaultMaterialImageGrey }; // linearGrey
        MaterialImage _TangentMap{ DefaultMaterialImageGrey }; // bump
        MaterialImage _TangentMapOS{ DefaultMaterialImageWhite }; // white
        MaterialImage _AnisotropyMap{ DefaultMaterialImageWhite }; // white
        MaterialImage _SubsurfaceMaskMap{ DefaultMaterialImageWhite }; // white. Subsurface Radius Map
        MaterialImage _TransmissionMaskMap{ DefaultMaterialImageWhite }; // white. Transmission Mask Map
        MaterialImage _ThicknessMap{ DefaultMaterialImageWhite }; // white. Thickness Map
        MaterialImage _IridescenceThicknessMap{ DefaultMaterialImageWhite }; // white. Iridescence Thickness Map
        MaterialImage _IridescenceMaskMap{ DefaultMaterialImageWhite }; // white. Iridescence Mask Map
        MaterialImage _CoatMaskMap{ DefaultMaterialImageWhite }; // white
        MaterialImage _EmissiveColorMap{ DefaultMaterialImageBlack }; // white
        MaterialImage _TransmittanceColorMap{ DefaultMaterialImageWhite }; // white
        glm::float4 _BaseColor{ 1, 1, 1, 1 };
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
        float _NormalScale{ 1.0f };
        float _HeightAmplitude{ 0.02f }; // Height Amplitude. In world units. Default value of _HeightAmplitude must be (_HeightMax - _HeightMin) * 0.01
        float _HeightCenter{ 0.5f }; // Height Center. In texture space
        int _HeightMapParametrization{ 0 }; // Heightmap Parametrization. MinMax, 0, Amplitude, 1
        // These parameters are for vertex displacement/Tessellation
        float _HeightOffset{ 0 }; // Height Offset
        // MinMax mode
        float _HeightMin{ -1 }; // Heightmap Min
        float _HeightMax{ 1 }; // Heightmap Max
        // Amplitude mode
        float _HeightTessAmplitude{ 2.0f }; // in Centimeters
        float _HeightTessCenter{ 0.5f }; // In texture space
        // These parameters are for pixel displacement
        float _HeightPoMAmplitude{ 2.0f }; // Height Amplitude. In centimeters
        float _DetailAlbedoScale{ 1.0f };
        float _DetailNormalScale{ 1.0f };
        float _DetailSmoothnessScale{ 1.0f };
        float _Anisotropy{ 0 };
        float _DiffusionProfileHash{ 0 };
        float _SubsurfaceMask{ 1.0f }; // Subsurface Radius
        float _TransmissionMask{ 1.0f }; // Transmission Mask
        float _Thickness{ 1.0f };
        glm::float4 _ThicknessRemap{ 0, 1, 0, 0 };
        glm::float4 _IridescenceThicknessRemap{ 0, 1, 0, 0 };
        float _IridescenceThickness{ 1.0f };
        float _IridescenceMask{ 1.0f };
        float _CoatMask{ 0 };
        float _EnergyConservingSpecularColor{ 1.0f };
        int _SpecularOcclusionMode{ 1 }; // Off, 0, From Ambient Occlusion, 1, From AO and Bent Normals, 2
        glm::float3 _EmissiveColor{ 0, 0, 0 };
        float _AlbedoAffectEmissive{ 0 }; // Albedo Affect Emissive
        int _EmissiveIntensityUnit{ 0 }; // Emissive Mode
        int _UseEmissiveIntensity{ 0 }; // Use Emissive Intensity
        float _EmissiveIntensity{ 1.0f }; // Emissive Intensity
        float _EmissiveExposureWeight{ 1.0f }; // Emissive Pre Exposure
        float _UseShadowThreshold{ 0 };
        float _AlphaCutoffEnable{ 0 };
        float _AlphaCutoff{ 0.5f };
        float _AlphaCutoffShadow{ 0.5f };
        float _AlphaCutoffPrepass{ 0.5f };
        float _AlphaCutoffPostpass{ 0.5f };
        float _TransparentDepthPrepassEnable{ 0 };
        float _TransparentBackfaceEnable{ 0 };
        float _TransparentDepthPostpassEnable{ 0 };
        float _TransparentSortPriority{ 0 };
        // Transparency
        int _RefractionModel{ 0 }; // None, 0, Planar, 1, Sphere, 2, Thin, 3
        float _Ior{ 1.5f };
        glm::float3 _TransmittanceColor{ 1.0f, 1.0f, 1.0f };
        float _ATDistance{ 1.0f };
        float _TransparentWritingMotionVec{ 0 };
        // Blending state
        float _SurfaceType{ 0 };
        float _BlendMode{ 0 };
        float _SrcBlend{ 1.0f };
        float _DstBlend{ 0 };
        float _AlphaSrcBlend{ 1.0f };
        float _AlphaDstBlend{ 0 };
        float _EnableFogOnTransparent{ 1.0f }; // Enable Fog
        float _DoubleSidedEnable{ 0 }; // Double sided enable
        float _DoubleSidedNormalMode{ 1 }; // Flip, 0, Mirror, 1, None, 2
        glm::float4 _DoubleSidedConstants{ 1, 1, -1, 0 };
        float _DoubleSidedGIMode{ 0 }; // Auto, 0, On, 1, Off, 2
        float _UVBase{ 0 }; // UV0, 0, UV1, 1, UV2, 2, UV3, 3, Planar, 4, Triplanar, 5
        float _ObjectSpaceUVMapping{ 0 }; // WorldSpace, 0, ObjectSpace, 1
        float _TexWorldScale{ 1 }; // Scale to apply on world coordinate
        glm::float4 _UVMappingMask{ 1, 0, 0, 0 };
        float _NormalMapSpace{ 0 }; // TangentSpace, 0, ObjectSpace, 1
        int _MaterialID{ 1 }; // Subsurface Scattering, 0, Standard, 1, Anisotropy, 2, Iridescence, 3, Specular Color, 4, Translucent, 5
        float _TransmissionEnable{ 1 };
        float _PPDMinSamples{ 5 }; // Min sample for POM
        float _PPDMaxSamples{ 15 }; // Max sample for POM
        float _PPDLodThreshold{ 5 }; // Start lod to fade out the POM effect
        float _PPDPrimitiveLength{ 1 }; // Primitive length for POM
        float _PPDPrimitiveWidth{ 1 }; // Primitive width for POM
        glm::float4 _InvPrimScale{ 1, 1, 0, 0 }; // Inverse primitive scale for non-planar POM
        glm::float4 _UVDetailsMappingMask{ 1, 0, 0, 0 };
        float _UVDetail{ 0 }; // UV0, 0, UV1, 1, UV2, 2, UV3, 3. UV Set for detail
        float _LinkDetailsWithBase{ 1.0f };
        float _EmissiveColorMode{ 1 }; // Use Emissive Color, 0, Use Emissive Mask, 1. Emissive color mode
        float _UVEmissive{ 0 }; // UV0, 0, UV1, 1, UV2, 2, UV3, 3, Planar, 4, Triplanar, 5, Same as Base, 6. UV Set for emissive
    };

    inline void to_json(json& j, const MaterialRes& P)
    {
#define MaterialResJsonPair(Name) {#Name, P.Name}
        j = {
            MaterialResJsonPair(_ShaderName),
            MaterialResJsonPair(_BaseColorMap),
            MaterialResJsonPair(_MaskMap),
            MaterialResJsonPair(_NormalMap),
            MaterialResJsonPair(_NormalMapOS),
            MaterialResJsonPair(_BentNormalMap),
            MaterialResJsonPair(_BentNormalMapOS),
            MaterialResJsonPair(_HeightMap),
            MaterialResJsonPair(_DetailMap),
            MaterialResJsonPair(_TangentMap),
            MaterialResJsonPair(_TangentMapOS),
            MaterialResJsonPair(_AnisotropyMap),
            MaterialResJsonPair(_SubsurfaceMaskMap),
            MaterialResJsonPair(_TransmissionMaskMap),
            MaterialResJsonPair(_ThicknessMap),
            MaterialResJsonPair(_IridescenceThicknessMap),
            MaterialResJsonPair(_IridescenceMaskMap),
            MaterialResJsonPair(_CoatMaskMap),
            MaterialResJsonPair(_EmissiveColorMap),
            MaterialResJsonPair(_TransmittanceColorMap),
            MaterialResJsonPair(_BaseColor),
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
            MaterialResJsonPair(_NormalScale),
            MaterialResJsonPair(_HeightAmplitude),
            MaterialResJsonPair(_HeightCenter),
            MaterialResJsonPair(_HeightMapParametrization),
            MaterialResJsonPair(_HeightOffset),
            MaterialResJsonPair(_HeightMin),
            MaterialResJsonPair(_HeightMax),
            MaterialResJsonPair(_HeightTessAmplitude),
            MaterialResJsonPair(_HeightTessCenter),
            MaterialResJsonPair(_HeightPoMAmplitude),
            MaterialResJsonPair(_DetailAlbedoScale),
            MaterialResJsonPair(_DetailNormalScale),
            MaterialResJsonPair(_DetailSmoothnessScale),
            MaterialResJsonPair(_Anisotropy),
            MaterialResJsonPair(_DiffusionProfileHash),
            MaterialResJsonPair(_SubsurfaceMask),
            MaterialResJsonPair(_TransmissionMask),
            MaterialResJsonPair(_Thickness),
            MaterialResJsonPair(_ThicknessRemap),
            MaterialResJsonPair(_IridescenceThicknessRemap),
            MaterialResJsonPair(_IridescenceThickness),
            MaterialResJsonPair(_IridescenceMask),
            MaterialResJsonPair(_CoatMask),
            MaterialResJsonPair(_EnergyConservingSpecularColor),
            MaterialResJsonPair(_SpecularOcclusionMode),
            MaterialResJsonPair(_EmissiveColor),
            MaterialResJsonPair(_AlbedoAffectEmissive),
            MaterialResJsonPair(_EmissiveIntensityUnit),
            MaterialResJsonPair(_UseEmissiveIntensity),
            MaterialResJsonPair(_EmissiveIntensity),
            MaterialResJsonPair(_EmissiveExposureWeight),
            MaterialResJsonPair(_UseShadowThreshold),
            MaterialResJsonPair(_AlphaCutoffEnable),
            MaterialResJsonPair(_AlphaCutoff),
            MaterialResJsonPair(_AlphaCutoffShadow),
            MaterialResJsonPair(_AlphaCutoffPrepass),
            MaterialResJsonPair(_AlphaCutoffPostpass),
            MaterialResJsonPair(_TransparentDepthPrepassEnable),
            MaterialResJsonPair(_TransparentBackfaceEnable),
            MaterialResJsonPair(_TransparentDepthPostpassEnable),
            MaterialResJsonPair(_TransparentSortPriority),
            MaterialResJsonPair(_RefractionModel),
            MaterialResJsonPair(_Ior),
            MaterialResJsonPair(_TransmittanceColor),
            MaterialResJsonPair(_ATDistance),
            MaterialResJsonPair(_TransparentWritingMotionVec),
            MaterialResJsonPair(_SurfaceType),
            MaterialResJsonPair(_BlendMode),
            MaterialResJsonPair(_SrcBlend),
            MaterialResJsonPair(_DstBlend),
            MaterialResJsonPair(_AlphaSrcBlend),
            MaterialResJsonPair(_AlphaDstBlend),
            MaterialResJsonPair(_EnableFogOnTransparent),
            MaterialResJsonPair(_DoubleSidedEnable),
            MaterialResJsonPair(_DoubleSidedNormalMode),
            MaterialResJsonPair(_DoubleSidedConstants),
            MaterialResJsonPair(_DoubleSidedGIMode),
            MaterialResJsonPair(_UVBase),
            MaterialResJsonPair(_ObjectSpaceUVMapping),
            MaterialResJsonPair(_TexWorldScale),
            MaterialResJsonPair(_UVMappingMask),
            MaterialResJsonPair(_NormalMapSpace),
            MaterialResJsonPair(_MaterialID),
            MaterialResJsonPair(_TransmissionEnable),
            MaterialResJsonPair(_PPDMinSamples),
            MaterialResJsonPair(_PPDMaxSamples),
            MaterialResJsonPair(_PPDLodThreshold),
            MaterialResJsonPair(_PPDPrimitiveLength),
            MaterialResJsonPair(_PPDPrimitiveWidth),
            MaterialResJsonPair(_InvPrimScale),
            MaterialResJsonPair(_UVDetailsMappingMask),
            MaterialResJsonPair(_UVDetail),
            MaterialResJsonPair(_LinkDetailsWithBase),
            MaterialResJsonPair(_EmissiveColorMode),
            MaterialResJsonPair(_UVEmissive),
        };
    }

    inline void from_json(const json& j, MaterialRes& P)
    {
#define MaterialResJsonPair2(Name, Type) P.Name = j.at(#Name).get<Type>();

        MaterialResJsonPair2(_ShaderName, std::string)
        MaterialResJsonPair2(_BaseColorMap, MaterialImage)
        MaterialResJsonPair2(_MaskMap, MaterialImage)
        MaterialResJsonPair2(_NormalMap, MaterialImage)
        MaterialResJsonPair2(_NormalMapOS, MaterialImage)
        MaterialResJsonPair2(_BentNormalMap, MaterialImage)
        MaterialResJsonPair2(_BentNormalMapOS, MaterialImage)
        MaterialResJsonPair2(_HeightMap, MaterialImage)
        MaterialResJsonPair2(_DetailMap, MaterialImage)
        MaterialResJsonPair2(_TangentMap, MaterialImage)
        MaterialResJsonPair2(_TangentMapOS, MaterialImage)
        MaterialResJsonPair2(_AnisotropyMap, MaterialImage)
        MaterialResJsonPair2(_SubsurfaceMaskMap, MaterialImage)
        MaterialResJsonPair2(_TransmissionMaskMap, MaterialImage)
        MaterialResJsonPair2(_ThicknessMap, MaterialImage)
        MaterialResJsonPair2(_IridescenceThicknessMap, MaterialImage)
        MaterialResJsonPair2(_IridescenceMaskMap, MaterialImage)
        MaterialResJsonPair2(_CoatMaskMap, MaterialImage)
        MaterialResJsonPair2(_EmissiveColorMap, MaterialImage)
        MaterialResJsonPair2(_TransmittanceColorMap, MaterialImage)
        MaterialResJsonPair2(_BaseColor, glm::float4)
        MaterialResJsonPair2(_Metallic, float)
        MaterialResJsonPair2(_Smoothness, float)
        MaterialResJsonPair2(_MetallicRemapMin, float)
        MaterialResJsonPair2(_MetallicRemapMax, float)
        MaterialResJsonPair2(_SmoothnessRemapMin, float)
        MaterialResJsonPair2(_SmoothnessRemapMax, float)
        MaterialResJsonPair2(_AlphaRemapMin, float)
        MaterialResJsonPair2(_AlphaRemapMax, float)
        MaterialResJsonPair2(_AORemapMin, float)
        MaterialResJsonPair2(_AORemapMax, float)
        MaterialResJsonPair2(_NormalScale, float)
        MaterialResJsonPair2(_HeightAmplitude, float)
        MaterialResJsonPair2(_HeightCenter, float)
        MaterialResJsonPair2(_HeightMapParametrization, int)
        MaterialResJsonPair2(_HeightOffset, float)
        MaterialResJsonPair2(_HeightMin, float)
        MaterialResJsonPair2(_HeightMax, float)
        MaterialResJsonPair2(_HeightTessAmplitude, float)
        MaterialResJsonPair2(_HeightTessCenter, float)
        MaterialResJsonPair2(_HeightPoMAmplitude, float)
        MaterialResJsonPair2(_DetailAlbedoScale, float)
        MaterialResJsonPair2(_DetailNormalScale, float)
        MaterialResJsonPair2(_DetailSmoothnessScale, float)
        MaterialResJsonPair2(_Anisotropy, float)
        MaterialResJsonPair2(_DiffusionProfileHash, float)
        MaterialResJsonPair2(_SubsurfaceMask, float)
        MaterialResJsonPair2(_TransmissionMask, float)
        MaterialResJsonPair2(_Thickness, float)
        MaterialResJsonPair2(_ThicknessRemap, glm::float4)
        MaterialResJsonPair2(_IridescenceThicknessRemap, glm::float4)
        MaterialResJsonPair2(_IridescenceThickness, float)
        MaterialResJsonPair2(_IridescenceMask, float)
        MaterialResJsonPair2(_CoatMask, float)
        MaterialResJsonPair2(_EnergyConservingSpecularColor, float)
        MaterialResJsonPair2(_SpecularOcclusionMode, int)
        MaterialResJsonPair2(_EmissiveColor, glm::float3)
        MaterialResJsonPair2(_AlbedoAffectEmissive, float)
        MaterialResJsonPair2(_EmissiveIntensityUnit, int)
        MaterialResJsonPair2(_UseEmissiveIntensity, int)
        MaterialResJsonPair2(_EmissiveIntensity, float)
        MaterialResJsonPair2(_EmissiveExposureWeight, float)
        MaterialResJsonPair2(_UseShadowThreshold, float)
        MaterialResJsonPair2(_AlphaCutoffEnable, float)
        MaterialResJsonPair2(_AlphaCutoff, float)
        MaterialResJsonPair2(_AlphaCutoffShadow, float)
        MaterialResJsonPair2(_AlphaCutoffPrepass, float)
        MaterialResJsonPair2(_AlphaCutoffPostpass, float)
        MaterialResJsonPair2(_TransparentDepthPrepassEnable, float)
        MaterialResJsonPair2(_TransparentBackfaceEnable, float)
        MaterialResJsonPair2(_TransparentDepthPostpassEnable, float)
        MaterialResJsonPair2(_TransparentSortPriority, float)
        MaterialResJsonPair2(_RefractionModel, int)
        MaterialResJsonPair2(_Ior, float)
        MaterialResJsonPair2(_TransmittanceColor, glm::float3)
        MaterialResJsonPair2(_ATDistance, float)
        MaterialResJsonPair2(_TransparentWritingMotionVec, float)
        MaterialResJsonPair2(_SurfaceType, float)
        MaterialResJsonPair2(_BlendMode, float)
        MaterialResJsonPair2(_SrcBlend, float)
        MaterialResJsonPair2(_DstBlend, float)
        MaterialResJsonPair2(_AlphaSrcBlend, float)
        MaterialResJsonPair2(_AlphaDstBlend, float)
        MaterialResJsonPair2(_EnableFogOnTransparent, float)
        MaterialResJsonPair2(_DoubleSidedEnable, float)
        MaterialResJsonPair2(_DoubleSidedNormalMode, float)
        MaterialResJsonPair2(_DoubleSidedConstants, glm::float4)
        MaterialResJsonPair2(_DoubleSidedGIMode, float)
        MaterialResJsonPair2(_UVBase, float)
        MaterialResJsonPair2(_ObjectSpaceUVMapping, float)
        MaterialResJsonPair2(_TexWorldScale, float)
        MaterialResJsonPair2(_UVMappingMask, glm::float4)
        MaterialResJsonPair2(_NormalMapSpace, float)
        MaterialResJsonPair2(_MaterialID, int)
        MaterialResJsonPair2(_TransmissionEnable, float)
        MaterialResJsonPair2(_PPDMinSamples, float)
        MaterialResJsonPair2(_PPDMaxSamples, float)
        MaterialResJsonPair2(_PPDLodThreshold, float)
        MaterialResJsonPair2(_PPDPrimitiveLength, float)
        MaterialResJsonPair2(_PPDPrimitiveWidth, float)
        MaterialResJsonPair2(_InvPrimScale, glm::float4)
        MaterialResJsonPair2(_UVDetailsMappingMask, glm::float4)
        MaterialResJsonPair2(_UVDetail, float)
        MaterialResJsonPair2(_LinkDetailsWithBase, float)
        MaterialResJsonPair2(_EmissiveColorMode, float)
        MaterialResJsonPair2(_UVEmissive, float)
    }

} // namespace MoYu