#pragma once

#include "runtime/core/math/moyu_math2.h"

namespace MoYu
{
    /// <summary>
    /// An utility class to compute samples on the Halton sequence.
    /// https://en.wikipedia.org/wiki/Halton_sequence
    /// </summary>
    class HaltonSequence
    {
    public:
        /// <summary>
        /// Gets a deterministic sample in the Halton sequence.
        /// </summary>
        /// <param name="index">The index in the sequence.</param>
        /// <param name="radix">The radix of the sequence.</param>
        /// <returns>A sample from the Halton sequence.</returns>
        inline static float Get(int index, int radix)
        {
            float result = 0.0f;
            float fraction = 1.0f / radix;

            while (index > 0)
            {
                result += (index % radix) * fraction;

                index /= radix;
                fraction /= radix;
            }

            return result;
        }
    };


} // namespace MoYu
