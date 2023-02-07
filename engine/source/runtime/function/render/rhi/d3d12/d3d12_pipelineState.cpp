#include "d3d12_pipelineState.h"
#include "d3d12_pipelineLibrary.h"
#include "d3d12_device.h"
#include "d3d12_rootSignature.h"
#include "rhi_d3d12_mappings.h"
#include "../shader_compiler.h"

#include <unordered_set>
#include "runtime/core/base/macro.h"
#include "runtime/platform/system/stopwatch.h"

namespace RHI
{
    void D3D12PipelineParserCallbacks::RootSignatureCb(D3D12RootSignature* RootSignature)
    {
        this->RootSignature = RootSignature;
    }

	void D3D12PipelineParserCallbacks::InputLayoutCb(D3D12InputLayout* InputLayout)
    {
        InputElements = InputLayout->InputElements;
    }

    void D3D12PipelineParserCallbacks::VSCb(Shader* VS)
    {
        Type     = RHI_PIPELINE_STATE_TYPE::Graphics;
        this->VS = VS;
    }

    void D3D12PipelineParserCallbacks::PSCb(Shader* PS)
    {
        Type     = RHI_PIPELINE_STATE_TYPE::Graphics;
        this->PS = PS;
    }

    void D3D12PipelineParserCallbacks::DSCb(Shader* DS)
    {
        Type     = RHI_PIPELINE_STATE_TYPE::Graphics;
        this->DS = DS;
    }

    void D3D12PipelineParserCallbacks::HSCb(Shader* HS)
    {
        Type     = RHI_PIPELINE_STATE_TYPE::Graphics;
        this->HS = HS;
    }

    void D3D12PipelineParserCallbacks::GSCb(Shader* GS)
    {
        Type     = RHI_PIPELINE_STATE_TYPE::Graphics;
        this->GS = GS;
    }

    void D3D12PipelineParserCallbacks::CSCb(Shader* CS)
    {
        Type     = RHI_PIPELINE_STATE_TYPE::Compute;
        this->CS = CS;
    }

    void D3D12PipelineParserCallbacks::ASCb(Shader* AS)
    {
        Type     = RHI_PIPELINE_STATE_TYPE::Graphics;
        this->AS = AS;
    }

    void D3D12PipelineParserCallbacks::MSCb(Shader* MS)
    {
        Type     = RHI_PIPELINE_STATE_TYPE::Graphics;
        this->MS = MS;
    }

    void D3D12PipelineParserCallbacks::BlendStateCb(const RHIBlendState& BlendState)
    {
        Type             = RHI_PIPELINE_STATE_TYPE::Graphics;
        this->BlendState = BlendState;
    }

    void D3D12PipelineParserCallbacks::RasterizerStateCb(const RHIRasterizerState& RasterizerState)
    {
        Type                  = RHI_PIPELINE_STATE_TYPE::Graphics;
        this->RasterizerState = RasterizerState;
    }

    void D3D12PipelineParserCallbacks::DepthStencilStateCb(const RHIDepthStencilState& DepthStencilState)
    {
        Type                    = RHI_PIPELINE_STATE_TYPE::Graphics;
        this->DepthStencilState = DepthStencilState;
    }

    void D3D12PipelineParserCallbacks::RenderTargetStateCb(const RHIRenderTargetState& RenderTargetState)
    {
        Type                    = RHI_PIPELINE_STATE_TYPE::Graphics;
        this->RenderTargetState = RenderTargetState;
    }

    void D3D12PipelineParserCallbacks::SampleStateCb(const RHISampleState& SampleState)
    {
        Type                    = RHI_PIPELINE_STATE_TYPE::Graphics;
        this->SampleState = SampleState;
    }

    void D3D12PipelineParserCallbacks::PrimitiveTopologyTypeCb(RHI_PRIMITIVE_TOPOLOGY PrimitiveTopology)
    {
        Type                    = RHI_PIPELINE_STATE_TYPE::Graphics;
        this->PrimitiveTopology = PrimitiveTopology;
    }

    void D3D12PipelineParserCallbacks::ErrorBadInputParameter(size_t Index)
    {
        LOG_ERROR("Bad input parameter at {}", Index);
    }

