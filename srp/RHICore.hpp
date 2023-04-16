#include "RHICommon.hpp"

namespace RHI
{

    #define	RHI_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT	( 256 )
    #define	RHI_RESOURCE_BARRIER_ALL_SUBRESOURCES	( 0xffffffff )
    #define RHI_DEFAULT_DEPTH_BIAS ( 0 )
    #define RHI_DEFAULT_DEPTH_BIAS_CLAMP	( 0.0f )
    #define RHI_DEFAULT_SLOPE_SCALED_DEPTH_BIAS	( 0.0f )

    constexpr UINT RHICalcSubresource( UINT MipSlice, UINT ArraySlice, UINT PlaneSlice, UINT MipLevels, UINT ArraySize ) noexcept
    {
        return MipSlice + ArraySlice * MipLevels + PlaneSlice * MipLevels * ArraySize;
    }

    template <typename T, typename U, typename V>
    inline void RHIDecomposeSubresource( UINT Subresource, UINT MipLevels, UINT ArraySize, _Out_ T& MipSlice, _Out_ U& ArraySlice, _Out_ V& PlaneSlice ) noexcept
    {
        MipSlice = static_cast<T>(Subresource % MipLevels);
        ArraySlice = static_cast<U>((Subresource / MipLevels) % ArraySize);
        PlaneSlice = static_cast<V>(Subresource / (MipLevels * ArraySize));
    }

    typedef enum RHI_FORMAT
    {
        RHI_FORMAT_UNKNOWN	                               = 0,
        RHI_FORMAT_R32G32B32A32_TYPELESS                   = 1,
        RHI_FORMAT_R32G32B32A32_FLOAT                      = 2,
        RHI_FORMAT_R32G32B32A32_UINT                       = 3,
        RHI_FORMAT_R32G32B32A32_SINT                       = 4,
        RHI_FORMAT_R32G32B32_TYPELESS                      = 5,
        RHI_FORMAT_R32G32B32_FLOAT                         = 6,
        RHI_FORMAT_R32G32B32_UINT                          = 7,
        RHI_FORMAT_R32G32B32_SINT                          = 8,
        RHI_FORMAT_R16G16B16A16_TYPELESS                   = 9,
        RHI_FORMAT_R16G16B16A16_FLOAT                      = 10,
        RHI_FORMAT_R16G16B16A16_UNORM                      = 11,
        RHI_FORMAT_R16G16B16A16_UINT                       = 12,
        RHI_FORMAT_R16G16B16A16_SNORM                      = 13,
        RHI_FORMAT_R16G16B16A16_SINT                       = 14,
        RHI_FORMAT_R32G32_TYPELESS                         = 15,
        RHI_FORMAT_R32G32_FLOAT                            = 16,
        RHI_FORMAT_R32G32_UINT                             = 17,
        RHI_FORMAT_R32G32_SINT                             = 18,
        RHI_FORMAT_R32G8X24_TYPELESS                       = 19,
        RHI_FORMAT_D32_FLOAT_S8X24_UINT                    = 20,
        RHI_FORMAT_R32_FLOAT_X8X24_TYPELESS                = 21,
        RHI_FORMAT_X32_TYPELESS_G8X24_UINT                 = 22,
        RHI_FORMAT_R10G10B10A2_TYPELESS                    = 23,
        RHI_FORMAT_R10G10B10A2_UNORM                       = 24,
        RHI_FORMAT_R10G10B10A2_UINT                        = 25,
        RHI_FORMAT_R11G11B10_FLOAT                         = 26,
        RHI_FORMAT_R8G8B8A8_TYPELESS                       = 27,
        RHI_FORMAT_R8G8B8A8_UNORM                          = 28,
        RHI_FORMAT_R8G8B8A8_UNORM_SRGB                     = 29,
        RHI_FORMAT_R8G8B8A8_UINT                           = 30,
        RHI_FORMAT_R8G8B8A8_SNORM                          = 31,
        RHI_FORMAT_R8G8B8A8_SINT                           = 32,
        RHI_FORMAT_R16G16_TYPELESS                         = 33,
        RHI_FORMAT_R16G16_FLOAT                            = 34,
        RHI_FORMAT_R16G16_UNORM                            = 35,
        RHI_FORMAT_R16G16_UINT                             = 36,
        RHI_FORMAT_R16G16_SNORM                            = 37,
        RHI_FORMAT_R16G16_SINT                             = 38,
        RHI_FORMAT_R32_TYPELESS                            = 39,
        RHI_FORMAT_D32_FLOAT                               = 40,
        RHI_FORMAT_R32_FLOAT                               = 41,
        RHI_FORMAT_R32_UINT                                = 42,
        RHI_FORMAT_R32_SINT                                = 43,
        RHI_FORMAT_R24G8_TYPELESS                          = 44,
        RHI_FORMAT_D24_UNORM_S8_UINT                       = 45,
        RHI_FORMAT_R24_UNORM_X8_TYPELESS                   = 46,
        RHI_FORMAT_X24_TYPELESS_G8_UINT                    = 47,
        RHI_FORMAT_R8G8_TYPELESS                           = 48,
        RHI_FORMAT_R8G8_UNORM                              = 49,
        RHI_FORMAT_R8G8_UINT                               = 50,
        RHI_FORMAT_R8G8_SNORM                              = 51,
        RHI_FORMAT_R8G8_SINT                               = 52,
        RHI_FORMAT_R16_TYPELESS                            = 53,
        RHI_FORMAT_R16_FLOAT                               = 54,
        RHI_FORMAT_D16_UNORM                               = 55,
        RHI_FORMAT_R16_UNORM                               = 56,
        RHI_FORMAT_R16_UINT                                = 57,
        RHI_FORMAT_R16_SNORM                               = 58,
        RHI_FORMAT_R16_SINT                                = 59,
        RHI_FORMAT_R8_TYPELESS                             = 60,
        RHI_FORMAT_R8_UNORM                                = 61,
        RHI_FORMAT_R8_UINT                                 = 62,
        RHI_FORMAT_R8_SNORM                                = 63,
        RHI_FORMAT_R8_SINT                                 = 64,
        RHI_FORMAT_A8_UNORM                                = 65,
        RHI_FORMAT_R1_UNORM                                = 66,
        RHI_FORMAT_R9G9B9E5_SHAREDEXP                      = 67,
        RHI_FORMAT_R8G8_B8G8_UNORM                         = 68,
        RHI_FORMAT_G8R8_G8B8_UNORM                         = 69,
        RHI_FORMAT_BC1_TYPELESS                            = 70,
        RHI_FORMAT_BC1_UNORM                               = 71,
        RHI_FORMAT_BC1_UNORM_SRGB                          = 72,
        RHI_FORMAT_BC2_TYPELESS                            = 73,
        RHI_FORMAT_BC2_UNORM                               = 74,
        RHI_FORMAT_BC2_UNORM_SRGB                          = 75,
        RHI_FORMAT_BC3_TYPELESS                            = 76,
        RHI_FORMAT_BC3_UNORM                               = 77,
        RHI_FORMAT_BC3_UNORM_SRGB                          = 78,
        RHI_FORMAT_BC4_TYPELESS                            = 79,
        RHI_FORMAT_BC4_UNORM                               = 80,
        RHI_FORMAT_BC4_SNORM                               = 81,
        RHI_FORMAT_BC5_TYPELESS                            = 82,
        RHI_FORMAT_BC5_UNORM                               = 83,
        RHI_FORMAT_BC5_SNORM                               = 84,
        RHI_FORMAT_B5G6R5_UNORM                            = 85,
        RHI_FORMAT_B5G5R5A1_UNORM                          = 86,
        RHI_FORMAT_B8G8R8A8_UNORM                          = 87,
        RHI_FORMAT_B8G8R8X8_UNORM                          = 88,
        RHI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM              = 89,
        RHI_FORMAT_B8G8R8A8_TYPELESS                       = 90,
        RHI_FORMAT_B8G8R8A8_UNORM_SRGB                     = 91,
        RHI_FORMAT_B8G8R8X8_TYPELESS                       = 92,
        RHI_FORMAT_B8G8R8X8_UNORM_SRGB                     = 93,
        RHI_FORMAT_BC6H_TYPELESS                           = 94,
        RHI_FORMAT_BC6H_UF16                               = 95,
        RHI_FORMAT_BC6H_SF16                               = 96,
        RHI_FORMAT_BC7_TYPELESS                            = 97,
        RHI_FORMAT_BC7_UNORM                               = 98,
        RHI_FORMAT_BC7_UNORM_SRGB                          = 99,

        RHI_FORMAT_FORCE_UINT                  = 0xffffffff
    } RHI_FORMAT;

