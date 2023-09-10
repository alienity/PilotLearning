/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef IBL_CUBEMAPUTILSIMPL_H
#define IBL_CUBEMAPUTILSIMPL_H

#include "CubemapUtils.h"

namespace MoYu {
namespace ibl {

    template<typename STATE>
    void CubemapUtils::process(
            Cubemap& cm,
            CubemapUtils::ScanlineProc<STATE> proc,
            ReduceProc<STATE> reduce,
            const STATE& prototype)
    {
        const size_t dim = cm.getDimensions();

        STATE s;
        s = prototype;

        for (size_t faceIndex = 0; faceIndex < 6; faceIndex++) {
            const Cubemap::Face f = (Cubemap::Face)faceIndex;
            Image& image(cm.getImageForFace(f));
            for (size_t y = 0; y < dim; y++) {
                Cubemap::Texel* data = static_cast<Cubemap::Texel*>(image.getPixelRef(0, y));
                proc(s, y, f, data, dim);
            }
        }
        reduce(s);
    }

} // namespace ibl
} // namespace MoYu

#endif // IBL_CUBEMAPUTILSIMPL_H
