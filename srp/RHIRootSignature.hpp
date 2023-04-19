#include "RHICore.hpp"

namespace RHI
{

    class RHIDescriptorTable
    {
    public:
        explicit RHIDescriptorTable(size_t NumRanges) { DescriptorRanges.reserve(NumRanges); }

        template<UINT BaseShaderRegister, UINT RegisterSpace>
        RHIDescriptorTable& AddSRVRange(UINT NumDescriptors, RHI_DESCRIPTOR_RANGE_FLAGS Flags, UINT OffsetInDescriptorsFromTableStart = RHI_DESCRIPTOR_RANGE_OFFSET_APPEND)
        {
            return AddDescriptorRange<BaseShaderRegister, RegisterSpace>(RHI_DESCRIPTOR_RANGE_TYPE_SRV, NumDescriptors, Flags, OffsetInDescriptorsFromTableStart);
        }

        template<UINT BaseShaderRegister, UINT RegisterSpace>
        RHIDescriptorTable& AddUAVRange(UINT NumDescriptors, RHI_DESCRIPTOR_RANGE_FLAGS Flags, UINT OffsetInDescriptorsFromTableStart = RHI_DESCRIPTOR_RANGE_OFFSET_APPEND)
        {
            return AddDescriptorRange<BaseShaderRegister, RegisterSpace>(RHI_DESCRIPTOR_RANGE_TYPE_UAV, NumDescriptors, Flags, OffsetInDescriptorsFromTableStart);
        }

        template<UINT BaseShaderRegister, UINT RegisterSpace>
        RHIDescriptorTable& AddCBVRange(UINT NumDescriptors, RHI_DESCRIPTOR_RANGE_FLAGS Flags, UINT OffsetInDescriptorsFromTableStart = RHI_DESCRIPTOR_RANGE_OFFSET_APPEND)
        {
            return AddDescriptorRange<BaseShaderRegister, RegisterSpace>(RHI_DESCRIPTOR_RANGE_TYPE_CBV, NumDescriptors, Flags, OffsetInDescriptorsFromTableStart);
        }

        template<UINT BaseShaderRegister, UINT RegisterSpace>
        RHIDescriptorTable&
        AddSamplerRange(UINT NumDescriptors, RHI_DESCRIPTOR_RANGE_FLAGS Flags, UINT OffsetInDescriptorsFromTableStart = RHI_DESCRIPTOR_RANGE_OFFSET_APPEND)
        {
            return AddDescriptorRange<BaseShaderRegister, RegisterSpace>(RHI_DESCRIPTOR_RANGE_TYPE_SAMPLER, NumDescriptors, Flags, OffsetInDescriptorsFromTableStart);
        }

        template<UINT BaseShaderRegister, UINT RegisterSpace>
        RHIDescriptorTable& AddDescriptorRange(RHI_DESCRIPTOR_RANGE_TYPE Type, UINT NumDescriptors, RHI_DESCRIPTOR_RANGE_FLAGS Flags, UINT OffsetInDescriptorsFromTableStart)
        {
            CD3DX12_DESCRIPTOR_RANGE1& Range = DescriptorRanges.emplace_back();
            Range.Init(Type, NumDescriptors, BaseShaderRegister, RegisterSpace, Flags, OffsetInDescriptorsFromTableStart);
            return *this;
        }

        UINT                        size() const noexcept { return static_cast<UINT>(DescriptorRanges.size()); }
        const RHI_DESCRIPTOR_RANGE* data() const noexcept { return DescriptorRanges.data(); }

    private:
        std::vector<RHI_DESCRIPTOR_RANGE> DescriptorRanges;
    };

    class RHIRootSignatureDesc
    {
    public:
        RHIRootSignatureDesc();

        RHIRootSignatureDesc& AddDescriptorTable(const RHIDescriptorTable& DescriptorTable);

