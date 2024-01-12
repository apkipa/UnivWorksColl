#pragma once

#include <mutex>
#include <winrt/MicaEditor.h>
#include "Code/Logger.hpp"

#include "MainWindow.g.h"

namespace winrt::TinyC::App::implementation {
    struct MainWindow : MainWindowT<MainWindow> {
        MainWindow() = default;
        ~MainWindow();

        void InitializeComponent();

        void MenuFileNewItem_Click(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        fire_and_forget MenuFileOpenItem_Click(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        fire_and_forget MenuFileSaveItem_Click(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void MenuFileExitItem_Click(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void MenuEditUndoItem_Click(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void MenuEditRedoItem_Click(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void MenuBuildCompileItem_Click(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void MenuBuildRunItem_Click(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void MenuHelpViewHelpItem_Click(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void MenuHelpAboutItem_Click(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);

    private:
        void ClearCompilationOutput() {
            CompilationOutputText().Inlines().Clear();
            CompilationOutputTextScroller().ScrollToVerticalOffset(0);
            m_compilation_output_str = {};
        }
        void AddCompilationOutput(hstring const& str) {
            std::scoped_lock guard{ m_mutex_compilation_output };
            m_compilation_output_str = m_compilation_output_str + str + L"\n";

            if (!m_compilation_output_is_pending_update) {
                m_compilation_output_is_pending_update = true;
                LayoutRoot().Dispatcher().RunAsync(Windows::UI::Core::CoreDispatcherPriority::Low, [&] {
                    std::scoped_lock guard{ m_mutex_compilation_output };
                    m_compilation_output_is_pending_update = false;
                    CompilationOutputText().Text(m_compilation_output_str);
                });
            }
        }
        Windows::Storage::Streams::IBuffer DuplicateEditorBuffer() {
            auto editor = CodeEditCtrl().Editor();
            auto str_size = static_cast<uint32_t>(editor.Length());
            auto buffer = Windows::Storage::Streams::Buffer(str_size + 1);
            buffer.Length(str_size);
            editor.GetTextWriteBuffer(str_size, buffer);
            return buffer;
        }

        struct CompilationOutputLogger : CTinyC::Logger {
            CompilationOutputLogger() : m_parent(nullptr) {}

            void init(MainWindow* parent) {
                m_parent = parent;
            }

            void log(Severity severity, hstring const& str) {
                if (!m_parent) {
                    throw hresult_illegal_method_call(L"CompilationOutputLogger not inited");
                }
                m_parent->AddCompilationOutput(str);
            }
        private:
            MainWindow* m_parent;
        };

        CompilationOutputLogger m_compilation_logger;

        bool m_is_dragging{};
        Windows::Foundation::Point m_last_drag_pt{};
        double m_last_scroller_height{};
        bool m_compilation_output_is_pending_update{};
        std::mutex m_mutex_compilation_output;
        hstring m_compilation_output_str;

        HANDLE m_sub_exec_proc_handle{};
    };
}

namespace winrt::TinyC::App::factory_implementation {
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {};
}
