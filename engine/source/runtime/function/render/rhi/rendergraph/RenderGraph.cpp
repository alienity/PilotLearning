#include "RenderGraph.h"
#include <algorithm>

namespace RHI
{
	RenderPass::RenderPass(std::string_view Name)
		: Name(Name)
	{
	}

	RenderPass& RenderPass::Read(RgResourceHandle Resource)
	{
		// Only allow buffers/textures
        ASSERT(Resource.IsValid());
        ASSERT(Resource.Type == RgResourceType::Buffer || Resource.Type == RgResourceType::Texture);
		Reads.insert(Resource);
		ReadWrites.insert(Resource);
		return *this;
	}

	RenderPass& RenderPass::Write(RgResourceHandle Resource)
	{
		// Only allow buffers/textures
        ASSERT(Resource && Resource->IsValid());
        ASSERT(Resource->Type == RgResourceType::Buffer || Resource->Type == RgResourceType::Texture);
		//Resource->Version++;
		Writes.insert(Resource);
		ReadWrites.insert(Resource);
		return *this;
	}

	RenderPass& RenderPass::Resolve(RgResourceHandle SrcResource, RgResourceHandle DstResource)
	{
        // Only allow buffers/textures
        ASSERT(Resource && Resource->IsValid());
        ASSERT(Resource->Type == RgResourceType::Texture);
        // Resource->Version++;
        ResolveSrcDstPairs.push_back(std::pair<RgResourceHandle, RgResourceHandle>(SrcResource, DstResource));
        Reads.insert(SrcResource);
        Writes.insert(DstResource);
        ReadWrites.insert(SrcResource);
        ReadWrites.insert(DstResource);
        return *this;
	}

	bool RenderPass::HasDependency(RgResourceHandle Resource) const
	{
		return ReadWrites.contains(Resource);
	}

	bool RenderPass::WritesTo(RgResourceHandle Resource) const
	{
		return Writes.contains(Resource);
	}

	bool RenderPass::ReadsFrom(RgResourceHandle Resource) const
	{
		return Reads.contains(Resource);
	}

	bool RenderPass::HasAnyDependencies() const noexcept
	{
		return !ReadWrites.empty();
	}

	void RenderGraphDependencyLevel::AddRenderPass(RenderPass* RenderPass)
	{
		RenderPasses.push_back(RenderPass);
		Reads.insert(RenderPass->Reads.begin(), RenderPass->Reads.end());
		Writes.insert(RenderPass->Writes.begin(), RenderPass->Writes.end());
        ResolveSrcDstPairs.insert(ResolveSrcDstPairs.end(), RenderPass->ResolveSrcDstPairs.begin(), RenderPass->ResolveSrcDstPairs.end());
	}

	void RenderGraphDependencyLevel::Execute(RenderGraph* RenderGraph, D3D12CommandContext* Context)
	{
		// Figure out all the barriers needed for each level
		// Handle resource transitions for all registered resources
		for (auto Read : Reads)
		{
            bool isResolveSrc = false;
            for (size_t i = 0; i < ResolveSrcDstPairs.size(); i++)
            {
				if (ResolveSrcDstPairs[i].first == Read)
				{
                    isResolveSrc = true;
                    break;
				}
			}
            if (isResolveSrc)
                continue;

			D3D12_RESOURCE_STATES ReadState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			if (RenderGraph->AllowUnorderedAccess(Read))
			{
				ReadState |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			}

			D3D12Texture* pTexture = RenderGraph->GetRegistry()->GetD3D12Texture(Read);

			Context->TransitionBarrier(pTexture, ReadState);
		}
		for (auto Write : Writes)
		{
            bool isResolveDst = false;
            for (size_t i = 0; i < ResolveSrcDstPairs.size(); i++)
            {
                if (ResolveSrcDstPairs[i].second == Write)
                {
                    isResolveDst = true;
                    break;
                }
            }
            if (isResolveDst)
                continue;

			D3D12_RESOURCE_STATES WriteState = D3D12_RESOURCE_STATE_COMMON;
			if (RenderGraph->AllowRenderTarget(Write))
			{
				WriteState |= D3D12_RESOURCE_STATE_RENDER_TARGET;
			}
			if (RenderGraph->AllowDepthStencil(Write))
			{
				WriteState |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
			}
			if (RenderGraph->AllowUnorderedAccess(Write))
			{
				WriteState |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			}
            D3D12Texture* pTexture = RenderGraph->GetRegistry()->GetD3D12Texture(Write);

			Context->TransitionBarrier(pTexture, WriteState);
		}

		Context->FlushResourceBarriers();

		for (auto& RenderPass : RenderPasses)
		{
			if (RenderPass->Callback)
			{
				D3D12ScopedEvent(Context, RenderPass->Name);
				RenderPass->Callback(RenderGraph->GetRegistry(), Context);
			}
		}
	}

