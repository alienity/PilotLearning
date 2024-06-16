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
        MaterialRes m_mat_data = { shaderName,
                                   pbrMaterial._BaseColor,
                                   pbrMaterial._BaseColorMap,
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
                                   pbrMaterial._NormalMap,
                                   pbrMaterial._NormalMapOS,
                                   pbrMaterial._NormalScale,
                                   pbrMaterial._BentNormalMap,
                                   pbrMaterial._BentNormalMapOS,
                                   pbrMaterial._HeightMap,
                                   pbrMaterial._HeightAmplitude,
                                   pbrMaterial._HeightCenter,
                                   pbrMaterial._DetailMap,
                                   pbrMaterial._DetailAlbedoScale,
                                   pbrMaterial._DetailNormalScale,
                                   pbrMaterial._DetailSmoothnessScale,
                                   pbrMaterial._TangentMap,
                                   pbrMaterial._TangentMapOS,
                                   pbrMaterial._Anisotropy,
                                   pbrMaterial._AnisotropyMap,
                                   pbrMaterial._SubsurfaceMasks,
                                   pbrMaterial._SubsurfaceMaskMap,
                                   pbrMaterial._TransmissionMask,
                                   pbrMaterial._TransmissionMaskMap,
                                   pbrMaterial._Thickness,
                                   pbrMaterial._ThicknessMap,
                                   pbrMaterial._ThicknessRemap,
                                   pbrMaterial._IridescenceThickness,
                                   pbrMaterial._IridescenceThicknessMap,
                                   pbrMaterial._IridescenceThicknessRemap,
                                   pbrMaterial._IridescenceMask,
                                   pbrMaterial._IridescenceMaskMap,
                                   pbrMaterial._CoatMask,
                                   pbrMaterial._CoatMaskMap,
                                   pbrMaterial._EnergyConservingSpecularColor,
                                   pbrMaterial._SpecularColor,
                                   pbrMaterial._SpecularColorMap,
                                   pbrMaterial._SpecularOcclusionMode,
                                   pbrMaterial._EmissiveColor,
                                   pbrMaterial._AlbedoAffectEmissive,
                                   pbrMaterial._EmissiveExposureWeight,
                                   pbrMaterial._UseShadowThreshold,
                                   pbrMaterial._AlphaCutoff,
                                   pbrMaterial._AlphaCutoffShadow,
                                   pbrMaterial._AlphaCutoffPrepass,
                                   pbrMaterial._AlphaCutoffPostpass,
                                   pbrMaterial._Ior,
                                   pbrMaterial._TransmittanceColor,
                                   pbrMaterial._TransmittanceColorMap,
                                   pbrMaterial._ATDistance,
                                   pbrMaterial._SurfaceType,
                                   pbrMaterial._BlendMode,
                                   pbrMaterial._EnableBlendModePreserveSpecularLighting,
                                   pbrMaterial._DoubleSidedEnable,
                                   pbrMaterial._ObjectSpaceUVMapping,
                                   pbrMaterial._InvTilingScale,
                                   pbrMaterial._TexWorldScale,
                                   pbrMaterial._UVMappingMask,
                                   pbrMaterial._PPDMinSamples,
                                   pbrMaterial._PPDMaxSamples,
                                   pbrMaterial._PPDLodThreshold };
        return m_mat_data;
    }

    StandardLightMaterial ToPBRMaterial(const MaterialRes& materialRes)
    {
        StandardLightMaterial m_mat_data = {
                                    materialRes._BaseColor,
                                    materialRes._BaseColorMap,
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
                                    materialRes._NormalMap,
                                    materialRes._NormalMapOS,
                                    materialRes._NormalScale,
                                    materialRes._BentNormalMap,
                                    materialRes._BentNormalMapOS,
                                    materialRes._HeightMap,
                                    materialRes._HeightAmplitude,
                                    materialRes._HeightCenter,
                                    materialRes._DetailMap,
                                    materialRes._DetailAlbedoScale,
                                    materialRes._DetailNormalScale,
                                    materialRes._DetailSmoothnessScale,
                                    materialRes._TangentMap,
                                    materialRes._TangentMapOS,
                                    materialRes._Anisotropy,
                                    materialRes._AnisotropyMap,
                                    materialRes._SubsurfaceMasks,
                                    materialRes._SubsurfaceMaskMap,
                                    materialRes._TransmissionMask,
                                    materialRes._TransmissionMaskMap,
                                    materialRes._Thickness,
                                    materialRes._ThicknessMap,
                                    materialRes._ThicknessRemap,
                                    materialRes._IridescenceThickness,
                                    materialRes._IridescenceThicknessMap,
                                    materialRes._IridescenceThicknessRemap,
                                    materialRes._IridescenceMask,
                                    materialRes._IridescenceMaskMap,
                                    materialRes._CoatMask,
                                    materialRes._CoatMaskMap,
                                    materialRes._EnergyConservingSpecularColor,
                                    materialRes._SpecularColor,
                                    materialRes._SpecularColorMap,
                                    materialRes._SpecularOcclusionMode,
                                    materialRes._EmissiveColor,
                                    materialRes._AlbedoAffectEmissive,
                                    materialRes._EmissiveExposureWeight,
                                    materialRes._UseShadowThreshold,
                                    materialRes._AlphaCutoff,
                                    materialRes._AlphaCutoffShadow,
                                    materialRes._AlphaCutoffPrepass,
                                    materialRes._AlphaCutoffPostpass,
                                    materialRes._Ior,
                                    materialRes._TransmittanceColor,
                                    materialRes._TransmittanceColorMap,
                                    materialRes._ATDistance,
                                    materialRes._SurfaceType,
                                    materialRes._BlendMode,
                                    materialRes._EnableBlendModePreserveSpecularLighting,
                                    materialRes._DoubleSidedEnable,
                                    materialRes._ObjectSpaceUVMapping,
                                    materialRes._InvTilingScale,
                                    materialRes._TexWorldScale,
                                    materialRes._UVMappingMask,
                                    materialRes._PPDMinSamples,
                                    materialRes._PPDMaxSamples,
                                    materialRes._PPDLodThreshold };
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
        { "TEXCOORD0",   0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",       0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static_assert(sizeof(D3D12MeshVertexStandard) == 64, "Vertex struct/layout mismatch");

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