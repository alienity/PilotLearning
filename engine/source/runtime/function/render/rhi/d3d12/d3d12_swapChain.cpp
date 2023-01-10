#include "d3d12_swapChain.h"
#include "d3d12_device.h"

namespace RHI
{
    // clang-format off
    D3D12SwapChain::D3D12SwapChain(D3D12Device* Parent, HWND HWnd)
        : D3D12DeviceChild(Parent)
        , WindowHandle(HWnd)
        , p_SwapChain4(InitializeSwapChain())
        , Fence(Parent, 0, D3D12_FENCE_FLAG_NONE)
    {
        // Check display HDR support and initialize ST.2084 support to match the display's support.
        DisplayHDRSupport();
        EnableST2084 = HDRSupport;

        DXGI_SWAP_CHAIN_DESC1 Desc = {};
        p_SwapChain4->GetDesc1(&Desc);
        Resize(Desc.Width, Desc.Height);
    }
    // clang-format on

    D3D12SwapChain::~D3D12SwapChain()
    {
        if (SyncHandle)
        {
            SyncHandle.WaitForCompletion();
        }
    }

    void D3D12SwapChain::DisplayHDRSupport()
    {
        // If the display's advanced color state has changed (e.g. HDR display plug/unplug, or OS HDR setting on/off),
        // then this app's DXGI factory is invalidated and must be created anew in order to retrieve up-to-date display
        // information.
        if (!Parent->GetDxgiFactory6()->IsCurrent())
        {
            Parent->CreateDxgiFactory();
        }

        IDXGIAdapter3* Adapter = Parent->GetDxgiAdapter3();

        // Compute the overlay area of two rectangles, A and B.
        // (ax1, ay1) = left-top coordinates of A; (ax2, ay2) = right-bottom coordinates of A
        // (bx1, by1) = left-top coordinates of B; (bx2, by2) = right-bottom coordinates of B
        auto ComputeIntersectionArea = [](int ax1, int ay1, int ax2, int ay2, int bx1, int by1, int bx2, int by2) {
            return std::max(0, std::min(ax2, bx2) - std::max(ax1, bx1)) *
                   std::max(0, std::min(ay2, by2) - std::max(ay1, by1));
        };

        // Iterate through the DXGI outputs associated with the DXGI adapter,
        // and find the output whose bounds have the greatest overlap with the
        // app window (i.e. the output for which the intersection area is the
        // greatest).

        UINT                                Index = 0;
        Microsoft::WRL::ComPtr<IDXGIOutput> OutputIterator;
        Microsoft::WRL::ComPtr<IDXGIOutput> BestOutput;
        float                               BestIntersectArea = -1;

        while (SUCCEEDED(Adapter->EnumOutputs(Index, &OutputIterator)))
        {
            // Get the retangle bounds of the app window
            int ax1 = WindowBounds.left;
            int ay1 = WindowBounds.top;
            int ax2 = WindowBounds.right;
            int ay2 = WindowBounds.bottom;

            // Get the rectangle bounds of current output
            DXGI_OUTPUT_DESC desc;
            VERIFY_D3D12_API(OutputIterator->GetDesc(&desc));
            RECT r   = desc.DesktopCoordinates;
            int  bx1 = r.left;
            int  by1 = r.top;
            int  bx2 = r.right;
            int  by2 = r.bottom;

            // Compute the intersection
            float IntersectArea = static_cast<float>(ComputeIntersectionArea(ax1, ay1, ax2, ay2, bx1, by1, bx2, by2));
            if (IntersectArea > BestIntersectArea)
            {
                BestOutput        = OutputIterator;
                BestIntersectArea = IntersectArea;
            }

            Index++;
        }

        // Having determined the output (display) upon which the app is primarily being
        // rendered, retrieve the HDR capabilities of that display by checking the color space.
        Microsoft::WRL::ComPtr<IDXGIOutput6> Output6;
        if (SUCCEEDED(BestOutput->QueryInterface(IID_PPV_ARGS(&Output6))))
        {
            DXGI_OUTPUT_DESC1 Desc1 = {};
            if (SUCCEEDED(Output6->GetDesc1(&Desc1)))
            {
                HDRSupport = Desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
            }
        }
    }

    void D3D12SwapChain::EnsureSwapChainColorSpace(BitDepth BitDepth, bool EnableST2084)
    {
        DXGI_COLOR_SPACE_TYPE ColorSpace;
        switch (BitDepth)
        {
            case _8:
                DisplayCurve = sRGB;
                ColorSpace   = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
                break;

            case _10:
                DisplayCurve = EnableST2084 ? ST2084 : sRGB;
                ColorSpace =
                    EnableST2084 ? DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 : DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
                break;

            case _16:
                DisplayCurve = None;
                ColorSpace   = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
                break;
        }

        if (CurrentColorSpace != ColorSpace)
        {
            UINT ColorSpaceSupport = 0;
            if (SUCCEEDED(p_SwapChain4->CheckColorSpaceSupport(ColorSpace, &ColorSpaceSupport)) &&
                (ColorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) ==
                    DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT)
            {
                VERIFY_D3D12_API(p_SwapChain4->SetColorSpace1(ColorSpace));
                CurrentColorSpace = ColorSpace;
            }
        }
    }

