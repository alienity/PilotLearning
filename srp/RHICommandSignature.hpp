#include "RHICore.hpp"

namespace RHI
{
    class RHIRootSignature;

    class RHICommandSignatureDesc
    {
    public:
        explicit RHICommandSignatureDesc(size_t NumParameters)
        {
            Parameters.reserve(NumParameters);
        }

        RHI_COMMAND_SIGNATURE_DESC Build() noexcept;

        void AddDraw()
        {
            RHI_INDIRECT_ARGUMENT_DESC& Desc = Parameters.emplace_back();
            Desc.Type                          = RHI_INDIRECT_ARGUMENT_TYPE_DRAW;
        }

        void AddDrawIndexed()
        {
            RHI_INDIRECT_ARGUMENT_DESC& Desc = Parameters.emplace_back();
            Desc.Type                          = RHI_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
        }

        void AddDispatch()
        {
            RHI_INDIRECT_ARGUMENT_DESC& Desc = Parameters.emplace_back();
            Desc.Type                          = RHI_INDIRECT_ARGUMENT_TYPE_DISPATCH;
        }

        void AddVertexBufferView(UINT Slot)
        {
            RHI_INDIRECT_ARGUMENT_DESC& Desc = Parameters.emplace_back();
            Desc.Type                          = RHI_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
            Desc.VertexBuffer.Slot             = Slot;
        }

        void AddIndexBufferView()
        {
            RHI_INDIRECT_ARGUMENT_DESC& Desc = Parameters.emplace_back();
            Desc.Type                          = RHI_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
        }

        void AddConstant(UINT RootParameterIndex, UINT DestOffsetIn32BitValues, UINT Num32BitValuesToSet)
        {
            RHI_INDIRECT_ARGUMENT_DESC& Desc    = Parameters.emplace_back();
            Desc.Type                             = RHI_INDIRECT_ARGUMENT_TYPE_CONSTANT;
            Desc.Constant.RootParameterIndex      = RootParameterIndex;
            Desc.Constant.DestOffsetIn32BitValues = DestOffsetIn32BitValues;
            Desc.Constant.Num32BitValuesToSet     = Num32BitValuesToSet;
            RequiresRootSignature                 = true;
        }

        void AddConstantBufferView(UINT RootParameterIndex)
        {
            RHI_INDIRECT_ARGUMENT_DESC& Desc         = Parameters.emplace_back();
            Desc.Type                                  = RHI_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
            Desc.ConstantBufferView.RootParameterIndex = RootParameterIndex;
            RequiresRootSignature                      = true;
        }

        void AddShaderResourceView(UINT RootParameterIndex)
        {
            RHI_INDIRECT_ARGUMENT_DESC& Desc         = Parameters.emplace_back();
            Desc.Type                                  = RHI_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
            Desc.ShaderResourceView.RootParameterIndex = RootParameterIndex;
            RequiresRootSignature                      = true;
        }

        void AddUnorderedAccessView(UINT RootParameterIndex)
        {
            RHI_INDIRECT_ARGUMENT_DESC& Desc          = Parameters.emplace_back();
            Desc.Type                                   = RHI_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW;
            Desc.UnorderedAccessView.RootParameterIndex = RootParameterIndex;
            RequiresRootSignature                       = true;
        }

        void AddDispatchRays()
        {
            RHI_INDIRECT_ARGUMENT_DESC& Desc = Parameters.emplace_back();
            Desc.Type                          = RHI_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS;
        }

        void AddDispatchMesh()
        {
            RHI_INDIRECT_ARGUMENT_DESC& Desc = Parameters.emplace_back();
            Desc.Type                          = RHI_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;
        }

    private:
        std::vector<RHI_INDIRECT_ARGUMENT_DESC> Parameters;
        bool                                      RequiresRootSignature = false;

        friend class RHICommandSignature;
    };

    class RHICommandSignature
    {
    public:
        // The root signature must be specified if and only if the command signature changes one of the root arguments.
        RHICommandSignature() noexcept = default;
        RHICommandSignature(RHICommandSignatureDesc& Builder, RHIRootSignature* RootSignature = nullptr);
    };  


}