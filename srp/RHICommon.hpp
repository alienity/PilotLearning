#include <vector>
#include <string>
#include <optional>
#include <memory>

#incldue "glm.hpp"

namespace RHI
{
    #define DEFINE_RHI_ENUM_FLAG_OPERATORS(ENUMTYPE) \
    extern "C++" { \
    inline ENUMTYPE operator | (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a) | ((int)b)); } \
    inline ENUMTYPE &operator |= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) |= ((int)b)); } \
    inline ENUMTYPE operator & (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a) & ((int)b)); } \
    inline ENUMTYPE &operator &= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) &= ((int)b)); } \
    inline ENUMTYPE operator ~ (ENUMTYPE a) { return ENUMTYPE(~((int)a)); } \
    inline ENUMTYPE operator ^ (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a) ^ ((int)b)); } \
    inline ENUMTYPE &operator ^= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) ^= ((int)b)); } \
    }


    typedef signed char    INT8;
    typedef signed short   INT16;
    typedef signed int     INT32;
    typedef signed long    INT64;
    typedef unsigned char  UINT8;
    typedef unsigned short UINT16;
    typedef unsigned int   UINT32;
    typedef unsigned long  UINT64;
    typedef long           LONG;
    typedef unsigned long  SIZE_T;
    typedef float          FLOAT;
    typedef bool           BOOL;
    
    typedef INT32 INT;
    typedef UINT32 UINT;

    typedef std::byte BYTE;

    typedef glm::vec3 FLOAT3;
    typedef glm::vec4 FLOAT4;

    typedef glm::vec4 COLOR;

    #define TRUE 1
    #define FALSE 0

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