        template<UINT ShaderRegister, UINT RegisterSpace, typename T>
        RHIRootSignatureDesc& Add32BitConstants()
        {
            static_assert(sizeof(T) % 4 == 0);
            RHI_ROOT_PARAMETER Parameter = {};
            Parameter.InitAsConstants(sizeof(T) / 4, ShaderRegister, RegisterSpace);
            return AddParameter(Parameter);
        }

        template<UINT ShaderRegister, UINT RegisterSpace>
        RHIRootSignatureDesc& Add32BitConstants(UINT Num32BitValues)
        {
            CD3DX12_ROOT_PARAMETER1 Parameter = {};
            Parameter.InitAsConstants(Num32BitValues, ShaderRegister, RegisterSpace);
            return AddParameter(Parameter);
        }

        template<UINT ShaderRegister, UINT RegisterSpace>
        RHIRootSignatureDesc& AddConstantBufferView(RHI_ROOT_DESCRIPTOR_FLAGS Flags = RHI_ROOT_DESCRIPTOR_FLAG_NONE)
        {
            CD3DX12_ROOT_PARAMETER1 Parameter = {};
            Parameter.InitAsConstantBufferView(ShaderRegister, RegisterSpace, Flags);
            return AddParameter(Parameter);
        }

        template<UINT ShaderRegister, UINT RegisterSpace>
        RHIRootSignatureDesc& AddShaderResourceView(RHI_ROOT_DESCRIPTOR_FLAGS Flags = RHI_ROOT_DESCRIPTOR_FLAG_NONE)
        {
            CD3DX12_ROOT_PARAMETER1 Parameter = {};
            Parameter.InitAsShaderResourceView(ShaderRegister, RegisterSpace, Flags);
            return AddParameter(Parameter);
        }

        template<UINT ShaderRegister, UINT RegisterSpace>
        RHIRootSignatureDesc& AddUnorderedAccessView(RHI_ROOT_DESCRIPTOR_FLAGS Flags = RHI_ROOT_DESCRIPTOR_FLAG_NONE)
        {
            CD3DX12_ROOT_PARAMETER1 Parameter = {};
            Parameter.InitAsUnorderedAccessView(ShaderRegister, RegisterSpace, Flags);
            return AddParameter(Parameter);
        }

        template<UINT ShaderRegister, UINT RegisterSpace>
        RHIRootSignatureDesc& AddUnorderedAccessViewWithCounter(RHI_DESCRIPTOR_RANGE_FLAGS Flags = RHI_DESCRIPTOR_RANGE_FLAG_NONE)
        {
            RHIDescriptorTable DescriptorTable(1);
            DescriptorTable.AddUAVRange<ShaderRegister, RegisterSpace>(1, Flags);
            return AddDescriptorTable(DescriptorTable);
        }

        template<UINT ShaderRegister, UINT RegisterSpace>
        RHIRootSignatureDesc& AddRaytracingAccelerationStructure(RHI_DESCRIPTOR_RANGE_FLAGS Flags = RHI_DESCRIPTOR_RANGE_FLAG_NONE)
        {
            RHIDescriptorTable DescriptorTable(1);
            DescriptorTable.AddSRVRange<ShaderRegister, RegisterSpace>(1, Flags);
            return AddDescriptorTable(DescriptorTable);
        }

        template<UINT ShaderRegister, UINT RegisterSpace>
        RHIRootSignatureDesc& AddStaticSampler(RHI_STATIC_SAMPLER_DESC staticSampler)
        {
            staticSampler.ShaderRegister = ShaderRegister;
            staticSampler.RegisterSpace  = RegisterSpace;
            StaticSamplers.push_back(staticSampler);
            return *this;
        }

