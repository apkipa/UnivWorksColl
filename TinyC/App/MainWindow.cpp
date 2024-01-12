#include "pch.h"
#include "MainWindow.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include "Code/public.h"
#include "Code/Lexer.hpp"
#include "Code/Parser.hpp"
#include "Code/CodeGen.hpp"
#include "Code/Executor.hpp"

using namespace std::literals;
using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;
using namespace Windows::Storage::Streams;
using namespace Windows::Security::Cryptography;

namespace util {
    namespace misc {
        // Similar to std::experimental::scope_exit
        template<typename T>
        struct ScopeExit final {
            explicit ScopeExit(T&& func) : m_func(std::forward<T>(func)) {}
            ~ScopeExit() { if (m_active) { std::invoke(m_func); } }
            void release(void) { m_active = false; }
        private:
            bool m_active{ true };
            T m_func;
        };
        template<typename T>
        inline auto scope_exit(T&& func) {
            return ScopeExit{ std::forward<T>(func) };
        }
    }
}

inline HWND WindowIdToHwnd(Windows::UI::WindowId wndId) {
    return (HWND)(uintptr_t)wndId.Value;
}

namespace winrt::TinyC::App::implementation {
    MainWindow::~MainWindow() {
        // TODO: Use job objects
        if (auto h = std::exchange(m_sub_exec_proc_handle, {})) {
            TerminateProcess(h, EXIT_FAILURE);
            CloseHandle(h);
        }
    }
    void MainWindow::InitializeComponent() {
        MainWindowT::InitializeComponent();

        this->ExtendsContentIntoTitleBar(true);
        this->SetTitleBar(TopRectangle());

        auto cec = CodeEditCtrl();
        cec.HighlightingLanguage(L"cpp");
        cec.Editor().UseTabs(false);

        m_compilation_logger.init(this);

        GridSizeBar().PointerEntered([](auto&& sender, PointerRoutedEventArgs const& e) {
            e.Handled(true);
            CoreWindow::GetForCurrentThread().PointerCursor(CoreCursor(CoreCursorType::SizeNorthSouth, 0));
            VisualStateManager::GoToState(sender.as<Control>(), L"PointerOver", true);
        });
        GridSizeBar().PointerCanceled([this](auto&& sender, PointerRoutedEventArgs const& e) {
            e.Handled(true);
            CoreWindow::GetForCurrentThread().PointerCursor(CoreCursor(CoreCursorType::Arrow, 0));
            VisualStateManager::GoToState(sender.as<Control>(), L"Normal", true);
            m_is_dragging = false;
        });
        GridSizeBar().PointerExited([this](auto&& sender, PointerRoutedEventArgs const& e) {
            e.Handled(true);
            if (!m_is_dragging) {
                CoreWindow::GetForCurrentThread().PointerCursor(CoreCursor(CoreCursorType::Arrow, 0));
                VisualStateManager::GoToState(sender.as<Control>(), L"Normal", true);
                m_is_dragging = false;
            }
        });
        GridSizeBar().PointerPressed([this](auto&& sender, PointerRoutedEventArgs const& e) {
            UIElement se = sender.as<UIElement>();
            e.Handled(true);
            m_is_dragging = true;
            m_last_drag_pt = e.GetCurrentPoint(LayoutRoot()).Position();
            m_last_scroller_height = CompilationOutputTextScroller().Height();
            if (!se.CapturePointer(e.Pointer())) {
                OutputDebugStringW(L"WARN: Pointer capture failed");
            }
        });
        GridSizeBar().PointerReleased([this](auto&& sender, PointerRoutedEventArgs const& e) {
            UIElement se = sender.as<UIElement>();
            e.Handled(true);
            m_is_dragging = false;
            se.ReleasePointerCapture(e.Pointer());

            CoreWindow::GetForCurrentThread().PointerCursor(CoreCursor(CoreCursorType::Arrow, 0));
            VisualStateManager::GoToState(sender.as<Control>(), L"Normal", true);
        });
        GridSizeBar().PointerCaptureLost([this](auto&& sender, PointerRoutedEventArgs const& e) {
            UIElement se = sender.as<UIElement>();
            e.Handled(true);
            m_is_dragging = false;

            CoreWindow::GetForCurrentThread().PointerCursor(CoreCursor(CoreCursorType::Arrow, 0));
            VisualStateManager::GoToState(sender.as<Control>(), L"Normal", true);
        });
        GridSizeBar().PointerMoved([this](auto&& sender, PointerRoutedEventArgs const& e) {
            e.Handled(true);
            //const double scale = GetDpiForWindow(WindowIdToHwnd(this->Id())) / 96.0;
            const double scale = 1;
            if (m_is_dragging) {
                auto last_pt = m_last_drag_pt;
                auto cur_pt = e.GetCurrentPoint(LayoutRoot()).Position();
                auto new_scroller_height = std::clamp(
                    (m_last_scroller_height - (cur_pt.Y - last_pt.Y)) * scale,
                    40.0, 1e6
                );
                CompilationOutputTextScroller().Height(new_scroller_height);
    }
        });
    }
    void MainWindow::MenuFileNewItem_Click(IInspectable const&, RoutedEventArgs const&) {
        auto editor = CodeEditCtrl().Editor();
        editor.ClearAll();
        editor.EmptyUndoBuffer();
    }
    fire_and_forget MainWindow::MenuFileOpenItem_Click(IInspectable const&, RoutedEventArgs const&) {
        FileOpenPicker picker;
        picker.as<IInitializeWithWindow>()->Initialize(WindowIdToHwnd(this->Id()));
        picker.FileTypeFilter().ReplaceAll({ L".txt", L".c", L".cpp" });
        auto file = co_await picker.PickSingleFileAsync();
        if (!file) { co_return; }
        auto basic_props = co_await file.GetBasicPropertiesAsync();
        auto file_size = basic_props.Size();
        if (file_size > 1024ull * 1024) {
            ContentDialog cd;
            cd.XamlRoot(LayoutRoot().XamlRoot());
            cd.Title(box_value(L"Unable to Open File"));
            cd.Content(box_value(L""
                "The specified file is too large to open. Only small text files "
                "are supported at the moment."
            ));
            cd.CloseButtonText(L"OK");
            cd.ShowAsync();
        }
        auto stm = co_await file.OpenReadAsync();
        IBuffer buffer = Buffer{ (uint32_t)file_size + 1 };
        buffer = co_await stm.ReadAsync(buffer, (uint32_t)file_size, InputStreamOptions::None);
        if (!buffer) { co_return; }
        buffer.data()[file_size] = '\0';

        /*auto str = CryptographicBuffer::ConvertBinaryToString(BinaryStringEncoding::Utf8, buffer);
        CodeEditCtrl().Editor().SetText(str);*/
        CodeEditCtrl().Editor().SetTextFromBuffer(buffer);
        CodeEditCtrl().Editor().EmptyUndoBuffer();
    }
    fire_and_forget MainWindow::MenuFileSaveItem_Click(IInspectable const&, RoutedEventArgs const&) {
        FileSavePicker picker;
        picker.as<IInitializeWithWindow>()->Initialize(WindowIdToHwnd(this->Id()));
        {
            auto choices = picker.FileTypeChoices();
            choices.Insert(L"Plain Text", winrt::single_threaded_vector<hstring>({ L".txt" }));
            choices.Insert(L"C Source Code", winrt::single_threaded_vector<hstring>({ L".c" }));
        }
        auto file = co_await picker.PickSaveFileAsync();
        if (!file) { co_return; }
        auto stm = co_await file.OpenAsync(FileAccessMode::ReadWrite);
        co_await stm.WriteAsync(DuplicateEditorBuffer());
        co_await stm.FlushAsync();
        CodeEditCtrl().Editor().SetSavePoint();
    }
    void MainWindow::MenuFileExitItem_Click(IInspectable const&, RoutedEventArgs const&) {
        LayoutRoot().Dispatcher().RunAsync(CoreDispatcherPriority::Low, [this] {
            this->Close();
        });
    }
    void MainWindow::MenuEditUndoItem_Click(IInspectable const&, RoutedEventArgs const&) {
        CodeEditCtrl().Editor().Undo();
    }
    void MainWindow::MenuEditRedoItem_Click(IInspectable const&, RoutedEventArgs const&) {
        CodeEditCtrl().Editor().Redo();
    }
    void MainWindow::MenuBuildCompileItem_Click(IInspectable const&, RoutedEventArgs const&) {
        auto dispatcher = Window::Current().Dispatcher();

        this->ClearCompilationOutput();
        auto ts = std::chrono::system_clock::now();
        std::chrono::zoned_time zoned_ts{ std::chrono::current_zone(),
            std::chrono::time_point_cast<std::chrono::seconds>(ts) };
        this->AddCompilationOutput(hstring(std::format(L"Build started at {:%F %T}...", zoned_ts)));

        auto buf = DuplicateEditorBuffer();
        auto code_str = std::string_view{ reinterpret_cast<char*>(buf.data()), buf.Length() };
        try {
            CTinyC::Lexer lexer(&m_compilation_logger);
            lexer.init(code_str);
            /*while (auto otoken = lexer.next_token()) {
                this->AddCompilationOutput(hstring(std::format(L"[DEBUG] Got token `{}` at ({},{})",
                    to_hstring(otoken->str), otoken->line, otoken->column)));
            }*/

            this->AddCompilationOutput(L"Compiling <source>...");

            CTinyC::Parser parser(&m_compilation_logger);
            auto ast_root = parser.parse(&lexer);
            if (!ast_root) {
                throw std::runtime_error("compilation failed");
            }

            this->AddCompilationOutput(L"Generating code...");
            CTinyC::CodeGenerator code_gen(&m_compilation_logger);
            auto code_info = code_gen.ast_to_code(*ast_root, 0x100);

            this->AddCompilationOutput(L"Build result: PASSED");
        }
        catch (...) {
            this->AddCompilationOutput(L"Build result: FAILED");
        }
        auto ts2 = std::chrono::system_clock::now();
        std::chrono::zoned_time zoned_ts2{ std::chrono::current_zone(),
            std::chrono::time_point_cast<std::chrono::seconds>(ts2) };
        this->AddCompilationOutput(hstring(std::format(L"Build completed at {:%F %T} and took {:%S} seconds.",
            zoned_ts, ts2 - ts)));
    }
    void MainWindow::MenuBuildRunItem_Click(IInspectable const&, RoutedEventArgs const&) {
        this->ClearCompilationOutput();
        auto ts = std::chrono::system_clock::now();
        std::chrono::zoned_time zoned_ts{ std::chrono::current_zone(),
            std::chrono::time_point_cast<std::chrono::seconds>(ts) };
        this->AddCompilationOutput(hstring(std::format(L"Build started at {:%F %T}...", zoned_ts)));

        auto buf = DuplicateEditorBuffer();
        auto code_str = std::string_view{ reinterpret_cast<char*>(buf.data()), buf.Length() };

        CTinyC::Lexer lexer(&m_compilation_logger);
        CTinyC::Parser parser(&m_compilation_logger);
        std::vector<uint8_t> code;
        CTinyC::CodeMetadata metadata;
        bool failed{};
        try {
            lexer.init(code_str);

            this->AddCompilationOutput(L"Compiling <source>...");

            auto ast_root = parser.parse(&lexer);
            if (!ast_root) {
                throw std::runtime_error("compilation failed");
            }

            this->AddCompilationOutput(L"Generating code...");
            CTinyC::CodeGenerator code_gen(&m_compilation_logger);
            std::tie(code, metadata) = code_gen.ast_to_code(*ast_root, 1000);

            this->AddCompilationOutput(L"Build result: PASSED");
        }
        catch (...) {
            this->AddCompilationOutput(L"Build result: FAILED");
            failed = true;
        }
        if (!failed) {
            try {
                this->AddCompilationOutput(L"Running code...");

                if (auto h = std::exchange(m_sub_exec_proc_handle, {})) {
                    TerminateProcess(h, EXIT_FAILURE);
                    CloseHandle(h);
                }

                auto main_func_it = std::ranges::find(metadata.func_meta, "main",
                    [](CTinyC::CodeMetadata::FuncMetadata& v) { return v.name; }
                );
                if (main_func_it == end(metadata.func_meta)) {
                    this->AddCompilationOutput(L"Error: function main not found");
                    throw std::runtime_error("function main not found");
                }

                [&]() -> fire_and_forget {
                    try {
                        using namespace CTinyC;

                        apartment_context ui_ctx;
                        auto dispatcher = this->LayoutRoot().Dispatcher();

                        auto weak_this = get_weak();

                        auto main_function_offset = main_func_it->offset;

                        SECURITY_ATTRIBUTES sa{ .nLength = sizeof sa, .bInheritHandle = true };
                        HANDLE pipein1, pipeout1;
                        check_bool(CreatePipe(&pipein1, &pipeout1, &sa, 0));
                        auto se_1 = util::misc::scope_exit([&] {
                            CloseHandle(pipein1);
                            CloseHandle(pipeout1);
                        });
                        HANDLE pipein2, pipeout2;
                        check_bool(CreatePipe(&pipein2, &pipeout2, &sa, 0));
                        auto se_2 = util::misc::scope_exit([&] {
                            CloseHandle(pipein2);
                            CloseHandle(pipeout2);
                        });

                        // Start a sub process and communicate
                        wchar_t buf[MAX_PATH + 32];
                        GetModuleFileNameW(nullptr, buf, MAX_PATH);
                        wcscat(buf, L" code-exec");
                        char envblock[128];
                        auto cnt = sprintf(envblock, "pipein=%08x", (uint32_t)pipein1) + 1;
                        cnt += sprintf(envblock + cnt, "pipeout=%08x", (uint32_t)pipeout2) + 1;
                        envblock[cnt] = '\0';
                        STARTUPINFOW si{ .cb = sizeof si, .dwFlags = STARTF_FORCEOFFFEEDBACK };
                        PROCESS_INFORMATION pi;
                        check_bool(CreateProcessW(nullptr, buf, &sa, &sa, true,
                            0, envblock, nullptr, &si, &pi
                        ));
                        CloseHandle(pi.hThread);
                        //CloseHandle(pi.hProcess);
                        m_sub_exec_proc_handle = pi.hProcess;

                        // Code size
                        write_u32(pipeout1, code.size());
                        // Code
                        write_bytes(pipeout1, code.data(), code.size());
                        // Start ip
                        write_u32(pipeout1, 1000 + main_function_offset);

                        while (true) {
                            co_await resume_background();

                            auto add_log_fn = [&](hstring const& s) {
                                    auto that = weak_this.get();
                                if (!that) { return false; }
                                that->AddCompilationOutput(s);
                                return true;
                            };

                            auto msg_type = read_u32(pipein2);
                            if (msg_type == 0) {
                                auto body_size = read_u32(pipein2);
                                auto buf_size = (body_size - 4) / 2;
                                auto severity = (CTinyC::Logger::Severity)read_u32(pipein2);
                                std::vector<wchar_t> buf(buf_size);
                                read_bytes(pipein2, buf.data(), body_size - 4);
                                add_log_fn({ buf.data(), buf_size });
                            }
                            else if (msg_type == 1) {
                                auto body_size = read_u32(pipein2);
                                auto is_success = read_u32(pipein2);
                                if (is_success) {
                                    add_log_fn(L"Run result: SUCCESS");
                                }
                                else {
                                    add_log_fn(L"Run result: FAILED");
                                }
                            }
                            else {
                                throw hresult_error(E_FAIL, L"unknown msg type");
                            }
                        }
                    }
                    catch (...) {
                        OutputDebugStringW(L"[DEBUG] Run thread died unexpectedly");
                    }
                }();
                }
            catch (...) {
                this->AddCompilationOutput(L"Run result: FAILED");
            }
        }
        auto ts2 = std::chrono::system_clock::now();
        std::chrono::zoned_time zoned_ts2{ std::chrono::current_zone(),
            std::chrono::time_point_cast<std::chrono::seconds>(ts2) };
        this->AddCompilationOutput(hstring(std::format(L"Build completed at {:%F %T} and took {:%S} seconds.",
            zoned_ts, ts2 - ts)));
    }
    void MainWindow::MenuHelpViewHelpItem_Click(IInspectable const& sender, RoutedEventArgs const& args) {
        ContentDialog cd;
        cd.XamlRoot(LayoutRoot().XamlRoot());
        cd.Title(box_value(L"Help"));
        cd.Content(box_value(L""
            "Supported language features: define an [array of] int, nested functions, etc.\n"
            "Available built-in functions:\n"
            "* int input(void);\n"
            "* void output(int value);\n"
            "Signature of main must be: void main();"
        ));
        cd.CloseButtonText(L"OK");
        cd.ShowAsync();
    }
    void MainWindow::MenuHelpAboutItem_Click(IInspectable const& sender, RoutedEventArgs const& args) {
        ContentDialog cd;
        cd.XamlRoot(LayoutRoot().XamlRoot());
        cd.Title(box_value(L"About"));
        cd.Content(box_value(L""
            "此程序为编译原理课程设计，实现了一个 C 语言子集的编译及运行。\n"
        ));
        cd.CloseButtonText(L"OK");
        cd.ShowAsync();
    }
}
