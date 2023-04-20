// clang-format on
#include "InterlopHeader.hlsli"

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

    ConstantBufferStruct SceneAmbientLight
    {
        float3 color;
    };

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

    ConstantBufferStruct ScenePointLight
    {
        float3 position;
        float  radius;
        float3 color;
        float  intensity;
    };

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

    ConstantBufferStruct MeshPerframeStorageBufferObject
    {
        uint                  pointLightCount;
        uint                  spotLightCount;
        uint                  totalMeshCount;
        CameraInstance        cameraInstance;
        SceneAmbientLight     sceneAmbientLight;
        SceneDirectionalLight sceneDirectionalLight;
        ScenePointLight       scenePointLights[MaxPointLightCount];
        SceneSpotLight        sceneSpotLights[MaxSpotLightCount];
    };

    ConstantBufferStruct BoundingBox
    {
        float3 center;
        float3 extents;
    };

    ConstantBufferStruct MeshInstance
    {
        float    enableVertexBlending;
        float4x4 modelMatrix;
        float4x4 modelMatrixInverse;

        D3D12_VERTEX_BUFFER_VIEW vertexBuffer;
        D3D12_INDEX_BUFFER_VIEW  indexBuffer;

        D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments;

        BoundingBox boundingBox;

        uint materialIndex;
    };

    ConstantBufferStruct MaterialInstance
    {
        uint uniformBufferViewIndex;
        uint baseColorViewIndex;
        uint metallicRoughnessViewIndex;
        uint normalViewIndex;
        uint occlusionViewIndex;
        uint emissionViewIndex;
    };

    ConstantBufferStruct MeshPointLightShadowPerframeStorageBufferObject
    {
        uint   pointLightCount;
        float4 pointLightsPositionAndRadius[MaxPointLightCount];
    };

    ConstantBufferStruct MeshSpotLightShadowPerframeStorageBufferObject
    {
        uint   spotLightCount;
        float4 spotLightsPositionAndRadius[MaxSpotLightCount];
    };

    struct MeshDirectionalLightShadowPerframeStorageBufferObject
    {
        float4x4 lightProjView;
        uint     shadowWidth;
        uint     shadowHeight;
        uint     shadowDepth;
    };

    struct BitonicSortCommandSigParams
    {
        uint32_t MeshIndex;
        float    MeshToCamDis;
    };

    struct CommandSignatureParams
    {
        uint32_t                     MeshIndex;
        D3D12_VERTEX_BUFFER_VIEW     VertexBuffer;
        D3D12_INDEX_BUFFER_VIEW      IndexBuffer;
        D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments;
    };

    struct MeshPerMaterialUniformBuffer
    {
        glm::vec4 baseColorFactor {1.0f, 1.0f, 1.0f, 1.0f};

        float metallicFactor           = 0.0f;
        float roughnessFactor          = 0.0f;
        float reflectanceFactor        = 0.0f;
        float clearCoatFactor          = 0.0f;
        float clearCoatRoughnessFactor = 0.0f;
        float anisotropyFactor         = 0.0f;

        float normalScale       = 0.0f;
        float occlusionStrength = 0.0f;

        glm::vec3 emissiveFactor  = {1.0f, 1.0f, 1.0f};
        uint32_t  is_blend        = 0;
        uint32_t  is_double_sided = 0;

        uint32_t _padding_uniform_1;
        uint32_t _padding_uniform_2;
        uint32_t _padding_uniform_3;
    };


































    ConstantBufferStruct SceneBuffer
    {
        float4x4 viewMatrix;
        float4x4 projectionMatrix;
        float4x4 inverseViewMatrix;
        float4x4 inverseProjectionMatrix;
        float4x4 viewProjectionMatrix;
    };

    ConstantBufferStruct TransformBuffer
    {
        float4x4 modelMatrix;
        float4x4 inverseModelMatrix;
    };

    enum class TextureDimensionType
    {
        HeightWidthEven,
        HeightEvenWidthOdd,
        HeightOddWidthEven,
        HeightWidthOdd
    };

    ConstantBufferStruct MipMapGenerationBuffer
    {
        int isSRGB;

        uint sourceMipLevel;

        // 1.0f  / outputDimension.size
        float2 texelSize;

        uint numberMipLevels;

        TextureDimensionType dimensionType;
    };

    
    // There is a max limit to the number of lights in the engine.
    // Index 0 of both the light buffer will be
    // reserved for directional light.

    static const uint MAX_LIGHTS = 10;

    // Light resources that are used while instanced rendering.
    ConstantBufferStruct LightInstancedRenderingBuffer
    {
        float4x4 modelMatrix[MAX_LIGHTS];
    };

    // Lights resources / properties for all lights in the scene.
    ConstantBufferStruct LightBuffer
    {
        // Note : lightPosition essentially stores the light direction if the type is directional light.
        // The shader can differentiate between directional and point lights based on the 'w' value. If 1 (i.e it is a position), light is a point light, while
        // if it is zero, then it is a light direction.
        // Light intensity is not automatically multiplied to the light color on the C++ side, shading shader need to manually multiply them.
        float4 lightPosition[MAX_LIGHTS];
        float4 viewSpaceLightPosition[MAX_LIGHTS];

        float4 lightColor[MAX_LIGHTS];
        // float4 because of struct packing (16byte alignment).
        // radiusIntensity[0] stores the radius, while index 1 stores the intensity.
        float4 radiusIntensity[MAX_LIGHTS];
        
        uint numberOfLights;
    };

    ConstantBufferStruct PostProcessingBuffer
    {
        uint debugShowSSAOTexture;
        uint enableBloom;
        float bloomStrength;
        float4 padding;
    };

    // Note : By using the values in this buffer, the PBR renderer will most likely 'break' and become physically inaccurate.
    // These are used for debugging and testing purposes only.
    ConstantBufferStruct MaterialBuffer
    {
        float roughnessFactor;
        float metallicFactor;
        float emissiveFactor;
        float padding;
        float3 albedoColor;
        float padding2;
    };

    ConstantBufferStruct ShadowBuffer
    {
        float4x4 lightViewProjectionMatrix;
        float backOffDistance;
        float extents;
        float nearPlane;
        float farPlane;
    };

    static const uint SAMPLE_VECTOR_COUNT = 32u;

    ConstantBufferStruct SSAOBuffer
    {
        float4 sampleVectors[SAMPLE_VECTOR_COUNT];
        float radius;
        float bias;
        float power;
        float occlusionMultiplier;
        uint sampleVectorCount;
        float noiseTextureWidth;
        float noiseTextureHeight;
        float screenWidth;
        float screenHeight;
    };

    static const uint BLOOM_PASSES = 7u;

    ConstantBufferStruct BloomBuffer
    {
        float threshHold;
        float radius;
    };
} // namespace interlop