	RenderGraph::RenderGraph(RenderGraphAllocator& Allocator, RenderGraphRegistry& Registry) :
        Allocator(Allocator), Registry(Registry), PassIndex(0)
    {
        Allocator.Reset();

        // Allocate epilogue pass after allocator reset
        ProloguePass = Allocator.Construct<RenderPass>("Prologue");
        ProloguePass->PassIndex = PassIndex++;
        EpiloguePass = Allocator.Construct<RenderPass>("Epilogue");
        RenderPasses.push_back(ProloguePass);
    }

	RenderGraph::~RenderGraph()
	{
		for (auto RenderPass : RenderPasses)
		{
			Allocator.Destruct(RenderPass);
		}
		RenderPasses.clear();
	}

	RenderPass& RenderGraph::AddRenderPass(std::string_view Name)
	{
		RenderPass* NewRenderPass = Allocator.Construct<RenderPass>(Name);
        NewRenderPass->PassIndex  = PassIndex++;
		RenderPasses.emplace_back(NewRenderPass);
		return *NewRenderPass;
	}

	RenderPass& RenderGraph::GetProloguePass() const noexcept
	{
		return *ProloguePass;
	}

	RenderPass& RenderGraph::GetEpiloguePass() const noexcept
	{
		return *EpiloguePass;
	}

	RenderGraphRegistry* RenderGraph::GetRegistry() const noexcept
	{
		return &Registry;
	}

	void RenderGraph::Execute(D3D12CommandContext* Context)
	{
        EpiloguePass->PassIndex = PassIndex++;
		RenderPasses.push_back(EpiloguePass);
		Setup();
		Registry.RealizeResources(this, Context->GetParentLinkedDevice()->GetParentDevice());

		D3D12ScopedEvent(Context, "Render Graph");
		for (auto& DependencyLevel : DependencyLevels)
		{
			DependencyLevel.Execute(this, Context);
		}
	}

	bool RenderGraph::AllowRenderTarget(RgResourceHandle Resource) const noexcept
	{
        ASSERT(Resource.Type == RgResourceType::Texture);
		if (Resource.IsImported())
		{
            ASSERT(Resource.Id < pImportedTextures.size());
            auto TexDesc = pImportedTextures[Resource.Id]->GetDesc();
            return TexDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}
		else
		{
            ASSERT(Resource.Id < Textures.size());
            return Textures[Resource.Id].Desc.AllowRenderTarget;
		}
	}

	bool RenderGraph::AllowDepthStencil(RgResourceHandle Resource) const noexcept
	{
        ASSERT(Resource.Type == RgResourceType::Texture);
        if (Resource.IsImported())
		{
            ASSERT(Resource.Id < pImportedTextures.size());
            auto TexDesc = pImportedTextures[Resource.Id]->GetDesc();
            return TexDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		}
		else
		{
            ASSERT(Resource.Id < Textures.size());
            return Textures[Resource.Id].Desc.AllowDepthStencil;
		}
	}

	bool RenderGraph::AllowUnorderedAccess(RgResourceHandle Resource) const noexcept
	{
        ASSERT(Resource.Type == RgResourceType::Buffer || Resource.Type == RgResourceType::Texture);
        if (Resource.Type == RgResourceType::Texture)
		{
            if (Resource.IsImported())
            {
                ASSERT(Resource.Id < pImportedTextures.size());
                auto TexDesc = pImportedTextures[Resource.Id]->GetDesc();
                return TexDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            }
            else
            {
                ASSERT(Resource.Id < Textures.size());
                return Textures[Resource.Id].Desc.AllowUnorderedAccess;
            }
		}
        else if (Resource.Type == RgResourceType::Buffer)
		{
            if (Resource.IsImported())
            {
                ASSERT(Resource.Id < pImportedBuffers.size());
                auto BufferDesc = pImportedBuffers[Resource.Id]->GetDesc();
                return BufferDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            }
            else
            {
                ASSERT(Resource.Id < Buffers.size());
                return Buffers[Resource.Id].Desc.UnorderedAccess;
            }
		}
        return false;
	}