    struct RHI_SUBRESOURCE_DATA
    {
        const void *pData;
        INT64 RowPitch;
        INT64 SlicePitch;
    };

    typedef 
    enum RHI_RESOURCE_STATES
    {
        RHI_RESOURCE_STATE_COMMON	= 0,
        RHI_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER	= 0x1,
        RHI_RESOURCE_STATE_INDEX_BUFFER	= 0x2,
        RHI_RESOURCE_STATE_RENDER_TARGET	= 0x4,
        RHI_RESOURCE_STATE_UNORDERED_ACCESS	= 0x8,
        RHI_RESOURCE_STATE_DEPTH_WRITE	= 0x10,
        RHI_RESOURCE_STATE_DEPTH_READ	= 0x20,
        RHI_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE	= 0x40,
        RHI_RESOURCE_STATE_PIXEL_SHADER_RESOURCE	= 0x80,
        RHI_RESOURCE_STATE_STREAM_OUT	= 0x100,
        RHI_RESOURCE_STATE_INDIRECT_ARGUMENT	= 0x200,
        RHI_RESOURCE_STATE_COPY_DEST	= 0x400,
        RHI_RESOURCE_STATE_COPY_SOURCE	= 0x800,
        RHI_RESOURCE_STATE_RESOLVE_DEST	= 0x1000,
        RHI_RESOURCE_STATE_RESOLVE_SOURCE	= 0x2000,
        RHI_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE	= 0x400000,
        RHI_RESOURCE_STATE_SHADING_RATE_SOURCE	= 0x1000000,
        RHI_RESOURCE_STATE_GENERIC_READ	= ( ( ( ( ( 0x1 | 0x2 )  | 0x40 )  | 0x80 )  | 0x200 )  | 0x800 ) ,
        RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE	= ( 0x40 | 0x80 ) ,
        RHI_RESOURCE_STATE_PRESENT	= 0,
        RHI_RESOURCE_STATE_PREDICATION	= 0x200,
    } 	RHI_RESOURCE_STATES;
    DEFINE_RHI_ENUM_FLAG_OPERATORS( RHI_RESOURCE_STATES );

