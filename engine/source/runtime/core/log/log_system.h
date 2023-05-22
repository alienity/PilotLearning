#pragma once

#include <spdlog/spdlog.h>

#include <cstdint>
#include <stdexcept>

namespace MoYu
{

    class LogSystem final
    {
    public:
        enum class LogLevel : uint8_t
        {
            Trace,
            Debug,
            Info,
            Warn,
            Error,
            Fatal
        };

    public:
        LogSystem();
        ~LogSystem();

        template<typename... TARGS>
        void log(LogLevel level, TARGS&&... args)
        {
            switch (level)
            {
                case LogLevel::Trace:
                    m_logger->trace(std::forward<TARGS>(args)...);
                    break;
                case LogLevel::Debug:
                    m_logger->debug(std::forward<TARGS>(args)...);
                    break;
                case LogLevel::Info:
                    m_logger->info(std::forward<TARGS>(args)...);
                    break;
                case LogLevel::Warn:
                    m_logger->warn(std::forward<TARGS>(args)...);
                    break;
                case LogLevel::Error:
                    m_logger->error(std::forward<TARGS>(args)...);
                    break;
                case LogLevel::Fatal:
                    m_logger->critical(std::forward<TARGS>(args)...);
                    fatalCallback(std::forward<TARGS>(args)...);
                    break;
                default:
                    break;
            }
        }

        template<typename... TARGS>
        void fatalCallback(TARGS&&... args)
        {
            //std::wstring format_str = fmt::format(std::forward<TARGS>(args)...);
            //string out_str(format_str.begin(), format_str.end());
            //throw std::runtime_error(out_str);
        }

    private:
        std::shared_ptr<spdlog::logger> m_logger;
    };

} // namespace MoYu