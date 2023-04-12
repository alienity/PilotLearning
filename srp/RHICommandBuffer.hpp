
typedef float FLOAT;
typedef unsigned int UINT;
typedef int INT;

namespace RHI
{
    class RHISyncHandle;

    struct DWParam
    {
        DWParam(FLOAT f) : Float(f) {}
        DWParam(UINT u) : Uint(u) {}
        DWParam(INT i) : Int(i) {}

        void operator=(FLOAT f) { Float = f; }
        void operator=(UINT u) { Uint = u; }
        void operator=(INT i) { Int = i; }

        union
        {
            FLOAT Float;
            UINT  Uint;
            INT   Int;
        };
    };

    class RHICommandBuffer
    {
        RHISyncHandle Execute();

    };





}



