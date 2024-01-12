#pragma once

#include "Logger.hpp"

namespace CTinyC {
    template<typename... Ts>
    ::winrt::hstring logsrc(std::wstring_view src, int line, int column, const std::wformat_string<Ts...> fmt, Ts&&... ts) {
        return ::winrt::hstring{ std::format(L"{}({},{}): ", src, line, column) +
            std::format(fmt, std::forward<Ts>(ts)...) };
    }

    inline uint32_t read_u32(HANDLE h) {
        uint32_t buf;
        DWORD cnt;
        ::winrt::check_bool(ReadFile(h, &buf, sizeof buf, &cnt, nullptr));
        if (cnt != sizeof buf) {
            throw ::winrt::hresult_error(E_FAIL, L"read size is not 4");
        }
        return buf;
    }
    inline void read_bytes(HANDLE h, void* buf, size_t len) {
        DWORD cnt;
        ::winrt::check_bool(ReadFile(h, buf, len, &cnt, nullptr));
        if (cnt != len) {
            throw ::winrt::hresult_error(E_FAIL, L"read is not complete");
        }
    }
    inline void write_u32(HANDLE h, uint32_t dw) {
        DWORD cnt;
        ::winrt::check_bool(WriteFile(h, &dw, sizeof dw, &cnt, nullptr));
        if (cnt != sizeof dw) {
            throw ::winrt::hresult_error(E_FAIL, L"write size is not 4");
        }
    }
    inline void write_bytes(HANDLE h, const void* buf, size_t len) {
        DWORD cnt;
        ::winrt::check_bool(WriteFile(h, buf, len, &cnt, nullptr));
        if (cnt != len) {
            throw ::winrt::hresult_error(E_FAIL, L"write is not complete");
        }
    }
}