    void D3D12PipelineParserCallbacks::ErrorDuplicateSubobject(RHI_PIPELINE_STATE_SUBOBJECT_TYPE Type)
    {
        LOG_ERROR("Duplicate subobject {}", GetRHIPipelineStateSubobjectTypeString(Type));
    }

    void D3D12PipelineParserCallbacks::ErrorUnknownSubobject(size_t Index)
    {
        LOG_ERROR("Unknown subobject at {}", Index);
    }

    D3D12_SHADER_BYTECODE RHITranslateD3D12(Shader* Shader)
    {
        if (Shader)
        {
            return {Shader->GetPointer(), Shader->GetSize()};
        }
        return {};
    }

    /*
    static VOID NTAPI CompileFromCoroutineHandle(_Inout_ PTP_CALLBACK_INSTANCE Instance,
                                                 _Inout_opt_ PVOID             Context,
                                                 _Inout_ PTP_WORK              Work) noexcept
    {
        UNREFERENCED_PARAMETER(Instance);
        std::coroutine_handle<>::from_address(Context)();
        CloseThreadpoolWork(Work);
    }

    struct awaitable_CompilePsoThread
    {
        constexpr bool await_ready() const noexcept { return false; }

        constexpr void await_resume() const noexcept {}

        void await_suspend(std::coroutine_handle<> handle) const
        {
            Device->GetPsoCompilationThreadPool()->QueueThreadpoolWork(CompileFromCoroutineHandle, handle.address());
        }

        D3D12Device* Device;
    };
    */

    D3D12PipelineState::D3D12PipelineState(D3D12Device*                   Parent,
                                           std::wstring                   Name,
                                           const PipelineStateStreamDesc& Desc) :
        D3D12DeviceChild(Parent)
    {
        D3D12PipelineParserCallbacks Parser;
        RHIParsePipelineStream(Desc, &Parser);
        //CompilationWork = Create(Parent, Name, Parser.Type, Parser);
        PipelineState = Create(Parent, Name, Parser.Type, Parser);
    }

    D3D12PipelineState::D3D12PipelineState(D3D12Device*                       Parent,
                                           std::wstring                       Name,
                                           D3D12_GRAPHICS_PIPELINE_STATE_DESC Desc) :
        D3D12DeviceChild(Parent)
    {
        PipelineState = Compile<D3D12_GRAPHICS_PIPELINE_STATE_DESC>(Parent, Name, Desc);
    }

    //D3D12PipelineState::D3D12PipelineState(D3D12Device*                           Parent,
    //                                       std::wstring                           Name,
    //                                       D3DX12_MESH_SHADER_PIPELINE_STATE_DESC Desc) :
    //    D3D12DeviceChild(Parent)
    //{
    //    PipelineState = Compile<D3DX12_MESH_SHADER_PIPELINE_STATE_DESC>(Parent, Name, Desc);
    //}

    D3D12PipelineState::D3D12PipelineState(D3D12Device*                      Parent,
                                           std::wstring                      Name,
                                           D3D12_COMPUTE_PIPELINE_STATE_DESC Desc) :
        D3D12DeviceChild(Parent)
    {
        PipelineState = Compile<D3D12_COMPUTE_PIPELINE_STATE_DESC>(Parent, Name, Desc);
    }

