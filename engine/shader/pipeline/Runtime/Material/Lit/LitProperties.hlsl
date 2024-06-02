#ifndef MATERIAL_PROPERTIES_HLSL
#define MATERIAL_PROPERTIES_HLSL

struct PropertiesPerMaterial
{
    uint _EmissiveColorMapIndex;
    uint _DiffuseLightingMapIndex;
    uint _BaseColorMapIndex;
    uint _MaskMapIndex;
    uint _BentNormalMapIndex;
    uint _BentNormalMapOSIndex;
    uint _NormalMapIndex;
    uint _NormalMapOSIndex;
    uint _DetailMapIndex;
    uint _HeightMapIndex;
    uint _TangentMapIndex;
    uint _TangentMapOSIndex;
    uint _AnisotropyMapIndex;
    uint _SubsurfaceMaskMapIndex;
    uint _TransmissionMaskMapIndex;
    uint _ThicknessMapIndex;
    uint _IridescenceThicknessMapIndex;
    uint _IridescenceMaskMapIndex;
    uint _SpecularColorMapIndex;
    uint _TransmittanceColorMapIndex;
    uint _CoatMaskMapIndex;
    
    // shared constant between lit and layered lit
    float _AlphaCutoff;
    float _UseShadowThreshold;
    float _AlphaCutoffShadow;
    float _AlphaCutoffPrepass;
    float _AlphaCutoffPostpass;
    float4 _DoubleSidedConstants;
    float _BlendMode;
    float _EnableBlendModePreserveSpecularLighting;

    float _PPDMaxSamples;
    float _PPDMinSamples;
    float _PPDLodThreshold;
    
    float3 _EmissiveColor;
    float _AlbedoAffectEmissive;
    float _EmissiveExposureWeight;

    int _SpecularOcclusionMode;
    
    // Transparency
    float3 _TransmittanceColor;
    float _Ior;
    float _ATDistance;
    
    float3 _EmissionColor;
    float4 _EmissiveColorMap_ST;
    float _TexWorldScaleEmissive;
    float4 _UVMappingMaskEmissive;
    float _ObjectSpaceUVMappingEmissive;
    
    float4 _InvPrimScale; // Only XY are used

    // Specular AA
    float _EnableGeometricSpecularAA;
    float _SpecularAAScreenSpaceVariance;
    float _SpecularAAThreshold;
    
    // Set of users variables
    float4 _BaseColor;
    float4 _BaseColorMap_ST;
    float4 _BaseColorMap_TexelSize;
    float4 _BaseColorMap_MipInfo;

    float _Metallic;
    float _MetallicRemapMin;
    float _MetallicRemapMax;
    float _Smoothness;
    float _SmoothnessRemapMin;
    float _SmoothnessRemapMax;
    float _AlphaRemapMin;
    float _AlphaRemapMax;
    float _AORemapMin;
    float _AORemapMax;

    float _NormalScale;

    float4 _DetailMap_ST;
    float _DetailAlbedoScale;
    float _DetailNormalScale;
    float _DetailSmoothnessScale;

    float4 _HeightMap_TexelSize; // Unity facility. This will provide the size of the heightmap to the shader

    float _HeightAmplitude;
    float _HeightCenter;

    float _Anisotropy;

    float _DiffusionProfileHash;
    float _SubsurfaceMask;
    float _TransmissionMask;
    float _Thickness;
    float4 _ThicknessRemap;


    float _IridescenceThickness;
    float4 _IridescenceThicknessRemap;
    float _IridescenceMask;

    float _CoatMask;

    float4 _SpecularColor;
    float _EnergyConservingSpecularColor;

    float _TexWorldScale;
    float _InvTilingScale;
    float4 _UVMappingMask;
    float4 _UVDetailsMappingMask;
    float _LinkDetailsWithBase;
    float _ObjectSpaceUVMapping;
};

#endif