	void RenderGraph::Setup()
	{
		// https://levelup.gitconnected.com/organizing-gpu-work-with-directed-acyclic-graphs-f3fd5f2c2af3
		// https://www.gdcvault.com/play/1024612/FrameGraph-Extensible-Rendering-Architecture-in
		// https://media.contentapi.ea.com/content/dam/ea/seed/presentations/wihlidal-halcyonarchitecture-notes.pdf

		// Adjacency lists
		AdjacencyLists.resize(RenderPasses.size());

		for (size_t i = 0; i < RenderPasses.size(); ++i)
		{
			RenderPass* Node = RenderPasses[i];
			if (!Node->HasAnyDependencies())
			{
				continue;
			}

			std::vector<std::uint64_t>& Indices = AdjacencyLists[i];

			// Reverse iterate the render passes here, because often or not, the adjacency list should be built upon
			// the latest changes to the render passes since those pass are more likely to change the resource we are writing to from other passes
			// if we were to iterate from 0 to RenderPasses.size(), it'd often break the algorithm and creates an incorrect adjacency list
			for (size_t j = RenderPasses.size(); j-- != 0;)
			{
				if (i == j)
				{
					continue;
				}

				RenderPass* Neighbor = RenderPasses[j];
				for (auto Resource : Node->Writes)
				{
					// If the neighbor reads from a resource written to by the current pass, then it is a dependency, add it to the adjacency list
					if (Neighbor->ReadsFrom(Resource))
					{
						Indices.push_back(j);
						break;
					}
				}
			}
		}

		// Topological sort
		std::stack<size_t> Stack;
		std::vector<bool>  Visited(RenderPasses.size(), false);

		for (size_t i = 0; i < RenderPasses.size(); i++)
		{
			if (!Visited[i])
			{
				DepthFirstSearch(i, Visited, Stack);
			}
		}

		TopologicalSortedPasses.reserve(Stack.size());
		while (!Stack.empty())
		{
			size_t i						  = Stack.top();
			RenderPasses[i]->TopologicalIndex = i;
			TopologicalSortedPasses.push_back(RenderPasses[i]);
			Stack.pop();
		}

		// Longest path search
		// Render passes in a dependency level share the same recursion depth,
		// or rather maximum recursion depth AKA longest path in a DAG
		std::vector<int> Distances(TopologicalSortedPasses.size(), 0);

		for (auto& TopologicalSortedPass : TopologicalSortedPasses)
		{
			size_t u = TopologicalSortedPass->TopologicalIndex;
			for (auto v : AdjacencyLists[u])
			{
				if (Distances[v] < Distances[u] + 1)
				{
					Distances[v] = Distances[u] + 1;
				}
			}
		}

		// DependencyLevels.resize(*std::ranges::max_element(Distances) + 1);
        DependencyLevels.resize(*std::max_element(Distances.begin(), Distances.end()) + 1);

		// sort renderpass according to PassIndex
        std::vector<std::vector<RHI::RenderPass*>> sorttedRenderPassList(DependencyLevels.size());
        for (size_t i = 0; i < TopologicalSortedPasses.size(); ++i)
        {
            int level = Distances[i];
            std::vector<RHI::RenderPass*>& curLevelRenderPass = sorttedRenderPassList[level];
            curLevelRenderPass.push_back(TopologicalSortedPasses[i]);
            for (size_t j = curLevelRenderPass.size() - 1; j > 0; j--)
            {
                size_t curPassIndex = curLevelRenderPass[j]->PassIndex;
                size_t prePassIndex = curLevelRenderPass[j - 1]->PassIndex;
				if (curPassIndex < prePassIndex)
				{
                    RHI::RenderPass* tmpPass = curLevelRenderPass[j];
                    curLevelRenderPass[j]    = curLevelRenderPass[j - 1];
                    curLevelRenderPass[j - 1] = tmpPass;
				}
			}
        }

		for (size_t l = 0; l < sorttedRenderPassList.size(); ++l)
		{
            std::vector<RHI::RenderPass*>& curLevelRenderPass = sorttedRenderPassList[l];
            for (size_t j = 0; j < curLevelRenderPass.size(); j++)
            {
                DependencyLevels[l].AddRenderPass(curLevelRenderPass[j]);
			}
		}
	}

	void RenderGraph::DepthFirstSearch(size_t n, std::vector<bool>& Visited, std::stack<size_t>& Stack)
	{
		Visited[n] = true;

		for (auto i : AdjacencyLists[n])
		{
			if (!Visited[i])
			{
				DepthFirstSearch(i, Visited, Stack);
			}
		}

		Stack.push(n);
	}

	void RenderGraph::ExportDgml(DgmlBuilder& Builder) const
	{
		for (size_t i = 0; i < AdjacencyLists.size(); ++i)
		{
			RenderPass* Node = RenderPasses[i];
			std::ignore		 = Builder.AddNode(Node->Name, Node->Name);

			for (size_t j = 0; j < AdjacencyLists[i].size(); ++j)
			{
				RenderPass* Neighbor = RenderPasses[AdjacencyLists[i][j]];
				for (auto Resource : Node->Writes)
				{
					if (Neighbor->ReadsFrom(Resource))
					{
						std::ignore = Builder.AddLink(Node->Name.data(), Neighbor->Name.data(), GetResourceName(Resource));
					}
				}
			}
		}
	}
} // namespace RHI