    ID3D12PipelineState* D3D12PipelineState::GetApiHandle() const noexcept
    {
        //if (CompilationWork)
        //{
        //    CompilationWork.Wait();
        //    PipelineState   = CompilationWork.Get();
        //    CompilationWork = nullptr;
        //}
        return PipelineState.Get();
    }

    Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12PipelineState::Create(D3D12Device*                 Device,
                                                                           std::wstring                 Name,
                                                                           RHI_PIPELINE_STATE_TYPE      Type,
                                                                           D3D12PipelineParserCallbacks Parser)
    {
        // if (Device->AllowAsyncPsoCompilation())
        //{
        //     co_await awaitable_CompilePsoThread {Device};
        // }
        // else
        //{
        //     co_await std::suspend_never {};
        // }

        if (Type == RHI_PIPELINE_STATE_TYPE::Graphics)
        {
            if (!Parser.MS)
            {
                D3D12_GRAPHICS_PIPELINE_STATE_DESC Desc = {};
                Desc.pRootSignature                     = Parser.RootSignature->GetApiHandle();
                Desc.VS                                 = RHITranslateD3D12(Parser.VS);
                Desc.PS                                 = RHITranslateD3D12(Parser.PS);
                Desc.DS                                 = RHITranslateD3D12(Parser.DS);
                Desc.HS                                 = RHITranslateD3D12(Parser.HS);
                Desc.GS                                 = RHITranslateD3D12(Parser.GS);
                Desc.StreamOutput                       = D3D12_STREAM_OUTPUT_DESC();
                Desc.BlendState                         = RHITranslateD3D12(Parser.BlendState);
                Desc.SampleMask                         = DefaultSampleMask();
                Desc.RasterizerState                    = RHITranslateD3D12(Parser.RasterizerState);
                Desc.DepthStencilState                  = RHITranslateD3D12(Parser.DepthStencilState);
                Desc.InputLayout     = {Parser.InputElements.data(), static_cast<UINT>(Parser.InputElements.size())};
                Desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
                Desc.PrimitiveTopologyType = RHITranslateD3D12(Parser.PrimitiveTopology);
                Desc.NumRenderTargets      = Parser.RenderTargetState.NumRenderTargets;
                Desc.DSVFormat             = Parser.RenderTargetState.DSFormat;
                Desc.SampleDesc            = RHITranslateD3D12(Parser.SampleState); // DefaultSampleDesc();
                Desc.NodeMask              = Device->GetAllNodeMask();
                Desc.CachedPSO             = D3D12_CACHED_PIPELINE_STATE();
                Desc.Flags                 = D3D12_PIPELINE_STATE_FLAG_NONE;

                memcpy(Desc.RTVFormats, Parser.RenderTargetState.RTFormats, sizeof(Desc.RTVFormats));
                return Compile<D3D12_GRAPHICS_PIPELINE_STATE_DESC>(Device, Name, Desc);
            }
            else
            {
                D3DX12_MESH_SHADER_PIPELINE_STATE_DESC Desc = {};
                Desc.pRootSignature                         = Parser.RootSignature->GetApiHandle();
                Desc.MS                                     = RHITranslateD3D12(Parser.MS);
                Desc.PS                                     = RHITranslateD3D12(Parser.PS);
                Desc.BlendState = RHITranslateD3D12(Parser.BlendState), Desc.SampleMask = DefaultSampleMask();
                Desc.RasterizerState       = RHITranslateD3D12(Parser.RasterizerState);
                Desc.DepthStencilState     = RHITranslateD3D12(Parser.DepthStencilState);
                Desc.PrimitiveTopologyType = RHITranslateD3D12(Parser.PrimitiveTopology);
                Desc.NumRenderTargets      = Parser.RenderTargetState.NumRenderTargets;
                Desc.DSVFormat = Parser.RenderTargetState.DSFormat, Desc.SampleDesc = DefaultSampleDesc();
                Desc.SampleDesc = RHITranslateD3D12(Parser.SampleState);
                Desc.NodeMask = Device->GetAllNodeMask(), Desc.CachedPSO = D3D12_CACHED_PIPELINE_STATE();
                Desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

                memcpy(Desc.RTVFormats, Parser.RenderTargetState.RTFormats, sizeof(Desc.RTVFormats));

                CD3DX12_PIPELINE_STATE_STREAM2   Stream(Desc);
                D3D12_PIPELINE_STATE_STREAM_DESC StreamDesc = {sizeof(Stream), &Stream};
                return Compile<D3D12_PIPELINE_STATE_STREAM_DESC>(Device, Name, StreamDesc);
            }
        }
        else if (Type == RHI_PIPELINE_STATE_TYPE::Compute)
        {
            D3D12_COMPUTE_PIPELINE_STATE_DESC Desc = {};
            Desc.pRootSignature                    = Parser.RootSignature->GetApiHandle();
            Desc.CS                                = RHITranslateD3D12(Parser.CS);
            Desc.NodeMask                          = Device->GetAllNodeMask();
            Desc.CachedPSO                         = D3D12_CACHED_PIPELINE_STATE();
            Desc.Flags                             = D3D12_PIPELINE_STATE_FLAG_NONE;

            return Compile<D3D12_COMPUTE_PIPELINE_STATE_DESC>(Device, Name, Desc);
        }

        return nullptr;
    }