    typedef 
    enum RHI_PREDICATION_OP
    {
        RHI_PREDICATION_OP_EQUAL_ZERO	= 0,
        RHI_PREDICATION_OP_NOT_EQUAL_ZERO	= 1
    } 	RHI_PREDICATION_OP;

    typedef UINT64 RHI_GPU_VIRTUAL_ADDRESS;

    typedef struct RHI_CPU_DESCRIPTOR_HANDLE
    {
    SIZE_T ptr;
    } 	RHI_CPU_DESCRIPTOR_HANDLE;

    typedef struct RHI_GPU_DESCRIPTOR_HANDLE
    {
    UINT64 ptr;
    } 	RHI_GPU_DESCRIPTOR_HANDLE;

    typedef struct RHI_DISPATCH_ARGUMENTS
    {
    UINT ThreadGroupCountX;
    UINT ThreadGroupCountY;
    UINT ThreadGroupCountZ;
    } 	RHI_DISPATCH_ARGUMENTS;

    typedef struct RHI_VERTEX_BUFFER_VIEW
    {
    RHI_GPU_VIRTUAL_ADDRESS BufferLocation;
    UINT SizeInBytes;
    UINT StrideInBytes;
    } 	RHI_VERTEX_BUFFER_VIEW;

    typedef struct RHI_INDEX_BUFFER_VIEW
    {
    RHI_GPU_VIRTUAL_ADDRESS BufferLocation;
    UINT SizeInBytes;
    RHI_FORMAT Format;
    } 	RHI_INDEX_BUFFER_VIEW;

    enum class RHI_SHADER_MODEL
    {
        ShaderModel_6_5,
        ShaderModel_6_6
    };

    enum class RHI_SHADER_TYPE
    {
        Vertex,
        Hull,
        Domain,
        Geometry,
        Pixel,
        Compute,
        Amplification,
        Mesh
    };

    enum class RHI_PIPELINE_STATE_TYPE
    {
        Graphics,
        Compute,
    };

    enum class RHI_PIPELINE_STATE_SUBOBJECT_TYPE
    {
        RootSignature,
        InputLayout,
        VS,
        PS,
        DS,
        HS,
        GS,
        CS,
        AS,
        MS,
        BlendState,
        RasterizerState,
        DepthStencilState,
        RenderTargetState,
        SampleState,
        PrimitiveTopology,

        NumTypes
    };

    inline const char* GetRHIPipelineStateSubobjectTypeString(RHI_PIPELINE_STATE_SUBOBJECT_TYPE Type)
    {
        switch (Type)
        {
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::RootSignature:
                return "Root Signature";
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::InputLayout:
                return "Input Layout";
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::VS:
                return "Vertex Shader";
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::PS:
                return "Pixel Shader";
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::DS:
                return "Domain Shader";
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::HS:
                return "Hull Shader";
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::GS:
                return "Geometry Shader";
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::CS:
                return "Compute Shader";
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::AS:
                return "Amplification Shader";
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::MS:
                return "Mesh Shader";
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::BlendState:
                return "Blend State";
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::RasterizerState:
                return "Rasterizer State";
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::DepthStencilState:
                return "Depth Stencil State";
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::RenderTargetState:
                return "Render Target State";
            case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::PrimitiveTopology:
                return "Primitive Topology";
        }
        return "<unknown>";
    }

    enum RHI_PRIMITIVE_TOPOLOGY
    {
        Undefined,
        PointList,
        LineList,
        LineStrip,
        TriangleList,
        TriangleStrip
    };

    enum class RHI_COMPARISON_FUNC
    {
        Never,        // Comparison always fails
        Less,         // Passes if source is less than the destination
        Equal,        // Passes if source is equal to the destination
        LessEqual,    // Passes if source is less than or equal to the destination
        Greater,      // Passes if source is greater than to the destination
        NotEqual,     // Passes if source is not equal to the destination
        GreaterEqual, // Passes if source is greater than or equal to the destination
        Always        // Comparison always succeeds
    };

    enum class RHI_SAMPLER_FILTER
    {
        Point,
        Linear,
        Anisotropic
    };

    enum class RHI_SAMPLER_ADDRESS_MODE
    {
        Wrap,
        Mirror,
        Clamp,
        Border,
    };

    enum class RHI_BLEND_OP
    {
        Add,
        Subtract,
        ReverseSubtract,
        Min,
        Max
    };

    enum class RHI_FACTOR
    {
        Zero,                // (0, 0, 0, 0)
        One,                 // (1, 1, 1, 1)
        SrcColor,            // The pixel-shader output color
        OneMinusSrcColor,    // One minus the pixel-shader output color
        DstColor,            // The render-target color
        OneMinusDstColor,    // One minus the render-target color
        SrcAlpha,            // The pixel-shader output alpha value
        OneMinusSrcAlpha,    // One minus the pixel-shader output alpha value
        DstAlpha,            // The render-target alpha value
        OneMinusDstAlpha,    // One minus the render-target alpha value
        BlendFactor,         // Constant color, set using CommandList
        OneMinusBlendFactor, // One minus constant color, set using CommandList
        SrcAlphaSaturate,    // (f, f, f, 1), where f = min(pixel shader output alpha, 1 - render-target pixel alpha)
        Src1Color,           // Fragment-shader output color 1
        OneMinusSrc1Color,   // One minus pixel-shader output color 1
        Src1Alpha,           // Fragment-shader output alpha 1
        OneMinusSrc1Alpha    // One minus pixel-shader output alpha 1
    };

