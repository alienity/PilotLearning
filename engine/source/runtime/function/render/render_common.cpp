#include "render_common.h"
#include "runtime/resource/res_type/components/material.h"

namespace MoYu
{
    SceneImage DefaultSceneImageWhite{ true, false, 1, "asset/texture/default/white.tga"};
    SceneImage DefaultSceneImageBlack{ true, false, 1, "asset/texture/default/black.tga"};
    SceneImage DefaultSceneImageGrey { true, false, 1, "asset/texture/default/grey.tga"};
    SceneImage DefaultSceneImageRed  { true, false, 1, "asset/texture/default/red.tga"};
    SceneImage DefaultSceneImageGreen{ true, false, 1, "asset/texture/default/green.tga"};
    SceneImage DefaultSceneImageBlue { true, false, 1, "asset/texture/default/blue.tga"};

    MaterialImage DefaultMaterialImageWhite{ DefaultSceneImageWhite, glm::float2(1,1) };
    MaterialImage DefaultMaterialImageBlack{ DefaultSceneImageBlack, glm::float2(1,1) };
    MaterialImage DefaultMaterialImageGrey{ DefaultSceneImageGrey, glm::float2(1,1) };
    MaterialImage DefaultMaterialImageRed{ DefaultSceneImageRed, glm::float2(1,1) };
    MaterialImage DefaultMaterialImageGreen{ DefaultSceneImageGreen, glm::float2(1,1) };
    MaterialImage DefaultMaterialImageBlue{ DefaultSceneImageBlue, glm::float2(1,1) };

    SceneCommonIdentifier _UndefCommonIdentifier {K_Invalid_Object_Id, K_Invalid_Component_Id};

    MaterialRes ToMaterialRes(const StandardLightMaterial& pbrMaterial, const std::string shaderName)
    {
        MaterialRes m_mat_data = { 
            shaderName,
            pbrMaterial._BaseColorMap,
            pbrMaterial._MaskMap,
            pbrMaterial._NormalMap,
            pbrMaterial._NormalMapOS,
            pbrMaterial._BentNormalMap,
            pbrMaterial._BentNormalMapOS,
            pbrMaterial._HeightMap,
            pbrMaterial._DetailMap,
            pbrMaterial._TangentMap,
            pbrMaterial._TangentMapOS,
            pbrMaterial._AnisotropyMap,
            pbrMaterial._SubsurfaceMaskMap,
            pbrMaterial._TransmissionMaskMap,
            pbrMaterial._ThicknessMap,
            pbrMaterial._IridescenceThicknessMap,
            pbrMaterial._IridescenceMaskMap,
            pbrMaterial._CoatMaskMap,
            pbrMaterial._EmissiveColorMap,
            pbrMaterial._TransmittanceColorMap,
            pbrMaterial._BaseColor,
            pbrMaterial._Metallic,
            pbrMaterial._Smoothness,
            pbrMaterial._MetallicRemapMin,
            pbrMaterial._MetallicRemapMax,
            pbrMaterial._SmoothnessRemapMin,
            pbrMaterial._SmoothnessRemapMax,
            pbrMaterial._AlphaRemapMin,
            pbrMaterial._AlphaRemapMax,
            pbrMaterial._AORemapMin,
            pbrMaterial._AORemapMax,
            pbrMaterial._NormalScale,
            pbrMaterial._HeightAmplitude,
            pbrMaterial._HeightCenter,
            pbrMaterial._HeightMapParametrization,
            pbrMaterial._HeightOffset,
            pbrMaterial._HeightMin,
            pbrMaterial._HeightMax,
            pbrMaterial._HeightTessAmplitude,
            pbrMaterial._HeightTessCenter,
            pbrMaterial._HeightPoMAmplitude,
            pbrMaterial._DetailAlbedoScale,
            pbrMaterial._DetailNormalScale,
            pbrMaterial._DetailSmoothnessScale,
            pbrMaterial._Anisotropy,
            pbrMaterial._DiffusionProfileHash,
            pbrMaterial._SubsurfaceMask,
            pbrMaterial._TransmissionMask,
            pbrMaterial._Thickness,
            pbrMaterial._ThicknessRemap,
            pbrMaterial._IridescenceThicknessRemap,
            pbrMaterial._IridescenceThickness,
            pbrMaterial._IridescenceMask,
            pbrMaterial._CoatMask,
            pbrMaterial._EnergyConservingSpecularColor,
            pbrMaterial._SpecularOcclusionMode,
            pbrMaterial._EmissiveColor,
            pbrMaterial._AlbedoAffectEmissive,
            pbrMaterial._EmissiveIntensityUnit,
            pbrMaterial._UseEmissiveIntensity,
            pbrMaterial._EmissiveIntensity,
            pbrMaterial._EmissiveExposureWeight,
            pbrMaterial._UseShadowThreshold,
            pbrMaterial._AlphaCutoffEnable,
            pbrMaterial._AlphaCutoff,
            pbrMaterial._AlphaCutoffShadow,
            pbrMaterial._AlphaCutoffPrepass,
            pbrMaterial._AlphaCutoffPostpass,
            pbrMaterial._TransparentDepthPrepassEnable,
            pbrMaterial._TransparentBackfaceEnable,
            pbrMaterial._TransparentDepthPostpassEnable,
            pbrMaterial._TransparentSortPriority,
            pbrMaterial._RefractionModel,
            pbrMaterial._Ior,
            pbrMaterial._TransmittanceColor,
            pbrMaterial._ATDistance,
            pbrMaterial._TransparentWritingMotionVec,
            pbrMaterial._SurfaceType,
            pbrMaterial._BlendMode,
            pbrMaterial._SrcBlend,
            pbrMaterial._DstBlend,
            pbrMaterial._AlphaSrcBlend,
            pbrMaterial._AlphaDstBlend,
            pbrMaterial._EnableFogOnTransparent,
            pbrMaterial._EnableBlendModePreserveSpecularLighting,
            pbrMaterial._DoubleSidedEnable,
            pbrMaterial._DoubleSidedNormalMode,
            pbrMaterial._DoubleSidedConstants,
            pbrMaterial._DoubleSidedGIMode,
            pbrMaterial._UVBase,
            pbrMaterial._ObjectSpaceUVMapping,
            pbrMaterial._TexWorldScale,
            pbrMaterial._UVMappingMask,
            pbrMaterial._NormalMapSpace,
            pbrMaterial._MaterialID,
            pbrMaterial._TransmissionEnable,
            pbrMaterial._PPDMinSamples,
            pbrMaterial._PPDMaxSamples,
            pbrMaterial._PPDLodThreshold,
            pbrMaterial._PPDPrimitiveLength,
            pbrMaterial._PPDPrimitiveWidth,
            pbrMaterial._InvPrimScale,
            pbrMaterial._UVDetailsMappingMask,
            pbrMaterial._UVDetail,
            pbrMaterial._LinkDetailsWithBase,
            pbrMaterial._EmissiveColorMode,
            pbrMaterial._UVEmissive };
        return m_mat_data;
    }