    template<typename TDesc>
    Microsoft::WRL::ComPtr<ID3D12PipelineState>
    D3D12PipelineState::Compile(D3D12Device* Device, const std::wstring& Name, TDesc& Desc)
    {
        ScopedTimer Timer([&](std::int64_t Milliseconds) {
            LOG_INFO(
                L"Thread: {} has finished compiling PSO: {} in {}ms", GetCurrentThreadId(), Name.c_str(), Milliseconds);
        });

        ID3D12Device5* Device5 = Device->GetD3D12Device5();

        Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState;

        if (Device->GetPipelineLibrary())
        {
            ID3D12PipelineLibrary1* PipelineLibrary1 = Device->GetPipelineLibrary()->GetLibrary1();
            HRESULT Result = (PipelineLibrary1->*D3D12PipelineStateTraits<TDesc>::Load())(
                Name.data(), &Desc, IID_PPV_ARGS(&PipelineState));
            if (Result == E_INVALIDARG)
            {
                VERIFY_D3D12_API(
                    (Device5->*D3D12PipelineStateTraits<TDesc>::Create())(&Desc, IID_PPV_ARGS(&PipelineState)));
                StorePipeline(Device, Name, PipelineState.Get());
            }
        }
        else
        {
            VERIFY_D3D12_API(
                (Device5->*D3D12PipelineStateTraits<TDesc>::Create())(&Desc, IID_PPV_ARGS(&PipelineState)));
        }
        return PipelineState;
    }

    void
    D3D12PipelineState::StorePipeline(D3D12Device* Device, const std::wstring& Name, ID3D12PipelineState* PipelineState)
    {
        HRESULT Result = Device->GetPipelineLibrary()->GetLibrary1()->StorePipeline(Name.data(), PipelineState);
        if (Result == E_INVALIDARG)
        {
            // A PSO with the specified name already exists in the library.
            // If that is the case, we invalidate disk cache, so when next time app
            // starts, pipeline library will be renewed with the updated PSOs.
            Device->GetPipelineLibrary()->InvalidateDiskCache();
        }
        else
        {
            VERIFY_D3D12_API(Result);
        }
    }