    struct RHIRenderTargetBlendDesc
    {
        BOOL         BlendEnable   = FALSE;
        RHI_FACTOR   SrcBlendRgb   = RHI_FACTOR::One;
        RHI_FACTOR   DstBlendRgb   = RHI_FACTOR::Zero;
        RHI_BLEND_OP BlendOpRgb    = RHI_BLEND_OP::Add;
        RHI_FACTOR   SrcBlendAlpha = RHI_FACTOR::One;
        RHI_FACTOR   DstBlendAlpha = RHI_FACTOR::Zero;
        RHI_BLEND_OP BlendOpAlpha  = RHI_BLEND_OP::Add;

        struct WriteMask
        {
            BOOL R = TRUE;
            BOOL G = TRUE;
            BOOL B = TRUE;
            BOOL A = TRUE;
        } WriteMask;
    };

    enum class RHI_FILL_MODE
    {
        Wireframe, // Draw lines connecting the vertices.
        Solid      // Fill the triangles formed by the vertices
    };

    enum class RHI_CULL_MODE
    {
        None,  // Always draw all triangles
        Front, // Do not draw triangles that are front-facing
        Back   // Do not draw triangles that are back-facing
    };

    enum class RHI_STENCIL_OP
    {
        Keep,             // Keep the stencil value
        Zero,             // Set the stencil value to zero
        Replace,          // Replace the stencil value with the reference value
        IncreaseSaturate, // Increase the stencil value by one, clamp if necessary
        DecreaseSaturate, // Decrease the stencil value by one, clamp if necessary
        Invert,           // Invert the stencil data (bitwise not)
        Increase,         // Increase the stencil value by one, wrap if necessary
        Decrease          // Decrease the stencil value by one, wrap if necessary
    };

    struct RHIDepthStencilOpDesc
    {
        RHI_STENCIL_OP      StencilFailOp      = RHI_STENCIL_OP::Keep;
        RHI_STENCIL_OP      StencilDepthFailOp = RHI_STENCIL_OP::Keep;
        RHI_STENCIL_OP      StencilPassOp      = RHI_STENCIL_OP::Keep;
        RHI_COMPARISON_FUNC StencilFunc        = RHI_COMPARISON_FUNC::Always;
    };

    struct RHIBlendState
    {
        BOOL                     AlphaToCoverageEnable  = FALSE;
        BOOL                     IndependentBlendEnable = FALSE;
        RHIRenderTargetBlendDesc RenderTargets[8];
    };

    enum RHI_CONSERVATIVE_RASTERIZATION_MODE
    {
        RHI_CONSERVATIVE_RASTERIZATION_MODE_OFF	= 0,
        RHI_CONSERVATIVE_RASTERIZATION_MODE_ON	= 1
    };

    struct RHIRasterizerState
    {
        RHI_FILL_MODE FillMode              = RHI_FILL_MODE::Solid;
        RHI_CULL_MODE CullMode              = RHI_CULL_MODE::Back;
        BOOL          FrontCounterClockwise = TRUE;
        INT           DepthBias             = RHI_DEFAULT_DEPTH_BIAS;
        FLOAT         DepthBiasClamp        = RHI_DEFAULT_DEPTH_BIAS_CLAMP;
        FLOAT         SlopeScaledDepthBias  = RHI_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        BOOL          DepthClipEnable       = TRUE;
        BOOL          MultisampleEnable     = FALSE;
        BOOL          AntialiasedLineEnable = FALSE;
        UINT          ForcedSampleCount     = 0;
        BOOL          ConservativeRaster    = RHI_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    };

    struct RHIDepthStencilState
    {
        // Default states
        // https://docs.microsoft.com/en-us/windows/win32/api/RHI/ns-RHI-RHI_depth_stencil_desc#remarks
        BOOL                  DepthEnable      = TRUE;
        BOOL                  DepthWrite       = TRUE;
        RHI_COMPARISON_FUNC   DepthFunc        = RHI_COMPARISON_FUNC::Less;
        BOOL                  StencilEnable    = FALSE;
        BYTE                  StencilReadMask  = BYTE {0xff};
        BYTE                  StencilWriteMask = BYTE {0xff};
        RHIDepthStencilOpDesc FrontFace;
        RHIDepthStencilOpDesc BackFace;
    };

    struct RHIRenderTargetState
    {
        RHI_FORMAT RTFormats[8]     = {RHI_FORMAT_UNKNOWN};
        UINT       NumRenderTargets = 0;
        RHI_FORMAT DSFormat         = RHI_FORMAT_UNKNOWN;
    };

    struct RHISampleState
    {
        UINT Count   = 1;
        UINT Quality = 0;
    };

    struct RHIViewport
    {
        FLOAT TopLeftX;
        FLOAT TopLeftY;
        FLOAT Width;
        FLOAT Height;
        FLOAT MinDepth;
        FLOAT MaxDepth;
    };

    struct RHIRect
    {
        LONG Left;
        LONG Top;
        LONG Right;
        LONG Bottom;
    };

