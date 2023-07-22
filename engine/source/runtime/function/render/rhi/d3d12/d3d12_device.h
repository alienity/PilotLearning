#pragma once
#include "d3d12_core.h"
#include "../rhi_core.h"

namespace RHI
{
    class D3D12LinkedDevice;
    class D3D12PipelineLibrary;
    class D3D12PipelineState;
    class RaytracingPipelineStateDesc;
    class D3D12RaytracingPipelineState;
    class RootSignatureDesc;
    class D3D12RootSignature;
    class D3D12Fence;

    struct DeviceOptions
    {
        bool EnableDebugLayer;
        bool EnableGpuBasedValidation;
        bool EnableAutoDebugName;

        bool                  WaveIntrinsics;
        bool                  Raytracing;
        bool                  DynamicResources;
        bool                  MeshShaders;
        std::filesystem::path PsoCachePath;
    };

    struct RootParameters
    {
        struct DescriptorTable
        {
            enum
            {
                ShaderResourceDescriptorTable,
                UnorderedAccessDescriptorTable,
                SamplerDescriptorTable,
                NumRootParameters
            };
        };
    };

    class D3D12Device
    {
    public:
        explicit D3D12Device(const DeviceOptions& Options);
        ~D3D12Device();

        void CreateDxgiFactory();

        [[nodiscard]] auto GetDxgiFactory6() const noexcept -> IDXGIFactory6* { return m_Factory6.Get(); }
        [[nodiscard]] auto GetDxgiAdapter3() const noexcept -> IDXGIAdapter3* { return m_Adapter3.Get(); }
        [[nodiscard]] auto GetD3D12Device() const noexcept -> ID3D12Device* { return m_Device.Get(); }
        [[nodiscard]] auto GetD3D12Device1() const noexcept -> ID3D12Device1* { return m_Device1.Get(); }
        [[nodiscard]] auto GetD3D12Device5() const noexcept -> ID3D12Device5* { return m_Device5.Get(); }
        [[nodiscard]] auto GetDStorageFactory() const noexcept -> IDStorageFactory* { return m_DStorageFactory.Get(); }
        [[nodiscard]] auto GetDStorageQueue(DSTORAGE_REQUEST_SOURCE_TYPE Type) const noexcept -> IDStorageQueue*
        {
            return m_DStorageQueues[Type].Get();
        }
        [[nodiscard]] auto GetDStorageFence() noexcept -> D3D12Fence* { return m_DStorageFence.get(); }
        [[nodiscard]] auto GetAllNodeMask() const noexcept -> D3D12NodeMask { return m_AllNodeMask; }
        [[nodiscard]] auto GetSizeOfDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type) const noexcept -> UINT
        {
            return m_DescriptorSizeCache[Type];
        }
        [[nodiscard]] auto GetLinkedDevice() noexcept -> D3D12LinkedDevice* { return m_LinkedDevice.get(); }
        [[nodiscard]] bool AllowAsyncPsoCompilation() const noexcept;
        //[[nodiscard]] auto GetPsoCompilationThreadPool() const noexcept -> ThreadPool*
        //{
        //    return PsoCompilationThreadPool.get();
        //}
        [[nodiscard]] auto GetPipelineLibrary() const noexcept -> D3D12PipelineLibrary* { return m_Library.get(); }

        [[nodiscard]] bool SupportsWaveIntrinsics() const noexcept;
        [[nodiscard]] bool SupportsRaytracing() const noexcept;
        [[nodiscard]] bool SupportsDynamicResources() const noexcept;
        [[nodiscard]] bool SupportsMeshShaders() const noexcept;

        void OnBeginFrame();
        void OnEndFrame();

        void BeginCapture(const std::filesystem::path& Path) const;
        void EndCapture() const;

        void WaitIdle();

        [[nodiscard]] D3D12SyncHandle DStorageSubmit(DSTORAGE_REQUEST_SOURCE_TYPE Type);

        [[nodiscard]] D3D12RootSignature CreateRootSignature(RootSignatureDesc& Desc);

        template<typename PipelineStateStream>
        [[nodiscard]] D3D12PipelineState CreatePipelineState(std::wstring Name, PipelineStateStream& Stream)
        {
            PipelineStateStreamDesc Desc;
            Desc.SizeInBytes                   = sizeof(Stream);
            Desc.pPipelineStateSubobjectStream = &Stream;
            return D3D12PipelineState(this, Name, Desc);
        }

        [[nodiscard]] D3D12RaytracingPipelineState CreateRaytracingPipelineState(RaytracingPipelineStateDesc& Desc);

    private:
        using TDescriptorSizeCache = std::array<UINT, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES>;

        static void ReportLiveObjects();
        static void OnDeviceRemoved(PVOID Context, BOOLEAN);

        template<typename T>
        Microsoft::WRL::ComPtr<T> DeviceQueryInterface()
        {
            Microsoft::WRL::ComPtr<T> Interface;
            VERIFY_D3D12_API(m_Device->QueryInterface(IID_PPV_ARGS(&Interface)));
            return Interface;
        }

        DWORD dxgiFactoryFlags = 0;

        void                                 InternalCreateDxgiFactory(bool Debug);
        Microsoft::WRL::ComPtr<ID3D12Device> InitializeDevice(const DeviceOptions& Options);
        CD3DX12FeatureSupport                InitializeFeatureSupport(const DeviceOptions& Options);
        TDescriptorSizeCache                 InitializeDescriptorSizeCache();

    private:
        //struct ReportLiveObjectGuard
        //{
        //    ~ReportLiveObjectGuard() { D3D12Device::ReportLiveObjects(); }
        //} MemoryGuard;

        /*struct WinPix
        {
            WinPix()
                : Module(PIXLoadLatestWinPixGpuCapturerLibrary())
            {
            }
            ~WinPix()
            {
                if (Module)
                {
                    FreeLibrary(Module);
                }
            }

            HMODULE Module;
        } WinPix;*/

        Microsoft::WRL::ComPtr<IDXGIFactory6> m_Factory6;
        Microsoft::WRL::ComPtr<IDXGIAdapter3> m_Adapter3;
        DXGI_ADAPTER_DESC2                    m_AdapterDesc = {};

        Microsoft::WRL::ComPtr<ID3D12Device>  m_Device;
        Microsoft::WRL::ComPtr<ID3D12Device1> m_Device1;
        Microsoft::WRL::ComPtr<ID3D12Device5> m_Device5;

        Microsoft::WRL::ComPtr<IDStorageFactory> m_DStorageFactory;
        Microsoft::WRL::ComPtr<IDStorageQueue>   m_DStorageQueues[2];
        std::shared_ptr<D3D12Fence>              m_DStorageFence;

        D3D12NodeMask         m_AllNodeMask;
        CD3DX12FeatureSupport m_FeatureSupport;
        TDescriptorSizeCache  m_DescriptorSizeCache;

        struct Dred
        {
            Dred() = default;
            Dred(ID3D12Device* Device);
            ~Dred();

            Microsoft::WRL::ComPtr<ID3D12Fence> DeviceRemovedFence;
            HANDLE                              DeviceRemovedWaitHandle;
            wil::unique_event                   DeviceRemovedEvent;
        };

        std::shared_ptr<Dred> m_Dred;

        // Represents a single node
        // TODO: Add Multi-Adapter support
        std::shared_ptr<D3D12LinkedDevice> m_LinkedDevice;
        // std::unique_ptr<ThreadPool>           PsoCompilationThreadPool;
        std::unique_ptr<D3D12PipelineLibrary> m_Library;

        mutable HRESULT m_CaptureStatus = S_FALSE;
    };
} // namespace RHI