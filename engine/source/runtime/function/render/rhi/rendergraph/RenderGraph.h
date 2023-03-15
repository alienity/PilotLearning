#pragma once
#include <cassert>
#include <stack>
#include <queue>

#include "DgmlBuilder.h"
#include "RenderGraphRegistry.h"
#include "runtime/platform/system/system_core.h"
#include "runtime/platform/system/delegate.h"
#include "runtime/function/render/rhi/rhi.h"
#include "runtime/function/render/rhi/d3d12/d3d12_core.h"


namespace RHI
{
    class RenderPass;

    enum RgHandleOp
    {
        Read,
        Write
    };

    struct RgHandleOpPassIdx
    {
        RgHandleOp  op;
        RenderPass* passPtr;
    };

    class RenderGraphAllocator
    {
    public:
        explicit RenderGraphAllocator(size_t SizeInBytes) :
            BaseAddress(std::make_unique<std::byte[]>(SizeInBytes)), Ptr(BaseAddress.get()),
            Sentinel(Ptr + SizeInBytes), CurrentMemoryUsage(0)
        {}

        void* Allocate(size_t SizeInBytes, size_t Alignment)
        {
            SizeInBytes = D3D12RHIUtils::AlignUp(SizeInBytes, Alignment);
            assert(Ptr + SizeInBytes <= Sentinel);
            std::byte* Result = Ptr;// +SizeInBytes;

            Ptr += SizeInBytes;
            CurrentMemoryUsage += SizeInBytes;
            return Result;
        }

        template<typename T, typename... TArgs>
        T* Construct(TArgs&&... Args)
        {
            void* Memory = Allocate(sizeof(T), 16);
            return new (Memory) T(std::forward<TArgs>(Args)...);
        }

        template<typename T>
        static void Destruct(T* Ptr)
        {
            Ptr->~T();
        }

        void Reset()
        {
            Ptr                = BaseAddress.get();
            CurrentMemoryUsage = 0;
        }

    private:
        std::unique_ptr<std::byte[]> BaseAddress;
        std::byte*                   Ptr;
        std::byte*                   Sentinel;
        std::size_t                  CurrentMemoryUsage;
    };

    class RenderPass
    {
    public:
        using ExecuteCallback = Delegate<void(RenderGraphRegistry* Registry, D3D12CommandContext* Context)>;

        RenderPass(std::string_view Name, RenderGraph* Graph);

        RenderPass& Read(RgResourceHandle Resource, bool IgnoreBarrier = false);
        RenderPass& Write(RgResourceHandle Resource, bool IgnoreBarrier = false);
        
        template<typename PFNRenderPassCallback>
        void Execute(PFNRenderPassCallback&& Callback)
        {
            this->Callback = std::move(Callback);
        }

        [[nodiscard]] bool HasDependency(RgResourceHandle Resource) const;
        [[nodiscard]] bool WritesTo(RgResourceHandle Resource) const;
        [[nodiscard]] bool ReadsFrom(RgResourceHandle Resource) const;

        [[nodiscard]] bool HasAnyDependencies() const noexcept;

        RenderGraph* ParentGraph;

        std::string_view Name;
        size_t           TopologicalIndex = 0;
        size_t           PassIndex        = 0;

        robin_hood::unordered_set<RgResourceHandle> Reads;
        robin_hood::unordered_set<RgResourceHandle> Writes;
        robin_hood::unordered_set<RgResourceHandle> ReadWrites;

        std::vector<RgResourceHandle> IgnoreReads;
        std::vector<RgResourceHandle> IgnoreWrites;
        
        ExecuteCallback Callback;
    };

    class RenderGraphDependencyLevel
    {
    public:
        void AddRenderPass(RenderPass* RenderPass);

        void Execute(RenderGraph* RenderGraph, D3D12CommandContext* Context);

    private:
        std::vector<RenderPass*> RenderPasses;

        // Apply barriers at a dependency level to reduce redudant barriers
        robin_hood::unordered_set<RgResourceHandle> Reads;
        robin_hood::unordered_set<RgResourceHandle> Writes;

        std::vector<RgResourceHandle> IgnoreReads;
        std::vector<RgResourceHandle> IgnoreWrites;
    };

