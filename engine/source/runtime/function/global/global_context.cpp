#include "runtime/function/global/global_context.h"

#include "core/log/log_system.h"

namespace Pilot
{
    RuntimeGlobalContext g_runtime_global_context;

    void RuntimeGlobalContext::startSystems(const EngineInitParams& init_params)
    {
        m_logger_system = std::make_shared<LogSystem>();

    }

    void RuntimeGlobalContext::shutdownSystems()
    {
        m_logger_system.reset();
    }
} // namespace Pilot