    template<typename TDesc, RHI_PIPELINE_STATE_SUBOBJECT_TYPE TType>
    class alignas(void*) PipelineStateStreamSubobject
    {
    public:
        PipelineStateStreamSubobject() noexcept : Type(TType), Desc(TDesc()) {}
        PipelineStateStreamSubobject(const TDesc& Desc) noexcept : Type(TType), Desc(Desc) {}
        PipelineStateStreamSubobject& operator=(const TDesc& Desc) noexcept
        {
            this->Type = TType;
            this->Desc = Desc;
            return *this;
        }
                    operator const TDesc&() const noexcept { return Desc; }
                    operator TDesc&() noexcept { return Desc; }
        TDesc*       operator&() noexcept { return &Desc; }
        const TDesc* operator&() const noexcept { return &Desc; }

        TDesc& operator->() noexcept { return Desc; }

    private:
        RHI_PIPELINE_STATE_SUBOBJECT_TYPE Type;
        TDesc                             Desc;
    };

    class RHIRootSignature;
    class RHIInputLayout;
    class RHIShader;

    using PipelineStateStreamRootSignature = PipelineStateStreamSubobject<RHI::RHIRootSignature*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::RootSignature>;
    using PipelineStateStreamInputLayout = PipelineStateStreamSubobject<RHI::RHIInputLayout*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::InputLayout>;
    using PipelineStateStreamVS = PipelineStateStreamSubobject<RHIShader*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::VS>;
    using PipelineStateStreamPS = PipelineStateStreamSubobject<RHIShader*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::PS>;
    using PipelineStateStreamDS = PipelineStateStreamSubobject<RHIShader*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::DS>;
    using PipelineStateStreamHS = PipelineStateStreamSubobject<RHIShader*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::HS>;
    using PipelineStateStreamGS = PipelineStateStreamSubobject<RHIShader*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::GS>;
    using PipelineStateStreamCS = PipelineStateStreamSubobject<RHIShader*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::CS>;
    using PipelineStateStreamAS = PipelineStateStreamSubobject<RHIShader*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::AS>;
    using PipelineStateStreamMS = PipelineStateStreamSubobject<RHIShader*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::MS>;
    using PipelineStateStreamBlendState = PipelineStateStreamSubobject<RHIBlendState, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::BlendState>;
    using PipelineStateStreamRasterizerState = PipelineStateStreamSubobject<RHIRasterizerState, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::RasterizerState>;
    using PipelineStateStreamDepthStencilState = PipelineStateStreamSubobject<RHIDepthStencilState, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::DepthStencilState>;
    using PipelineStateStreamRenderTargetState = PipelineStateStreamSubobject<RHIRenderTargetState, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::RenderTargetState>;
    using PipelineStateStreamSampleState = PipelineStateStreamSubobject<RHISampleState, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::SampleState>;
    using PipelineStateStreamPrimitiveTopology = PipelineStateStreamSubobject<RHI_PRIMITIVE_TOPOLOGY, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::PrimitiveTopology>;

    class IPipelineParserCallbacks
    {
    public:
        virtual ~IPipelineParserCallbacks() = default;

        // Subobject Callbacks
        virtual void RootSignatureCb(RHI::RHIRootSignature*) {}
        virtual void InputLayoutCb(RHI::RHIInputLayout*) {}
        virtual void VSCb(RHIShader*) {}
        virtual void PSCb(RHIShader*) {}
        virtual void DSCb(RHIShader*) {}
        virtual void HSCb(RHIShader*) {}
        virtual void GSCb(RHIShader*) {}
        virtual void CSCb(RHIShader*) {}
        virtual void ASCb(RHIShader*) {}
        virtual void MSCb(RHIShader*) {}
        virtual void BlendStateCb(const RHIBlendState&) {}
        virtual void RasterizerStateCb(const RHIRasterizerState&) {}
        virtual void DepthStencilStateCb(const RHIDepthStencilState&) {}
        virtual void RenderTargetStateCb(const RHIRenderTargetState&) {}
        virtual void SampleStateCb(const RHISampleState&) {}
        virtual void PrimitiveTopologyTypeCb(RHI_PRIMITIVE_TOPOLOGY) {}

        // Error Callbacks
        virtual void ErrorBadInputParameter(size_t /*ParameterIndex*/) {}
        virtual void ErrorDuplicateSubobject(RHI_PIPELINE_STATE_SUBOBJECT_TYPE /*DuplicateType*/) {}
        virtual void ErrorUnknownSubobject(size_t /*UnknownTypeValue*/) {}
    };

    struct PipelineStateStreamDesc
    {
        SIZE_T SizeInBytes;
        void*  pPipelineStateSubobjectStream;
    };

    void RHIParsePipelineStream(const PipelineStateStreamDesc& Desc, IPipelineParserCallbacks* Callbacks);


    //-------------------------------------------------------------------------

    typedef 
    enum RHI_BUFFER_SRV_FLAGS
        {
            RHI_BUFFER_SRV_FLAG_NONE	= 0,
            RHI_BUFFER_SRV_FLAG_RAW	= 0x1
        } 	RHI_BUFFER_SRV_FLAGS;

    DEFINE_RHI_ENUM_FLAG_OPERATORS( RHI_BUFFER_SRV_FLAGS );
    typedef struct RHI_BUFFER_SRV
        {
        UINT64 FirstElement;
        UINT NumElements;
        UINT StructureByteStride;
        RHI_BUFFER_SRV_FLAGS Flags;
        } 	RHI_BUFFER_SRV;

