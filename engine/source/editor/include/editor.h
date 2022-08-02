#pragma once

#include "runtime/core/math/moyu_math.h"

#include <memory>

namespace Pilot
{
    class EditorUI;
    class PilotEngine;

    class PilotEditor
    {
        friend class EditorUI;

    public:
        PilotEditor();
        virtual ~PilotEditor();

        void initialize(PilotEngine* engine_runtime);
        void clear();

        void run();

    protected:
        std::shared_ptr<EditorUI> m_editor_ui;
        PilotEngine*              m_engine_runtime {nullptr};
    };
} // namespace Pilot
