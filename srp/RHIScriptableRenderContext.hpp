
namespace RHI
{
    class RHISyncHandle;
    class RHICommandBuffer;

    class RHIScriptableRenderContext
    {
        RHISyncHandle Execute(RHICommandBuffer* pCommandBuffer);
        void WaitForSyncHandle(const RHISyncHandle& SyncHandle);
        
        void Submit();
    }


}
