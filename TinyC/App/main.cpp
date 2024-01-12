#include "pch.h"
#include "App.h"

#include <conio.h>

#include <winrt/Win32Xaml.h>

#include "Code/public.h"
#include "Code/Executor.hpp"

#include "appmodel.h"

using namespace winrt;

uint32_t read_hex_u32_from_env_var(const wchar_t* name) {
    wchar_t buf[8 + 1];
    auto len = GetEnvironmentVariableW(name, buf, 8 + 1);
    wchar_t* end_ptr;
    auto value = std::wcstoll(buf, &end_ptr, 16);
    if (end_ptr != buf + len) {
        throw std::runtime_error("handle value is invalid");
    }
    return static_cast<uint32_t>(value);
}

// Used to execute code from parent pipe
int slave_exec_main() try {
    using namespace CTinyC;

    auto pipein = (HANDLE)read_hex_u32_from_env_var(L"pipein");
    auto pipeout = (HANDLE)read_hex_u32_from_env_var(L"pipeout");

    struct SlaveLogger : CTinyC::Logger {
        SlaveLogger(HANDLE pipein, HANDLE pipeout) : m_pipein(pipein), m_pipeout(pipeout) {}

        void log(Severity severity, hstring const& str) override {
            // Msg type
            write_u32(m_pipeout, 0);
            // Msg body size
            write_u32(m_pipeout, 4 + str.size() * 2);
            // Severity
            write_u32(m_pipeout, (uint32_t)severity);
            write_bytes(m_pipeout, str.data(), str.size() * 2);
        }

    private:
        HANDLE m_pipein, m_pipeout;
    };

    SlaveLogger logger(pipein, pipeout);
    CTinyC::Executor executor(&logger);

    std::vector<uint8_t> buf;
    auto code_size = read_u32(pipein);
    buf.resize(code_size);
    read_bytes(pipein, buf.data(), code_size);
    auto ip = read_u32(pipein);
    auto mem_size = 1024 * 1024 * 16;
    auto start_offset = 1000;

    executor.load(buf.data(), code_size, mem_size, start_offset);
    executor.set_ip(ip);

    std::atomic_bool interrupt_flag{};
    bool is_success = !executor.execute(50000, interrupt_flag);

    printf("\n\n---------- End of Execution ----------\n");

    // Msg type
    write_u32(pipeout, 1);
    // Msg body size
    write_u32(pipeout, 4);
    // Is success?
    write_u32(pipeout, is_success);

    return 0;
}
catch (winrt::hresult_error const& e) {
    printf("\n\n[ERROR] Unhandled exception: 0x%08x\n", (uint32_t)e.code());
    return EXIT_FAILURE;
}
catch (std::exception const& e) {
    printf("\n\n[ERROR] Unhandled exception: %s\n", e.what());
    return EXIT_FAILURE;
}
catch (...) {
    return EXIT_FAILURE;
}

// The raw application entry point in DLL; logical entry point resides in
// App::App() and App::OnLaunched()
int __declspec(dllexport) app_main(HINSTANCE hInstance, LPWSTR lpCmdLine, int nShowCmd) {
    if (std::wstring_view{ lpCmdLine }.ends_with(L"code-exec")) {
        AllocConsole();
        freopen("CONIN$", "r", stdin);
        freopen("CONOUT$", "w", stdout);

        auto ret = slave_exec_main();

        printf("\nPress any key to exit...");
        _getch();

        return ret;
    }

    EnableMouseInPointer(true);

    Win32Xaml::AppService::InitializeForApplication();

    TinyC::App::App app;
    auto wxm = Windows::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();

    Win32Xaml::AppService::RunLoop();

    app.Exit();
    wxm.Close();

    return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int nShowCmd) {
    winrt::init_apartment(winrt::apartment_type::single_threaded);

    LoadLibraryW(L"twinapi.appcore.dll");
    LoadLibraryW(L"threadpoolwinrt.dll");
    /*wchar_t buf[32];
    AppPolicyWindowingModel wnd_model{};
    AppPolicyGetWindowingModel(GetCurrentProcess(), &wnd_model);
    wsprintfW(buf, L"WindowingModel: %d", (int)wnd_model);
    OutputDebugStringW(buf);
    if (!IsDebuggerPresent()) {
        MessageBoxW(nullptr, buf, nullptr, 0);
    }*/
    return app_main(hInstance, lpCmdLine, nShowCmd);
}
