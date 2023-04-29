#include "RenderGraph.h"
#include <algorithm>

namespace RHI
{

    RenderPass::RenderPass(std::string_view Name, RenderGraph* Graph) : Name(Name), ParentGraph(Graph) {}

	RenderPass& RenderPass::Read(RgResourceHandle Resource, bool IgnoreBarrier)
	{
		// Only allow buffers/textures
        ASSERT(Resource.IsValid());
        ASSERT(Resource.Type == RgResourceType::Buffer || Resource.Type == RgResourceType::Texture);

        if (!ReadsFrom(Resource))
        {
            RgResourceHandleExt ResourceExt = ToRgResourceHandle(Resource, IgnoreBarrier);
            Reads.push_back(ResourceExt);
        }

		return *this;
	}

	RenderPass& RenderPass::Write(RgResourceHandle& Resource, bool IgnoreBarrier)
	{
		// Only allow buffers/textures
        ASSERT(Resource.IsValid());
        ASSERT(Resource.Type == RgResourceType::Buffer || Resource.Type == RgResourceType::Texture);

		Resource.Version++;
        
        if (!WritesTo(Resource))
        {
            RgResourceHandleExt ResourceExt = ToRgResourceHandle(Resource, IgnoreBarrier);
            Writes.push_back(ResourceExt);
        }

		return *this;
	}

	bool RenderPass::HasDependency(RgResourceHandle& Resource) const
    {
        return ReadsFrom(Resource) || WritesTo(Resource);
    }

	bool RenderPass::WritesTo(RgResourceHandle& Resource) const
	{
        for (size_t i = 0; i < Writes.size(); i++)
        {
            if (Writes[i].rgHandle == Resource)
            {
                return true;
            }
        }
        return false;
	}

	bool RenderPass::ReadsFrom(RgResourceHandle& Resource) const
	{
        for (size_t i = 0; i < Reads.size(); i++)
        {
            if (Reads[i].rgHandle == Resource)
            {
                return true;
            }
        }
        return false;
	}

	bool RenderPass::HasAnyDependencies() const noexcept
	{
		return !Reads.empty() || !Writes.empty();
	}

	void RenderGraphDependencyLevel::AddRenderPass(RenderPass* RenderPass)
	{
		RenderPasses.push_back(RenderPass);
		Reads.insert(RenderPass->Reads.begin(), RenderPass->Reads.end());
		Writes.insert(RenderPass->Writes.begin(), RenderPass->Writes.end());
	}

