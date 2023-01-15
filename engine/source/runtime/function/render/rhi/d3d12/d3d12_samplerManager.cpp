#include "d3d12_samplerManager.h"
#include "d3d12_linkedDevice.h"
#include "runtime/core/base/utility.h"
#include <map>

namespace RHI
{

	std::map<size_t, DescriptorHeapAllocation> s_SamplerCache;

    D3D12_CPU_DESCRIPTOR_HANDLE SamplerDesc::CreateDescriptor(D3D12LinkedDevice* pLinkedDevice)
    {
        size_t hashValue = Utility::Hash64(this, sizeof(SamplerDesc));
        auto   iter      = s_SamplerCache.find(hashValue);
        if (iter != s_SamplerCache.end())
        {
            return iter->second.GetCpuHandle(0);
        }

        GPUDescriptorHeap* pSamplerDescriptorHeap = pLinkedDevice->GetSamplerDescriptorHeap();
        DescriptorHeapAllocation descriptorAllocation = pSamplerDescriptorHeap->Allocate(1);

        D3D12_CPU_DESCRIPTOR_HANDLE Handle = descriptorAllocation.GetCpuHandle();

        pLinkedDevice->GetDevice()->CreateSampler(this, Handle);

        s_SamplerCache[hashValue] = std::move(descriptorAllocation);

        return Handle;
    }

    void SamplerDesc::RetireAll(D3D12LinkedDevice* pLinkedDevice)
    {
        for (std::map<size_t, DescriptorHeapAllocation>::iterator it = s_SamplerCache.begin(); it != s_SamplerCache.end(); ++it)
        {
            pLinkedDevice->Retire(std::move(it->second));
        }
        s_SamplerCache.clear();
    }

}