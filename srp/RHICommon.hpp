#include <vector>

namespace RHI
{

    typedef float FLOAT;
    typedef unsigned int UINT;
    typedef int INT;
    typedef long long INT64;
    typedef long LONG;

    #define	RHI_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT	( 256 )
    #define	RHI_RESOURCE_BARRIER_ALL_SUBRESOURCES	( 0xffffffff )

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



}
