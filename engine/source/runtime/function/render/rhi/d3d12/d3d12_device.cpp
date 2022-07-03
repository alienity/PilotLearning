#include "d3d12_device.h"
#include "runtime/core/base/macro.h"

// D3D12.DRED
// Enable device removed extended data\n
// DRED delivers automatic breadcrumbs as well as GPU page fault reporting\n
static bool CVar_Dred = true;

// D3D12.AsyncPsoCompile
// Enables asynchronous pipeline state object compilation
static bool CVar_AsyncPsoCompilation = false;

namespace RHI
{
    // clang-format off
    D3D12Device::D3D12Device(const DeviceOptions& Options) 
        : Device(InitializeDevice(Options))
        , Device1(DeviceQueryInterface<ID3D12Device1>())
        , Device5(DeviceQueryInterface<ID3D12Device5>())
        , AllNodeMask(D3D12NodeMask::FromIndex(Device->GetNodeCount() - 1))
        , FeatureSupport(InitializeFeatureSupport(Options))
        , DescriptorSizeCache(InitializeDescriptorSizeCache())
        , Dred(Device.Get())
        , LinkedDevice(this, D3D12NodeMask::FromIndex(0))
        // , PsoCompilationThreadPool(std::make_unique<ThreadPool>())
        , Library(!Options.PsoCachePath.empty() ? std::make_unique<D3D12PipelineLibrary>(this, Options.PsoCachePath) : nullptr)
    {
        VERIFY_D3D12_API(DStorageGetFactory(IID_PPV_ARGS(&DStorageFactory)));

        DSTORAGE_QUEUE_DESC QueueDesc = {DSTORAGE_REQUEST_SOURCE_FILE,
                                         DSTORAGE_MAX_QUEUE_CAPACITY,
                                         DSTORAGE_PRIORITY_NORMAL,
                                         "DStorageQueue: File",
                                         Device.Get()};
        VERIFY_D3D12_API(
            DStorageFactory->CreateQueue(&QueueDesc, IID_PPV_ARGS(&DStorageQueues[DSTORAGE_REQUEST_SOURCE_FILE])));

        QueueDesc = {DSTORAGE_REQUEST_SOURCE_MEMORY,
                     DSTORAGE_MAX_QUEUE_CAPACITY,
                     DSTORAGE_PRIORITY_NORMAL,
                     "DStorageQueue: Memory",
                     Device.Get()};
        VERIFY_D3D12_API(
            DStorageFactory->CreateQueue(&QueueDesc, IID_PPV_ARGS(&DStorageQueues[DSTORAGE_REQUEST_SOURCE_MEMORY])));

        DStorageFence = D3D12Fence(this, 0);

        Microsoft::WRL::ComPtr<ID3D12InfoQueue> InfoQueue;
        if (SUCCEEDED(Device->QueryInterface(IID_PPV_ARGS(&InfoQueue))))
        {
            VERIFY_D3D12_API(InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE));
            VERIFY_D3D12_API(InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE));
            VERIFY_D3D12_API(InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE));

            // Suppress messages based on their severity level
            D3D12_MESSAGE_SEVERITY Severities[] = {D3D12_MESSAGE_SEVERITY_INFO};

            // Suppress individual messages by their ID
            D3D12_MESSAGE_ID IDs[] = {
                D3D12_MESSAGE_ID_LOADPIPELINE_NAMENOTFOUND, // Could happen when we are trying to load a PSO but is not
                                                            // present, then we will use this warning to create the PSO
                D3D12_MESSAGE_ID_LOADPIPELINE_INVALIDDESC,  // Could happen when shader is modified and the resulting
                                                            // serialized PSO is not valid
                D3D12_MESSAGE_ID_STOREPIPELINE_DUPLICATENAME,
                D3D12_MESSAGE_ID_CREATEPIPELINELIBRARY_DRIVERVERSIONMISMATCH,
                D3D12_MESSAGE_ID_CREATERESOURCE_STATE_IGNORED};

            D3D12_INFO_QUEUE_FILTER InfoQueueFilter = {};
            InfoQueueFilter.DenyList.NumSeverities  = ARRAYSIZE(Severities);
            InfoQueueFilter.DenyList.pSeverityList  = Severities;
            InfoQueueFilter.DenyList.NumIDs         = ARRAYSIZE(IDs);
            InfoQueueFilter.DenyList.pIDList        = IDs;
            VERIFY_D3D12_API(InfoQueue->PushStorageFilter(&InfoQueueFilter));
        }
    }
    // clang-format off

	void D3D12Device::CreateDxgiFactory()
    {
#ifdef _DEBUG
        InternalCreateDxgiFactory(true);
#else
        InternalCreateDxgiFactory(false);
#endif
    }

	bool D3D12Device::AllowAsyncPsoCompilation() const noexcept { return CVar_AsyncPsoCompilation; }

	bool D3D12Device::SupportsWaveIntrinsics() const noexcept
    {
        if (SUCCEEDED(FeatureSupport.GetStatus()))
        {
            return FeatureSupport.WaveOps();
        }
        return false;
    }

	bool D3D12Device::SupportsRaytracing() const noexcept
    {
        if (SUCCEEDED(FeatureSupport.GetStatus()))
        {
            return FeatureSupport.RaytracingTier() >= D3D12_RAYTRACING_TIER_1_0;
        }
        return false;
    }

	bool D3D12Device::SupportsDynamicResources() const noexcept
    {
        if (SUCCEEDED(FeatureSupport.GetStatus()))
        {
            return FeatureSupport.HighestShaderModel() >= D3D_SHADER_MODEL_6_6 ||
                   FeatureSupport.ResourceBindingTier() >= D3D12_RESOURCE_BINDING_TIER_3;
        }
        return false;
    }

	bool D3D12Device::SupportsMeshShaders() const noexcept
    {
        if (SUCCEEDED(FeatureSupport.GetStatus()))
        {
            return FeatureSupport.MeshShaderTier() >= D3D12_MESH_SHADER_TIER_1;
        }
        return false;
    }

	void D3D12Device::OnBeginFrame() { LinkedDevice.OnBeginFrame(); }

    void D3D12Device::OnEndFrame() { LinkedDevice.OnEndFrame(); }

	void D3D12Device::BeginCapture(const std::filesystem::path& Path) const
    {
        PIXCaptureParameters CaptureParameters          = {};
        CaptureParameters.GpuCaptureParameters.FileName = Path.c_str();
        CaptureStatus                                   = PIXBeginCapture(PIX_CAPTURE_GPU, &CaptureParameters);
    }

    void D3D12Device::EndCapture() const
    {
        if (SUCCEEDED(CaptureStatus))
        {
            PIXEndCapture(FALSE);
        }
    }

	void D3D12Device::WaitIdle() { LinkedDevice.WaitIdle(); }

	D3D12SyncHandle D3D12Device::DStorageSubmit(DSTORAGE_REQUEST_SOURCE_TYPE Type)
    {
        UINT64 FenceValue = DStorageFence.Signal(DStorageQueues[Type].Get());
        DStorageQueues[Type]->Submit();
        return D3D12SyncHandle {&DStorageFence, FenceValue};
    }

    D3D12RootSignature D3D12Device::CreateRootSignature(RootSignatureDesc& Desc)
    {
        return D3D12RootSignature(this, Desc);
    }

    D3D12RaytracingPipelineState D3D12Device::CreateRaytracingPipelineState(RaytracingPipelineStateDesc& Desc)
    {
        return D3D12RaytracingPipelineState(this, Desc);
    }

    // ======================================== Private ========================================
    void D3D12Device::ReportLiveObjects()
    {
#ifdef _DEBUG
        Microsoft::WRL::ComPtr<IDXGIDebug> Debug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&Debug))))
        {
            Debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
        }