    StandardLightMaterial ToStandardMaterial(const MaterialRes& materialRes)
    {
        StandardLightMaterial m_mat_data = {
            materialRes._BaseColorMap,
            materialRes._MaskMap,
            materialRes._NormalMap,
            materialRes._NormalMapOS,
            materialRes._BentNormalMap,
            materialRes._BentNormalMapOS,
            materialRes._HeightMap,
            materialRes._DetailMap,
            materialRes._TangentMap,
            materialRes._TangentMapOS,
            materialRes._AnisotropyMap,
            materialRes._SubsurfaceMaskMap,
            materialRes._TransmissionMaskMap,
            materialRes._ThicknessMap,
            materialRes._IridescenceThicknessMap,
            materialRes._IridescenceMaskMap,
            materialRes._CoatMaskMap,
            materialRes._EmissiveColorMap,
            materialRes._TransmittanceColorMap,
            materialRes._BaseColor,
            materialRes._Metallic,
            materialRes._Smoothness,
            materialRes._MetallicRemapMin,
            materialRes._MetallicRemapMax,
            materialRes._SmoothnessRemapMin,
            materialRes._SmoothnessRemapMax,
            materialRes._AlphaRemapMin,
            materialRes._AlphaRemapMax,
            materialRes._AORemapMin,
            materialRes._AORemapMax,
            materialRes._NormalScale,
            materialRes._HeightAmplitude,
            materialRes._HeightCenter,
            materialRes._HeightMapParametrization,
            materialRes._HeightOffset,
            materialRes._HeightMin,
            materialRes._HeightMax,
            materialRes._HeightTessAmplitude,
            materialRes._HeightTessCenter,
            materialRes._HeightPoMAmplitude,
            materialRes._DetailAlbedoScale,
            materialRes._DetailNormalScale,
            materialRes._DetailSmoothnessScale,
            materialRes._Anisotropy,
            materialRes._DiffusionProfileHash,
            materialRes._SubsurfaceMask,
            materialRes._TransmissionMask,
            materialRes._Thickness,
            materialRes._ThicknessRemap,
            materialRes._IridescenceThicknessRemap,
            materialRes._IridescenceThickness,
            materialRes._IridescenceMask,
            materialRes._CoatMask,
            materialRes._EnergyConservingSpecularColor,
            materialRes._SpecularOcclusionMode,
            materialRes._EmissiveColor,
            materialRes._AlbedoAffectEmissive,
            materialRes._EmissiveIntensityUnit,
            materialRes._UseEmissiveIntensity,
            materialRes._EmissiveIntensity,
            materialRes._EmissiveExposureWeight,
            materialRes._UseShadowThreshold,
            materialRes._AlphaCutoffEnable,
            materialRes._AlphaCutoff,
            materialRes._AlphaCutoffShadow,
            materialRes._AlphaCutoffPrepass,
            materialRes._AlphaCutoffPostpass,
            materialRes._TransparentDepthPrepassEnable,
            materialRes._TransparentBackfaceEnable,
            materialRes._TransparentDepthPostpassEnable,
            materialRes._TransparentSortPriority,
            materialRes._RefractionModel,
            materialRes._Ior,
            materialRes._TransmittanceColor,
            materialRes._ATDistance,
            materialRes._TransparentWritingMotionVec,
            materialRes._SurfaceType,
            materialRes._BlendMode,
            materialRes._SrcBlend,
            materialRes._DstBlend,
            materialRes._AlphaSrcBlend,
            materialRes._AlphaDstBlend,
            materialRes._EnableFogOnTransparent,
            materialRes._EnableBlendModePreserveSpecularLighting,
            materialRes._DoubleSidedEnable,
            materialRes._DoubleSidedNormalMode,
            materialRes._DoubleSidedConstants,
            materialRes._DoubleSidedGIMode,
            materialRes._UVBase,
            materialRes._ObjectSpaceUVMapping,
            materialRes._TexWorldScale,
            materialRes._UVMappingMask,
            materialRes._NormalMapSpace,
            materialRes._MaterialID,
            materialRes._TransmissionEnable,
            materialRes._PPDMinSamples,
            materialRes._PPDMaxSamples,
            materialRes._PPDLodThreshold,
            materialRes._PPDPrimitiveLength,
            materialRes._PPDPrimitiveWidth,
            materialRes._InvPrimScale,
            materialRes._UVDetailsMappingMask,
            materialRes._UVDetail,
            materialRes._LinkDetailsWithBase,
            materialRes._EmissiveColorMode,
            materialRes._UVEmissive };
        return m_mat_data;
    }