    typedef struct RHI_TEX1D_SRV
        {
        UINT MostDetailedMip;
        UINT MipLevels;
        FLOAT ResourceMinLODClamp;
        } 	RHI_TEX1D_SRV;

    typedef struct RHI_TEX1D_ARRAY_SRV
        {
        UINT MostDetailedMip;
        UINT MipLevels;
        UINT FirstArraySlice;
        UINT ArraySize;
        FLOAT ResourceMinLODClamp;
        } 	RHI_TEX1D_ARRAY_SRV;

    typedef struct RHI_TEX2D_SRV
        {
        UINT MostDetailedMip;
        UINT MipLevels;
        UINT PlaneSlice;
        FLOAT ResourceMinLODClamp;
        } 	RHI_TEX2D_SRV;

    typedef struct RHI_TEX2D_ARRAY_SRV
        {
        UINT MostDetailedMip;
        UINT MipLevels;
        UINT FirstArraySlice;
        UINT ArraySize;
        UINT PlaneSlice;
        FLOAT ResourceMinLODClamp;
        } 	RHI_TEX2D_ARRAY_SRV;

    typedef struct RHI_TEX3D_SRV
        {
        UINT MostDetailedMip;
        UINT MipLevels;
        FLOAT ResourceMinLODClamp;
        } 	RHI_TEX3D_SRV;

    typedef struct RHI_TEXCUBE_SRV
        {
        UINT MostDetailedMip;
        UINT MipLevels;
        FLOAT ResourceMinLODClamp;
        } 	RHI_TEXCUBE_SRV;

    typedef struct RHI_TEXCUBE_ARRAY_SRV
        {
        UINT MostDetailedMip;
        UINT MipLevels;
        UINT First2DArrayFace;
        UINT NumCubes;
        FLOAT ResourceMinLODClamp;
        } 	RHI_TEXCUBE_ARRAY_SRV;

    typedef struct RHI_TEX2DMS_SRV
        {
        UINT UnusedField_NothingToDefine;
        } 	RHI_TEX2DMS_SRV;

    typedef struct RHI_TEX2DMS_ARRAY_SRV
        {
        UINT FirstArraySlice;
        UINT ArraySize;
        } 	RHI_TEX2DMS_ARRAY_SRV;

    typedef struct RHI_RAYTRACING_ACCELERATION_STRUCTURE_SRV
        {
        RHI_GPU_VIRTUAL_ADDRESS Location;
        } 	RHI_RAYTRACING_ACCELERATION_STRUCTURE_SRV;

    typedef 
    enum RHI_SRV_DIMENSION
        {
            RHI_SRV_DIMENSION_UNKNOWN	= 0,
            RHI_SRV_DIMENSION_BUFFER	= 1,
            RHI_SRV_DIMENSION_TEXTURE1D	= 2,
            RHI_SRV_DIMENSION_TEXTURE1DARRAY	= 3,
            RHI_SRV_DIMENSION_TEXTURE2D	= 4,
            RHI_SRV_DIMENSION_TEXTURE2DARRAY	= 5,
            RHI_SRV_DIMENSION_TEXTURE2DMS	= 6,
            RHI_SRV_DIMENSION_TEXTURE2DMSARRAY	= 7,
            RHI_SRV_DIMENSION_TEXTURE3D	= 8,
            RHI_SRV_DIMENSION_TEXTURECUBE	= 9,
            RHI_SRV_DIMENSION_TEXTURECUBEARRAY	= 10,
            RHI_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE	= 11
        } 	RHI_SRV_DIMENSION;

    typedef struct RHI_SHADER_RESOURCE_VIEW_DESC
        {
        RHI_FORMAT Format;
        RHI_SRV_DIMENSION ViewDimension;
        UINT Shader4ComponentMapping;
        union 
            {
            RHI_BUFFER_SRV Buffer;
            RHI_TEX1D_SRV Texture1D;
            RHI_TEX1D_ARRAY_SRV Texture1DArray;
            RHI_TEX2D_SRV Texture2D;
            RHI_TEX2D_ARRAY_SRV Texture2DArray;
            RHI_TEX2DMS_SRV Texture2DMS;
            RHI_TEX2DMS_ARRAY_SRV Texture2DMSArray;
            RHI_TEX3D_SRV Texture3D;
            RHI_TEXCUBE_SRV TextureCube;
            RHI_TEXCUBE_ARRAY_SRV TextureCubeArray;
            RHI_RAYTRACING_ACCELERATION_STRUCTURE_SRV RaytracingAccelerationStructure;
            } 	;
        } 	RHI_SHADER_RESOURCE_VIEW_DESC;

    typedef struct RHI_CONSTANT_BUFFER_VIEW_DESC
        {
        RHI_GPU_VIRTUAL_ADDRESS BufferLocation;
        UINT SizeInBytes;
        } 	RHI_CONSTANT_BUFFER_VIEW_DESC;