    RaytracingPipelineStateDesc::RaytracingPipelineStateDesc() noexcept :
        Desc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE), GlobalRootSignature(nullptr), ShaderConfig(),
        PipelineConfig()
    {}

    RaytracingPipelineStateDesc& RaytracingPipelineStateDesc::AddLibrary(const D3D12_SHADER_BYTECODE&          Library,
                                                                         const std::vector<std::wstring_view>& Symbols)
    {
        // Add a DXIL library to the pipeline. The exported symbols must correspond exactly to the
        // names of the shaders declared in the library, although unused ones can be omitted.
        Libraries.emplace_back(DxilLibrary(Library, Symbols));
        return *this;
    }

    RaytracingPipelineStateDesc&
    RaytracingPipelineStateDesc::AddHitGroup(std::wstring_view                HitGroupName,
                                             std::optional<std::wstring_view> AnyHitSymbol,
                                             std::optional<std::wstring_view> ClosestHitSymbol,
                                             std::optional<std::wstring_view> IntersectionSymbol)
    {
        // In DXR the hit-related shaders are grouped into hit groups. Such shaders are:
        // - The intersection shader, which can be used to intersect custom geometry, and is called upon
        //   hitting the bounding box the the object. A default one exists to intersect triangles
        // - The any hit shader, called on each intersection, which can be used to perform early
        //   alpha-testing and allow the ray to continue if needed. Default is a pass-through.
        // - The closest hit shader, invoked on the hit point closest to the ray start.
        // The shaders in a hit group share the same root signature, and are only referred to by the
        // hit group name in other places of the program.
        HitGroups.emplace_back(HitGroup(HitGroupName, AnyHitSymbol, ClosestHitSymbol, IntersectionSymbol));

        // 3 different shaders can be invoked to obtain an intersection: an
        // intersection shader is called when hitting the bounding box of non-triangular geometry.
        // An any-hit shader is called on potential intersections.
        // This shader can, for example, perform alpha-testing and
        // discard some intersections. Finally, the closest-hit program is invoked on
        // the intersection point closest to the ray origin. Those 3 shaders are bound
        // together into a hit group.

        // Note that for triangular geometry the intersection shader is built-in. An
        // empty any-hit shader is also defined by default, so in our simple case each
        // hit group contains only the closest hit shader. Note that since the
        // exported symbols are defined above the shaders can be simply referred to by
        // name.
        return *this;
    }

    RaytracingPipelineStateDesc&
    RaytracingPipelineStateDesc::AddRootSignatureAssociation(ID3D12RootSignature*                  RootSignature,
                                                             const std::vector<std::wstring_view>& Symbols)
    {
        RootSignatureAssociations.emplace_back(RootSignatureAssociation(RootSignature, Symbols));
        return *this;
    }

    RaytracingPipelineStateDesc&
    RaytracingPipelineStateDesc::SetGlobalRootSignature(ID3D12RootSignature* GlobalRootSignature)
    {
        this->GlobalRootSignature = GlobalRootSignature;
        return *this;
    }

    RaytracingPipelineStateDesc& RaytracingPipelineStateDesc::SetRaytracingShaderConfig(UINT MaxPayloadSizeInBytes,
                                                                                        UINT MaxAttributeSizeInBytes)
    {
        ShaderConfig.MaxPayloadSizeInBytes   = MaxPayloadSizeInBytes;
        ShaderConfig.MaxAttributeSizeInBytes = MaxAttributeSizeInBytes;
        return *this;
    }

    RaytracingPipelineStateDesc& RaytracingPipelineStateDesc::SetRaytracingPipelineConfig(UINT MaxTraceRecursionDepth)
    {
        PipelineConfig.MaxTraceRecursionDepth = MaxTraceRecursionDepth;
        return *this;
    }

    D3D12_STATE_OBJECT_DESC RaytracingPipelineStateDesc::Build()
    {
        // Add all the DXIL libraries
        for (const auto& [Library, Symbols] : Libraries)
        {
            auto DxilLibrarySubobject = Desc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            DxilLibrarySubobject->SetDXILLibrary(&Library);
            for (const auto& Symbol : Symbols)
            {
                DxilLibrarySubobject->DefineExport(Symbol.data());
            }
        }

        // Add all the hit group declarations
        for (const auto& [HitGroupName, AnyHit, ClosestHit, Intersection] : HitGroups)
        {
            auto HitGroupSubobject = Desc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            HitGroupSubobject->SetHitGroupExport(HitGroupName.data());
            HitGroupSubobject->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

            HitGroupSubobject->SetAnyHitShaderImport(!AnyHit.empty() ? AnyHit.data() : nullptr);
            HitGroupSubobject->SetClosestHitShaderImport(!ClosestHit.empty() ? ClosestHit.data() : nullptr);
            HitGroupSubobject->SetIntersectionShaderImport(!Intersection.empty() ? Intersection.data() : nullptr);
        }

        // Add 2 subobjects for every root signature association
        for (const auto& [RootSignature, Symbols] : RootSignatureAssociations)
        {
            // The root signature association requires two objects for each: one to declare the root
            // signature, and another to associate that root signature to a set of symbols

            // Add a subobject to declare the local root signature
            auto LocalRootSignatureSubobject = Desc.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            LocalRootSignatureSubobject->SetRootSignature(RootSignature);

            // Add a subobject for the association between the exported shader symbols and the local root signature
            auto SubobjectToExportsAssociationSubobject =
                Desc.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            for (const auto& Symbol : Symbols)
            {
                SubobjectToExportsAssociationSubobject->AddExport(Symbol.data());
            }
            SubobjectToExportsAssociationSubobject->SetSubobjectToAssociate(*LocalRootSignatureSubobject);
        }

        // Add a subobject for global root signature
        auto GlobalRootSignatureSubobject = Desc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
        GlobalRootSignatureSubobject->SetRootSignature(GlobalRootSignature);

        // Add a subobject for the raytracing shader config
        auto RaytracingShaderConfigSubobject = Desc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
        RaytracingShaderConfigSubobject->Config(ShaderConfig.MaxPayloadSizeInBytes,
                                                ShaderConfig.MaxAttributeSizeInBytes);

        // Add a subobject for the association between shaders and the payload
        std::vector<std::wstring_view> ExportedSymbols = BuildShaderExportList();
        auto                           SubobjectToExportsAssociationSubobject =
            Desc.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
        for (const auto& Symbol : ExportedSymbols)
        {
            SubobjectToExportsAssociationSubobject->AddExport(Symbol.data());
        }
        SubobjectToExportsAssociationSubobject->SetSubobjectToAssociate(*RaytracingShaderConfigSubobject);

        // Add a subobject for the raytracing pipeline configuration
        auto RaytracingPipelineConfigSubobject = Desc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
        RaytracingPipelineConfigSubobject->Config(PipelineConfig.MaxTraceRecursionDepth);

        return Desc;
    }

    std::vector<std::wstring_view> RaytracingPipelineStateDesc::BuildShaderExportList() const
    {
        // Get all names from libraries
        // Get names associated to hit groups
        // Return list of libraries + hit group names - shaders in hit groups
        std::unordered_set<std::wstring_view> Exports;

        // Add all the symbols exported by the libraries
        for (const auto& Library : Libraries)
        {
            for (const auto& Symbol : Library.Symbols)
            {
#ifdef _DEBUG
                // Sanity check in debug mode: check that no name is exported more than once
                if (Exports.find(Symbol) != Exports.end())
                {
                    throw std::logic_error("Multiple definition of a symbol in the imported DXIL libraries");
                }
#endif
                Exports.insert(Symbol);
            }
        }

#ifdef _DEBUG
        // Sanity check in debug mode: verify that the hit groups do not reference an unknown shader name
        {
            std::unordered_set<std::wstring_view> AllExports = Exports;

            for (const auto& HitGroup : HitGroups)
            {
                if (!HitGroup.AnyHitSymbol.empty() && !(Exports.find(HitGroup.AnyHitSymbol) != Exports.end()))
                {
                    throw std::logic_error("Any hit symbol not found in the imported DXIL libraries");
                }

                if (!HitGroup.ClosestHitSymbol.empty() &&
                    !(Exports.find(HitGroup.ClosestHitSymbol) != Exports.end()))
                {
                    throw std::logic_error("Closest hit symbol not found in the imported DXIL libraries");
                }

                if (!HitGroup.IntersectionSymbol.empty() &&
                    !(Exports.find(HitGroup.IntersectionSymbol) != Exports.end()))
                {
                    throw std::logic_error("Intersection symbol not found in the imported DXIL libraries");
                }

                AllExports.insert(HitGroup.HitGroupName);
            }

            // Sanity check in debug mode: verify that the root signature associations do not reference an
            // unknown shader or hit group name
            for (const auto& RootSignatureAssociation : RootSignatureAssociations)
            {
                for (const auto& Symbol : RootSignatureAssociation.Symbols)
                {
                    if (!Symbol.empty() && !(AllExports.find(Symbol) != AllExports.end()))
                    {
                        throw std::logic_error(
                            "Root association symbol not found in the imported DXIL libraries and hit group names");
                    }
                }
            }
        }
#endif

        // Go through all hit groups and remove the symbols corresponding to intersection, any hit and
        // closest hit shaders from the symbol set
        for (const auto& HitGroup : HitGroups)
        {
            if (!HitGroup.ClosestHitSymbol.empty())
            {
                Exports.erase(HitGroup.ClosestHitSymbol);
            }

            if (!HitGroup.AnyHitSymbol.empty())
            {
                Exports.erase(HitGroup.AnyHitSymbol);
            }

            if (!HitGroup.IntersectionSymbol.empty())
            {
                Exports.erase(HitGroup.IntersectionSymbol);
            }

            Exports.insert(HitGroup.HitGroupName);
        }

        return {Exports.begin(), Exports.end()};
    }

    D3D12RaytracingPipelineState::D3D12RaytracingPipelineState(D3D12Device* Parent, RaytracingPipelineStateDesc& Desc) :
        D3D12DeviceChild(Parent)
    {
        D3D12_STATE_OBJECT_DESC ApiDesc = Desc.Build();
        VERIFY_D3D12_API(Parent->GetD3D12Device5()->CreateStateObject(&ApiDesc, IID_PPV_ARGS(&StateObject)));
        VERIFY_D3D12_API(StateObject->QueryInterface(IID_PPV_ARGS(&StateObjectProperties)));
    }

    void* D3D12RaytracingPipelineState::GetShaderIdentifier(std::wstring_view ExportName) const
    {
        void* ShaderIdentifier = StateObjectProperties->GetShaderIdentifier(ExportName.data());
        assert(ShaderIdentifier && "Invalid ExportName");
        return ShaderIdentifier;
    }
}
