// clang-format on
#pragma once
#include "ShaderInterlop/InterlopHeader.hlsli"

namespace interlop
{
    static const uint MaterialLimit = 2048;
    static const uint LightLimit    = 32;
    static const uint MeshLimit     = 2048;

    static const uint PointLightShadowMapDimension       = 2048;
    static const uint DirectionalLightShadowMapDimension = 4096;

    static const uint MeshPerDrawcallMaxInstanceCount = 64;
    static const uint MeshVertexBlendingMaxJointCount = 1024;

    static const uint MaxPointLightCount = 16;
    static const uint MaxSpotLightCount  = 16;

    // AmbientLight信息结构体
    ConstantBufferStruct SceneAmbientLight
    {
        float3 color;
    };

    // DirectionalLight信息结构体
    ConstantBufferStruct SceneDirectionalLight
    {
        float3   direction;
        float3   color;
        float    intensity;
        uint     useShadowmap;      // 1 - use shadowmap, 0 - do not use shadowmap
        uint     shadowmapSrvIndex; // shadowmap srv in descriptorheap index
        uint     shadowmapWidth;
        float4x4 lightProjView;
    };

    // PointLight信息结构体
    ConstantBufferStruct ScenePointLight
    {
        float3 position;
        float  radius;
        float3 color;
        float  intensity;
    };

    // SpotLight信息结构体
    ConstantBufferStruct SceneSpotLight
    {
        float3   position;
        float    radius;
        float3   color;
        float    intensity;
        float3   direction;
        float    innerRadians;
        float    outerRadians;
        uint     useShadowmap;      // 1 - use shadowmap, 0 - do not use shadowmap
        uint     shadowmapSrvIndex; // shadowmap srv in descriptorheap index
        uint     shadowmapWidth;
        float4x4 lightProjView;
    };

    // 相机实例
    ConstantBufferStruct CameraInstance
    {
        float4x4 viewMatrix;
        float4x4 projMatrix;
        float4x4 projViewMatrix;
        float4x4 viewMatrixInverse;
        float4x4 projMatrixInverse;
        float4x4 projViewMatrixInverse;
        float3   cameraPosition;
    };

    // 场景Light实例
    ConstantBufferStruct SceneLightInstance
    {
        uint                  pointLightCount;
        uint                  spotLightCount;
        SceneAmbientLight     sceneAmbientLight;
        SceneDirectionalLight sceneDirectionalLight;
        ScenePointLight       scenePointLights[MaxPointLightCount];
        SceneSpotLight        sceneSpotLights[MaxSpotLightCount];
    };

    // 对象的boundingbox
    ConstantBufferStruct BoundingBox
    {
        float3 center;
        float3 extents;
    };

    // 对象Mesh实例
    ConstantBufferStruct MeshInstance
    {
        D3D12_VERTEX_BUFFER_VIEW vertexBuffer;
        D3D12_INDEX_BUFFER_VIEW  indexBuffer;
        D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments;
    };

    // 对象MeshRenderer实例
    ConstantBufferStruct MeshRendererInstance
    {
        float        enableVertexBlending;
        float4x4     modelMatrix;
        float4x4     modelMatrixInverse;
        MeshInstance meshInstance;
        BoundingBox  boundingBox;
        uint         materialIndex;
    };

    // 材质实例
    ConstantBufferStruct MaterialInstance
    {
        uint uniformBufferViewIndex;
        uint baseColorViewIndex;
        uint metallicRoughnessViewIndex;
        uint normalViewIndex;
        uint occlusionViewIndex;
        uint emissionViewIndex;
    };

    ConstantBufferStruct PositionRadius
    {
        float3 position;
        float  radius;
    };

    // PointLight阴影实例
    ConstantBufferStruct PointLightShadowInstance
    {
        uint lightCount;
        PositionRadius posRadius[MaxPointLightCount];
    };

    // SpotLight阴影实例
    ConstantBufferStruct SpotLightShadowInstance
    {
        uint lightCount;
        PositionRadius posRadius[MaxSpotLightCount];
    };

    // DirectionalLight阴影实例
    ConstantBufferStruct DirectionalLightShadowInstance
    {
        float4x4 lightProjView;
        uint     shadowWidth;
        uint     shadowHeight;
        uint     shadowDepth;
    };

    ConstantBufferStruct BitonicSortCommandSigParams
    {
        uint  MeshIndex;
        float MeshToCamDis;
    };

    ConstantBufferStruct CommandSignatureParams
    {
        uint                         MeshIndex;
        D3D12_VERTEX_BUFFER_VIEW     VertexBuffer;
        D3D12_INDEX_BUFFER_VIEW      IndexBuffer;
        D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments;
    };

    // 材质Uniform实例
    ConstantBufferStruct MaterialUniformBuffer
    {
        float4 baseColorFactor;

        float metallicFactor;
        float roughnessFactor;
        float reflectanceFactor;
        float clearCoatFactor;
        float clearCoatRoughnessFactor;
        float anisotropyFactor;

        float normalScale;
        float occlusionStrength;

        float3 emissiveFactor;
        uint   isBlend;
        uint   isDoubleSided;
    };

} // namespace interlop
