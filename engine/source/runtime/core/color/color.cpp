#include "color.h"

namespace Pilot
{
    const Color Color::White(1.f, 1.f, 1.f, 1.f);
    const Color Color::Black(0.f, 0.f, 0.f, 0.f);
    const Color Color::Red(1.f, 0.f, 0.f, 1.f);
    const Color Color::Green(0.f, 1.f, 0.f, 1.f);
    const Color Color::Blue(0.f, 1.f, 0.f, 1.f);

    Color Color::ToSRGB() const
    {
        float c[3] = {r, g, b};
        float o[3] = {r, g, b};
        for (size_t i = 0; i < 3; i++)
        {
            float _o = powf(c[i], 1.0f / 2.4f) * 1.055f - 0.055f;
            if (c[i] < 0.0031308f)
                _o = c[i] * 12.92f;
            o[i] = _o;
        }
        return Color(o[0], o[1], o[2], a);
    }

    Color Color::FromSRGB() const
    {
        float c[3] = {r, g, b};
        float o[3] = {r, g, b};
        for (size_t i = 0; i < 3; i++)
        {
            float _o = powf((c[i] + 0.055f) / 1.055f, 2.4f);
            if (c[i] < 0.0031308f)
                _o = c[i] / 12.92f;
            o[i] = _o;
        }
        return Color(o[0], o[1], o[2], a);
    }

    Color Color::ToREC709() const
    {
        float c[3] = {r, g, b};
        float o[3] = {r, g, b};
        for (size_t i = 0; i < 3; i++)
        {
            float _o = powf(c[i], 0.45f) * 1.099f - 0.099f;
            if (c[i] < 0.0018f)
                _o = c[i] * 4.5f;
            o[i] = _o;
        }
        return Color(o[0], o[1], o[2], a);
    }

    Color Color::FromREC709() const
    {
        float c[3] = {r, g, b};
        float o[3] = {r, g, b};
        for (size_t i = 0; i < 3; i++)
        {
            float _o = powf((c[i] + 0.099f) / 1.099f, 1.0f / 0.45f);
            if (c[i] < 0.0081f)
                _o = c[i] / 4.5f;
            o[i] = _o;
        }
        return Color(o[0], o[1], o[2], a);
    }

    uint32_t Color::R10G10B10A2() const
    {
        return 0;
    }

    uint32_t Color::R8G8B8A8() const
    {
        return 0;
    }

    uint32_t Color::R11G11B10F(bool RoundToEven) const
    {
        static const float kMaxVal   = float(1 << 16);
        static const float kF32toF16 = (1.0 / (1ull << 56)) * (1.0 / (1ull << 56));

        union
        {
            float    f;
            uint32_t u;
        } R, G, B;

        R.f = Math::clamp(r, 0.0f, kMaxVal) * kF32toF16;
        G.f = Math::clamp(g, 0.0f, kMaxVal) * kF32toF16;
        B.f = Math::clamp(b, 0.0f, kMaxVal) * kF32toF16;

        if (RoundToEven)
        {
            // Bankers rounding:  2.5 -> 2.0  ;  3.5 -> 4.0
            R.u += 0x0FFFF + ((R.u >> 16) & 1);
            G.u += 0x0FFFF + ((G.u >> 16) & 1);
            B.u += 0x1FFFF + ((B.u >> 17) & 1);
        }
        else
        {
            // Default rounding:  2.5 -> 3.0  ;  3.5 -> 4.0
            R.u += 0x00010000;
            G.u += 0x00010000;
            B.u += 0x00020000;
        }

        R.u &= 0x0FFE0000;
        G.u &= 0x0FFE0000;
        B.u &= 0x0FFC0000;

        return R.u >> 17 | G.u >> 6 | B.u << 4;
    }

    uint32_t Color::R9G9B9E5() const
    {
        static const float kMaxVal = float(0x1FF << 7);
        static const float kMinVal = float(1.f / (1 << 16));

        // Clamp RGB to [0, 1.FF*2^16]
        float _r = Math::clamp(r, 0.0f, kMaxVal);
        float _g = Math::clamp(g, 0.0f, kMaxVal);
        float _b = Math::clamp(b, 0.0f, kMaxVal);

        // Compute the maximum channel, no less than 1.0*2^-15
        float MaxChannel = Math::max(Math::max(_r, _g), Math::max(_b, kMinVal));

        // Take the exponent of the maximum channel (rounding up the 9th bit) and
        // add 15 to it.  When added to the channels, it causes the implicit '1.0'
        // bit and the first 8 mantissa bits to be shifted down to the low 9 bits
        // of the mantissa, rounding the truncated bits.
        union
        {
            float   f;
            int32_t i;
        } R, G, B, E;
        E.f = MaxChannel;
        E.i += 0x07804000; // Add 15 to the exponent and 0x4000 to the mantissa
        E.i &= 0x7F800000; // Zero the mantissa

        // This shifts the 9-bit values we need into the lowest bits, rounding as
        // needed.  Note that if the channel has a smaller exponent than the max
        // channel, it will shift even more.  This is intentional.
        R.f = _r + E.f;
        G.f = _g + E.f;
        B.f = _b + E.f;

        // Convert the Bias to the correct exponent in the upper 5 bits.
        E.i <<= 4;
        E.i += 0x10000000;

        // Combine the fields.  RGB floats have unwanted data in the upper 9
        // bits.  Only red needs to mask them off because green and blue shift
        // it out to the left.
        return E.i | B.i << 18 | G.i << 9 | R.i & 511;
    }

}