        template<UINT ShaderRegister, UINT RegisterSpace>
        RHIRootSignatureDesc&
        AddStaticSampler(RHI_FILTER               filter           = RHI_FILTER_ANISOTROPIC,
                         RHI_TEXTURE_ADDRESS_MODE addressU         = RHI_TEXTURE_ADDRESS_MODE_WRAP,
                         RHI_TEXTURE_ADDRESS_MODE addressV         = RHI_TEXTURE_ADDRESS_MODE_WRAP,
                         RHI_TEXTURE_ADDRESS_MODE addressW         = RHI_TEXTURE_ADDRESS_MODE_WRAP,
                         FLOAT                      mipLODBias       = 0,
                         UINT                       maxAnisotropy    = 16,
                         RHI_COMPARISON_FUNC      comparisonFunc   = RHI_COMPARISON_FUNC_LESS_EQUAL,
                         RHI_STATIC_BORDER_COLOR  borderColor      = RHI_STATIC_BORDER_COLOR_OPAQUE_WHITE,
                         FLOAT                      minLOD           = 0.f,
                         FLOAT                      maxLOD           = RHI_FLOAT32_MAX,
                         RHI_SHADER_VISIBILITY    shaderVisibility = RHI_SHADER_VISIBILITY_ALL)
        {
            CD3DX12_STATIC_SAMPLER_DESC& Desc = StaticSamplers.emplace_back();
            Desc.Init(ShaderRegister,
                      filter,
                      addressU,
                      addressV,
                      addressW,
                      mipLODBias,
                      maxAnisotropy,
                      comparisonFunc,
                      borderColor,
                      minLOD,
                      maxLOD,
                      shaderVisibility,
                      RegisterSpace);
            return *this;
        }

        template<UINT ShaderRegister, UINT RegisterSpace>
        RHIRootSignatureDesc& AddStaticSampler(const RHI_SAMPLER_DESC& NonStaticSamplerDesc,
                                            RHI_SHADER_VISIBILITY   Visibility = RHI_SHADER_VISIBILITY_ALL)
        {
            CD3DX12_STATIC_SAMPLER_DESC& Desc = StaticSamplers.emplace_back();
            Desc.Filter                       = NonStaticSamplerDesc.Filter;
            Desc.AddressU                     = NonStaticSamplerDesc.AddressU;
            Desc.AddressV                     = NonStaticSamplerDesc.AddressV;
            Desc.AddressW                     = NonStaticSamplerDesc.AddressW;
            Desc.MipLODBias                   = NonStaticSamplerDesc.MipLODBias;
            Desc.MaxAnisotropy                = NonStaticSamplerDesc.MaxAnisotropy;
            Desc.ComparisonFunc               = NonStaticSamplerDesc.ComparisonFunc;
            Desc.BorderColor                  = RHI_STATIC_BORDER_COLOR_OPAQUE_WHITE;
            Desc.MinLOD                       = NonStaticSamplerDesc.MinLOD;
            Desc.MaxLOD                       = NonStaticSamplerDesc.MaxLOD;
            Desc.ShaderRegister               = ShaderRegister;
            Desc.RegisterSpace                = RegisterSpace;
            Desc.ShaderVisibility             = Visibility;

            if (Desc.AddressU == RHI_TEXTURE_ADDRESS_MODE_BORDER ||
                Desc.AddressV == RHI_TEXTURE_ADDRESS_MODE_BORDER ||
                Desc.AddressW == RHI_TEXTURE_ADDRESS_MODE_BORDER)
            {
                ASSERT(
                    // Transparent Black
                    NonStaticSamplerDesc.BorderColor[0] == 0.0f && NonStaticSamplerDesc.BorderColor[1] == 0.0f &&
                        NonStaticSamplerDesc.BorderColor[2] == 0.0f && NonStaticSamplerDesc.BorderColor[3] == 0.0f ||
                    // Opaque Black
                    NonStaticSamplerDesc.BorderColor[0] == 0.0f && NonStaticSamplerDesc.BorderColor[1] == 0.0f &&
                        NonStaticSamplerDesc.BorderColor[2] == 0.0f && NonStaticSamplerDesc.BorderColor[3] == 1.0f ||
                    // Opaque White
                    NonStaticSamplerDesc.BorderColor[0] == 1.0f && NonStaticSamplerDesc.BorderColor[1] == 1.0f &&
                        NonStaticSamplerDesc.BorderColor[2] == 1.0f && NonStaticSamplerDesc.BorderColor[3] == 1.0f);

                if (NonStaticSamplerDesc.BorderColor[3] == 1.0f)
                {
                    if (NonStaticSamplerDesc.BorderColor[0] == 1.0f)
                        Desc.BorderColor = RHI_STATIC_BORDER_COLOR_OPAQUE_WHITE;
                    else
                        Desc.BorderColor = RHI_STATIC_BORDER_COLOR_OPAQUE_BLACK;
                }
                else
                    Desc.BorderColor = RHI_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            }
            return *this;
        }

