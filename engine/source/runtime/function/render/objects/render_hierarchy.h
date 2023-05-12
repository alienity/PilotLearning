#pragma once

#include "runtime/core/math/moyu_math.h"
#include "runtime/core/color/color.h"

#include <cstdint>
#include <vector>
#include <dxgiformat.h>

namespace RHI
{
    class D3D12Buffer;
    class D3D12Texture;

    class RenderCamera;
    class RenderLight;
    class RenderScene;
    class MeshRenderer;
}

namespace Hierarchy
{
    struct Config;
    struct Level;
    struct PointLight;
    struct SpotLight;
    struct DirecionLight;
    struct AmbientLight;
    struct Light;
    struct PerspectiveCamera;
    struct OrthographicCamera;
    struct Camera;
    struct Buffer;
    struct BufferData;
    struct BufferView;
    struct Accessor;
    struct Texture;
    struct Sampler;
    struct Image;
    struct ImageData;
    struct Scene;
    struct Node;
    struct Primitive;
    struct MeshData;
    struct Mesh;
    struct PrimitiveBuffer;
    struct MeshBuffer;
    struct MatTexture;
    struct Material;
    struct MeshRenderer;
    struct Skin;
    struct AnimationChannel;
    struct AnimationSampler;
    struct Animation;


    struct Config
    {
        std::vector<Image*>  textures;
        std::vector<Buffer*> buffers;
        float                configVal;
    };

    struct Level
    {
        Config config;
        Scene  scene;
    };

    enum LightType
    {
        Point,
        Spot,
        Direction,
        Ambient
    };

    struct PointLight
    {
        Pilot::Color color;
        float        radius;
        float        intensity;
    };

    struct SpotLight
    {
        Pilot::Color color;
        float        radius;
        float        intensity;

        float        direction;
        float        innerRadians;
        float        outerRadians;

        float          shadowmap;
        float          shadowNearPlane;
        float          shadowFarPlane;
        Pilot::Vector2 shadowBounds;
        Pilot::Vector2 shadowmapSize;
    };

    struct DirecionLight
    {
        Pilot::Color   color;
        float          intensity;

        Pilot::Vector3 direcion;

        float          shadowmap;
        float          shadowNearPlane;
        float          shadowFarPlane;
        Pilot::Vector2 shadowBounds;
        Pilot::Vector2 shadowmapSize;
    };

    struct AmbientLight
    {
        Pilot::Color color;
        float        intensity;
    };

    struct Light
    {
        LightType     type;
        PointLight    pointLight;
        SpotLight     spotLight;
        DirecionLight directionLight;
        AmbientLight  ambientLight;

        RHI::RenderLight* renderLight = nullptr;
    };

    enum CameraType
    {
        Perspective,
        Orthographic
    };

    struct PerspectiveCamera
    {
        float aspectRatio;
        float tfov;
        float zfar;
        float znear;
    };

    struct OrthographicCamera
    {
        float xmag;
        float ymag;
        float zfar;
        float znear;
    };

    struct Camera
    {
        CameraType         type;
        PerspectiveCamera  pers;
        OrthographicCamera orthos;

        RHI::RenderCamera* renderCamera = nullptr;
    };

    struct Buffer
    {
        std::string uri = ""; // "buffer01.bin"

        BufferData* bufferData;
        RHI::D3D12Buffer* d3d12Buffer;
    };

    struct BufferData
    {
        void* datas = nullptr;
        int   byteLength; // 102040
    };

    struct BufferView
    {
        Buffer buffer;
        int    byteOffset;
        int    byteLength;
        int    byteStride;
    };

    struct Accessor
    {
        BufferView  bufferView;
        int         byteOffset;
        std::string type;          // "VEC2"
        std::string componentType; // "FLOAT"
        int         count;         // 2
        std::string min;           //"[0.1, 0.2]"
        std::string max;           //"[0.9, 0.8]"
    };

    struct Texture
    {
        Image   image;
        Sampler sampler;
    };

    struct Sampler
    {
        std::string magFilter;
        std::string minFilter;
        std::string wrapS;
        std::string wrapT;
    };

    struct Image
    {
        std::string uri = ""; // "image01.png"

        ImageData* imageData;
        RHI::D3D12Texture* d3d12Texture;
    };

    struct ImageData
    {
        void*       pixels    = nullptr;
        int         width     = 1;
        int         height    = 1;
        int         mipLevels = 1;
        DXGI_FORMAT format    = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
    };

    struct Scene
    {
        std::vector<Node*> nodes;

        RHI::RenderScene* renderScene;
    };