    class RenderGraph
    {
    public:
        explicit RenderGraph(RenderGraphAllocator& Allocator, RenderGraphRegistry& Registry);
        ~RenderGraph();

        template<typename T>
        auto Import(T* ToBeImported) -> RgResourceHandle
        {
            auto& ImportedContainer = GetImportedContainer<T>();

            RgResourceHandle Handle = {RgResourceTraits<T>::Enum, RG_RESOURCE_FLAG_IMPORTED, 0, ImportedContainer.size()};
            InGraphResHandle.push_back(Handle);

            ImportedContainer.emplace_back(ToBeImported);
            return Handle;
        }


        template<typename T>
        auto Create(const typename RgResourceTraits<T>::Desc& Desc) -> RgResourceHandle
        {
            auto& Container = GetContainer<T>();

            RgResourceHandle Handle = {RgResourceTraits<T>::Enum, RG_RESOURCE_FLAG_NONE, 0, Container.size()};

            InGraphResHandle.push_back(Handle);

            auto& Resource  = Container.emplace_back();
            Resource.Handle = Handle;
            Resource.Desc   = Desc;
            return Handle;
        }

        RenderPass& AddRenderPass(std::string_view Name);

        [[nodiscard]] RenderPass& GetProloguePass() const noexcept;
        [[nodiscard]] RenderPass& GetEpiloguePass() const noexcept;

        [[nodiscard]] RenderGraphRegistry* GetRegistry() const noexcept;

        void Execute(D3D12CommandContext* Context);

        void ExportDgml(DgmlBuilder& Builder) const;

        [[nodiscard]] bool AllowRenderTarget(RgResourceHandle Resource) const noexcept;
        [[nodiscard]] bool AllowDepthStencil(RgResourceHandle Resource) const noexcept;
        [[nodiscard]] bool AllowUnorderedAccess(RgResourceHandle Resource) const noexcept;

    private:
        void Setup();

        //void DepthFirstSearch(size_t n, std::vector<bool>& Visited, std::stack<size_t>& Stack);

        [[nodiscard]] std::string_view GetResourceName(RgResourceHandle Handle) const
        {
            switch (Handle.Type)
            {
                case RgResourceType::Texture:
                    return Textures[Handle.Id].Desc.Name;
                case RgResourceType::Buffer:
                    return Buffers[Handle.Id].Desc.mName;
            }
            return "<unknown>";
        }

        template<typename T>
        [[nodiscard]] auto GetContainer() -> std::vector<typename RgResourceTraits<T>::Type>&
        {
            if constexpr (std::is_same_v<T, D3D12Buffer>)
            {
                return Buffers;
            }
            else if constexpr (std::is_same_v<T, D3D12Texture>)
            {
                return Textures;
            }
        }

        template<typename T>
        [[nodiscard]] auto GetImportedContainer() -> std::vector<typename RgResourceTraits<T>::ApiType*>&
        {
            if constexpr (std::is_same_v<T, D3D12Buffer>)
            {
                return pImportedBuffers;
            }
            else if constexpr (std::is_same_v<T, D3D12Texture>)
            {
                return pImportedTextures;
            }
        }

    private:
        friend class RenderGraphRegistry;
        friend class RenderPass;

        RenderGraphAllocator& Allocator;
        RenderGraphRegistry&  Registry;

        std::vector<D3D12Buffer*>  pImportedBuffers;
        std::vector<D3D12Texture*> pImportedTextures;

        std::vector<RgBuffer>  Buffers;
        std::vector<RgTexture> Textures;

        std::vector<RenderPass*> RenderPasses;
        RenderPass*              ProloguePass;
        RenderPass*              EpiloguePass;

        size_t PassIndex;

        
        std::vector<RgResourceHandle> InGraphResHandle;
        std::vector<RenderPass*>      InGraphPass;
        // 按照加入graph的顺序记录所有RgResourceHandle被各种pass做出的操作
        std::map<RgResourceHandle, std::queue<RgHandleOpPassIdx>> RgHandleOpMap;

        //std::vector<std::vector<std::uint64_t>> AdjacencyLists;
        //std::vector<RenderPass*>                TopologicalSortedPasses;

        std::vector<RenderGraphDependencyLevel> DependencyLevels;
    };
} // namespace RHI
