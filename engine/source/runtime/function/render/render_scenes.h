#pragma once

#include "runtime/core/math/moyu_math.h"

namespace PScene
{

#define Float2 Pilot::Vector2
#define Float3 Pilot::Vector3
#define Float4 Pilot::Vector4
#define Float4x4 Pilot::Matrix4x4
#define Quaternion Pilot::Quaternion

#define InputAtributeMask unsigned long long

    class Config;
    class Level;
    class Scene;
    class Camera;
    class Node;
    class PerspectiveCamera;
    class OrthographicCamera;
    class Camera;
    class PointLight;
    class SpotLight;
    class DirecionLight;
    class AmbientLight;
    class Light;
    class Primitive;
    class Mesh;
    class Skin;
    class Material;
    class Accessor;
    class Animation;
    class Texture;
    class Sampler;
    class Image;
    class BufferView;
    class Buffer;

    class Config
    {
        std::vector<Texture*> textures;
        std::vector<Buffer*>  buffers;
    };

    class Level
    {
        Config* config;
        Scene*  scene;
    };

    enum LightType
    {
        Point,
        Spot,
        Direction,
        Ambient
    };

    class PointLight
    {
        Float3 color;
        float  radius;
        float  intensity;
    };

    class SpotLight
    {
        Float3 color;
        float  radius;
        float  direction;
        float  intensity;
        float  innerRadians;
        float  outerRadians;

        float  shadowmap;
        Float2 shadowBounds;
        float  shadowNearPlane;
        float  shadowFarPlane;
        Float2 shadowmapSize;
    };

    class DirecionLight
    {
        Float3 color;
        float  intensity;
        Float3 direcion;

        float  shadowmap;
        Float2 shadowBounds;
        float  shadowNearPlane;
        float  shadowFarPlane;
        Float2 shadowmapSize;
    };

    class AmbientLight
    {
        Float3 color;
    };

    class Light
    {
        LightType type;

        PointLight*    pointLight;
        SpotLight*     spotLight;
        DirecionLight* directionLight;
        AmbientLight*  ambientLight;
    };

    enum CameraType
    {
        Perspective,
        Orthographic
    };

    class PerspectiveCamera
    {
        float aspectRatio = 1.5;
        float tfov        = 0.65;
        float zfar        = 100;
        float znear       = 0.01;
    };

    class OrthographicCamera
    {
        float xmag  = 1.0;
        float ymag  = 1.0;
        float zfar  = 100;
        float znear = 0.01;
    };

    class Camera
    {
        CameraType type;

        PerspectiveCamera*  pers;
        OrthographicCamera* orthos;
    };

    class Buffer
    {
        std::string uri; // "buffer01.bin"
        int byteLength;  // 102040
    };

    class BufferView
    {
        Buffer* buffer     = nullptr;
        int     byteOffset = 4;
        int     byteLength = 28;
        int     byteStride = 12;
    };

    class Accessor
    {
        BufferView* bufferView;
        int         byteOffset    = 4;
        std::string type          = "VEC2";
        std::string componentType = "FLOAT";
        int         count         = 2;
        std::string min           = "[0.1, 0.2]";
        std::string max           = "[0.9, 0.8]";
    };

    class Texture
    {
        Image*   image;
        Sampler* sampler;
    };

    class Sampler
    {
        std::string magFilter;
        std::string minFilter;
        std::string wrapS;
        std::string wrapT;
    };

    class Image
    {
        bool autoGenMips;
        int mipLevels;
        std::string uri; // "image01.png"
    };

    class Scene
    {
        std::vector<Node*> nodes;
    };

    class Node
    {
        Float4x4   localMatrix; // M=T*R*S; column major storage
        Float3     translation;
        Quaternion rotation;
        Float3     scale;

        Node* parent;
        std::vector<Node*> children;

        Camera* camera = nullptr;
        Light*  light  = nullptr;
        Mesh*   mesh   = nullptr;
        Skin*   skin   = nullptr;
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

    InputAtributeMask PositionFlag  = 1 << POSITION;
    InputAtributeMask NormalFlag    = 1 << NORMAL;
    InputAtributeMask TangnetFlag   = 1 << TANGENT;
    InputAtributeMask TexCoord0Flag = 1 << TEXCOORD0;
    InputAtributeMask TexCoord1Flag = 1 << TEXCOORD1;
    InputAtributeMask TexCoord2Flag = 1 << TEXCOORD2;
    InputAtributeMask PosNormalFlag = PositionFlag | NormalFlag;

    class Primitive
    {
        PrimitiveMode mode;
        Accessor* indicesPtr;
        struct
        {
            InputAtributeMask mask;
            Accessor* verticesPtr;
        } vertexAttr;
        Material* materialPtr; // 需要检查材质请求的texcoord的合理性
    };

    class Mesh
    {
        std::vector<Primitive> primitives;
    };

    class MatTexture
    {
        Texture* texture;
        int texCoordIndex; // 对应到TEXCCORD_<n>
    };

    enum MatRenderType
    {
        Opaque,
        Transparent
    };

    class Material
    {
        MatRenderType renderType = Opaque; 

        struct
        {
            MatTexture baseColorTexture;
            Float4     baseColorFactor; // = (1.0, 0.75, 0.35, 1.0);
            MatTexture metallicRoughnessTexture;
            float      metallicFactor  = 1.0;
            float      roughnessFactor = 0.0;
        } pbrMetallicRoughness;

        MatTexture normalTexture;
        float      normalScale = 0.8;

        MatTexture occlusionTexture;
        float      occlusionStrength = 0.8;

        MatTexture emissiveTexture;
        Float3     emissiveFactor; // = "0.4, 0.8, 0.6";
    };

    class Skin
    {
        Accessor* inverseBindMatrices; // 包含每一个IK的逆矩阵
        std::vector<Node*> joints;
    };

    enum AnimationPath
    {
        TRANSLATION,
        ROTATION,
        SCALE
    };

    class AnimationChannel
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

    class AnimationSampler
    {
        Accessor*         input;
        AnimInterpolation interpolation = LINEAR;
        Accessor*         output;
    };

    class Animation
    {
        std::vector<AnimationChannel> channels;
        std::vector<AnimationSampler> samplers;
    };

} // namespace PScene

