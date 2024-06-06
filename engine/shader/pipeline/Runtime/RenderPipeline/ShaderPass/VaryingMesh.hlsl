struct AttributesMesh
{
    float3 positionOS   : POSITION;
#ifdef ATTRIBUTES_NEED_NORMAL
    float3 normalOS     : NORMAL;
#endif
#ifdef ATTRIBUTES_NEED_TANGENT
    float4 tangentOS    : TANGENT; // Store sign in w
#endif
#ifdef ATTRIBUTES_NEED_TEXCOORD0
    float2 uv0          : TEXCOORD0;
#endif
#ifdef ATTRIBUTES_NEED_TEXCOORD1
    float2 uv1          : TEXCOORD1;
#endif
#ifdef ATTRIBUTES_NEED_TEXCOORD2
    float2 uv2          : TEXCOORD2;
#endif
#ifdef ATTRIBUTES_NEED_TEXCOORD3
    float2 uv3          : TEXCOORD3;
#endif
#ifdef ATTRIBUTES_NEED_COLOR
    float4 color        : COLOR;
#endif
};

struct VaryingsMeshToPS
{
    float4 positionCS;
#ifdef VARYINGS_NEED_POSITION_WS
    float3 positionRWS;
#endif
#ifdef VARYINGS_NEED_TANGENT_TO_WORLD
    float3 normalWS;
    float4 tangentWS;  // w contain mirror sign
#endif
#ifdef VARYINGS_NEED_TEXCOORD0
    float2 texCoord0;
#endif
#ifdef VARYINGS_NEED_TEXCOORD1
    float2 texCoord1;
#endif
#ifdef VARYINGS_NEED_TEXCOORD2
    float2 texCoord2;
#endif
#ifdef VARYINGS_NEED_TEXCOORD3
    float2 texCoord3;
#endif
#ifdef VARYINGS_NEED_COLOR
    float4 color;
#endif
};

struct PackedVaryingsMeshToPS
{
    float4 positionCS : SV_Position;

#ifdef VARYINGS_NEED_POSITION_WS
    float3 interpolators0 : TEXCOORD0;
#endif

#ifdef VARYINGS_NEED_TANGENT_TO_WORLD
    float3 interpolators1 : TEXCOORD1;
    float4 interpolators2 : TEXCOORD2;
#endif

    // Allocate only necessary space if shader compiler in the future are able to automatically pack
#ifdef VARYINGS_NEED_TEXCOORD1
    float4 interpolators3 : TEXCOORD3;
#elif defined(VARYINGS_NEED_TEXCOORD0)
    float2 interpolators3 : TEXCOORD3;
#endif

#ifdef VARYINGS_NEED_TEXCOORD3
    float4 interpolators4 : TEXCOORD4;
#elif defined(VARYINGS_NEED_TEXCOORD2)
    float2 interpolators4 : TEXCOORD4;
#endif

#ifdef VARYINGS_NEED_COLOR
    float4 interpolators5 : TEXCOORD5;
#endif
};

// Functions to pack data to use as few interpolator as possible, the ShaderGraph should generate these functions
PackedVaryingsMeshToPS PackVaryingsMeshToPS(VaryingsMeshToPS input)
{
    PackedVaryingsMeshToPS output;
    
    output.positionCS = input.positionCS;

#ifdef VARYINGS_NEED_POSITION_WS
    output.interpolators0 = input.positionRWS;
#endif

#ifdef VARYINGS_NEED_TANGENT_TO_WORLD
    output.interpolators1 = input.normalWS;
    output.interpolators2 = input.tangentWS;
#endif

#ifdef VARYINGS_NEED_TEXCOORD0
    output.interpolators3.xy = input.texCoord0;
#endif
#ifdef VARYINGS_NEED_TEXCOORD1
    output.interpolators3.zw = input.texCoord1;
#endif
#ifdef VARYINGS_NEED_TEXCOORD2
    output.interpolators4.xy = input.texCoord2;
#endif
#ifdef VARYINGS_NEED_TEXCOORD3
    output.interpolators4.zw = input.texCoord3;
#endif

#ifdef VARYINGS_NEED_COLOR
    output.interpolators5 = input.color;
#endif

    return output;
}

FragInputs UnpackVaryingsMeshToFragInputs(PackedVaryingsMeshToPS input)
{
    FragInputs output;
    ZERO_INITIALIZE(FragInputs, output);
    
    // Init to some default value to make the computer quiet (else it output "divide by zero" warning even if value is not used).
    // TODO: this is a really poor workaround, but the variable is used in a bunch of places
    // to compute normals which are then passed on elsewhere to compute other values...
    output.tangentToWorld = k_identity3x3;

    output.positionSS = input.positionCS; // input.positionCS is SV_Position

#ifdef VARYINGS_NEED_POSITION_WS
    output.positionRWS.xyz = input.interpolators0.xyz;
#endif

#ifdef VARYINGS_NEED_TANGENT_TO_WORLD
    float4 tangentWS = float4(input.interpolators2.xyz, input.interpolators2.w > 0.0 ? 1.0 : -1.0); // must not be normalized (mikkts requirement)
    output.tangentToWorld = BuildTangentToWorld(tangentWS, input.interpolators1.xyz);
#endif // VARYINGS_NEED_TANGENT_TO_WORLD

#ifdef VARYINGS_NEED_TEXCOORD0
    output.texCoord0.xy = input.interpolators3.xy;
#endif
#ifdef VARYINGS_NEED_TEXCOORD1
    output.texCoord1.xy = input.interpolators3.zw;
#endif
#ifdef VARYINGS_NEED_TEXCOORD2
    output.texCoord2.xy = input.interpolators4.xy;
#endif
#ifdef VARYINGS_NEED_TEXCOORD3
    output.texCoord3.xy = input.interpolators4.zw;
#endif
#ifdef VARYINGS_NEED_COLOR
    output.color = input.interpolators5;
#endif

    return output;
}