    D3D12Texture* D3D12SwapChain::GetBackBuffer(UINT Index) { return p_BackBuffers[Index].get(); }

    D3D12Texture* D3D12SwapChain::GetCurrentBackBufferResource()
    {
        //UINT backBufferIndex = p_SwapChain4->GetCurrentBackBufferIndex();
        return p_BackBuffers[m_CurrentBackBufferIndex].get();
    }

    RHIViewport D3D12SwapChain::GetViewport() const noexcept
    {
        return RHIViewport {0.0f, 0.0f, static_cast<FLOAT>(Width), static_cast<FLOAT>(Height)};
    }

    RHIRect D3D12SwapChain::GetScissorRect() const noexcept
    {
        return RHIRect {0, 0, static_cast<long>(Width), static_cast<long>(Height)};
    }

    void D3D12SwapChain::Resize(UINT Width, UINT Height)
    {
        if (SyncHandle)
        {
            SyncHandle.WaitForCompletion();
            SyncHandle = nullptr;
        }

        GetWindowRect(WindowHandle, &WindowBounds);

        for (size_t i = 0; i < 3; i++)
        {
            p_BackBuffers[i] = nullptr;
        }

        if (this->Width != Width || this->Height != Height)
        {
            this->Width  = Width;
            this->Height = Height;

            assert(Width > 0 && Height > 0);
        }

        DXGI_SWAP_CHAIN_DESC1 Desc = {};
        VERIFY_D3D12_API(p_SwapChain4->GetDesc1(&Desc));
        VERIFY_D3D12_API(p_SwapChain4->ResizeBuffers(0, Width, Height, DXGI_FORMAT_UNKNOWN, Desc.Flags));

        EnsureSwapChainColorSpace(CurrentBitDepth, EnableST2084);

        for (UINT i = 0; i < BackBufferCount; ++i)
        {
            Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
            VERIFY_D3D12_API(p_SwapChain4->GetBuffer(i, IID_PPV_ARGS(&Resource)));
            p_BackBuffers[i] = D3D12Texture::CreateFromSwapchain(GetParentDevice()->GetLinkedDevice(),
                                                                 std::move(Resource),
                                                                 D3D12_RESOURCE_STATE_PRESENT,
                                                                 L"SwapChainBackBuffer");
        }
    }

	void D3D12SwapChain::Present(bool VSync, IPresent& Present)
    {
        Present.PrePresent();
        {
            UINT    SyncInterval = VSync ? 1u : 0u;
            UINT    PresentFlags = (TearingSupport && !VSync) ? DXGI_PRESENT_ALLOW_TEARING : 0u;
            HRESULT Result       = p_SwapChain4->Present(SyncInterval, PresentFlags);
        }
        Present.PostPresent();

        UINT64 ValueToWaitFor = Fence.Signal(GetParentDevice()->GetLinkedDevice()->GetGraphicsQueue());
        SyncHandle            = D3D12SyncHandle(&Fence, ValueToWaitFor);

        m_CurrentBackBufferIndex = (m_CurrentBackBufferIndex + 1) % BackBufferCount;
    }

    Microsoft::WRL::ComPtr<IDXGISwapChain4> D3D12SwapChain::InitializeSwapChain()
    {
        IDXGIFactory6*      Factory      = GetParentDevice()->GetDxgiFactory6();
        ID3D12CommandQueue* CommandQueue = GetParentDevice()->GetLinkedDevice()->GetGraphicsQueue()->GetCommandQueue();

        // Check tearing support
        BOOL AllowTearing = FALSE;
        if (FAILED(
                Factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &AllowTearing, sizeof(AllowTearing))))
        {
            AllowTearing = FALSE;
        }
        TearingSupport = AllowTearing == TRUE;

        // Width/Height can be set to 0
        DXGI_SWAP_CHAIN_DESC1 Desc = {0,
                                      0,
                                      Format,
                                      FALSE,
                                      DXGI_SAMPLE_DESC {1, 0},
                                      DXGI_USAGE_RENDER_TARGET_OUTPUT,
                                      BackBufferCount,
                                      DXGI_SCALING_NONE,
                                      DXGI_SWAP_EFFECT_FLIP_DISCARD,
                                      DXGI_ALPHA_MODE_UNSPECIFIED,
                                      TearingSupport ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u};

        Microsoft::WRL::ComPtr<IDXGISwapChain1> SwapChain1;
        Microsoft::WRL::ComPtr<IDXGISwapChain4> SwapChain4;
        VERIFY_D3D12_API(Factory->CreateSwapChainForHwnd(
            CommandQueue, WindowHandle, &Desc, nullptr, nullptr, SwapChain1.GetAddressOf()));
        // No full screen via alt + enter
        VERIFY_D3D12_API(Factory->MakeWindowAssociation(WindowHandle, DXGI_MWA_NO_ALT_ENTER));
        VERIFY_D3D12_API(SwapChain1->QueryInterface(IID_PPV_ARGS(SwapChain4.ReleaseAndGetAddressOf())));

        return SwapChain4;
    }
}