        RHIRootSignatureDesc& AllowInputLayout() noexcept;
        RHIRootSignatureDesc& DenyVSAccess() noexcept;
        RHIRootSignatureDesc& DenyHSAccess() noexcept;
        RHIRootSignatureDesc& DenyDSAccess() noexcept;
        RHIRootSignatureDesc& DenyTessellationShaderAccess() noexcept;
        RHIRootSignatureDesc& DenyGSAccess() noexcept;
        RHIRootSignatureDesc& DenyPSAccess() noexcept;
        RHIRootSignatureDesc& AsLocalRootSignature() noexcept;
        RHIRootSignatureDesc& DenyASAccess() noexcept;
        RHIRootSignatureDesc& DenyMSAccess() noexcept;
        RHIRootSignatureDesc& AllowResourceDescriptorHeapIndexing() noexcept;
        RHIRootSignatureDesc& AllowSampleDescriptorHeapIndexing() noexcept;

        [[nodiscard]] bool IsLocal() const noexcept;

        RHI_ROOT_SIGNATURE_DESC1 Build() noexcept;

    private:
        RHIRootSignatureDesc& AddParameter(RHI_ROOT_PARAMETER1 Parameter);

    private:
        std::vector<CD3DX12_ROOT_PARAMETER1>     Parameters;
        std::vector<CD3DX12_STATIC_SAMPLER_DESC> StaticSamplers;
        RHI_ROOT_SIGNATURE_FLAGS                 Flags = RHI_ROOT_SIGNATURE_FLAG_NONE;

        std::vector<UINT>               DescriptorTableIndices;
        std::vector<RHIDescriptorTable> DescriptorTables;
    };

    class RHIRootSignature
    {
    public:
        RHIRootSignature() noexcept = default;
        explicit RHIRootSignature(RHIRootSignatureDesc& Desc);

        RHIRootSignature(RHIRootSignature&&) noexcept = default;
        RHIRootSignature& operator=(RHIRootSignature&&) noexcept = default;

        RHIRootSignature(const RHIRootSignature&) = delete;
        RHIRootSignature& operator=(const RHIRootSignature&) = delete;

        [[nodiscard]] UINT GetNumParameters() const noexcept { return m_NumParameters; }
        [[nodiscard]] UINT GetNumStaticSamplers() const noexcept { return m_NumStaticSamplers; }

        [[nodiscard]] UINT32 GetDescriptorTableBitMask(RHI_DESCRIPTOR_HEAP_TYPE Type) const noexcept;

        [[nodiscard]] UINT GetDescriptorTableSize(UINT RootParameterIndex) const noexcept;

    //private:
    //    // Pre SM6.6 bindless root parameter setup
    //    static void AddBindlessParameters(RootSignatureDesc& Desc);

    private:
        UINT m_NumParameters = 0;
        UINT m_NumStaticSamplers = 0;
        UINT32 m_DescriptorTableBitMask;  // One bit is set for root parameters that are non-sampler descriptor tables
        UINT32 m_SamplerTableBitMask;     // One bit is set for root parameters that are sampler descriptor tables
        UINT32 m_DescriptorTableSize[16]; // Non-sampler descriptor tables need to know their descriptor count
    };


}