    class Node
    {
        Pilot::Matrix4x4  localMatrix; // M=T*R*S; column major storage
        Pilot::Vector3    translation;
        Pilot::Quaternion rotation;
        Pilot::Vector3    scale;

        Node*              parent   = nullptr;
        std::vector<Node*> children = {};

        Camera* camera = nullptr;
        Light*  light  = nullptr;
        Skin*   skin   = nullptr;

        MeshRenderer* meshRenderer = nullptr;
    };

    enum PrimitiveMode
    {
        POINTS = 0,
        LINES,
        TRIANGLES
    };

    enum PrimitiveAttribute
    {
        POSITION = 0,
        NORMAL,
        TANGENT,
        TEXCOORD0,
        TEXCOORD1,
        TEXCOORD2,
    };

    typedef unsigned long long InputAtributeMask;

    InputAtributeMask PositionFlag  = 1 << POSITION;
    InputAtributeMask NormalFlag    = 1 << NORMAL;
    InputAtributeMask TangnetFlag   = 1 << TANGENT;
    InputAtributeMask TexCoord0Flag = 1 << TEXCOORD0;
    InputAtributeMask TexCoord1Flag = 1 << TEXCOORD1;
    InputAtributeMask TexCoord2Flag = 1 << TEXCOORD2;
    InputAtributeMask PosNormalFlag = PositionFlag | NormalFlag;

    struct Primitive
    {
        BufferData        indicesPtr;
        InputAtributeMask mask;
        BufferData        verticesPtr;
    };

    struct MeshData
    {
        std::vector<Primitive> primitives;
    };

    struct PrimitiveBuffer
    {
        InputAtributeMask mask;
        int mesh_index_count;
        std::shared_ptr<RHI::D3D12Buffer> p_mesh_index_buffer;
        int mesh_vertex_count;
        std::shared_ptr<RHI::D3D12Buffer> p_mesh_vertex_buffer;
    };

    struct MeshBuffer
    {
        std::vector<PrimitiveBuffer> primitiveBuffers;
    };

    // 所有的Mesh默认加载进来的InputPlayer都要求是一样的
    struct Mesh
    {
        std::string uri = "";

        MeshData* meshData;
        MeshBuffer* d3d12MeshBuffer;
    };

    struct MatTexture
    {
        Texture texture;
        int     texCoordIndex; // 对应到TEXCCORD_<n>
    };

    struct Material
    {
        struct
        {
            MatTexture   baseColorTexture;
            Pilot::Color baseColorFactor; // "1.0, 0.75, 0.35, 1.0"

            MatTexture metallicRoughnessTexture;
            float      metallicFactor  = 1.0;
            float      roughnessFactor = 0.0;
        } pbrMetallicRoughness;

        MatTexture normalTexture;
        float      normalScale = 0.8;

        MatTexture occlusionTexture;
        float      occlusionStrength = 0.8;

        MatTexture     emissiveTexture;
        Pilot::Vector3 emissiveFactor; //"0.4, 0.8, 0.6";
    };

    struct MeshRenderer
    {
        PrimitiveMode mode;
        Mesh          mesh;
        Material      materialPtr; // 需要检查材质请求的texcoord的合理性

        RHI::MeshRenderer* meshRenderer;
    };

    //*********************

    class Skin
    {
        Accessor*          inverseBindMatrices; // 包含每一个IK的逆矩阵
        std::vector<Node*> joints;
    };

    enum AnimationPath
    {
        TRANSLATION,
        ROTATION,
        SCALE
    };

    struct AnimationChannel
    {
        struct
        {
            Node*         node;
            AnimationPath path = TRANSLATION;
        } Target;
        AnimationSampler* sampler;
    };

    enum AnimInterpolation
    {
        LINEAR,
        STEP,
        CUBICSPLINE
    };

    struct AnimationSampler
    {
        Accessor*         input;
        AnimInterpolation interpolation = LINEAR;
        Accessor*         output;
    };

    struct Animation
    {
        std::vector<AnimationChannel> channels;
        std::vector<AnimationSampler> samplers;
    };

    //*********************


    extern std::unordered_map<std::string, std::shared_ptr<MeshData>>   uri2MeshData;
    extern std::unordered_map<std::string, std::shared_ptr<MeshBuffer>> uri2MeshBuffer;

    extern std::unordered_map<std::string, std::shared_ptr<BufferData>>       uri2BufferData;
    extern std::unordered_map<std::string, std::shared_ptr<RHI::D3D12Buffer>> uri2Buffer;

    extern std::unordered_map<std::string, std::shared_ptr<ImageData>>         uri2ImageData;
    extern std::unordered_map<std::string, std::shared_ptr<RHI::D3D12Texture>> uri2Texture;

} // namespace Hierarchy