#endif
    }

    void D3D12Device::OnDeviceRemoved(PVOID Context, BOOLEAN)
    {
        auto    D3D12Device   = static_cast<ID3D12Device*>(Context);
        HRESULT RemovedReason = D3D12Device->GetDeviceRemovedReason();
        if (FAILED(RemovedReason))
        {
            Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedData> Dred;
            if (SUCCEEDED(D3D12Device->QueryInterface(IID_PPV_ARGS(&Dred))))
            {
                D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT AutoBreadcrumbsOutput = {};
                D3D12_DRED_PAGE_FAULT_OUTPUT       PageFaultOutput       = {};

                if (SUCCEEDED(Dred->GetAutoBreadcrumbsOutput(&AutoBreadcrumbsOutput)))
                {
                    for (const D3D12_AUTO_BREADCRUMB_NODE* Node = AutoBreadcrumbsOutput.pHeadAutoBreadcrumbNode; Node;
                         Node                                   = Node->pNext)
                    {
                        INT32 LastCompletedOp = static_cast<INT32>(*Node->pLastBreadcrumbValue);

                        auto CommandListName =
                            Node->pCommandListDebugNameW ? Node->pCommandListDebugNameW : L"<unknown>";
                        auto CommandQueueName =
                            Node->pCommandQueueDebugNameW ? Node->pCommandQueueDebugNameW : L"<unknown>";
                        LOG_ERROR(L"({0} Commandlist {1} on CommandQueue {2}, {3} completed of {4})",
                                  L"[DRED]",
                                  CommandListName,
                                  CommandQueueName,
                                  LastCompletedOp,
                                  Node->BreadcrumbCount);

                        INT32 FirstOp = std::max(LastCompletedOp - 5, 0);
                        INT32 LastOp  = std::min(LastCompletedOp + 5, std::max(INT32(Node->BreadcrumbCount) - 1, 0));

                        for (INT32 Op = FirstOp; Op <= LastOp; ++Op)
                        {
                            D3D12_AUTO_BREADCRUMB_OP BreadcrumbOp = Node->pCommandHistory[Op];
                            LOG_ERROR(L"(    Op: {0:d}, {1} {2})",
                                      Op,
                                      GetAutoBreadcrumbOpString(BreadcrumbOp),
                                      Op + 1 == LastCompletedOp ? TEXT("- Last completed") : TEXT(""));
                        }
                    }
                }

                if (SUCCEEDED(Dred->GetPageFaultAllocationOutput(&PageFaultOutput)))
                {
                    LOG_ERROR(L"[DRED] PageFault at VA GPUAddress: {0:x};", PageFaultOutput.PageFaultVA);

                    LOG_ERROR(L"[DRED] Active objects with VA ranges that match the faulting VA:");
                    for (const D3D12_DRED_ALLOCATION_NODE* Node = PageFaultOutput.pHeadExistingAllocationNode; Node;
                         Node                                   = Node->pNext)
                    {
                        auto ObjectName = Node->ObjectNameW ? Node->ObjectNameW : L"<unknown>";
                        LOG_ERROR(
                            L"    Name: {} (Type: {})", ObjectName, GetDredAllocationTypeString(Node->AllocationType));
                    }

                    LOG_ERROR(L"[DRED] Recent freed objects with VA ranges that match the faulting VA:");
                    for (const D3D12_DRED_ALLOCATION_NODE* Node = PageFaultOutput.pHeadRecentFreedAllocationNode; Node;
                         Node                                   = Node->pNext)
                    {
                        auto ObjectName = Node->ObjectNameW ? Node->ObjectNameW : L"<unknown>";
                        LOG_ERROR(
                            L"    Name: {} (Type: {})", ObjectName, GetDredAllocationTypeString(Node->AllocationType));
                    }
                }
            }
        }
    }

    void D3D12Device::InternalCreateDxgiFactory(bool Debug)
    {
        UINT FactoryFlags = Debug ? DXGI_CREATE_FACTORY_DEBUG : 0;
        // Create DXGIFactory
        VERIFY_D3D12_API(CreateDXGIFactory2(FactoryFlags, IID_PPV_ARGS(&Factory6)));
    }

    void D3D12Device::InitializeDxgiObjects()
    {
        CreateDxgiFactory();

        // Enumerate hardware for an adapter that supports D3D12
        Microsoft::WRL::ComPtr<IDXGIAdapter3> AdapterIterator;
        UINT                                  AdapterId = 0;
        while (SUCCEEDED(Factory6->EnumAdapterByGpuPreference(
            AdapterId, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(AdapterIterator.ReleaseAndGetAddressOf()))))
        {
            if (SUCCEEDED(
                    D3D12CreateDevice(AdapterIterator.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
            {
                Adapter3 = std::move(AdapterIterator);
                if (SUCCEEDED(Adapter3->GetDesc2(&AdapterDesc)))
                {
                    LOG_INFO("Adapter Vendor: {}", GetRHIVendorString(static_cast<RHI_VENDOR>(AdapterDesc.VendorId)));
                    LOG_INFO(L"Adapter: {}", AdapterDesc.Description);
                }
                break;
            }

            ++AdapterId;
        }
    }

    Microsoft::WRL::ComPtr<ID3D12Device> D3D12Device::InitializeDevice(const DeviceOptions& Options)
    {
        InitializeDxgiObjects();

        // Enable the D3D12 debug layer
        if (Options.EnableDebugLayer || Options.EnableGpuBasedValidation)
        {
            // NOTE: Enabling the debug layer after creating the ID3D12Device will cause the DX runtime to remove the
            // device.
            Microsoft::WRL::ComPtr<ID3D12Debug> Debug;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&Debug))))
            {
                if (Options.EnableDebugLayer)
                {
                    Debug->EnableDebugLayer();
                }
            }

            Microsoft::WRL::ComPtr<ID3D12Debug3> Debug3;
            if (SUCCEEDED(Debug->QueryInterface(IID_PPV_ARGS(&Debug3))))
            {
                Debug3->SetEnableGPUBasedValidation(Options.EnableGpuBasedValidation);
            }

            Microsoft::WRL::ComPtr<ID3D12Debug5> Debug5;
            if (SUCCEEDED(Debug->QueryInterface(IID_PPV_ARGS(&Debug5))))
            {
                Debug5->SetEnableAutoName(Options.EnableAutoDebugName);
            }
        }

        if (CVar_Dred)
        {
            Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedDataSettings> DredSettings;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&DredSettings))))
            {
                LOG_INFO("DRED Enabled");
                // Turn on auto-breadcrumbs and page fault reporting.
                DredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
                DredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            }
        }
        else
        {
            LOG_INFO("DRED Disabled");
        }

        Microsoft::WRL::ComPtr<ID3D12Device> Device;
        VERIFY_D3D12_API(D3D12CreateDevice(Adapter3.Get(), Options.FeatureLevel, IID_PPV_ARGS(&Device)));
        return Device;
    }

    CD3DX12FeatureSupport D3D12Device::InitializeFeatureSupport(const DeviceOptions& Options)
    {
        CD3DX12FeatureSupport FeatureSupport;
        if (FAILED(FeatureSupport.Init(Device.Get())))
        {
            LOG_WARN("Failed to initialize CD3DX12FeatureSupport, certain features might be unavailable.");
        }

        if (Options.WaveIntrinsics)
        {
            if (!FeatureSupport.WaveOps())
            {
                LOG_ERROR("Wave operation not supported on device");
            }
        }

        if (Options.Raytracing)
        {
            if (FeatureSupport.RaytracingTier() < D3D12_RAYTRACING_TIER_1_0)
            {
                LOG_ERROR("Raytracing not supported on device");
            }
        }

        // https://microsoft.github.io/DirectX-Specs/d3d/HLSL_ShaderModel6_6.html
        // https://microsoft.github.io/DirectX-Specs/d3d/HLSL_ShaderModel6_6.html#dynamic-resource
        if (Options.DynamicResources)
        {
            if (FeatureSupport.HighestShaderModel() < D3D_SHADER_MODEL_6_6 ||
                FeatureSupport.ResourceBindingTier() < D3D12_RESOURCE_BINDING_TIER_3)
            {
                LOG_ERROR("Dynamic resources not supported on device");
            }
        }

        if (Options.MeshShaders)
        {
            if (FeatureSupport.MeshShaderTier() < D3D12_MESH_SHADER_TIER_1)
            {
                LOG_ERROR("Mesh shaders not supported on device");
            }
        }

        BOOL EnhancedBarriersSupported = FeatureSupport.EnhancedBarriersSupported();
        if (!EnhancedBarriersSupported)
        {
            LOG_ERROR("Enhanced barriers not supported");
        }

        return FeatureSupport;
    }

    D3D12Device::TDescriptorSizeCache D3D12Device::InitializeDescriptorSizeCache()
    {
        TDescriptorSizeCache                 DescriptorSizeCache;
        constexpr D3D12_DESCRIPTOR_HEAP_TYPE Types[] = {D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                                        D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
                                                        D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
                                                        D3D12_DESCRIPTOR_HEAP_TYPE_DSV};
        for (size_t i = 0; i < std::size(Types); ++i)
        {
            DescriptorSizeCache[i] = Device->GetDescriptorHandleIncrementSize(Types[i]);
        }
        return DescriptorSizeCache;
    }

    D3D12Device::Dred::Dred(ID3D12Device* Device) :
        DeviceRemovedWaitHandle(
            INVALID_HANDLE_VALUE) // Don't need to call CloseHandle based on RegisterWaitForSingleObject doc
    {
        if (CVar_Dred)
        {
            // Dred
            // https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device5-removedevice#remarks
            VERIFY_D3D12_API(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&DeviceRemovedFence)));

            DeviceRemovedEvent.create();
            // When a device is removed, it signals all fences to UINT64_MAX, we can use this to register events prior
            // to what happened.
            VERIFY_D3D12_API(DeviceRemovedFence->SetEventOnCompletion(UINT64_MAX, DeviceRemovedEvent.get()));

            RegisterWaitForSingleObject(
                &DeviceRemovedWaitHandle, DeviceRemovedEvent.get(), OnDeviceRemoved, Device, INFINITE, 0);
        }
    }

    D3D12Device::Dred::~Dred()
    {
        if (DeviceRemovedFence)
        {
            // Need to gracefully exit the event
            DeviceRemovedFence->Signal(UINT64_MAX);
            BOOL b = UnregisterWaitEx(DeviceRemovedWaitHandle, INVALID_HANDLE_VALUE);
            assert(b);
        }
    }

}