    typedef 
    enum RHI_FILTER
        {
            RHI_FILTER_MIN_MAG_MIP_POINT	= 0,
            RHI_FILTER_MIN_MAG_POINT_MIP_LINEAR	= 0x1,
            RHI_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT	= 0x4,
            RHI_FILTER_MIN_POINT_MAG_MIP_LINEAR	= 0x5,
            RHI_FILTER_MIN_LINEAR_MAG_MIP_POINT	= 0x10,
            RHI_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR	= 0x11,
            RHI_FILTER_MIN_MAG_LINEAR_MIP_POINT	= 0x14,
            RHI_FILTER_MIN_MAG_MIP_LINEAR	= 0x15,
            RHI_FILTER_ANISOTROPIC	= 0x55,
            RHI_FILTER_COMPARISON_MIN_MAG_MIP_POINT	= 0x80,
            RHI_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR	= 0x81,
            RHI_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT	= 0x84,
            RHI_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR	= 0x85,
            RHI_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT	= 0x90,
            RHI_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR	= 0x91,
            RHI_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT	= 0x94,
            RHI_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR	= 0x95,
            RHI_FILTER_COMPARISON_ANISOTROPIC	= 0xd5
        } 	RHI_FILTER;

    typedef 
    enum RHI_BUFFER_UAV_FLAGS
        {
            RHI_BUFFER_UAV_FLAG_NONE	= 0,
            RHI_BUFFER_UAV_FLAG_RAW	= 0x1
        } 	RHI_BUFFER_UAV_FLAGS;

    DEFINE_RHI_ENUM_FLAG_OPERATORS( RHI_BUFFER_UAV_FLAGS );
    typedef struct RHI_BUFFER_UAV
        {
        UINT64 FirstElement;
        UINT NumElements;
        UINT StructureByteStride;
        UINT64 CounterOffsetInBytes;
        RHI_BUFFER_UAV_FLAGS Flags;
        } 	RHI_BUFFER_UAV;

    typedef struct RHI_TEX1D_UAV
        {
        UINT MipSlice;
        } 	RHI_TEX1D_UAV;

    typedef struct RHI_TEX1D_ARRAY_UAV
        {
        UINT MipSlice;
        UINT FirstArraySlice;
        UINT ArraySize;
        } 	RHI_TEX1D_ARRAY_UAV;

    typedef struct RHI_TEX2D_UAV
        {
        UINT MipSlice;
        UINT PlaneSlice;
        } 	RHI_TEX2D_UAV;

    typedef struct RHI_TEX2D_ARRAY_UAV
        {
        UINT MipSlice;
        UINT FirstArraySlice;
        UINT ArraySize;
        UINT PlaneSlice;
        } 	RHI_TEX2D_ARRAY_UAV;

    typedef struct RHI_TEX2DMS_UAV
        {
        UINT UnusedField_NothingToDefine;
        } 	RHI_TEX2DMS_UAV;

    typedef struct RHI_TEX2DMS_ARRAY_UAV
        {
        UINT FirstArraySlice;
        UINT ArraySize;
        } 	RHI_TEX2DMS_ARRAY_UAV;

    typedef struct RHI_TEX3D_UAV
        {
        UINT MipSlice;
        UINT FirstWSlice;
        UINT WSize;
        } 	RHI_TEX3D_UAV;

    typedef 
    enum RHI_UAV_DIMENSION
        {
            RHI_UAV_DIMENSION_UNKNOWN	= 0,
            RHI_UAV_DIMENSION_BUFFER	= 1,
            RHI_UAV_DIMENSION_TEXTURE1D	= 2,
            RHI_UAV_DIMENSION_TEXTURE1DARRAY	= 3,
            RHI_UAV_DIMENSION_TEXTURE2D	= 4,
            RHI_UAV_DIMENSION_TEXTURE2DARRAY	= 5,
            RHI_UAV_DIMENSION_TEXTURE2DMS	= 6,
            RHI_UAV_DIMENSION_TEXTURE2DMSARRAY	= 7,
            RHI_UAV_DIMENSION_TEXTURE3D	= 8
        } 	RHI_UAV_DIMENSION;

    typedef struct RHI_UNORDERED_ACCESS_VIEW_DESC
        {
        RHI_FORMAT Format;
        RHI_UAV_DIMENSION ViewDimension;
        union 
            {
            RHI_BUFFER_UAV Buffer;
            RHI_TEX1D_UAV Texture1D;
            RHI_TEX1D_ARRAY_UAV Texture1DArray;
            RHI_TEX2D_UAV Texture2D;
            RHI_TEX2D_ARRAY_UAV Texture2DArray;
            RHI_TEX2DMS_UAV Texture2DMS;
            RHI_TEX2DMS_ARRAY_UAV Texture2DMSArray;
            RHI_TEX3D_UAV Texture3D;
            } 	;
        } 	RHI_UNORDERED_ACCESS_VIEW_DESC;

    typedef struct RHI_BUFFER_RTV
        {
        UINT64 FirstElement;
        UINT NumElements;
        } 	RHI_BUFFER_RTV;

    typedef struct RHI_TEX1D_RTV
        {
        UINT MipSlice;
        } 	RHI_TEX1D_RTV;

    typedef struct RHI_TEX1D_ARRAY_RTV
        {
        UINT MipSlice;
        UINT FirstArraySlice;
        UINT ArraySize;
        } 	RHI_TEX1D_ARRAY_RTV;

    typedef struct RHI_TEX2D_RTV
        {
        UINT MipSlice;
        UINT PlaneSlice;
        } 	RHI_TEX2D_RTV;

