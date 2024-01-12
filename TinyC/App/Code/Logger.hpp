#pragma once

namespace CTinyC {
    struct Logger {
        enum class Severity {
            Trace,
            Debug,
            Info,
            Warn,
            Error,
        };

        virtual void log(Severity severity, ::winrt::hstring const& str) = 0;

        void trace(::winrt::param::hstring const& str) { return log(Severity::Trace, str); }
        void debug(::winrt::param::hstring const& str) { return log(Severity::Debug, str); }
        void info(::winrt::param::hstring const& str) { return log(Severity::Info, str); }
        void warn(::winrt::param::hstring const& str) { return log(Severity::Warn, str); }
        void error(::winrt::param::hstring const& str) { return log(Severity::Error, str); }
    };
}
