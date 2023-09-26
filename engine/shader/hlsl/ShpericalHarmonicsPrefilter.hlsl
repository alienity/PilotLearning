#include "IBLHelper.hlsli"
#include "SphericalHarmonicsHelper.hlsli"
#include "CommonMath.hlsli"

cbuffer Constants : register(b0)
{
    int _IBLRadiansTexIndex;
    int _SHOutputBufferIndex;
};

SamplerState defaultSampler : register(s10);

[numthreads(1, 1, 1)]
void CSMain(uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID) {

    TextureCube<float4> m_IBLRadians = ResourceDescriptorHeap[_IBLRadiansTexIndex];
    RWStructuredBuffer<float4> m_OutputBuffer = ResourceDescriptorHeap[_SHOutputBufferIndex];

    uint width, height, elements;
    m_IBLRadians.GetDimensions(0, width, height, elements);

    float3 SH[9];

    for(uint f = 0; f < elements; ++f)
    {
        for(uint x = 0; x < width; ++x)
        {
            for(uint y = 0; y < height; ++y)
            {
                float2 texXY = float2((x + 0.5f)/(float)width, (y + 0.5f)/(float)height) * 2.0f - 1.0f;

                float3 s = getDirectionForCubemap(f, texXY);

                // sample a color
                float3 color = m_IBLRadians.Sample(defaultSampler, s).rgb;
                // take solid angle into account
                color *= solidAngle(width, x, y);
            
                float SHb[9];
                computeShBasisBand2(SHb, s);

                // apply coefficients to the sampled color
                for (uint i=0 ; i<9 ; i++)
                {
                    SH[i] += color * SHb[i];
                }

            }
        }
    }

    // precompute the scaling factor K
    float K[9];
    KiBand2(K);

    // apply truncated cos (irradiance)
    for (uint l = 0; l < 3; l++) {
        const float truncatedCosSh = computeTruncatedCosSh(uint(l));
        K[SHindex(0, l)] *= truncatedCosSh;
        for (uint m = 1; m <= l; m++) {
            K[SHindex(-m, l)] *= truncatedCosSh;
            K[SHindex( m, l)] *= truncatedCosSh;
        }
    }

    // apply all the scale factors
    for (uint i = 0; i < 9; i++)
    {
        SH[i] *= K[i];
    }

    preprocessSH(SH);

    GroupMemoryBarrierWithGroupSync();

    if (DTid.x == 0 && DTid.y == 0 && DTid.z == 0)
    {
        float _sh[27];
        uint  _shIndex = 0;
        uint  i;
        for (i = 0; i < 9; ++i)
        {
            for (uint j = 0; j < 3; ++j)
            {
                _sh[_shIndex++] = SH[i][j];
            }
        }

        float4 _packSH[7];
        PackSH(_sh, _packSH);

        for (i = 0; i < 7; ++i)
        {
            m_OutputBuffer[i] = _packSH[i];
        }
    }
}