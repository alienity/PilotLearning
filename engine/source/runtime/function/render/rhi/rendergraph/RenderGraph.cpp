#include "RenderGraph.h"
#include "runtime/core/log/log_system.h"
#include "runtime/function/render/rhi/d3d12/rhi_d3d12_mappings.h"
#include <algorithm>
#include <string>
#include <locale>
#include <codecvt>

namespace RHI
{

    RenderPass::RenderPass(std::string_view Name, RenderGraph* Graph) : Name(Name), ParentGraph(Graph) {}

	RenderPass& RenderPass::Read(RgResourceHandle Resource, bool IgnoreBarrier, RgResourceState subType, RgResourceState counterType)
	{
		// Only allow buffers/textures
        ASSERT(Resource.IsValid());
        ASSERT(Resource.Type == RgResourceType::Buffer || Resource.Type == RgResourceType::Texture);

        if (!ReadsFrom(Resource))
        {
            RgResourceHandleExt ResourceExt = ToRgResourceHandle(Resource, subType, counterType, IgnoreBarrier);
            Reads.push_back(ResourceExt);
        }

		return *this;
	}

	RenderPass& RenderPass::Write(RgResourceHandle& Resource, bool IgnoreBarrier, RgResourceState subType, RgResourceState counterType)
	{
		// Only allow buffers/textures
        ASSERT(Resource.IsValid());
        ASSERT(Resource.Type == RgResourceType::Buffer || Resource.Type == RgResourceType::Texture);

		Resource.Version++;
        
        if (!WritesTo(Resource))
        {
            RgResourceHandleExt ResourceExt = ToRgResourceHandle(Resource, subType, counterType, IgnoreBarrier);
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
        //Context->FlushResourceBarriers();

		// Figure out all the barriers needed for each level
		// Handle resource transitions for all registered resources
		for (auto& Read : Reads)
		{
			if (Read.rgTransFlag == RgBarrierFlag::NoneBarrier)
			{
                continue;
			}

            RHI::D3D12Resource* _ResourcePtr = nullptr;
            RHI::D3D12Resource* _BufferCounterPtr = nullptr;
            if (Read.rgHandle.Type == RgResourceType::Texture)
            {
                _ResourcePtr = RenderGraph->GetRegistry()->GetD3D12Texture(Read.rgHandle);
            }
            else if (Read.rgHandle.Type == RgResourceType::Buffer)
            {
                D3D12Buffer* _BufferPtr = RenderGraph->GetRegistry()->GetD3D12Buffer(Read.rgHandle);
                _BufferCounterPtr = _BufferPtr->GetCounterBuffer().get();
                _ResourcePtr = _BufferPtr;
            }

            if (_ResourcePtr != nullptr && Read.rgSubType != RgResourceState::RHI_RESOURCE_STATE_NONE)
            {
                D3D12_RESOURCE_STATES _TargetState = RHITranslateD3D12(Read.rgSubType);
                Context->TransitionBarrier(_ResourcePtr, _TargetState);
            }
            if (_BufferCounterPtr != nullptr && Read.rgCounterType != RgResourceState::RHI_RESOURCE_STATE_NONE)
            {
                D3D12_RESOURCE_STATES _TargetState = RHITranslateD3D12(Read.rgCounterType);
                Context->TransitionBarrier(_BufferCounterPtr, _TargetState);
            }

            /*
			if (Read.rgHandle.Type == RgResourceType::Texture)
			{
                ASSERT(Read.rgSubType != RgResourceSubType::NoneType);



				if (Read.rgSubType == RgResourceSubType::NoneType)
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
			}
            else if (Read.rgHandle.Type == RgResourceType::Buffer)
            {
                D3D12Buffer* pCBuffer = RenderGraph->GetRegistry()->GetD3D12Buffer(Read.rgHandle);
                if (Read.rgSubType == RgResourceSubType::VertexAndConstantBuffer)
                {
                    Context->TransitionBarrier(pCBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                }
                else if (Read.rgSubType == RgResourceSubType::IndirectArgBuffer)
                {
                    Context->TransitionBarrier(pCBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
                }
                else if (Read.rgSubType == RgResourceSubType::PSAccess)
                {
                    Context->TransitionBarrier(pCBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                }
                else if (Read.rgSubType == RgResourceSubType::PSNonAccess)
                {
                    Context->TransitionBarrier(pCBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                }

                if (Read.rgCounterType == RgResourceSubType::IndirectArgBuffer)
                {
                    D3D12Buffer* pCBuffer = RenderGraph->GetRegistry()->GetD3D12Buffer(Read.rgHandle);
                }
            }
            */
		}
		for (auto Write : Writes)
		{
            if (Write.rgTransFlag == RgBarrierFlag::NoneBarrier)
            {
                continue;
            }

            RHI::D3D12Resource* _ResourcePtr = nullptr;
            RHI::D3D12Resource* _BufferCounterPtr = nullptr;
            if (Write.rgHandle.Type == RgResourceType::Texture)
            {
                _ResourcePtr = RenderGraph->GetRegistry()->GetD3D12Texture(Write.rgHandle);
            }
            else if (Write.rgHandle.Type == RgResourceType::Buffer)
            {
                D3D12Buffer* _BufferPtr = RenderGraph->GetRegistry()->GetD3D12Buffer(Write.rgHandle);
                _BufferCounterPtr = _BufferPtr->GetCounterBuffer().get();
                _ResourcePtr = _BufferPtr;
            }

            if (_ResourcePtr != nullptr && Write.rgSubType != RgResourceState::RHI_RESOURCE_STATE_NONE)
            {
                D3D12_RESOURCE_STATES _TargetState = RHITranslateD3D12(Write.rgSubType);
                Context->TransitionBarrier(_ResourcePtr, _TargetState);
            }
            if (_BufferCounterPtr != nullptr && Write.rgCounterType != RgResourceState::RHI_RESOURCE_STATE_NONE)
            {
                D3D12_RESOURCE_STATES _TargetState = RHITranslateD3D12(Write.rgCounterType);
                Context->TransitionBarrier(_BufferCounterPtr, _TargetState);
            }

            /*
			D3D12_RESOURCE_STATES WriteState = D3D12_RESOURCE_STATE_COMMON;
            if (Write.rgHandle.Type == RgResourceType::Texture)
			{
                if (RenderGraph->AllowRenderTarget(Write.rgHandle) && RenderGraph->AllowUnorderedAccess(Write.rgHandle))
                {
                    if (Write.rgSubType == RgResourceSubType::RenderTarget)
                    {
                        WriteState |= D3D12_RESOURCE_STATE_RENDER_TARGET;
                    }
                    else if (Write.rgSubType == RgResourceSubType::UnorderedAccess)
                    {
                        WriteState |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                    }
                    else
                    {
                        WriteState |= D3D12_RESOURCE_STATE_RENDER_TARGET;
                        MoYu::LogSystem::Instance()->log(MoYu::LogSystem::LogLevel::Error,
                                                         "RgResourceHandle id = [{0:d}] has not set as RenderTarget or UnorderedAccess, Default as RenderTarget",
                                                         (int)Write.rgHandle.Id);
                    }
                }
                else
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
            */
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

    bool RenderGraph::IsPassAvailable(PassIdx passIdx, InGraphPassIdx2ReadWriteHandle& pass2Handles, InGraphHandle2WritePassIdx& handle2WritePassIdx)
    {
        auto& mReadWriteHandles = pass2Handles[passIdx];

        for (size_t i = 0; i < mReadWriteHandles.first.size(); i++)
        {
            if (!handle2WritePassIdx[mReadWriteHandles.first[i].rgHandle].empty())
                return false;
        }
        return true;
    }

    bool RenderGraph::IsPassAvailable(std::vector<PassIdx>& passInSameLevel, InGraphPassIdx2ReadWriteHandle& pass2Handles, InGraphHandle2WritePassIdx& handle2WritePassIdx, RgResourceHandleExt& resource)
    {
        // 2.1 pass的reads没有被作为其他pass的write写过
        if (!handle2WritePassIdx[resource.rgHandle].empty())
        {
            return false;
        }

        // 2.2 pass的reads跟前面遍历过的pass的reads的handle一样，但是读取状态不一样，也不能要
        if (resource.rgTransFlag == RgBarrierFlag::NoneBarrier)
            return true;

        for (size_t i = 0; i < passInSameLevel.size(); i++)
        {
            PassIdx prevPassIdx = passInSameLevel[i];
            auto& prevPassReads = pass2Handles[prevPassIdx].first;
            for (size_t j = 0; j < prevPassReads.size(); j++)
            {
                auto& prevPassRead = prevPassReads[j];
                if (prevPassRead.rgHandle == resource.rgHandle &&
                    prevPassRead.rgTransFlag == RgBarrierFlag::AutoBarrier)
                {
                    if (prevPassRead.rgSubType != resource.rgSubType ||
                        prevPassRead.rgCounterType != resource.rgCounterType)
                    {
                        return false;
                    }
                }
            }
        }
        return true;
    }

	void RenderGraph::Setup()
	{
		// 准备RgHandle的所有操作数据，看看可以按照Read和Write的顺序，先给所有的pass排个序不

        InGraphPassIdx2ReadWriteHandle mPassIdx2ReadWriteHandle;
        //InGraphHandle2ReadPassIdx      mHandle2ReadPassIdx;
        InGraphHandle2WritePassIdx     mHandle2WritePassIdx;

        std::vector<PassIdx> mInGraphPassIdx(InGraphPass.size());
        
        // 转换所有Pass的Reads和Writes到Idx
        for (size_t i = 0; i < InGraphPass.size(); i++)
        {
            RHI::RenderPass* pass = InGraphPass[i];
            PassIdx passIdx = pass->PassIndex;

            std::vector<RHI::RgResourceHandleExt> readHandles(pass->Reads.size());
            std::vector<RHI::RgResourceHandleExt> writeHandles(pass->Writes.size());

            for (size_t j = 0; j < pass->Reads.size(); j++)
            {
                RHI::RgResourceHandleExt readHandleExt = pass->Reads[j];
                readHandles[j] = readHandleExt;
                //mHandle2ReadPassIdx[readHandle].insert(passIdx);
            }
            for (size_t j = 0; j < pass->Writes.size(); j++)
            {
                RHI::RgResourceHandleExt writeHandleExt = pass->Writes[j];
                writeHandles[j] = writeHandleExt;
                mHandle2WritePassIdx[writeHandleExt.rgHandle].insert(passIdx);
            }

            mPassIdx2ReadWriteHandle[passIdx] = std::pair<std::vector<RHI::RgResourceHandleExt>, std::vector<RHI::RgResourceHandleExt>>(readHandles, writeHandles);

            mInGraphPassIdx[i] = passIdx;
        }

        // 直接遍历所有的pass，找出所有没有readHandle或者readHandle没有被Write的pass
        while (!mInGraphPassIdx.empty())
        {
            RenderGraphDependencyLevel dependencyLevel = {};

            std::vector<PassIdx> passInSameLevel;

            std::vector<PassIdx>::iterator graphPassIter;
            for (graphPassIter = mInGraphPassIdx.begin(); graphPassIter < mInGraphPassIdx.end();)
            {
                PassIdx mPassIdx = *graphPassIter;

                auto& mPassReads = mPassIdx2ReadWriteHandle[mPassIdx].first;

                // 1. 查找所有reads为空的pass
                bool isPassReadsEmpty = mPassReads.empty();
                if (isPassReadsEmpty)
                {
                    passInSameLevel.push_back(mPassIdx);
                    graphPassIter = mInGraphPassIdx.erase(graphPassIter);
                    continue;
                }

                // 2. 查找所有reads没有被其他pass做Write的pass
                bool isPassReadsClean = true;
                for (size_t j = 0; j < mPassReads.size(); j++)
                {
                    if (!IsPassAvailable(passInSameLevel, mPassIdx2ReadWriteHandle, mHandle2WritePassIdx, mPassReads[j]))
                    {
                        isPassReadsClean = false;
                        break;
                    }
                }
                if (isPassReadsClean)
                {
                    passInSameLevel.push_back(mPassIdx);
                    graphPassIter = mInGraphPassIdx.erase(graphPassIter);
                    continue;
                }

                graphPassIter++;
            }

            // 清理掉pass的writes的Handle的所有对当前Pass的索引状态
            for (size_t i = 0; i < passInSameLevel.size(); i++)
            {
                PassIdx mPassIdx = passInSameLevel[i];

                auto& mPassWrites = mPassIdx2ReadWriteHandle[mPassIdx].second;

                std::vector<RHI::RgResourceHandleExt>::iterator passWriteIter;

                for (passWriteIter = mPassWrites.begin(); passWriteIter < mPassWrites.end(); passWriteIter++)
                {
                    RHI::RgResourceHandle passWriteHandle = passWriteIter->rgHandle;
                    
                    mHandle2WritePassIdx[passWriteHandle].erase(mPassIdx);
                }

                dependencyLevel.AddRenderPass(InGraphPass[mPassIdx]);
            }

            DependencyLevels.push_back(dependencyLevel);
        }

	}

	void RenderGraph::ExportDgml(DgmlBuilder& Builder) const
	{
        for (int i = 0; i < InGraphResHandle.size(); i++)
        {
            auto handle = InGraphResHandle[i];

            std::string _nodeId = (std::string)fmt::format("Node_{}", (int)(handle.Id));

            std::string _name;

            // Setup converter
            using convert_type = std::codecvt_utf8<wchar_t>;
            std::wstring_convert<convert_type, wchar_t> converter;

            // Convert wstring to UTF-8 string
            if (handle.Type == RgResourceType::Buffer)
                _name = converter.to_bytes(Registry.GetD3D12Buffer(handle)->GetResourceName());
            else if (handle.Type == RgResourceType::Texture)
                _name = converter.to_bytes(Registry.GetD3D12Texture(handle)->GetResourceName());

            std::string_view nodeName = _name;

            Builder.AddNode(_nodeId, "Node", nodeName);
        }

        for (int i = 0; i < DependencyLevels.size(); i++)
        {
            auto& renderPasses = DependencyLevels[i].RenderPasses;
            for (int j = 0; j < renderPasses.size(); j++)
            {
                std::string _passId = (std::string) fmt::format("Pass_{}", renderPasses[j]->PassIndex);
                std::string_view passId = _passId;
                std::string_view passName = renderPasses[j]->Name;

                Builder.AddNode(passId, "Pass", passName);

                auto& pReads = renderPasses[j]->Reads;
                auto& pWrites = renderPasses[j]->Writes;

                for (int m = 0; m < pReads.size(); m++)
                {
                    std::string _nodeId = (std::string)fmt::format("Node_{}", (int)(pReads[m].rgHandle.Id));

                    Builder.AddLink(_nodeId, _passId, "");
                }

                for (int m = 0; m < pWrites.size(); m++)
                {
                    std::string _nodeId = (std::string)fmt::format("Node_{}", (int)(pWrites[m].rgHandle.Id));

                    Builder.AddLink(_passId, _nodeId, "");
                }
            }
        }


	}
} // namespace RHI