	void RenderGraphDependencyLevel::Execute(RenderGraph* RenderGraph, D3D12CommandContext* Context)
	{
		// Figure out all the barriers needed for each level
		// Handle resource transitions for all registered resources
		for (auto& Read : Reads)
		{
			if (Read.rgTransFlag == RgBarrierFlag::None)
			{
                continue;
			}

			if (Read.rgHandle.Type == RgResourceType::Texture || Read.rgHandle.Type == RgResourceType::Buffer)
			{
				if (Read.rgSubType == RgResourceSubType::None)
				{
                    D3D12_RESOURCE_STATES ReadState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
                    if (RenderGraph->AllowUnorderedAccess(Read.rgHandle))
                    {
                        ReadState |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
                    }
                    if (Read.rgHandle.Type == RgResourceType::Texture)
                    {
                        D3D12Texture* pTexture = RenderGraph->GetRegistry()->GetD3D12Texture(Read.rgHandle);
                        Context->TransitionBarrier(pTexture, ReadState);
                    }
                    if (Read.rgHandle.Type == RgResourceType::Buffer)
                    {
                        D3D12Buffer* pBuffer = RenderGraph->GetRegistry()->GetD3D12Buffer(Read.rgHandle);
                        Context->TransitionBarrier(pBuffer, ReadState);
                    }
				}
                else if (Read.rgSubType == RgResourceSubType::VertexAndConstantBuffer)
				{
                    D3D12Buffer* pCBuffer = RenderGraph->GetRegistry()->GetD3D12Buffer(Read.rgHandle);
                    Context->TransitionBarrier(pCBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
				}
                else if (Read.rgSubType == RgResourceSubType::IndirectArgBuffer)
				{
                    D3D12Buffer* pCBuffer = RenderGraph->GetRegistry()->GetD3D12Buffer(Read.rgHandle);
                    Context->TransitionBarrier(pCBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
				}
			}
		}
		for (auto Write : Writes)
		{
            if (Write.rgTransFlag == RgBarrierFlag::None)
            {
                continue;
            }

			D3D12_RESOURCE_STATES WriteState = D3D12_RESOURCE_STATE_COMMON;
            if (Write.rgHandle.Type == RgResourceType::Texture)
			{
                if (RenderGraph->AllowRenderTarget(Write.rgHandle))
                {
                    WriteState |= D3D12_RESOURCE_STATE_RENDER_TARGET;
                }
                if (RenderGraph->AllowDepthStencil(Write.rgHandle))
                {
                    WriteState |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
                }
                if (RenderGraph->AllowUnorderedAccess(Write.rgHandle))
                {
                    WriteState |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                }
                D3D12Texture* pTexture = RenderGraph->GetRegistry()->GetD3D12Texture(Write.rgHandle);
                Context->TransitionBarrier(pTexture, WriteState);
			}
			else
			{
                if (RenderGraph->AllowUnorderedAccess(Write.rgHandle))
                {
                    WriteState |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                }
                D3D12Buffer* pBuffer = RenderGraph->GetRegistry()->GetD3D12Buffer(Write.rgHandle);
                Context->TransitionBarrier(pBuffer, WriteState);
			}
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
        ProloguePass = Allocator.Construct<RenderPass>("Prologue", this);
        ProloguePass->PassIndex = PassIndex++;
        EpiloguePass = Allocator.Construct<RenderPass>("Epilogue", this);
        RenderPasses.push_back(ProloguePass);
        InGraphPass.push_back(ProloguePass);
    }

	RenderGraph::~RenderGraph()
	{
		for (auto RenderPass : RenderPasses)
		{
			Allocator.Destruct(RenderPass);
		}
		RenderPasses.clear();

        InGraphResHandle.clear();
        InGraphPass.clear();
	}

	RenderPass& RenderGraph::AddRenderPass(std::string_view Name)
	{
		RenderPass* NewRenderPass = Allocator.Construct<RenderPass>(Name, this);
        NewRenderPass->PassIndex  = PassIndex++;
		RenderPasses.emplace_back(NewRenderPass);
        InGraphPass.push_back(NewRenderPass);

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
        InGraphPass.push_back(EpiloguePass);
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
                return Buffers[Resource.Id].Desc.mRHIBufferTarget & RHIBufferTarget::RHIBufferRandomReadWrite;
            }
		}
        return false;
	}

    bool RenderGraph::IsPassAvailable(PassIdx passIdx, InGraphPassIdx2ReadWriteIdx& passIdx2ReadWriteIdx, InGraphReadHandle2PassIdx& readHandle2PassIdx)
    {
        auto& mReadWriteIdx = passIdx2ReadWriteIdx[passIdx];

        for (size_t i = 0; i < mReadWriteIdx.first.size(); i++)
        {
            if (!readHandle2PassIdx[mReadWriteIdx.first[i]].empty())
                return false;
        }
        return true;
    }

	void RenderGraph::Setup()
	{
		// 准备RgHandle的所有操作数据，看看可以按照Read和Write的顺序，先给所有的pass排个序不

        InGraphPassIdx2ReadWriteIdx mPassIdx2ReadWriteIdx;
        InGraphReadHandle2PassIdx   mHandle2PassWriterIdx;
        std::vector<PassIdx> InGraphPassIdx;
        
        // 转换所有Pass的Reads和Writes到Idx
        for (size_t i = 0; i < InGraphPass.size(); i++)
        {
            RHI::RenderPass* pass = InGraphPass[i];
            PassIdx passIdx = pass->PassIndex;

            std::vector<HandleIdx> readsIdx(pass->Reads.size());
            std::vector<HandleIdx> writesIdx(pass->Writes.size());

            for (size_t j = 0; j < pass->Reads.size(); j++)
            {
                HandleIdx readIdx = InGraphHandle2Idx[pass->Reads[j].rgHandle];
                readsIdx[j] = readIdx;
                mHandle2PassWriterIdx[readIdx].insert(passIdx);
            }
            for (size_t j = 0; j < pass->Writes.size(); j++)
            {
                HandleIdx writeIdx = InGraphHandle2Idx[pass->Writes[j].rgHandle];
                writesIdx[j] = writeIdx;
                mHandle2PassWriterIdx[writeIdx].insert(passIdx);
            }

            mPassIdx2ReadWriteIdx[passIdx] =
                std::pair<std::vector<HandleIdx>, std::vector<HandleIdx>>(readsIdx, writesIdx);
        }

        // 找有向无环图
        std::set<PassIdx> mPassIdxQueue = {};
        do
        {
            if (!mPassIdxQueue.empty())
            {
                RenderGraphDependencyLevel dependencyLevel = {};
                while (!mPassIdxQueue.empty())
                {
                    RenderPass* rgPassPtr = InGraphPass[*mPassIdxQueue.begin()];
                    mPassIdxQueue.erase(mPassIdxQueue.begin());

                    if (IsPassAvailable(rgPassPtr->PassIndex, mPassIdx2ReadWriteIdx, mHandle2PassWriterIdx))
                    {
                        for (size_t i = 0; i < rgPassPtr->Reads.size(); i++)
                        {

                        }







                        for (auto it = rgPassPtr->Reads.begin(); it != rgPassPtr->Reads.end(); it++)
                        {
                            RemovePassOpFromResHandleMap(*it, rgPassPtr, RgHandleOpMap);
                        }
                        for (auto it = rgPassPtr->Writes.begin(); it != rgPassPtr->Writes.end(); it++)
                        {
                            RemovePassOpFromResHandleMap(*it, rgPassPtr, RgHandleOpMap);
                        }
                        dependencyLevel.AddRenderPass(rgPassPtr);
                    }
                }
                DependencyLevels.push_back(dependencyLevel);

                std::vector<RenderPass*>& mBatchLevel = dependencyLevel.GetRenderPasses();

                for (size_t i = 0; i < mBatchLevel.size(); i++)
                {
                    for (auto it = InGraphPass.begin(); it != InGraphPass.end();)
                    {
                        if (mBatchLevel[i] == *it)
                        {
                            it = InGraphPass.erase(it);
                        }
                        else
                        {
                            ++it;
                        }
                    }
                }
            }


        } while (!gRenderPassQueue.empty());

















        /*
        std::set<RenderPass*> gRenderPassQueue = {};
        do
        {
			// 3. ѭ�������ҳ����в���ִ�е�pass������˳�������������
			if (!gRenderPassQueue.empty())
			{
                RenderGraphDependencyLevel dependencyLevel = {};
                // ����ֻ��д����Pass
                while (!gRenderPassQueue.empty())
                {
                    RenderPass* rgPassPtr = *gRenderPassQueue.begin();
                    gRenderPassQueue.erase(gRenderPassQueue.begin());
                    
					if (IsPassAvailable(rgPassPtr))
                    {
                        for (auto it = rgPassPtr->Reads.begin(); it != rgPassPtr->Reads.end(); it++)
                        {
                            RemovePassOpFromResHandleMap(*it, rgPassPtr, RgHandleOpMap);
                        }
                        for (auto it = rgPassPtr->Writes.begin(); it != rgPassPtr->Writes.end(); it++)
                        {
                            RemovePassOpFromResHandleMap(*it, rgPassPtr, RgHandleOpMap);
                        }
                        dependencyLevel.AddRenderPass(rgPassPtr);
					}
                }
                DependencyLevels.push_back(dependencyLevel);

				std::vector<RenderPass*>& mBatchLevel = dependencyLevel.GetRenderPasses();
                
				for (size_t i = 0; i < mBatchLevel.size(); i++)
                {
                    for (auto it = InGraphPass.begin(); it != InGraphPass.end();)
                    {
                        if (mBatchLevel[i] == *it)
						{
                            it = InGraphPass.erase(it);
						}
						else
						{
                            ++it;
						}
					}
				}
			}

            // 1. �ҳ�ֻ��Read��Pass
            for (size_t i = 0; i < InGraphResHandle.size(); i++)
            {
                RgResourceHandle& resHandle = InGraphResHandle[i];
                std::deque<RgHandleOpPassIdx>& rgHandleOpQueue = RgHandleOpMap[resHandle];
                for (auto it = rgHandleOpQueue.begin(); it != rgHandleOpQueue.end(); ++it)
                {
                    RgHandleOpPassIdx opidx = *it;
                    if (opidx.op == RgHandleOp::Write)
                        break;
                    if (IsPassAvailable(opidx.passPtr))
                    {
                        gRenderPassQueue.insert(opidx.passPtr);
                    }
                }
            }
            
			// 2. �ҳ�û��Reader��Pass
            for (size_t i = 0; i < InGraphPass.size(); i++)
            {
                RenderPass* passPtr = InGraphPass[i];
                if (passPtr->Reads.empty())
                {
                    gRenderPassQueue.insert(passPtr);
                }
            }

        } while (!gRenderPassQueue.empty());
        */
	}

	void RenderGraph::ExportDgml(DgmlBuilder& Builder) const
	{

	}
} // namespace RHI