    typedef struct RHI_TEX2DMS_RTV
        {
        UINT UnusedField_NothingToDefine;
        } 	RHI_TEX2DMS_RTV;

    typedef struct RHI_TEX2D_ARRAY_RTV
        {
        UINT MipSlice;
        UINT FirstArraySlice;
        UINT ArraySize;
        UINT PlaneSlice;
        } 	RHI_TEX2D_ARRAY_RTV;

    typedef struct RHI_TEX2DMS_ARRAY_RTV
        {
        UINT FirstArraySlice;
        UINT ArraySize;
        } 	RHI_TEX2DMS_ARRAY_RTV;

    typedef struct RHI_TEX3D_RTV
        {
        UINT MipSlice;
        UINT FirstWSlice;
        UINT WSize;
        } 	RHI_TEX3D_RTV;

    typedef 
    enum RHI_RTV_DIMENSION
        {
            RHI_RTV_DIMENSION_UNKNOWN	= 0,
            RHI_RTV_DIMENSION_BUFFER	= 1,
            RHI_RTV_DIMENSION_TEXTURE1D	= 2,
            RHI_RTV_DIMENSION_TEXTURE1DARRAY	= 3,
            RHI_RTV_DIMENSION_TEXTURE2D	= 4,
            RHI_RTV_DIMENSION_TEXTURE2DARRAY	= 5,
            RHI_RTV_DIMENSION_TEXTURE2DMS	= 6,
            RHI_RTV_DIMENSION_TEXTURE2DMSARRAY	= 7,
            RHI_RTV_DIMENSION_TEXTURE3D	= 8
        } 	RHI_RTV_DIMENSION;

    typedef struct RHI_RENDER_TARGET_VIEW_DESC
        {
        RHI_FORMAT Format;
        RHI_RTV_DIMENSION ViewDimension;
        union 
            {
            RHI_BUFFER_RTV Buffer;
            RHI_TEX1D_RTV Texture1D;
            RHI_TEX1D_ARRAY_RTV Texture1DArray;
            RHI_TEX2D_RTV Texture2D;
            RHI_TEX2D_ARRAY_RTV Texture2DArray;
            RHI_TEX2DMS_RTV Texture2DMS;
            RHI_TEX2DMS_ARRAY_RTV Texture2DMSArray;
            RHI_TEX3D_RTV Texture3D;
            } 	;
        } 	RHI_RENDER_TARGET_VIEW_DESC;

    typedef struct RHI_TEX1D_DSV
        {
        UINT MipSlice;
        } 	RHI_TEX1D_DSV;

    typedef struct RHI_TEX1D_ARRAY_DSV
        {
        UINT MipSlice;
        UINT FirstArraySlice;
        UINT ArraySize;
        } 	RHI_TEX1D_ARRAY_DSV;

    typedef struct RHI_TEX2D_DSV
        {
        UINT MipSlice;
        } 	RHI_TEX2D_DSV;

    typedef struct RHI_TEX2D_ARRAY_DSV
        {
        UINT MipSlice;
        UINT FirstArraySlice;
        UINT ArraySize;
        } 	RHI_TEX2D_ARRAY_DSV;

    typedef struct RHI_TEX2DMS_DSV
        {
        UINT UnusedField_NothingToDefine;
        } 	RHI_TEX2DMS_DSV;

    typedef struct RHI_TEX2DMS_ARRAY_DSV
        {
        UINT FirstArraySlice;
        UINT ArraySize;
        } 	RHI_TEX2DMS_ARRAY_DSV;

    typedef 
    enum RHI_DSV_FLAGS
        {
            RHI_DSV_FLAG_NONE	= 0,
            RHI_DSV_FLAG_READ_ONLY_DEPTH	= 0x1,
            RHI_DSV_FLAG_READ_ONLY_STENCIL	= 0x2
        } 	RHI_DSV_FLAGS;

    DEFINE_RHI_ENUM_FLAG_OPERATORS( RHI_DSV_FLAGS );
    typedef 
    enum RHI_DSV_DIMENSION
        {
            RHI_DSV_DIMENSION_UNKNOWN	= 0,
            RHI_DSV_DIMENSION_TEXTURE1D	= 1,
            RHI_DSV_DIMENSION_TEXTURE1DARRAY	= 2,
            RHI_DSV_DIMENSION_TEXTURE2D	= 3,
            RHI_DSV_DIMENSION_TEXTURE2DARRAY	= 4,
            RHI_DSV_DIMENSION_TEXTURE2DMS	= 5,
            RHI_DSV_DIMENSION_TEXTURE2DMSARRAY	= 6
        } 	RHI_DSV_DIMENSION;

    typedef struct RHI_DEPTH_STENCIL_VIEW_DESC
        {
        RHI_FORMAT Format;
        RHI_DSV_DIMENSION ViewDimension;
        RHI_DSV_FLAGS Flags;
        union 
            {
            RHI_TEX1D_DSV Texture1D;
            RHI_TEX1D_ARRAY_DSV Texture1DArray;
            RHI_TEX2D_DSV Texture2D;
            RHI_TEX2D_ARRAY_DSV Texture2DArray;
            RHI_TEX2DMS_DSV Texture2DMS;
            RHI_TEX2DMS_ARRAY_DSV Texture2DMSArray;
            } 	;
        } 	RHI_DEPTH_STENCIL_VIEW_DESC;
























}