    // clang-format off

    //--------------------------------------------------------------------------------------
    // Vertex struct holding position information.
    const D3D12_INPUT_ELEMENT_DESC D3D12MeshVertexPosition::InputElements[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static_assert(sizeof(D3D12MeshVertexPosition) == 12, "Vertex struct/layout mismatch");

    const RHI::D3D12InputLayout D3D12MeshVertexPosition::InputLayout = 
        RHI::D3D12InputLayout( D3D12MeshVertexPosition::InputElements, 
            D3D12MeshVertexPosition::InputElementCount);

    //--------------------------------------------------------------------------------------
    // Vertex struct holding position and texture mapping information.
    const D3D12_INPUT_ELEMENT_DESC D3D12MeshVertexPositionTexture::InputElements[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static_assert(sizeof(D3D12MeshVertexPositionTexture) == 20, "Vertex struct/layout mismatch");

    const RHI::D3D12InputLayout D3D12MeshVertexPositionTexture::InputLayout = 
        RHI::D3D12InputLayout( D3D12MeshVertexPositionTexture::InputElements, 
            D3D12MeshVertexPositionTexture::InputElementCount);

    //--------------------------------------------------------------------------------------
    // Vertex struct holding position, normal, tangent and texture mapping information.
    const D3D12_INPUT_ELEMENT_DESC D3D12MeshVertexPositionNormalTangentTexture::InputElements[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static_assert(sizeof(D3D12MeshVertexPositionNormalTangentTexture) == 48, "Vertex struct/layout mismatch");

    const RHI::D3D12InputLayout D3D12MeshVertexPositionNormalTangentTexture::InputLayout = 
        RHI::D3D12InputLayout( D3D12MeshVertexPositionNormalTangentTexture::InputElements, 
            D3D12MeshVertexPositionNormalTangentTexture::InputElementCount);

    //--------------------------------------------------------------------------------------
    // Vertex struct holding position, normal, tangent and texture mapping information.
    const D3D12_INPUT_ELEMENT_DESC D3D12MeshVertexPositionNormalTangentTextureJointBinding::InputElements[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES",0, DXGI_FORMAT_R32G32B32A32_UINT,  0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static_assert(sizeof(D3D12MeshVertexPositionNormalTangentTextureJointBinding) == 80, "Vertex struct/layout mismatch");

    const RHI::D3D12InputLayout D3D12MeshVertexPositionNormalTangentTextureJointBinding::InputLayout = 
        RHI::D3D12InputLayout( D3D12MeshVertexPositionNormalTangentTextureJointBinding::InputElements, 
            D3D12MeshVertexPositionNormalTangentTextureJointBinding::InputElementCount);

    //--------------------------------------------------------------------------------------
    const D3D12_INPUT_ELEMENT_DESC D3D12MeshVertexStandard::InputElements[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static_assert(sizeof(D3D12MeshVertexStandard) == 48, "Vertex struct/layout mismatch");

    const RHI::D3D12InputLayout D3D12MeshVertexStandard::InputLayout = 
        RHI::D3D12InputLayout(D3D12MeshVertexStandard::InputElements, D3D12MeshVertexStandard::InputElementCount);

    //--------------------------------------------------------------------------------------
    // Vertex struct holding position, normal, tangent and texture mapping information.
    const D3D12_INPUT_ELEMENT_DESC D3D12TerrainPatch::InputElements[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",       0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static_assert(sizeof(D3D12TerrainPatch) == 64, "Vertex struct/layout mismatch");

    const RHI::D3D12InputLayout D3D12TerrainPatch::InputLayout = 
        RHI::D3D12InputLayout( D3D12TerrainPatch::InputElements, D3D12TerrainPatch::InputElementCount);


    // clang-format on



}