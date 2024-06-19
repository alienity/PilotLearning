#ifndef MATERIAL_PROPERTIES_HLSL
#define MATERIAL_PROPERTIES_HLSL

#ifdef _CPP_MACRO_
#define uint glm::uint
#define uint2 glm::uvec2
#define uint3 glm::uvec3
#define uint4 glm::uvec4
#define int2 glm::int2
#define int3 glm::int3
#define int4 glm::int4
#define float2 glm::fvec2
#define float3 glm::fvec3
#define float4 glm::fvec4
#define float4x4 glm::float4x4
#define float2x4 glm::float2x4
#endif

struct PropertiesPerMaterial
{
    //=======================Texture=======================
    uint _BaseColorMapIndex; // white
    uint _MaskMapIndex; // MaskMap is RGBA: Metallic, Ambient Occlusion (Optional), detail Mask (Optional), Smoothness
    uint _NormalMapIndex; // bump. Tangent space normal map
    uint _NormalMapOSIndex; // white. Object space normal map - no good default value    
    uint _BentNormalMapIndex; // bump. Tangent space normal map
    uint _BentNormalMapOSIndex; // white. Object space normal map - no good default value
    uint _HeightMapIndex; // black
    uint _DetailMapIndex; // linearGrey
    uint _TangentMapIndex; // bump
    uint _TangentMapOSIndex; // white
    uint _AnisotropyMapIndex; // white
    uint _SubsurfaceMaskMapIndex; // white. Subsurface Radius Map
    uint _TransmissionMaskMapIndex; // white. Transmission Mask Map
    uint _ThicknessMapIndex; // white. Thickness Map
    uint _IridescenceThicknessMapIndex; // white. Iridescence Thickness Map
    uint _IridescenceMaskMapIndex; // white. Iridescence Mask Map
    uint _CoatMaskMapIndex; // white
    uint _SpecularColorMapIndex; // white
    uint _EmissiveColorMapIndex; // white
    uint _TransmittanceColorMapIndex; // white
    //=====================================================
    
    //=======================Properties====================
    float4 _BaseColor;
    float _Metallic;
    float _Smoothness;
    float _MetallicRemapMin;
    float _MetallicRemapMax;
    float _SmoothnessRemapMin;
    float _SmoothnessRemapMax;
    float _AlphaRemapMin;
    float _AlphaRemapMax;
    float _AORemapMin;
    float _AORemapMax;
    float _NormalScale;
    float _HeightAmplitude; // Height Amplitude. In world units. Default value of _HeightAmplitude must be (_HeightMax - _HeightMin) * 0.01
    float _HeightCenter; // Height Center. In texture space
    int _HeightMapParametrization; // Heightmap Parametrization. MinMax, 0, Amplitude, 1
    // These parameters are for vertex displacement/Tessellation
    float _HeightOffset; // Height Offset
    // MinMax mode
    float _HeightMin; // Heightmap Min
    float _HeightMax; // Heightmap Max
    // Amplitude mode
    float _HeightTessAmplitude; // in Centimeters
    float _HeightTessCenter; // In texture space
    // These parameters are for pixel displacement
    float _HeightPoMAmplitude; // Height Amplitude. In centimeters
    float _DetailAlbedoScale;
    float _DetailNormalScale;
    float _DetailSmoothnessScale;
    float _Anisotropy;
    float _DiffusionProfileHash;
    float _SubsurfaceMask; // Subsurface Radius
    float _TransmissionMask; // Transmission Mask
    float _Thickness;
    float4 _ThicknessRemap;
    float4 _IridescenceThicknessRemap;
    float _IridescenceThickness;
    float _IridescenceMask;
    float _CoatMask;
    float _EnergyConservingSpecularColor;
    float4 _SpecularColor;
    int _SpecularOcclusionMode; // Off, 0, From Ambient Occlusion, 1, From AO and Bent Normals, 2
    float3 _EmissiveColor;
    float _AlbedoAffectEmissive; // Albedo Affect Emissive
    int _EmissiveIntensityUnit; // Emissive Mode
    int _UseEmissiveIntensity; // Use Emissive Intensity
    float _EmissiveIntensity; // Emissive Intensity
    float _EmissiveExposureWeight; // Emissive Pre Exposure
    float _UseShadowThreshold;
    float _AlphaCutoffEnable;
    float _AlphaCutoff;
    float _AlphaCutoffShadow;
    float _AlphaCutoffPrepass;
    float _AlphaCutoffPostpass;
    float _TransparentDepthPrepassEnable;
    float _TransparentBackfaceEnable;
    float _TransparentDepthPostpassEnable;
    float _TransparentSortPriority;
    // Transparency
    int _RefractionModel; // None, 0, Planar, 1, Sphere, 2, Thin, 3
    float _Ior;
    float3 _TransmittanceColor;
    float _ATDistance;
    float _TransparentWritingMotionVec;
    // Blending state
    float _SurfaceType;
    float _BlendMode;
    float _SrcBlend;
    float _DstBlend; 
    float _AlphaSrcBlend;
    float _AlphaDstBlend;
    float _EnableFogOnTransparent; // Enable Fog
    float _EnableBlendModePreserveSpecularLighting; // Enable Blend Mode Preserve Specular Lighting
    float _DoubleSidedEnable; // Double sided enable
    float _DoubleSidedNormalMode; // Flip, 0, Mirror, 1, None, 2
    float4 _DoubleSidedConstants;
    float _DoubleSidedGIMode; // Auto, 0, On, 1, Off, 2
    float _UVBase; // UV0, 0, UV1, 1, UV2, 2, UV3, 3, Planar, 4, Triplanar, 5
    float _ObjectSpaceUVMapping; // WorldSpace, 0, ObjectSpace, 1
    float _TexWorldScale; // Scale to apply on world coordinate
    float4 _UVMappingMask;
    float _NormalMapSpace; // TangentSpace, 0, ObjectSpace, 1
    int _MaterialID; // Subsurface Scattering, 0, Standard, 1, Anisotropy, 2, Iridescence, 3, Specular Color, 4, Translucent, 5
    float _TransmissionEnable;
    float _PPDMinSamples; // Min sample for POM
    float _PPDMaxSamples; // Max sample for POM
    float _PPDLodThreshold; // Start lod to fade out the POM effect
    float _PPDPrimitiveLength; // Primitive length for POM
    float _PPDPrimitiveWidth; // Primitive width for POM
    float4 _InvPrimScale; // Inverse primitive scale for non-planar POM
    float4 _UVDetailsMappingMask;
    float _UVDetail; // UV0, 0, UV1, 1, UV2, 2, UV3, 3. UV Set for detail
    float _LinkDetailsWithBase;
    float _EmissiveColorMode; // Use Emissive Color, 0, Use Emissive Mask, 1. Emissive color mode
    float _UVEmissive; // UV0, 0, UV1, 1, UV2, 2, UV3, 3, Planar, 4, Triplanar, 5, Same as Base, 6. UV Set for emissive
    //=====================================================
};

#ifdef _CPP_MACRO_
#undef uint
#undef uint2
#undef uint3
#undef uint4
#undef int2
#undef int3
#undef int4
#undef float2
#undef float3
#undef float4
#undef float4x4
#undef float2x4
#endif

#endif
