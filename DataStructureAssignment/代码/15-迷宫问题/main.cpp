#include "util.h"

struct DisjointSet {
    std::vector<size_t> pa;
    DisjointSet(size_t size) : pa(size) {
        std::iota(pa.begin(), pa.end(), static_cast<size_t>(0));
    }
    size_t find(size_t x) {
        return pa.at(x) == x ? x : (pa[x] = find(pa[x]));
    }
    void unite(size_t a, size_t b) {
        auto parenta = find(a), parentb = find(b);
        pa[parenta] = parentb;
    }
};

// Transfer control flow to the message pump of the window
// WARN: The thread owning the message pump must enter an alertable state
//       for the function to work properly
struct awaitable_hwnd {
    awaitable_hwnd(HWND in_hwnd) : hwnd(in_hwnd) {}
    operator HWND() { return hwnd; }
    HWND hwnd;
};
inline auto operator co_await(awaitable_hwnd hwnd) noexcept {
    struct awaiter {
        awaiter(HWND hwnd) : m_hwnd(hwnd) {}
        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> resume) {
            auto thread_id = GetWindowThreadProcessId(m_hwnd, nullptr);
            auto thread_handle = OpenThread(THREAD_SET_CONTEXT, false, thread_id);
            if (!thread_handle) { winrt::throw_last_error(); }
            auto ret = QueueUserAPC([](ULONG_PTR param) {
                std::coroutine_handle<>::from_address(reinterpret_cast<void*>(param))();
            }, thread_handle, reinterpret_cast<ULONG_PTR>(resume.address()));
            CloseHandle(thread_handle);
            if (!ret) { winrt::throw_last_error(); }
        }
        void await_resume() {}
    private:
        HWND m_hwnd;
    };
    return awaiter{ hwnd };
}

struct D2D1SimpleWindow;

// A helper class to wrap & simplify drawing stuff
// WARN: Don't hold a D2D1DrawingSession; they may get invalidated
//       due to DeviceLost events
struct D2D1DrawingSession {
    D2D1DrawingSession(D2D1SimpleWindow* window);

    void fill_ellipse(
        const D2D1_POINT_2F& center,
        float radiusX, float radiusY,
        const D2D1_COLOR_F& fill_color)
    {
        init_solid_color_brush(fill_color);
        m_d2d1_dev_ctx->FillEllipse(D2D1::Ellipse(center, radiusX, radiusY), m_solid_color_brush.get());
    }
    void fill_rectangle(
        const D2D1_RECT_F& rect,
        const D2D1_COLOR_F& fill_color
    ) {
        init_solid_color_brush(fill_color);
        m_d2d1_dev_ctx->FillRectangle(rect, m_solid_color_brush.get());
    }
    void draw_rectangle(
        const D2D1_RECT_F& rect,
        const D2D1_COLOR_F& fill_color,
        float stroke_width
    ) {
        init_solid_color_brush(fill_color);
        m_d2d1_dev_ctx->DrawRectangle(rect, m_solid_color_brush.get(), stroke_width);
    }
    void draw_text_layout(
        const D2D1_POINT_2F& origin,
        const winrt::com_ptr<IDWriteTextLayout>& text_layout,
        const D2D1_COLOR_F& fill_color
    ) {
        init_solid_color_brush(fill_color);
        m_d2d1_dev_ctx->DrawTextLayout(
            origin, text_layout.get(), m_solid_color_brush.get(),
            D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT
        );
    }
    void draw_line(
        const D2D1_POINT_2F& pt1,
        const D2D1_POINT_2F& pt2,
        const D2D1_COLOR_F& fill_color,
        float stroke_width
    ) {
        init_solid_color_brush(fill_color);
        m_d2d1_dev_ctx->DrawLine(pt1, pt2, m_solid_color_brush.get(), stroke_width);
    }
    void draw_polyline(
        std::span<const D2D1_POINT_2F> pts,
        const D2D1_COLOR_F& fill_color,
        float stroke_width
    ) {
        if (pts.size() < 2) { throw winrt::hresult_invalid_argument(L"Too few points for polyline"); }
        init_solid_color_brush(fill_color);
        winrt::com_ptr<ID2D1PathGeometry> path;
        winrt::com_ptr<ID2D1GeometrySink> path_sink;
        winrt::check_hresult(m_d2d1_factory->CreatePathGeometry(path.put()));
        winrt::check_hresult(path->Open(path_sink.put()));
        path_sink->BeginFigure(pts.front(), D2D1_FIGURE_BEGIN_HOLLOW);
        path_sink->AddLines(pts.data() + 1, static_cast<UINT32>(pts.size() - 1));
        path_sink->EndFigure(D2D1_FIGURE_END_OPEN);
        winrt::check_hresult(path_sink->Close());
        m_d2d1_dev_ctx->DrawGeometry(path.get(), m_solid_color_brush.get(), stroke_width);
    }
    void draw_discrete_polylines(
        std::span<const std::span<const D2D1_POINT_2F>> lines,
        const D2D1_COLOR_F& fill_color,
        float stroke_width
    ) {
        for (auto pts : lines) {
            if (pts.size() < 2) { throw winrt::hresult_invalid_argument(L"Too few points for polyline"); }
        }
        init_solid_color_brush(fill_color);
        winrt::com_ptr<ID2D1PathGeometry> path;
        winrt::com_ptr<ID2D1GeometrySink> path_sink;
        winrt::check_hresult(m_d2d1_factory->CreatePathGeometry(path.put()));
        winrt::check_hresult(path->Open(path_sink.put()));
        for (auto pts : lines) {
            path_sink->BeginFigure(pts.front(), D2D1_FIGURE_BEGIN_HOLLOW);
            path_sink->AddLines(pts.data() + 1, static_cast<UINT32>(pts.size() - 1));
            path_sink->EndFigure(D2D1_FIGURE_END_OPEN);
        }
        winrt::check_hresult(path_sink->Close());
        m_d2d1_dev_ctx->DrawGeometry(path.get(), m_solid_color_brush.get(), stroke_width);
    }
    void draw_ellipse(
        const D2D1_ELLIPSE& ellipse,
        const D2D1_COLOR_F& fill_color,
        float stroke_width
    ) {
        init_solid_color_brush(fill_color);
        m_d2d1_dev_ctx->DrawEllipse(ellipse, m_solid_color_brush.get(), stroke_width);
    }
    void draw_arc(
        const D2D1_POINT_2F& pt_start,
        const D2D1_ARC_SEGMENT& arc_seg,
        const D2D1_COLOR_F& fill_color,
        float stroke_width
    ) {
        init_solid_color_brush(fill_color);
        winrt::com_ptr<ID2D1PathGeometry> path;
        winrt::com_ptr<ID2D1GeometrySink> path_sink;
        winrt::check_hresult(m_d2d1_factory->CreatePathGeometry(path.put()));
        winrt::check_hresult(path->Open(path_sink.put()));
        path_sink->BeginFigure(pt_start, D2D1_FIGURE_BEGIN_HOLLOW);
        path_sink->AddArc(arc_seg);
        path_sink->EndFigure(D2D1_FIGURE_END_OPEN);
        winrt::check_hresult(path_sink->Close());
        m_d2d1_dev_ctx->DrawGeometry(path.get(), m_solid_color_brush.get(), stroke_width);
    }
    void draw_arc(
        const D2D1_POINT_2F& pt_center,
        const D2D1_SIZE_F& radius,
        float degree,
        const D2D1_COLOR_F& fill_color,
        float stroke_width
    ) {
        if (degree <= 0) { return; }
        if (degree >= 360) {
            auto ellipse = D2D1::Ellipse(pt_center, radius.width, radius.height);
            return draw_ellipse(ellipse, fill_color, stroke_width);
        }
        init_solid_color_brush(fill_color);
        // NOTE: t is in radians
        auto parametric_ellipse_fn = [](double r_x, double r_y, double t) -> std::pair<double, double> {
            return { r_x * std::sin(t), r_y * std::cos(t) };
        };
        auto pt_from_t_fn = [&](double t) -> D2D1_POINT_2F {
            auto [x, y] = parametric_ellipse_fn(radius.width, radius.height, t);
            return { static_cast<float>(pt_center.x + x), static_cast<float>(pt_center.y - y) };
        };
        winrt::com_ptr<ID2D1PathGeometry> path;
        winrt::com_ptr<ID2D1GeometrySink> path_sink;
        winrt::check_hresult(m_d2d1_factory->CreatePathGeometry(path.put()));
        winrt::check_hresult(path->Open(path_sink.put()));
        path_sink->BeginFigure(pt_from_t_fn(0), D2D1_FIGURE_BEGIN_HOLLOW);
        size_t sub_n = 0;
        while (degree > (sub_n + 1) * 60) {
            sub_n++;
            auto arc_seg = D2D1::ArcSegment(
                pt_from_t_fn(sub_n * (std::numbers::pi / 3)),
                radius,
                0,
                D2D1_SWEEP_DIRECTION_CLOCKWISE,
                D2D1_ARC_SIZE_SMALL
            );
            path_sink->AddArc(arc_seg);
        }
        // NOTE: Magic number is for fixing graphic errors
        auto arc_seg = D2D1::ArcSegment(
            pt_from_t_fn((degree + 2) / 180 * std::numbers::pi),
            radius,
            0,
            D2D1_SWEEP_DIRECTION_CLOCKWISE,
            D2D1_ARC_SIZE_SMALL
        );
        path_sink->AddArc(arc_seg);
        path_sink->EndFigure(D2D1_FIGURE_END_OPEN);
        winrt::check_hresult(path_sink->Close());
        m_d2d1_dev_ctx->DrawGeometry(path.get(), m_solid_color_brush.get(), stroke_width);
    }
    void set_transform(const D2D1_MATRIX_3X2_F& mat) {
        m_d2d1_dev_ctx->SetTransform(mat);
    }
    void restore_transform(void) {
        m_d2d1_dev_ctx->SetTransform(D2D1::Matrix3x2F::Identity());
    }

private:
    winrt::com_ptr<ID2D1Factory7> m_d2d1_factory;
    winrt::com_ptr<ID2D1DeviceContext5> m_d2d1_dev_ctx;
    winrt::com_ptr<ID2D1SolidColorBrush> m_solid_color_brush;

    void init_solid_color_brush(const D2D1_COLOR_F& color) {
        if (!m_solid_color_brush) {
            winrt::check_hresult(
                m_d2d1_dev_ctx->CreateSolidColorBrush(color, m_solid_color_brush.put()));
        }
        else {
            m_solid_color_brush->SetColor(color);
        }
    }
};

struct D2D1ResourceManager : std::enable_shared_from_this<D2D1ResourceManager> {
    D2D1ResourceManager(D2D1SimpleWindow* window);

    winrt::com_ptr<IDWriteTextFormat> create_text_format(
        const winrt::hstring& font_family,
        DWRITE_FONT_WEIGHT font_weight,
        DWRITE_FONT_STYLE font_style,
        DWRITE_FONT_STRETCH font_stretch,
        float font_size,
        const winrt::hstring& locale_name = L"zh-cn"
    ) {
        winrt::com_ptr<IDWriteTextFormat> r;
        winrt::check_hresult(m_dwrite_factory->CreateTextFormat(
            font_family.c_str(), nullptr,
            font_weight, font_style, font_stretch,
            font_size,
            locale_name.c_str(),
            r.put()
        ));
        return r;
    }
    winrt::com_ptr<IDWriteTextLayout> create_text_layout(
        const winrt::hstring& str,
        const winrt::com_ptr<IDWriteTextFormat>& text_format,
        float max_width, float max_height
    ) {
        winrt::com_ptr<IDWriteTextLayout> r;
        winrt::check_hresult(m_dwrite_factory->CreateTextLayout(
            str.c_str(), str.size(),
            text_format.get(),
            max_width, max_height,
            r.put()
        ));
        return r;
    }

    void request_redraw(void);

private:
    winrt::com_ptr<IDWriteFactory> m_dwrite_factory;
    D2D1SimpleWindow* m_window;
};

struct D2D1SimpleWindow {
    static constexpr unsigned REDRAW_TIMER_ID = 1;
    static constexpr unsigned MENU_SYS_ID_RUN_AT_FULL_SPEED = 0x0001;

    D2D1SimpleWindow(HINSTANCE hInst, const wchar_t* window_title) {
        // Create device-independent resources
        // D2D1_FACTORY_OPTIONS d2d1_factory_options{ D2D1_DEBUG_LEVEL_INFORMATION };
        D2D1_FACTORY_OPTIONS d2d1_factory_options{ D2D1_DEBUG_LEVEL_NONE };
        winrt::check_hresult(D2D1CreateFactory(
            D2D1_FACTORY_TYPE_SINGLE_THREADED,
            get_guid(m_d2d1_factory),
            &d2d1_factory_options,
            m_d2d1_factory.put_void()
        ));
        m_wic_factory.capture(
            CoCreateInstance,
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER
        );
        winrt::check_hresult(DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            get_guid(m_dwrite_factory),
            reinterpret_cast<IUnknown**>(m_dwrite_factory.put())
        ));

        // Create window
        auto create_window_fn = [&](const wchar_t* window_title) {
            WNDCLASSW wc = { sizeof(WNDCLASSW) };
            wc.hInstance = hInst;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.style = CS_HREDRAW | CS_VREDRAW;
            wc.cbWndExtra = sizeof(LONG_PTR);
            wc.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept -> LRESULT {
                if (msg == WM_CREATE) {
                    auto* pcs = reinterpret_cast<LPCREATESTRUCTW>(lParam);
                    auto* that = reinterpret_cast<D2D1SimpleWindow*>(pcs->lpCreateParams);
                    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(that));
                    return 1;
                }
                auto* that = reinterpret_cast<D2D1SimpleWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
                // Also handle custom menus
                if (msg == WM_SYSCOMMAND && wParam == MENU_SYS_ID_RUN_AT_FULL_SPEED) {
                    if (that) {
                        that->m_run_at_full_speed = !that->m_run_at_full_speed;
                        CheckMenuItem(
                            GetSystemMenu(hwnd, false),
                            MENU_SYS_ID_RUN_AT_FULL_SPEED,
                            MF_BYCOMMAND | (that->m_run_at_full_speed ? MF_CHECKED : MF_UNCHECKED)
                        );
                    }
                    return 1;
                }
                // NOTE: Block execution if an exception was found
                if (that && !that->m_cur_exception) {
                    try { return that->window_proc(msg, wParam, lParam); }
                    catch (...) { that->m_cur_exception = std::current_exception(); }
                }
                return DefWindowProcW(hwnd, msg, wParam, lParam);
            };
            wc.lpszClassName = L"Direct2DAppWindowClass";
            if (RegisterClassW(&wc) == 0) {
                winrt::throw_last_error();
            }
            return CreateWindowExW(
                0,
                wc.lpszClassName,
                window_title,
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, CW_USEDEFAULT,
                CW_USEDEFAULT, CW_USEDEFAULT,
                nullptr,
                nullptr,
                hInst,
                this
            );
        };

        m_hwnd = create_window_fn(window_title);
        winrt::check_pointer(m_hwnd);

        // Create device-dependent resources
        try { recreate_device_resources(); }
        catch (...) {
            // Perform proper resource cleanup
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
            throw;
        }

        // Modify system menu to add "Run at full speed" item
        auto sys_menu = GetSystemMenu(m_hwnd, false);
        AppendMenuW(sys_menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(
            sys_menu,
            MF_STRING | (m_run_at_full_speed ? MF_CHECKED : MF_UNCHECKED),
            MENU_SYS_ID_RUN_AT_FULL_SPEED,
            L"Run at full speed (VSync)"
        );

        ShowWindow(m_hwnd, SW_SHOW);

        // Opt in the modern input stack
        EnableMouseInPointer(true);
    }
    ~D2D1SimpleWindow() {
        DestroyWindow(m_hwnd);
    }

    void recreate_device_resources(void) {
        // Explicitly release resources
        m_dxgi_swapchain = nullptr;
        m_d2d1_dev_ctx = nullptr;

        // Recreate resources
        auto init_d2d1_with_swapchain_fn = [&] {
            auto create_d3d11dev_fn = [](D3D_DRIVER_TYPE type, winrt::com_ptr<ID3D11Device>& dev) {
                D3D_FEATURE_LEVEL feature_levels[] = {
                    D3D_FEATURE_LEVEL_11_1,
                    D3D_FEATURE_LEVEL_11_0,
                };
                return D3D11CreateDevice(
                    nullptr,
                    type,
                    nullptr,
                    D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                    feature_levels,
                    static_cast<UINT>(std::size(feature_levels)),
                    D3D11_SDK_VERSION,
                    dev.put(),
                    nullptr,
                    nullptr
                );
            };
            winrt::com_ptr<ID3D11Device> d3d11_dev;
            {
                auto hr = create_d3d11dev_fn(D3D_DRIVER_TYPE_HARDWARE, d3d11_dev);
                if (hr == DXGI_ERROR_UNSUPPORTED) {
                    hr = create_d3d11dev_fn(D3D_DRIVER_TYPE_WARP, d3d11_dev);
                }
                winrt::check_hresult(hr);
            }
            winrt::com_ptr<IDXGIDevice1> dxgi_dev;
            d3d11_dev.as(dxgi_dev);
            DXGI_SWAP_CHAIN_DESC1 swapchain_desc{};
            swapchain_desc.Width = 0;
            swapchain_desc.Height = 0;
            swapchain_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            swapchain_desc.Stereo = false; 
            swapchain_desc.SampleDesc.Count = 1;
            swapchain_desc.SampleDesc.Quality = 0;
            swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapchain_desc.BufferCount = 2;
            swapchain_desc.Scaling = DXGI_SCALING_NONE;
            swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            swapchain_desc.Flags = 0;
            winrt::com_ptr<IDXGIAdapter> dxgi_adapter;
            winrt::check_hresult(dxgi_dev->GetAdapter(dxgi_adapter.put()));
            winrt::com_ptr<IDXGIFactory2> dxgi_factory;
            dxgi_factory.capture(dxgi_adapter, &IDXGIAdapter::GetParent);
            winrt::check_hresult(dxgi_factory->CreateSwapChainForHwnd(
                d3d11_dev.get(),
                m_hwnd,
                &swapchain_desc,
                nullptr,
                nullptr,
                m_dxgi_swapchain.put()
            ));
            winrt::check_hresult(dxgi_dev->SetMaximumFrameLatency(1));
            winrt::com_ptr<ID2D1Device> d2d1_dev;
            winrt::check_hresult(m_d2d1_factory->CreateDevice(
                dxgi_dev.get(), d2d1_dev.put()
            ));
            winrt::com_ptr<ID2D1DeviceContext> d2d1_dev_ctx;
            winrt::check_hresult(d2d1_dev->CreateDeviceContext(
                D2D1_DEVICE_CONTEXT_OPTIONS_NONE, d2d1_dev_ctx.put()
            ));
            d2d1_dev_ctx.as(m_d2d1_dev_ctx);
            recreate_device_swapchain_bitmap();
            float dpix, dpiy;
            dpix = dpiy = static_cast<float>(GetDpiForWindow(m_hwnd));
            m_d2d1_dev_ctx->SetDpi(dpix, dpiy);
        };
        init_d2d1_with_swapchain_fn();

        recreate_device_size_sensitive_resources();
    }
    void recreate_device_swapchain_bitmap(void) {
        winrt::com_ptr<IDXGISurface> dxgi_surface;
        dxgi_surface.capture(m_dxgi_swapchain, &IDXGISwapChain1::GetBuffer, 0);
        auto bmp_props = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)
        );
        winrt::com_ptr<ID2D1Bitmap1> d2d1_bmp;
        winrt::check_hresult(m_d2d1_dev_ctx->CreateBitmapFromDxgiSurface(
            dxgi_surface.get(),
            bmp_props,
            d2d1_bmp.put()
        ));
        m_d2d1_dev_ctx->SetTarget(d2d1_bmp.get());
    }

    void recreate_device_size_sensitive_resources(void) {
        on_window_size_dependent_resource_create();
    }

    void window_size_changed(uint32_t width, uint32_t height) {
        // Resize swapchain
        m_d2d1_dev_ctx->SetTarget(nullptr);
        auto hr = m_dxgi_swapchain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        if (hr == S_OK) {
            recreate_device_swapchain_bitmap();
        }
        else {
            recreate_device_resources();
        }
        on_resize(width, height);
        recreate_device_size_sensitive_resources();
    }

    LRESULT window_proc(UINT msg, WPARAM wParam, LPARAM lParam) {
        bool handled = false;
        LRESULT result = 0;
        /*
        auto print_pointer_stat_fn = [&](const winrt::hstring& label, uint32_t pointer_id) {
            using namespace winrt;
            using namespace Windows::UI::Input;
            // NOTE: Win32 APIs involve manual calculation (DPI, origin, ...),
            //       whereas WinRT APIs do the heavy lifting and expose
            //       data with much higher precision
            auto pointer_point = PointerPoint::GetCurrentPoint(pointer_id);
            SetWindowTextW(m_hwnd, std::format(
                L"({}, InContact: {}, RightPressed: {}, x: {}, y: {}, Pressure: {})",
                label,
                pointer_point.IsInContact(),
                pointer_point.Properties().IsRightButtonPressed(),
                pointer_point.Position().X,
                pointer_point.Position().Y,
                pointer_point.Properties().Pressure()
            ).c_str());
        };
        */
        if (msg == WM_DESTROY) {
            result = 1;
            handled = true;
        }
        else if (msg == WM_PAINT) {
            KillTimer(m_hwnd, REDRAW_TIMER_ID);
            ValidateRect(m_hwnd, nullptr);
            this->window_repaint();
            result = 0;
            handled = true;
        }
        else if (msg == WM_SIZE) {
            RECT rt;
            GetClientRect(m_hwnd, &rt);
            this->window_size_changed(rt.right, rt.bottom);
        }
        else if (msg == WM_DISPLAYCHANGE) {
            InvalidateRect(m_hwnd, nullptr, false);
            result = 0;
            handled = true;
        }
        else if (msg == WM_POINTERUPDATE) {
            SetCursor(LoadCursorW(nullptr, IDC_ARROW));
            // print_pointer_stat_fn(L"WM_POINTERUPDATE", GET_POINTERID_WPARAM(wParam));
            on_pointer_update(
                winrt::Windows::UI::Input::PointerPoint::GetCurrentPoint(GET_POINTERID_WPARAM(wParam)));
            result = 0;
            handled = true;
        }
        else if (msg == WM_POINTERCAPTURECHANGED) {
            // print_pointer_stat_fn(L"WM_POINTERCAPTURECHANGED", GET_POINTERID_WPARAM(wParam));
            on_pointer_capture_changed(
                winrt::Windows::UI::Input::PointerPoint::GetCurrentPoint(GET_POINTERID_WPARAM(wParam)));
            result = 0;
            handled = true;
        }
        else if (msg == WM_POINTERDOWN) {
            // print_pointer_stat_fn(L"WM_POINTERDOWN", GET_POINTERID_WPARAM(wParam));
            on_pointer_down(
                winrt::Windows::UI::Input::PointerPoint::GetCurrentPoint(GET_POINTERID_WPARAM(wParam)));
            result = 0;
            handled = true;
        }
        else if (msg == WM_POINTERUP) {
            // print_pointer_stat_fn(L"WM_POINTERUP", GET_POINTERID_WPARAM(wParam));
            on_pointer_up(
                winrt::Windows::UI::Input::PointerPoint::GetCurrentPoint(GET_POINTERID_WPARAM(wParam)));
            result = 0;
            handled = true;
        }
        else if (msg == WM_POINTERENTER) {
            // print_pointer_stat_fn(L"WM_POINTERENTER", GET_POINTERID_WPARAM(wParam));
            on_pointer_enter(
                winrt::Windows::UI::Input::PointerPoint::GetCurrentPoint(GET_POINTERID_WPARAM(wParam)));
            result = 0;
            handled = true;
        }
        else if (msg == WM_POINTERLEAVE) {
            // print_pointer_stat_fn(L"WM_POINTERLEAVE", GET_POINTERID_WPARAM(wParam));
            on_pointer_leave(
                winrt::Windows::UI::Input::PointerPoint::GetCurrentPoint(GET_POINTERID_WPARAM(wParam)));
            result = 0;
            handled = true;
        }
        if (!handled) {
            result = DefWindowProcW(m_hwnd, msg, wParam, lParam);
        }
        return result;
    }

    int main_loop(void) {
        MSG msg;
        while (true) {
            if (m_cur_exception) { std::rethrow_exception(m_cur_exception); }
            if (!IsWindow(m_hwnd)) { break; }
            if (IsIconic(m_hwnd)) {
                ValidateRect(m_hwnd, nullptr);
            }
            MsgWaitForMultipleObjectsEx(0, nullptr, 0, 0, MWMO_ALERTABLE);
            auto ret = GetMessageW(&msg, nullptr, 0, 0);
            if (ret == -1) {
                winrt::throw_last_error();
            }
            if (ret == 0) {
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return 0;
    }

    void window_repaint(void) {
        HRESULT hr;
        m_d2d1_dev_ctx->BeginDraw();
        try { on_render(); m_d2d1_dev_ctx->EndDraw(); }
        catch (...) { m_d2d1_dev_ctx->EndDraw(); throw; }
        hr = m_dxgi_swapchain->Present(1, 0);
        if (hr != S_OK && hr != DXGI_STATUS_OCCLUDED) {
            recreate_device_resources();
            return on_render();
        }

        if (m_run_at_full_speed) {
            request_redraw();
        }
    }

    HWND get_underlying_handle(void) {
        return m_hwnd;
    }

    void request_redraw(void) {
        KillTimer(m_hwnd, REDRAW_TIMER_ID);
        InvalidateRect(m_hwnd, nullptr, false);
    }
    // NOTE: If any redraws or idle redraw requests happen during
    //       the period, the existing request will be reset
    template<typename Clock, typename Duration>
    void request_idle_redraw_until(const std::chrono::time_point<Clock, Duration>& tp) {
        auto wait_milliseconds = round<std::chrono::milliseconds>(tp - std::chrono::system_clock::now()).count();
        if (wait_milliseconds <= 0) {
            request_redraw();
            return;
        }
        SetTimer(m_hwnd, REDRAW_TIMER_ID, wait_milliseconds, [](HWND hwnd, UINT, UINT_PTR, DWORD) {
            KillTimer(hwnd, REDRAW_TIMER_ID);
            InvalidateRect(hwnd, nullptr, false);
        });
    }

    D2D1DrawingSession get_drawing_session(void) {
        return { this };
    }
    std::shared_ptr<D2D1ResourceManager> get_resource_manager(void) {
        return std::make_shared<D2D1ResourceManager>(this);
    }

protected:
    virtual void on_device_independent_resource_create(void) {}
    virtual void on_device_dependent_resource_create(void) {}
    virtual void on_window_size_dependent_resource_create(void) {}
    virtual void on_resize(uint32_t width, uint32_t height) {}
    virtual void on_render(void) {}
    virtual void on_pointer_update(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) {}
    virtual void on_pointer_capture_changed(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) {}
    virtual void on_pointer_down(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) {}
    virtual void on_pointer_up(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) {}
    virtual void on_pointer_enter(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) {}
    virtual void on_pointer_leave(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) {}

    HWND m_hwnd{};
    bool m_run_at_full_speed{ false };
    winrt::com_ptr<ID2D1Factory7> m_d2d1_factory;
    winrt::com_ptr<IWICImagingFactory> m_wic_factory;
    winrt::com_ptr<IDWriteFactory> m_dwrite_factory;
    winrt::com_ptr<IDXGISwapChain1> m_dxgi_swapchain;
    winrt::com_ptr<ID2D1DeviceContext5> m_d2d1_dev_ctx;

private:
    friend struct D2D1DrawingSession;
    friend struct D2D1ResourceManager;

    std::exception_ptr m_cur_exception = nullptr;
};

inline D2D1DrawingSession::D2D1DrawingSession(D2D1SimpleWindow* window) :
    m_d2d1_factory(window->m_d2d1_factory), m_d2d1_dev_ctx(window->m_d2d1_dev_ctx) {}
inline D2D1ResourceManager::D2D1ResourceManager(D2D1SimpleWindow* window) :
    m_dwrite_factory(window->m_dwrite_factory), m_window(window) {}
inline void D2D1ResourceManager::request_redraw(void) {
    m_window->request_redraw();
}

// NOTE: Imitates Windows XAML controls (WinUI)
namespace controls {
    enum class HorizontalAlignment {
        Left, Center, Right, Stretch,
    };
    enum class VerticalAlignment {
        Top, Center, Bottom, Stretch,
    };
    enum class Orientation {
        Vertical, Horizontal,
    };
    struct Thickness {
        float left, top, right, bottom;
    };
    enum class GridUnitType {
        Auto, Pixel, Star,
    };
    struct GridLength {
        float value;
        GridUnitType type;
    };
    struct RowDefinition {
        GridLength height;
        float actual_height;
    };
    struct ColumnDefinition {
        GridLength width;
        float actual_width;
    };

    namespace details {
        // Precondition: x1 <= x2
        std::pair<float, float> calc_alignment(float x1, float x2, float s, HorizontalAlignment align) {
            switch (align) {
            case HorizontalAlignment::Left:
                return { x1, x1 + s };
            case HorizontalAlignment::Center:
                return { x1 + (x2 - x1 - s) / 2, x1 + (x2 - x1 + s) / 2 };
            case HorizontalAlignment::Right:
                return { x2 - s, x2 };
            case HorizontalAlignment::Stretch:
                return { x1, x2 };
            default:
                throw winrt::hresult_illegal_method_call(L"Invalid HorizontalAlignment value");
            }
        }
        std::pair<float, float> calc_alignment(float y1, float y2, float s, VerticalAlignment align) {
            switch (align) {
            case VerticalAlignment::Top:
                return { y1, y1 + s };
            case VerticalAlignment::Center:
                return { y1 + (y2 - y1 - s) / 2, y1 + (y2 - y1 + s) / 2 };
            case VerticalAlignment::Bottom:
                return { y2 - s, y2 };
            case VerticalAlignment::Stretch:
                return { y1, y2 };
            default:
                throw winrt::hresult_illegal_method_call(L"Invalid VerticalAlignment value");
            }
        }
    }

    struct D2D1Control : std::enable_shared_from_this<D2D1Control> {
    protected:
        D2D1_SIZE_F m_desired_size{};
        D2D1_SIZE_F m_actual_size{};
        D2D1_POINT_2F m_actual_offset{};
    public:
        D2D1Control(std::shared_ptr<D2D1ResourceManager> res_mgr) : m_res_mgr(std::move(res_mgr)) {}

        // Primitive two-pass layout
        void measure(const D2D1_SIZE_F& available_size) { on_measure(available_size); }
        void arrange(const D2D1_RECT_F& final_rect) { on_arrange(final_rect); }

        const D2D1_SIZE_F& desired_size{ m_desired_size };
        const D2D1_SIZE_F& actual_size{ m_actual_size };
        const D2D1_POINT_2F& actual_offset{ m_actual_offset };

        void margin(const Thickness& value) {
            m_margin = value;
            // NOTE: Caller should update layout
        }
        Thickness margin() { return m_margin; }

        void draw(D2D1DrawingSession& session) { on_render(session); }
        void trigger_device_lost(void) { on_device_lost(); }
        std::shared_ptr<D2D1Control> hit_test(const D2D1_POINT_2F& pt) { return on_hit_test(pt); }
        void trigger_pointer_entered(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) {
            on_pointer_entered(ptr_pt);
        }
        void trigger_pointer_exited(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) {
            on_pointer_exited(ptr_pt);
        }
        void trigger_pointer_moved(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) {
            on_pointer_moved(ptr_pt);
        }
        void trigger_pointer_pressed(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) {
            on_pointer_pressed(ptr_pt);
        }
        void trigger_pointer_released(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) {
            on_pointer_released(ptr_pt);
        }
        // NOTE: Analogy of abnormal pointer up
        void trigger_pointer_canceled(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) {
            on_pointer_canceled(ptr_pt);
        }

        void horizontal_alignment(HorizontalAlignment value) { m_horizontal_alignment = value; }
        HorizontalAlignment horizontal_alignment() { return m_horizontal_alignment; }
        void vertical_alignment(VerticalAlignment value) { m_vertical_alignment = value; }
        VerticalAlignment vertical_alignment() { return m_vertical_alignment; }

        // NOTE: Introduce property store to enable use cases like
        //       attached property in Windows XAML (Windows.UI.Xaml.DependencyObject)
        void set_prop(winrt::hstring key, std::any value) {
            m_props_store.insert_or_assign(std::move(key), std::move(value));
            on_prop_change();
        }
        template<typename T>
        std::optional<T> try_get_prop(const winrt::hstring& key) {
            auto it = m_props_store.find(key);
            if (it == m_props_store.end()) { return std::nullopt; }
            try { return std::any_cast<T>(it->second); }
            catch (const std::bad_any_cast&) {}
            return std::nullopt;
        }
        std::any try_get_prop(const winrt::hstring& key) {
            auto it = m_props_store.find(key);
            if (it == m_props_store.end()) { return {}; }
            return it->second;
        }
        void clear_props(void) {
            m_props_store.clear();
            on_prop_change();
        }
    protected:
        virtual void on_measure(const D2D1_SIZE_F& available_size) {
            m_desired_size = {};
        }
        virtual void on_arrange(const D2D1_RECT_F& final_rect) {
            m_actual_size = size_of(final_rect);
            m_actual_offset = { final_rect.left, final_rect.top };
        }
        virtual void on_render(D2D1DrawingSession& session) {}
        virtual void on_prop_change(void) {}
        // NOTE: This callback provides an opportunity to recreate
        //       resources invalidated due to device lost
        virtual void on_device_lost(void) {}
        virtual std::shared_ptr<D2D1Control> on_hit_test(const D2D1_POINT_2F& pt) {
            auto rt = D2D1::RectF(
                m_actual_offset.x,
                m_actual_offset.y,
                m_actual_offset.x + m_actual_size.width,
                m_actual_offset.y + m_actual_size.height
            );
            return ::hit_test(pt, rt) ? shared_from_this() : nullptr;
        }
        virtual void on_pointer_entered(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) {}
        virtual void on_pointer_exited(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) {}
        virtual void on_pointer_moved(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) {}
        virtual void on_pointer_pressed(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) {}
        virtual void on_pointer_released(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) {}
        virtual void on_pointer_canceled(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) {}

        std::unordered_map<winrt::hstring, std::any> m_props_store;
        Thickness m_margin{};
        HorizontalAlignment m_horizontal_alignment = HorizontalAlignment::Stretch;
        VerticalAlignment m_vertical_alignment = VerticalAlignment::Stretch;
        std::shared_ptr<D2D1ResourceManager> m_res_mgr;
    };
    // NOTE: Requires props "Grid.Row", "Grid.Column"
    struct D2D1Grid : D2D1Control {
        using D2D1Control::D2D1Control;
        auto& children(void) { return m_children; }
        auto& row_definitions(void) { return m_row_defs; }
        auto& column_definitions(void) { return m_col_defs; }
        static void set_row(const std::shared_ptr<D2D1Control>& ctrl, size_t idx) {
            ctrl->set_prop(L"Grid.Row", idx);
        }
        static void set_column(const std::shared_ptr<D2D1Control>& ctrl, size_t idx) {
            ctrl->set_prop(L"Grid.Column", idx);
        }
    protected:
        void on_device_lost(void) override {
            for (auto&& i : m_children) {
                i->trigger_device_lost();
            }
        }
        void on_measure(const D2D1_SIZE_F& available_size) override {
            std::vector<float> rows_size(1), cols_size(1);
            float row_star_sum = 0, col_star_sum = 0;
            if (m_row_defs.size() == 0) {
                row_star_sum = 1;
            }
            else {
                rows_size.resize(m_row_defs.size());
                for (auto&& i : m_row_defs) {
                    if (i.height.type != GridUnitType::Star) { continue; }
                    row_star_sum += i.height.value;
                }
            }
            if (m_col_defs.size() == 0) {
                col_star_sum = 1;
            }
            else {
                cols_size.resize(m_col_defs.size());
                for (auto&& i : m_col_defs) {
                    if (i.width.type != GridUnitType::Star) { continue; }
                    col_star_sum += i.width.value;
                }
            }
            auto get_row_def_fn = [this](size_t idx) -> RowDefinition {
                if (m_row_defs.size() == 0) { return { .height = { 1, GridUnitType::Star } }; }
                return m_row_defs.get_at(idx);
            };
            auto get_col_def_fn = [this](size_t idx) -> ColumnDefinition {
                if (m_col_defs.size() == 0) { return { .width = { 1, GridUnitType::Star } }; }
                return m_col_defs.get_at(idx);
            };
            // GridUnitType::Pixel
            for (size_t i = 0; i < m_row_defs.size(); i++) {
                auto&& len = m_row_defs.get_at(i);
                if (len.height.type == GridUnitType::Pixel) {
                    rows_size[i] = len.height.value;
                }
            }
            for (size_t i = 0; i < m_col_defs.size(); i++) {
                auto&& len = m_col_defs.get_at(i);
                if (len.width.type == GridUnitType::Pixel) {
                    cols_size[i] = len.width.value;
                }
            }
            // GridUnitType::Auto & GridUnitType::Star
            float row_remaining = available_size.height, column_remaining = available_size.width;
            auto update_row_remaining_fn = [&] {
                float new_value = available_size.height;
                for (size_t i = 0; i < m_row_defs.size(); i++) {
                    if (m_row_defs.get_at(i).height.type == GridUnitType::Star) { continue; }
                    new_value -= rows_size[i];
                }
                if (new_value < 0) { new_value = 0; }
                if (new_value != row_remaining) {
                    row_remaining = new_value;
                    return true;
                }
                return false;
            };
            auto update_column_remaining_fn = [&] {
                float new_value = available_size.width;
                for (size_t i = 0; i < m_col_defs.size(); i++) {
                    if (m_col_defs.get_at(i).width.type == GridUnitType::Star) { continue; }
                    new_value -= cols_size[i];
                }
                if (new_value < 0) { new_value = 0; }
                if (new_value != column_remaining) {
                    column_remaining = new_value;
                    return true;
                }
                return false;
            };
            auto measure_update_fn = [&](const std::shared_ptr<D2D1Control>& i) {
                D2D1_SIZE_F avail_size;
                auto rowidx = i->try_get_prop<size_t>(L"Grid.Row").value_or(0);
                auto colidx = i->try_get_prop<size_t>(L"Grid.Column").value_or(0);
                if (m_row_defs.size() > 0 && rowidx >= m_row_defs.size()) { rowidx = m_row_defs.size() - 1; }
                if (m_col_defs.size() > 0 && colidx >= m_col_defs.size()) { colidx = m_col_defs.size() - 1; }
                auto rowdef = get_row_def_fn(rowidx);
                auto coldef = get_col_def_fn(colidx);
                if (rowdef.height.type == GridUnitType::Pixel) {
                    avail_size.height = rows_size[rowidx];
                }
                else if (rowdef.height.type == GridUnitType::Auto) {
                    avail_size.height = FLT_MAX;
                }
                else if (rowdef.height.type == GridUnitType::Star) {
                    avail_size.height = row_remaining / row_star_sum * rowdef.height.value;
                }
                else {
                    throw winrt::hresult_illegal_method_call(L"Invalid GridUnitType");
                }
                if (coldef.width.type == GridUnitType::Pixel) {
                    avail_size.width = cols_size[colidx];
                }
                else if (coldef.width.type == GridUnitType::Auto) {
                    avail_size.width = FLT_MAX;
                }
                else if (coldef.width.type == GridUnitType::Star) {
                    avail_size.width = column_remaining / col_star_sum * coldef.width.value;
                }
                else {
                    throw winrt::hresult_illegal_method_call(L"Invalid GridUnitType");
                }
                auto margin = i->margin();
                avail_size.width -= margin.left + margin.right;
                avail_size.height -= margin.top + margin.bottom;
                if (avail_size.width < 0) { avail_size.width = 0; }
                if (avail_size.height < 0) { avail_size.height = 0; }
                i->measure(avail_size);
                auto desired_size = i->desired_size;
                desired_size.width += margin.left + margin.right;
                desired_size.height += margin.top + margin.bottom;
                if (rowdef.height.type != GridUnitType::Pixel) {
                    rows_size[rowidx] = std::max(rows_size[rowidx], desired_size.height);
                }
                if (coldef.width.type != GridUnitType::Pixel) {
                    cols_size[colidx] = std::max(cols_size[colidx], desired_size.width);
                }
            };
            int layout_counter = 0;
            while (true) {
                if (layout_counter++ >= 10) {
                    throw winrt::hresult_error(E_FAIL, L"Detected potential layout cycle");
                }
                // Reset previous data
                for (size_t i = 0; i < rows_size.size(); i++) {
                    if (get_row_def_fn(i).height.type != GridUnitType::Pixel) {
                        rows_size[i] = 0;
                    }
                }
                for (size_t i = 0; i < cols_size.size(); i++) {
                    if (get_col_def_fn(i).width.type != GridUnitType::Pixel) {
                        cols_size[i] = 0;
                    }
                }
                // Iterate calculation
                for (auto&& i : m_children) {
                    measure_update_fn(i);
                }
                if (update_row_remaining_fn() || update_column_remaining_fn()) {
                    // Start a new iteration to apply updated size
                    continue;
                }
                // Layout is now finished
                break;
            }
            // NOTE: Correctly arrange star rows & columns
            if (row_star_sum != 0) {
                float max_ratio = 0;
                for (size_t i = 0; i < m_row_defs.size(); i++) {
                    auto&& rowdef = m_row_defs.get_at(i);
                    if (rowdef.height.type != GridUnitType::Star) { continue; }
                    max_ratio = std::max(max_ratio, rows_size[i] / rowdef.height.value);
                }
                for (size_t i = 0; i < m_row_defs.size(); i++) {
                    auto&& rowdef = m_row_defs.get_at(i);
                    if (rowdef.height.type != GridUnitType::Star) { continue; }
                    rows_size[i] = max_ratio * rowdef.height.value;
                }
            }
            if (col_star_sum != 0) {
                float max_ratio = 0;
                for (size_t i = 0; i < m_col_defs.size(); i++) {
                    auto&& coldef = m_col_defs.get_at(i);
                    if (coldef.width.type != GridUnitType::Star) { continue; }
                    max_ratio = std::max(max_ratio, cols_size[i] / coldef.width.value);
                }
                for (size_t i = 0; i < m_col_defs.size(); i++) {
                    auto&& coldef = m_col_defs.get_at(i);
                    if (coldef.width.type != GridUnitType::Star) { continue; }
                    cols_size[i] = max_ratio * coldef.width.value;
                }
            }
            // Collect results
            m_desired_size = {};
            for (const auto& i : rows_size) {
                m_desired_size.height += i;
            }
            for (const auto& i : cols_size) {
                m_desired_size.width += i;
            }
        }
        void on_arrange(const D2D1_RECT_F& final_rect) override {
            m_actual_size = size_of(final_rect);
            m_actual_offset = { final_rect.left, final_rect.top };
            std::vector<float> rows_size(1), cols_size(1);
            float row_star_sum = 0, col_star_sum = 0;
            if (m_row_defs.size() == 0) {
                row_star_sum = 1;
            }
            else {
                rows_size.resize(m_row_defs.size());
                for (auto&& i : m_row_defs) {
                    if (i.height.type != GridUnitType::Star) { continue; }
                    row_star_sum += i.height.value;
                }
            }
            if (m_col_defs.size() == 0) {
                col_star_sum = 1;
            }
            else {
                cols_size.resize(m_col_defs.size());
                for (auto&& i : m_col_defs) {
                    if (i.width.type != GridUnitType::Star) { continue; }
                    col_star_sum += i.width.value;
                }
            }
            auto get_row_def_fn = [this](size_t idx) -> RowDefinition {
                if (m_row_defs.size() == 0) { return { .height = { 1, GridUnitType::Star } }; }
                return m_row_defs.get_at(idx);
            };
            auto get_col_def_fn = [this](size_t idx) -> ColumnDefinition {
                if (m_col_defs.size() == 0) { return { .width = { 1, GridUnitType::Star } }; }
                return m_col_defs.get_at(idx);
            };
            for (size_t i = 0; i < m_row_defs.size(); i++) {
                auto&& len = m_row_defs.get_at(i);
                if (len.height.type == GridUnitType::Pixel) {
                    rows_size[i] = len.height.value;
                }
            }
            for (size_t i = 0; i < m_col_defs.size(); i++) {
                auto&& len = m_col_defs.get_at(i);
                if (len.width.type == GridUnitType::Pixel) {
                    cols_size[i] = len.width.value;
                }
            }
            for (auto&& i : m_children) {
                auto rowidx = i->try_get_prop<size_t>(L"Grid.Row").value_or(0);
                auto colidx = i->try_get_prop<size_t>(L"Grid.Column").value_or(0);
                if (m_row_defs.size() > 0 && rowidx >= m_row_defs.size()) { rowidx = m_row_defs.size() - 1; }
                if (m_col_defs.size() > 0 && colidx >= m_col_defs.size()) { colidx = m_col_defs.size() - 1; }
                auto rowdef = get_row_def_fn(rowidx);
                auto coldef = get_col_def_fn(colidx);
                auto margin = i->margin();
                auto desired_size = i->desired_size;
                desired_size.width += margin.left + margin.right;
                desired_size.height += margin.top + margin.bottom;
                if (rowdef.height.type != GridUnitType::Pixel) {
                    rows_size[rowidx] = std::max(rows_size[rowidx], desired_size.height);
                }
                if (coldef.width.type != GridUnitType::Pixel) {
                    cols_size[colidx] = std::max(cols_size[colidx], desired_size.width);
                }
            }
            float row_remaining = m_actual_size.height, column_remaining = m_actual_size.width;
            auto update_row_remaining_fn = [&] {
                float new_value = m_actual_size.height;
                for (size_t i = 0; i < m_row_defs.size(); i++) {
                    if (m_row_defs.get_at(i).height.type == GridUnitType::Star) { continue; }
                    new_value -= rows_size[i];
                }
                if (new_value < 0) { new_value = 0; }
                if (new_value != row_remaining) {
                    row_remaining = new_value;
                    return true;
                }
                return false;
            };
            auto update_column_remaining_fn = [&] {
                float new_value = m_actual_size.width;
                for (size_t i = 0; i < m_col_defs.size(); i++) {
                    if (m_col_defs.get_at(i).width.type == GridUnitType::Star) { continue; }
                    new_value -= cols_size[i];
                }
                if (new_value < 0) { new_value = 0; }
                if (new_value != column_remaining) {
                    column_remaining = new_value;
                    return true;
                }
                return false;
            };
            update_row_remaining_fn();
            update_column_remaining_fn();
            if (row_star_sum != 0) {
                for (size_t i = 0; i < rows_size.size(); i++) {
                    auto&& rowdef = get_row_def_fn(i);
                    if (rowdef.height.type != GridUnitType::Star) { continue; }
                    rows_size[i] = row_remaining / row_star_sum * rowdef.height.value;
                }
            }
            if (col_star_sum != 0) {
                for (size_t i = 0; i < cols_size.size(); i++) {
                    auto&& coldef = get_col_def_fn(i);
                    if (coldef.width.type != GridUnitType::Star) { continue; }
                    cols_size[i] = column_remaining / col_star_sum * coldef.width.value;
                }
            }
            // Set actual size
            // TODO: Temporaily suppress notifications here
            for (size_t i = 0; i < m_row_defs.size(); i++) {
                auto&& rowdef = m_row_defs.get_at(i);
                rowdef.actual_height = rows_size[i];
                m_row_defs.set_at(i, std::move(rowdef));
            }
            for (size_t i = 0; i < m_col_defs.size(); i++) {
                auto&& coldef = m_col_defs.get_at(i);
                coldef.actual_width = cols_size[i];
                m_col_defs.set_at(i, std::move(coldef));
            }
            // Arrange children
            auto gen_sum_vec = [](const std::vector<float>& vec, float first_val) {
                std::vector<float> sum_vec;
                sum_vec.reserve(vec.size() + 1);
                sum_vec.push_back(first_val);
                std::copy(vec.begin(), vec.end(), std::back_inserter(sum_vec));
                std::partial_sum(sum_vec.begin(), sum_vec.end(), sum_vec.begin());
                return sum_vec;
            };
            std::vector<float> rows_sum_size = gen_sum_vec(rows_size, m_actual_offset.y);
            std::vector<float> cols_sum_size = gen_sum_vec(cols_size, m_actual_offset.x);
            for (auto&& i : m_children) {
                D2D1_RECT_F child_rt;
                auto rowidx = i->try_get_prop<size_t>(L"Grid.Row").value_or(0);
                auto colidx = i->try_get_prop<size_t>(L"Grid.Column").value_or(0);
                if (m_row_defs.size() > 0 && rowidx >= m_row_defs.size()) { rowidx = m_row_defs.size() - 1; }
                if (m_col_defs.size() > 0 && colidx >= m_col_defs.size()) { colidx = m_col_defs.size() - 1; }
                auto margin = i->margin();
                auto desired_size = i->desired_size;
                desired_size.width += margin.left + margin.right;
                desired_size.height += margin.top + margin.bottom;
                std::tie(child_rt.top, child_rt.bottom) = details::calc_alignment(
                    rows_sum_size[rowidx], rows_sum_size[rowidx + 1],
                    desired_size.height,
                    i->vertical_alignment()
                );
                std::tie(child_rt.left, child_rt.right) = details::calc_alignment(
                    cols_sum_size[colidx], cols_sum_size[colidx + 1],
                    desired_size.width,
                    i->horizontal_alignment()
                );
                child_rt.top += margin.top;
                child_rt.bottom -= margin.bottom;
                child_rt.left += margin.left;
                child_rt.right -= margin.right;
                i->arrange(child_rt);
            }
        }
        void on_render(D2D1DrawingSession& session) override {
            for (auto&& i : m_children) {
                i->draw(session);
            }
        }
        std::shared_ptr<D2D1Control> on_hit_test(const D2D1_POINT_2F& pt) override {
            auto rt = D2D1::RectF(
                m_actual_offset.x,
                m_actual_offset.y,
                m_actual_offset.x + m_actual_size.width,
                m_actual_offset.y + m_actual_size.height
            );
            if (!::hit_test(pt, rt)) { return nullptr; }
            for (auto&& i : m_children | std::views::reverse) {
                if (auto p = i->hit_test(pt)) { return p; }
            }
            return shared_from_this();
        }
    private:
        ObservableVector<std::shared_ptr<D2D1Control>> m_children;
        ObservableVector<RowDefinition> m_row_defs;
        ObservableVector<ColumnDefinition> m_col_defs;
    };
    struct D2D1StackPanel : D2D1Control {
        using D2D1Control::D2D1Control;
        auto& children(void) { return m_children; }
        void orientation(Orientation value) {
            switch (value) {
            case Orientation::Horizontal:
            case Orientation::Vertical:
                break;
            default:
                throw winrt::hresult_invalid_argument(L"Invalid Orientation");
            }
            m_orientation = value;
            // NOTE: Caller should update layout
        }
        Orientation orientation(void) { return m_orientation; }
        void spacing(float value) {
            m_spacing = value;
            // NOTE: Caller should update layout
        }
        float spacing(void) { return m_spacing; }
    protected:
        void on_device_lost(void) override {
            for (auto&& i : m_children) {
                i->trigger_device_lost();
            }
        }
        void on_measure(const D2D1_SIZE_F& available_size) override {
            D2D1_SIZE_F size{};
            if (m_orientation == Orientation::Horizontal) {
                bool first = true;
                for (auto&& i : m_children) {
                    if (!first) { size.width += m_spacing; }
                    auto margin = i->margin();
                    i->measure(D2D1::SizeF(FLT_MAX, available_size.height - (margin.top + margin.bottom)));
                    size.width += i->desired_size.width + margin.left + margin.right;
                    size.height = std::max(size.height, i->desired_size.height + margin.top + margin.bottom);
                    first = false;
                }
            }
            else if (m_orientation == Orientation::Vertical) {
                bool first = true;
                for (auto&& i : m_children) {
                    if (!first) { size.height += m_spacing; }
                    auto margin = i->margin();
                    i->measure(D2D1::SizeF(available_size.width - (margin.left + margin.right), FLT_MAX));
                    size.width = std::max(size.width, i->desired_size.width + margin.left + margin.right);
                    size.height += i->desired_size.height + margin.top + margin.bottom;
                    first = false;
                }
            }
            else throw winrt::hresult_illegal_method_call(L"Unreachable path");
            m_desired_size = size;
        }
        void on_arrange(const D2D1_RECT_F& final_rect) override {
            m_actual_size = size_of(final_rect);
            m_actual_offset = { final_rect.left, final_rect.top };
            D2D1_RECT_F child_rt{};
            if (m_orientation == Orientation::Horizontal) {
                bool first = true;
                child_rt.left = child_rt.right = m_actual_offset.x;
                for (auto&& i : m_children) {
                    if (!first) {
                        child_rt.left += m_spacing;
                        child_rt.right += m_spacing;
                    }
                    auto margin = i->margin();
                    std::tie(child_rt.top, child_rt.bottom) = details::calc_alignment(
                        final_rect.top, final_rect.bottom,
                        i->desired_size.height + margin.top + margin.bottom,
                        i->vertical_alignment()
                    );
                    child_rt.top += margin.top;
                    child_rt.bottom -= margin.bottom;
                    child_rt.left += margin.left;
                    child_rt.right += margin.left + i->desired_size.width;
                    i->arrange(child_rt);
                    child_rt.left += i->desired_size.width + margin.right;
                    child_rt.right += margin.right;
                    first = false;
                }
            }
            else if (m_orientation == Orientation::Vertical) {
                bool first = true;
                child_rt.top = child_rt.bottom = m_actual_offset.y;
                for (auto&& i : m_children) {
                    if (!first) {
                        child_rt.top += m_spacing;
                        child_rt.bottom += m_spacing;
                    }
                    auto margin = i->margin();
                    std::tie(child_rt.left, child_rt.right) = details::calc_alignment(
                        final_rect.left, final_rect.right,
                        i->desired_size.width + margin.left + margin.right,
                        i->horizontal_alignment()
                    );
                    child_rt.left += margin.left;
                    child_rt.right -= margin.right;
                    child_rt.top += margin.top;
                    child_rt.bottom += margin.top + i->desired_size.height;
                    i->arrange(child_rt);
                    child_rt.top += i->desired_size.height + margin.bottom;
                    child_rt.bottom += margin.bottom;
                    first = false;
                }
            }
            else throw winrt::hresult_illegal_method_call(L"Unreachable path");
        }
        void on_render(D2D1DrawingSession& session) override {
            for (auto&& i : m_children) {
                i->draw(session);
            }
        }
        std::shared_ptr<D2D1Control> on_hit_test(const D2D1_POINT_2F& pt) override {
            auto rt = D2D1::RectF(
                m_actual_offset.x,
                m_actual_offset.y,
                m_actual_offset.x + m_actual_size.width,
                m_actual_offset.y + m_actual_size.height
            );
            if (!::hit_test(pt, rt)) { return nullptr; }
            for (auto&& i : m_children | std::views::reverse) {
                if (auto p = i->hit_test(pt)) { return p; }
            }
            return shared_from_this();
        }
    private:
        ObservableVector<std::shared_ptr<D2D1Control>> m_children;
        Orientation m_orientation = Orientation::Horizontal;
        float m_spacing = 0;
    };
    struct D2D1TextBlock : D2D1Control {
        D2D1TextBlock(std::shared_ptr<D2D1ResourceManager> res_mgr) : D2D1Control(std::move(res_mgr)) {
            D2D1TextBlock::on_device_lost();
        }
        void font_size(float value) {
            if (value <= 0) { throw winrt::hresult_invalid_argument(L"Invalid font size"); }
            m_font_size = value;
            // NOTE: Caller should update layout
        }
        float font_size(void) { return m_font_size; }
        void text(winrt::hstring value) {
            m_text = std::move(value);
            // NOTE: Caller should update layout
        }
        winrt::hstring text(void) { return m_text; }
    protected:
        void on_device_lost(void) override {
            m_txt_fmt = m_res_mgr->create_text_format(
                L"Microsoft YaHei",
                DWRITE_FONT_WEIGHT_REGULAR,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                m_font_size
            );
            m_txt_layout = m_res_mgr->create_text_layout(
                m_text, m_txt_fmt, m_actual_size.width, m_actual_size.height
            );
        }
        void on_measure(const D2D1_SIZE_F& available_size) override {
            auto txt_layout = m_res_mgr->create_text_layout(
                m_text, m_txt_fmt, available_size.width, available_size.height
            );
            DWRITE_TEXT_METRICS txt_metrics;
            winrt::check_hresult(txt_layout->GetMetrics(&txt_metrics));
            // NOTE: Fix floating-point errors
            m_desired_size = { txt_metrics.width + 1e-3f, txt_metrics.height };
        }
        void on_arrange(const D2D1_RECT_F& final_rect) override {
            m_actual_size = size_of(final_rect);
            m_actual_offset = { final_rect.left, final_rect.top };
            m_txt_layout = m_res_mgr->create_text_layout(
                m_text, m_txt_fmt, m_actual_size.width, m_actual_size.height
            );
        }
        void on_render(D2D1DrawingSession& session) override {
            session.draw_text_layout(m_actual_offset, m_txt_layout, D2D1::ColorF(D2D1::ColorF::Black));
        }
    private:
        winrt::com_ptr<IDWriteTextFormat> m_txt_fmt;
        winrt::com_ptr<IDWriteTextLayout> m_txt_layout;
        winrt::hstring m_text = L"";
        float m_font_size = 16;
    };
    struct D2D1Button : D2D1Control {
        static constexpr float PADDING_H = 10, PADDING_V = 6;
        D2D1Button(std::shared_ptr<D2D1ResourceManager> res_mgr) : D2D1Control(std::move(res_mgr)) {
            D2D1Button::on_device_lost();
        }
        void content(std::shared_ptr<D2D1Control> value) {
            m_child = std::move(value);
            // NOTE: Caller should update layout
        }
        std::shared_ptr<D2D1Control> content(void) { return m_child; }
        winrt::event_token click(winrt::delegate<> delegate) {
            return m_ev_click.add(delegate);
        }
        void click(winrt::event_token token) {
            m_ev_click.remove(token);
        }
    protected:
        void on_device_lost(void) override {
            if (m_child) {
                m_child->trigger_device_lost();
            }
        }
        void on_measure(const D2D1_SIZE_F& available_size) override {
            auto child_avail_size = D2D1::SizeF(
                available_size.width - PADDING_H * 2,
                available_size.height - PADDING_V * 2
            );
            D2D1_SIZE_F child_desired_size{};
            if (m_child) {
                auto child_margin = m_child->margin();
                child_avail_size.width -= child_margin.left + child_margin.right;
                child_avail_size.height -= child_margin.top + child_margin.bottom;
                if (child_avail_size.width < 0) { child_avail_size.width = 0; }
                if (child_avail_size.height < 0) { child_avail_size.height = 0; }
                m_child->measure(child_avail_size);
                child_desired_size = m_child->desired_size;
                child_desired_size.width += child_margin.left + child_margin.right;
                child_desired_size.height += child_margin.top + child_margin.bottom;
            }
            m_desired_size = {
                child_desired_size.width + PADDING_H * 2,
                child_desired_size.height + PADDING_V * 2
            };
        }
        void on_arrange(const D2D1_RECT_F& final_rect) override {
            m_actual_size = size_of(final_rect);
            m_actual_offset = { final_rect.left, final_rect.top };
            if (m_child) {
                auto margin = m_child->margin();
                auto child_rt = D2D1::RectF(
                    final_rect.left + PADDING_H + margin.left,
                    final_rect.top + PADDING_V + margin.top,
                    final_rect.right - PADDING_H - margin.right,
                    final_rect.bottom - PADDING_V - margin.bottom
                );
                child_rt.right = std::max(child_rt.right, child_rt.left);
                child_rt.bottom = std::max(child_rt.bottom, child_rt.top);
                m_child->arrange(child_rt);
            }
        }
        void on_render(D2D1DrawingSession& session) override {
            auto rect = D2D1::RectF(
                m_actual_offset.x,
                m_actual_offset.y,
                m_actual_offset.x + m_actual_size.width,
                m_actual_offset.y + m_actual_size.height
            );
            // None: SystemBaseLowColor(#33000000)
            // PointerOver: {SystemBaseHighColor(#ff000000), opacity=0.1}
            // PointerPressed: SystemBaseMediumLowColor(#66000000)
            D2D1_COLOR_F bkg_clr;
            switch (m_visual_state) {
            case ButtonVisualState::PointerOver:
                bkg_clr = D2D1::ColorF(0, 0, 0, 0.1f);
                break;
            case ButtonVisualState::Pressed:
                bkg_clr = D2D1::ColorF(0, 0, 0, 0x66 / 255.0f);
                break;
            default:
                bkg_clr = D2D1::ColorF(0, 0, 0, 0x33 / 255.0f);
                break;
            }
            session.fill_rectangle(rect, bkg_clr);
            if (m_child) {
                m_child->draw(session);
            }
        }
        void on_pointer_entered(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) override {
            update_visual_state(ButtonVisualState::PointerOver);
        }
        void on_pointer_exited(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) override {
            update_visual_state(ButtonVisualState::None);
        }
        void on_pointer_moved(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) override {
            auto rt = D2D1::RectF(
                m_actual_offset.x,
                m_actual_offset.y,
                m_actual_offset.x + m_actual_size.width,
                m_actual_offset.y + m_actual_size.height
            );
            auto ptr_pos = ptr_pt.Position();
            auto pt = D2D1::Point2F(ptr_pos.X, ptr_pos.Y);
            if (::hit_test(pt, rt)) {
                if (m_pressed) {
                    update_visual_state(ButtonVisualState::Pressed);
                }
                else {
                    update_visual_state(ButtonVisualState::PointerOver);
                }
            }
            else {
                update_visual_state(ButtonVisualState::None);
            }
        }
        void on_pointer_pressed(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) override {
            if (ptr_pt.Properties().IsLeftButtonPressed()) {
                update_visual_state(ButtonVisualState::Pressed);
                m_pressed = true;
            }
        }
        void on_pointer_released(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) override {
            auto rt = D2D1::RectF(
                m_actual_offset.x,
                m_actual_offset.y,
                m_actual_offset.x + m_actual_size.width,
                m_actual_offset.y + m_actual_size.height
            );
            auto ptr_pos = ptr_pt.Position();
            auto pt = D2D1::Point2F(ptr_pos.X, ptr_pos.Y);
            if (::hit_test(pt, rt)) {
                if (m_pressed) {
                    m_ev_click();
                }
                update_visual_state(ButtonVisualState::PointerOver);
            }
            else {
                update_visual_state(ButtonVisualState::None);
            }
            m_pressed = false;
        }
        void on_pointer_canceled(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) override {
            update_visual_state(ButtonVisualState::None);
            m_pressed = false;
        }
    private:
        std::shared_ptr<D2D1Control> m_child;
        enum ButtonVisualState {
            None, PointerOver, Pressed,
        } m_visual_state = None;
        bool m_pressed = false;
        winrt::event<winrt::delegate<>> m_ev_click;

        void update_visual_state(ButtonVisualState new_state) {
            if (m_visual_state != new_state) {
                m_visual_state = new_state;
                m_res_mgr->request_redraw();
            }
        }
    };
    struct D2D1Slider : D2D1Control {
        static constexpr float TRACK_HEIGHT = 2, THUMB_WIDTH = 8, THUMB_HEIGHT = 24;
        D2D1Slider(std::shared_ptr<D2D1ResourceManager> res_mgr) : D2D1Control(std::move(res_mgr)) {
            D2D1Slider::on_device_lost();
        }
        void minimum(double value) {
            auto real_val = std::lerp(m_cur_min, m_cur_max, m_cur_proportion);
            m_cur_min = value;
            m_cur_max = std::max(m_cur_min, m_cur_max);
            // NOTE: We lock value to minimum in circumstances where minimum >= maximum
            //       (min >= max is allowed on purpose)
            if (m_cur_min >= m_cur_max) {
                update_current_proportion(0);
            }
            else {
                real_val = std::clamp(real_val, m_cur_min, m_cur_max);
                auto new_proportion = (real_val - m_cur_min) / (m_cur_max - m_cur_min);
                update_current_proportion(new_proportion);
            }
        }
        double minimum() { return m_cur_min; }
        void maximum(double value) {
            auto real_val = std::lerp(m_cur_min, m_cur_max, m_cur_proportion);
            m_cur_max = value;
            if (m_cur_min >= m_cur_max) {
                update_current_proportion(0);
            }
            else {
                real_val = std::clamp(real_val, m_cur_min, m_cur_max);
                auto new_proportion = (real_val - m_cur_min) / (m_cur_max - m_cur_min);
                update_current_proportion(new_proportion);
            }
        }
        double maximum() { return m_cur_max; }
        void value(double value) {
            if (m_cur_min >= m_cur_max) {
                update_current_proportion(0);
            }
            else {
                auto real_val = std::clamp(value, m_cur_min, m_cur_max);
                auto new_proportion = (real_val - m_cur_min) / (m_cur_max - m_cur_min);
                update_current_proportion(new_proportion);
            }
        }
        double value() {
            return std::lerp(m_cur_min, m_cur_max, m_cur_proportion);
        }
        // NOTE: delegate(new_value)
        winrt::event_token value_changed(winrt::delegate<double> delegate) {
            return m_ev_value_changed.add(delegate);
        }
        void value_changed(winrt::event_token token) {
            m_ev_value_changed.remove(token);
        }
    protected:
        void on_measure(const D2D1_SIZE_F& available_size) override {
            m_desired_size = { std::clamp(available_size.width, THUMB_WIDTH, 1e9f), THUMB_HEIGHT };
        }
        void on_arrange(const D2D1_RECT_F& final_rect) override {
            m_actual_size = size_of(final_rect);
            m_actual_offset = { final_rect.left, final_rect.top };
        }
        void on_render(D2D1DrawingSession& session) override {
            float track_size = std::max(m_actual_size.width - THUMB_WIDTH, 1e-3f);
            auto track_rt = D2D1::RectF(
                m_actual_offset.x + THUMB_WIDTH / 2,
                m_actual_offset.y + (m_actual_size.height - TRACK_HEIGHT) / 2,
                m_actual_offset.x + THUMB_WIDTH / 2 + track_size,
                m_actual_offset.y + (m_actual_size.height + TRACK_HEIGHT) / 2
            );
            auto thumb_rt = D2D1::RectF(
                static_cast<float>(m_actual_offset.x + track_size * m_cur_proportion),
                m_actual_offset.y,
                static_cast<float>(m_actual_offset.x + track_size * m_cur_proportion + THUMB_WIDTH),
                m_actual_offset.y + m_actual_size.height
            );
            // Thumb | Track
            // None: SystemAccentColor | SystemBaseMediumLowColor(#66000000)
            // PointerOver: SystemAccentColorLight1 | SystemBaseMediumColor(#99000000)
            // PointerPressed: SystemAccentColorDark1 | SystemBaseMediumLowColor(#66000000)
            D2D1_COLOR_F thumb_clr, track_clr;
            switch (m_visual_state) {
            case SliderVisualState::PointerOver:
                thumb_clr = get_system_accent_light1_color();
                track_clr = D2D1::ColorF(0, 0, 0, 0x66 / 255.0f);
                break;
            case SliderVisualState::Pressed:
                thumb_clr = get_system_accent_dark1_color();
                track_clr = D2D1::ColorF(0, 0, 0, 0x99 / 255.0f);
                break;
            default:
                thumb_clr = get_system_accent_color();
                track_clr = D2D1::ColorF(0, 0, 0, 0x66 / 255.0f);
                break;
            }
            session.fill_rectangle(track_rt, track_clr);
            session.fill_rectangle(thumb_rt, thumb_clr);
        }
        void on_pointer_entered(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) override {
            update_visual_state(SliderVisualState::PointerOver);
        }
        void on_pointer_exited(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) override {
            update_visual_state(SliderVisualState::None);
        }
        void on_pointer_moved(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) override {
            auto rt = D2D1::RectF(
                m_actual_offset.x,
                m_actual_offset.y,
                m_actual_offset.x + m_actual_size.width,
                m_actual_offset.y + m_actual_size.height
            );
            auto ptr_pos = ptr_pt.Position();
            auto pt = D2D1::Point2F(ptr_pos.X, ptr_pos.Y);
            if (m_ptr_off) {
                update_visual_state(SliderVisualState::Pressed);
                float track_size = std::max(m_actual_size.width - THUMB_WIDTH, 1e-3f);
                float new_thumb_pos = pt.x - (m_actual_offset.x + THUMB_WIDTH / 2) + *m_ptr_off;
                new_thumb_pos = std::clamp(new_thumb_pos, 0.0f, track_size);
                auto new_proportion = new_thumb_pos / track_size;
                if (m_cur_min >= m_cur_max) {
                    new_proportion = 0;
                }
                update_current_proportion(new_proportion);
            }
            else {
                if (::hit_test(pt, rt)) {
                    update_visual_state(SliderVisualState::PointerOver);
                }
                else {
                    update_visual_state(SliderVisualState::None);
                }
            }
        }
        void on_pointer_pressed(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) override {
            if (ptr_pt.Properties().IsLeftButtonPressed()) {
                update_visual_state(SliderVisualState::Pressed);
                float track_size = std::max(m_actual_size.width - THUMB_WIDTH, 1e-3f);
                auto thumb_rt = D2D1::RectF(
                    static_cast<float>(m_actual_offset.x + track_size * m_cur_proportion),
                    m_actual_offset.y,
                    static_cast<float>(m_actual_offset.x + track_size * m_cur_proportion + THUMB_WIDTH),
                    m_actual_offset.y + m_actual_size.height
                );
                auto ptr_pos = ptr_pt.Position();
                auto pt = D2D1::Point2F(ptr_pos.X, ptr_pos.Y);
                if (::hit_test(pt, thumb_rt)) {
                    m_ptr_off = (thumb_rt.left + thumb_rt.right) / 2 - pt.x;
                }
                else {
                    m_ptr_off = 0.0f;
                    // Trigger move event immediately
                    on_pointer_moved(ptr_pt);
                }
            }
        }
        void on_pointer_released(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) override {
            auto rt = D2D1::RectF(
                m_actual_offset.x,
                m_actual_offset.y,
                m_actual_offset.x + m_actual_size.width,
                m_actual_offset.y + m_actual_size.height
            );
            auto ptr_pos = ptr_pt.Position();
            auto pt = D2D1::Point2F(ptr_pos.X, ptr_pos.Y);
            if (::hit_test(pt, rt)) {
                update_visual_state(SliderVisualState::PointerOver);
            }
            else {
                update_visual_state(SliderVisualState::None);
            }
            m_ptr_off = std::nullopt;
        }
        void on_pointer_canceled(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) override {
            update_visual_state(SliderVisualState::None);
            m_ptr_off = std::nullopt;
        }
    private:
        enum SliderVisualState {
            None, PointerOver, Pressed,
        } m_visual_state = None;
        std::optional<float> m_ptr_off;     // ThumbPos - PointerPos
        double m_cur_min = 0, m_cur_max = 1;
        double m_cur_proportion = 0;
        winrt::event<winrt::delegate<double>> m_ev_value_changed;

        void update_visual_state(SliderVisualState new_state) {
            if (m_visual_state != new_state) {
                m_visual_state = new_state;
                m_res_mgr->request_redraw();
            }
        }
        void update_current_proportion(double new_proportion) {
            if (m_cur_proportion != new_proportion) {
                m_cur_proportion = new_proportion;
                m_res_mgr->request_redraw();
                m_ev_value_changed(std::lerp(m_cur_min, m_cur_max, new_proportion));
            }
        }
    };
    struct D2D1CheckBox : D2D1Control {
        static constexpr float BOX_SIZE = 20, BOX_SPACING = 4, MIN_HEIGHT = 32;
        static constexpr float BOX_BORDER_THICKNESS = 2, BOX_STROKE_THICKNESS = 1.5f;
        D2D1CheckBox(std::shared_ptr<D2D1ResourceManager> res_mgr) : D2D1Control(std::move(res_mgr)) {
            D2D1CheckBox::on_device_lost();
        }
        void content(std::shared_ptr<D2D1Control> value) {
            m_child = std::move(value);
            // NOTE: Caller should update layout
        }
        void is_checked(bool value) { update_checked(value); }
        bool is_checked(void) { return m_checked; }
        std::shared_ptr<D2D1Control> content(void) { return m_child; }
        winrt::event_token checked(winrt::delegate<> delegate) {
            return m_ev_checked.add(delegate);
        }
        void checked(winrt::event_token token) {
            m_ev_checked.remove(token);
        }
        winrt::event_token unchecked(winrt::delegate<> delegate) {
            return m_ev_unchecked.add(delegate);
        }
        void unchecked(winrt::event_token token) {
            m_ev_unchecked.remove(token);
        }
    protected:
        void on_device_lost(void) override {
            if (m_child) {
                m_child->trigger_device_lost();
            }
        }
        void on_measure(const D2D1_SIZE_F& available_size) override {
            auto child_avail_size = D2D1::SizeF(
                available_size.width - BOX_SIZE - BOX_SPACING,
                available_size.height
            );
            D2D1_SIZE_F child_desired_size{};
            if (m_child) {
                auto child_margin = m_child->margin();
                child_avail_size.width -= child_margin.left + child_margin.right;
                child_avail_size.height -= child_margin.top + child_margin.bottom;
                if (child_avail_size.width < 0) { child_avail_size.width = 0; }
                if (child_avail_size.height < 0) { child_avail_size.height = 0; }
                m_child->measure(child_avail_size);
                child_desired_size = m_child->desired_size;
                child_desired_size.width += child_margin.left + child_margin.right;
                child_desired_size.height += child_margin.top + child_margin.bottom;
            }
            m_desired_size = {
                child_desired_size.width + BOX_SIZE + BOX_SPACING,
                std::max(child_desired_size.height, MIN_HEIGHT)
            };
        }
        void on_arrange(const D2D1_RECT_F& final_rect) override {
            m_actual_size = size_of(final_rect);
            m_actual_offset = { final_rect.left, final_rect.top };
            if (m_child) {
                auto margin = m_child->margin();
                auto child_rt = D2D1::RectF(
                    final_rect.left + BOX_SIZE + BOX_SPACING + margin.left,
                    0,
                    final_rect.right - margin.right,
                    0
                );
                std::tie(child_rt.top, child_rt.bottom) = details::calc_alignment(
                    final_rect.top + margin.top, final_rect.bottom - margin.bottom,
                    m_child->desired_size.height,
                    VerticalAlignment::Center
                );
                child_rt.right = std::max(child_rt.right, child_rt.left);
                m_child->arrange(child_rt);
            }
        }
        void on_render(D2D1DrawingSession& session) override {
            auto box_rt = D2D1::RectF(
                m_actual_offset.x,
                0,
                m_actual_offset.x + BOX_SIZE,
                0
            );
            std::tie(box_rt.top, box_rt.bottom) = details::calc_alignment(
                m_actual_offset.y, m_actual_offset.y + m_actual_size.height,
                BOX_SIZE,
                VerticalAlignment::Center
            );
            auto box_border_rt = D2D1::RectF(
                box_rt.left + BOX_BORDER_THICKNESS / 2,
                box_rt.top + BOX_BORDER_THICKNESS / 2,
                box_rt.right - BOX_BORDER_THICKNESS / 2,
                box_rt.bottom - BOX_BORDER_THICKNESS / 2
            );

            /* Colors list:
            *  SystemBaseHighColor(#ff000000)
            *  SystemBaseMediumHighColor(#cc000000)
            *  SystemBaseMediumColor(#99000000)
            */
            // Border | Background | Stroke
            // UncheckedNormal: SystemBaseMediumHighColor | Transparent | Transparent
            // UncheckedPointerOver: SystemBaseHighColor | Transparent | Transparent
            // UncheckedPressed: Transparent | SystemBaseMediumColor | Transparent
            // CheckedNormal: Transparent | SystemAccentColor | White
            // CheckedPointerOver: Transparent | SystemAccentColorLight1 | White
            // CheckedPressed: Transparent | SystemAccentColorDark1 | White
            const D2D1_COLOR_F clr_transparent = D2D1::ColorF(1, 1, 1, 0),
                clr_sys_base_medium_high = D2D1::ColorF(0, 0, 0, 0xcc / 255.0f),
                clr_sys_base_high = D2D1::ColorF(0, 0, 0, 1),
                clr_sys_base_medium = D2D1::ColorF(0, 0, 0, 0x99 / 255.0f),
                clr_white = D2D1::ColorF(D2D1::ColorF::White);
            D2D1_COLOR_F border_clr, bkg_clr, stroke_clr;
            switch (m_visual_state) {
            case CheckBoxVisualState::UncheckedNormal:
                border_clr = clr_sys_base_medium_high;
                bkg_clr = clr_transparent;
                stroke_clr = clr_transparent;
                break;
            case CheckBoxVisualState::UncheckedPointerOver:
                border_clr = clr_sys_base_high;
                bkg_clr = clr_transparent;
                stroke_clr = clr_transparent;
                break;
            case CheckBoxVisualState::UncheckedPressed:
                border_clr = clr_transparent;
                bkg_clr = clr_sys_base_medium;
                stroke_clr = clr_transparent;
                break;
            case CheckBoxVisualState::CheckedNormal:
                border_clr = clr_transparent;
                bkg_clr = get_system_accent_color();
                stroke_clr = clr_white;
                break;
            case CheckBoxVisualState::CheckedPointerOver:
                border_clr = clr_transparent;
                bkg_clr = get_system_accent_light1_color();
                stroke_clr = clr_white;
                break;
            case CheckBoxVisualState::CheckedPressed:
                border_clr = clr_transparent;
                bkg_clr = get_system_accent_dark1_color();
                stroke_clr = clr_white;
                break;
            }
            session.draw_rectangle(box_border_rt, border_clr, BOX_BORDER_THICKNESS);
            session.fill_rectangle(box_rt, bkg_clr);
            const D2D1_POINT_2F stroke_pts[] = {
                box_rt.left + BOX_SIZE * 0.125f, box_rt.top + BOX_SIZE * 0.5f,
                box_rt.left + BOX_SIZE * 0.375f, box_rt.top + BOX_SIZE * 0.75f,
                box_rt.left + BOX_SIZE * 0.875f, box_rt.top + BOX_SIZE * 0.25f,
            };
            session.draw_polyline(stroke_pts, stroke_clr, BOX_STROKE_THICKNESS);
            if (m_child) {
                m_child->draw(session);
            }
        }
        void on_pointer_entered(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) override {
            update_visual_state(m_checked ?
                CheckBoxVisualState::CheckedPointerOver : CheckBoxVisualState::UncheckedPointerOver);
        }
        void on_pointer_exited(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) override {
            update_visual_state(m_checked ?
                CheckBoxVisualState::CheckedNormal : CheckBoxVisualState::UncheckedNormal);
        }
        void on_pointer_moved(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) override {
            auto rt = D2D1::RectF(
                m_actual_offset.x,
                m_actual_offset.y,
                m_actual_offset.x + m_actual_size.width,
                m_actual_offset.y + m_actual_size.height
            );
            auto ptr_pos = ptr_pt.Position();
            auto pt = D2D1::Point2F(ptr_pos.X, ptr_pos.Y);
            if (::hit_test(pt, rt)) {
                if (m_pressed) {
                    update_visual_state(m_checked ?
                        CheckBoxVisualState::CheckedPressed : CheckBoxVisualState::UncheckedPressed);
                }
                else {
                    update_visual_state(m_checked ?
                        CheckBoxVisualState::CheckedPointerOver : CheckBoxVisualState::UncheckedPointerOver);
                }
            }
            else {
                update_visual_state(m_checked ?
                    CheckBoxVisualState::CheckedNormal : CheckBoxVisualState::UncheckedNormal);
            }
        }
        void on_pointer_pressed(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) override {
            if (ptr_pt.Properties().IsLeftButtonPressed()) {
                update_visual_state(m_checked ?
                    CheckBoxVisualState::CheckedPressed : CheckBoxVisualState::UncheckedPressed);
                m_pressed = true;
            }
        }
        void on_pointer_released(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) override {
            auto rt = D2D1::RectF(
                m_actual_offset.x,
                m_actual_offset.y,
                m_actual_offset.x + m_actual_size.width,
                m_actual_offset.y + m_actual_size.height
            );
            auto ptr_pos = ptr_pt.Position();
            auto pt = D2D1::Point2F(ptr_pos.X, ptr_pos.Y);
            if (::hit_test(pt, rt)) {
                if (m_pressed) {
                    if (m_checked) {
                        m_checked = false;
                        m_ev_unchecked();
                    }
                    else {
                        m_checked = true;
                        m_ev_checked();
                    }
                }
                update_visual_state(m_checked ?
                    CheckBoxVisualState::CheckedPointerOver : CheckBoxVisualState::UncheckedPointerOver);
            }
            else {
                update_visual_state(m_checked ?
                    CheckBoxVisualState::CheckedNormal : CheckBoxVisualState::UncheckedNormal);
            }
            m_pressed = false;
        }
        void on_pointer_canceled(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) override {
            update_visual_state(m_checked ?
                CheckBoxVisualState::CheckedNormal : CheckBoxVisualState::UncheckedNormal);
            m_pressed = false;
        }
    private:
        std::shared_ptr<D2D1Control> m_child;
        enum CheckBoxVisualState {
            UncheckedNormal, UncheckedPointerOver, UncheckedPressed,
            CheckedNormal, CheckedPointerOver, CheckedPressed,
            CheckedFlag,
        } m_visual_state = UncheckedNormal;
        bool m_pressed = false;
        bool m_checked = false;
        winrt::event<winrt::delegate<>> m_ev_unchecked, m_ev_checked;

        void update_visual_state(CheckBoxVisualState new_state) {
            if (m_visual_state != new_state) {
                m_visual_state = new_state;
                m_res_mgr->request_redraw();
            }
        }
        void update_checked(bool new_value) {
            if (m_checked != new_value) {
                m_checked = new_value;
                (new_value ? m_ev_checked : m_ev_unchecked)();
            }
        }
    };
}

struct MazeProblemApp final : D2D1SimpleWindow {
    static constexpr float SIDEBAR_WIDTH = 240;
    static constexpr double DEFAULT_STEP_DURATION = 1;  // Unit: second
    static constexpr wchar_t
        STR_PLAY[] = L"\u2bc8",
        STR_PAUSE[] = L"\u275a\u275a",
        STR_STOP[] = L"\u2b1b",
        STR_SKIP[] = L"\U0001f782\u2759";

    // Reuses existing infrastructure to ease layout handling, etc.
    struct MazeBoardControl : controls::D2D1Control {
        using coord_t = std::pair<size_t, size_t>;

        MazeBoardControl(std::shared_ptr<D2D1ResourceManager> res_mgr, MazeProblemApp* that) :
            D2D1Control(std::move(res_mgr)), m_that(that)
        {
            MazeBoardControl::on_device_lost();
        }
        bool is_playing(void) const { return m_is_playing; }
        // NOTE: Returns whether the progress is completely zero
        bool is_halted(void) const {
            if (!m_start_pos) { return true; }
            return !m_visited[helper_coord_to_addr(*m_start_pos)];
        }
        void run_speed(double value) {
            if (value <= 0) { throw winrt::hresult_invalid_argument(L"Speed must be positive"); }
            m_run_speed = value;
        }
        double run_speed(void) { return m_run_speed; }
        void show_intermediate_frames(bool value) {
            update_show_intermediate_frames(value);
        }
        bool show_intermediate_frames(void) { return m_show_intermediate_frames; }
        // NOTE: delegate(is_playing)
        winrt::event_token work_state_changed(winrt::delegate<bool> delegate) {
            return m_ev_work_state_changed.add(delegate);
        }
        void work_state_changed(winrt::event_token token) { m_ev_work_state_changed.remove(token); }
        void play(void) {
            if (m_board.empty()) { return; }
            if (!m_is_playing) {
                m_last_ts = std::chrono::steady_clock::now();
                if (update_progress()) {
                    update_is_playing(true);
                }
            }
        }
        void pause(void) {
            update_progress();
            update_is_playing(false);
        }
        void stop(void) {
            update_is_playing(false);
            m_cur_step_progress = 0;
            m_visited.clear();
            m_visited.resize(m_board.size());
            m_walk_stack.clear();
            m_res_mgr->request_redraw();
        }
        void skip(void) {
            // TODO: Use a proper way to perform skipping
            update_is_playing(true);
            do {
                m_last_ts = {};
            } while (update_progress());
            update_is_playing(false);
        }
        void load_board(size_t width, size_t height, std::vector<uint8_t> board) {
            if (board.size() != width * height) { throw winrt::hresult_invalid_argument(L"Invalid board"); }
            update_is_playing(false);
            m_start_pos = std::nullopt;
            m_end_pos = std::nullopt;
            m_width = width;
            m_height = height;
            m_board = std::move(board);
            m_cur_step_progress = 0;
            m_visited.clear();
            m_visited.resize(m_board.size());
            m_walk_stack.clear();
            m_res_mgr->request_redraw();
        }
    protected:
        void on_measure(const D2D1_SIZE_F& available_size) override {
            if (m_board.empty()) {
                m_desired_size = {};
                return;
            }
            m_desired_size.width = std::min(available_size.width, available_size.height / m_height * m_width);
            m_desired_size.height = std::min(available_size.height, available_size.width / m_width * m_height);
        }
        void on_arrange(const D2D1_RECT_F& final_rect) override {
            m_actual_size = size_of(final_rect);
            m_actual_offset = { final_rect.left, final_rect.top };
        }
        void on_render(D2D1DrawingSession& session) override {
            if (m_board.empty()) { return; }
            auto board_rt = D2D1::RectF(
                m_actual_offset.x,
                m_actual_offset.y,
                m_actual_offset.x + m_actual_size.width,
                m_actual_offset.y + m_actual_size.height
            );
            const float block_size = m_actual_size.width / m_width;
            const float board_scale = block_size / 32;

            auto next_op = update_progress();
            if (next_op && m_is_playing) { m_res_mgr->request_redraw(); }
            else { update_is_playing(false); }

            auto expo_ease_out_fn = [](double t) {
                return t == 1 ? 1 : 1 - std::pow(2, -10 * t);
            };
            const float animation_progress = static_cast<float>(expo_ease_out_fn(m_cur_step_progress));

            auto calc_next_coord_fn = [](const std::pair<coord_t, size_t>& coord_facing) {
                auto [coord, facing] = coord_facing;
                switch (facing % 4) {
                case 0:     coord.second--;     break;
                case 1:     coord.first++;      break;
                case 2:     coord.second++;     break;
                case 3:     coord.first--;      break;
                }
                return coord;
            };

            // Draw blocks
            for (size_t y = 0; y < m_height; y++) {
                for (size_t x = 0; x < m_width; x++) {
                    // Draw background
                    D2D1_COLOR_F block_clr = D2D1::ColorF(D2D1::ColorF::White);
                    auto rt = D2D1::RectF(
                        m_actual_offset.x + x * block_size,
                        m_actual_offset.y + y * block_size,
                        m_actual_offset.x + (x + 1) * block_size,
                        m_actual_offset.y + (y + 1) * block_size
                    );
                    if (m_board[y * m_width + x] != 0) {
                        block_clr = D2D1::ColorF(D2D1::ColorF::Black);
                    }
                    else if (m_start_pos && *m_start_pos == coord_t{ x, y }) {
                        block_clr = D2D1::ColorF(D2D1::ColorF::Chartreuse);
                    }
                    else if (m_end_pos && *m_end_pos == coord_t{ x, y }) {
                        block_clr = D2D1::ColorF(D2D1::ColorF::DodgerBlue);
                    }
                    else if (m_show_intermediate_frames && next_op &&
                        next_op->type == NextOpType::SpinAndAdvance &&
                        coord_t{ x, y } == calc_next_coord_fn(m_walk_stack.back()))
                    {
                        block_clr = lerp_color(
                            D2D1::ColorF(D2D1::ColorF::White),
                            D2D1::ColorF(D2D1::ColorF::Yellow),
                            animation_progress
                        );
                    }
                    else if (m_show_intermediate_frames && next_op &&
                        next_op->type == NextOpType::Return &&
                        coord_t{ x, y } == m_walk_stack.back().first)
                    {
                        block_clr = lerp_color(
                            D2D1::ColorF(D2D1::ColorF::Yellow),
                            D2D1::ColorF(D2D1::ColorF::Red),
                            animation_progress
                        );
                    }
                    else if (m_visited[helper_coord_to_addr({ x, y })]) {
                        auto it = std::find_if(m_walk_stack.begin(), m_walk_stack.end(), [&](const auto& v) {
                            return v.first == coord_t{ x, y };
                        });
                        bool has_value = it != m_walk_stack.end();
                        block_clr = D2D1::ColorF(has_value ? D2D1::ColorF::Yellow : D2D1::ColorF::Red);
                    }
                    session.fill_rectangle(rt, block_clr);
                }
            }
            // Draw block indicators (arcs)
            for (const auto& [coord, facing] : m_walk_stack) {
                auto pt_center = D2D1::Point2F(
                    m_actual_offset.x + (coord.first + 0.5f) * block_size,
                    m_actual_offset.y + (coord.second + 0.5f) * block_size
                );
                if (m_show_intermediate_frames && next_op &&
                    (next_op->type == NextOpType::Spin || next_op->type == NextOpType::SpinAndAdvance) &&
                    m_walk_stack.back().first == coord)
                {
                    session.draw_arc(
                        pt_center,
                        { 0.35f * block_size, 0.35f * block_size },
                        (facing + animation_progress) * 90.0f,
                        D2D1::ColorF(D2D1::ColorF::SlateBlue), board_scale * 3
                    );
                }
                else if (m_show_intermediate_frames && next_op &&
                    next_op->type == NextOpType::Return &&
                    m_walk_stack.back().first == coord)
                {
                    session.draw_arc(
                        pt_center,
                        { 0.35f * block_size, 0.35f * block_size },
                        facing * 90.0f,
                        D2D1::ColorF(D2D1::ColorF::SlateBlue, 1 - animation_progress),
                        board_scale * 3
                    );
                }
                else {
                    session.draw_arc(
                        pt_center,
                        { 0.35f * block_size, 0.35f * block_size },
                        facing * 90.0f,
                        D2D1::ColorF(D2D1::ColorF::SlateBlue), board_scale * 3
                    );
                }
            }
            // Draw grid lines
            for (size_t x = 0; x <= m_width; x++) {
                float cur_x = m_actual_offset.x + x * block_size;
                session.draw_line(
                    D2D1::Point2F(cur_x, board_rt.top),
                    D2D1::Point2F(cur_x, board_rt.bottom),
                    D2D1::ColorF(D2D1::ColorF::LightSlateGray),
                    board_scale
                );
            }
            for (size_t y = 0; y <= m_height; y++) {
                float cur_y = m_actual_offset.y + y * block_size;
                session.draw_line(
                    D2D1::Point2F(board_rt.left, cur_y),
                    D2D1::Point2F(board_rt.right, cur_y),
                    D2D1::ColorF(D2D1::ColorF::LightSlateGray),
                    board_scale
                );
            }
            // Draw arrow
            if (!m_walk_stack.empty()) {
                auto draw_arrow_fn = [&](float x, float y, float facing, const D2D1_COLOR_F& arrow_clr) {
                    // Virtual viewport: 200x200
                    auto arrow_transform =
                        D2D1::Matrix3x2F::Rotation(facing * 90.0f) *
                        D2D1::Matrix3x2F::Scale(block_size / 200, block_size / 200) *
                        D2D1::Matrix3x2F::Translation(
                            m_actual_offset.x + (x + 0.5f) * block_size,
                            m_actual_offset.y + (y + 0.5f) * block_size);
                    const auto arrow_thickness = 10.0f;
                    session.set_transform(arrow_transform);
                    const D2D1_POINT_2F pts1[] = { -35, -15, 0, -50, 35, -15 };
                    const D2D1_POINT_2F pts2[] = { 0, 50, 0, -50 };
                    session.draw_discrete_polylines(
                        std::initializer_list<std::span<const D2D1_POINT_2F>>{ pts1, pts2 },
                        arrow_clr, arrow_thickness
                    );
                    session.restore_transform();
                };
                auto [coord, facing] = m_walk_stack.back();
                coord_t next_coord;
                size_t next_facing;
                if (m_show_intermediate_frames && next_op) {
                    switch (next_op->type) {
                    case NextOpType::Spin:
                        draw_arrow_fn(
                            coord.first * 1.0f, coord.second * 1.0f,
                            facing * 1.0f + animation_progress,
                            D2D1::ColorF(D2D1::ColorF::Black)
                        );
                        break;
                    case NextOpType::SpinAndAdvance:
                        next_coord = calc_next_coord_fn({ coord, facing });
                        next_facing = 0;
                        draw_arrow_fn(
                            std::lerp(coord.first * 1.0f, next_coord.first * 1.0f, animation_progress),
                            std::lerp(coord.second * 1.0f, next_coord.second * 1.0f, animation_progress),
                            std::lerp(facing * 1.0f, next_facing * 1.0f, animation_progress),
                            D2D1::ColorF(D2D1::ColorF::Black)
                        );
                        break;
                    case NextOpType::Return:
                        if (m_walk_stack.size() >= 2) {
                            std::tie(next_coord, next_facing) = m_walk_stack[m_walk_stack.size() - 2];
                            draw_arrow_fn(
                                std::lerp(coord.first * 1.0f, next_coord.first * 1.0f, animation_progress),
                                std::lerp(coord.second * 1.0f, next_coord.second * 1.0f, animation_progress),
                                std::lerp(facing * 1.0f, next_facing * 1.0f, animation_progress),
                                D2D1::ColorF(D2D1::ColorF::Black)
                            );
                        }
                        else {
                            draw_arrow_fn(
                                coord.first * 1.0f, coord.second * 1.0f,
                                facing * 1.0f,
                                D2D1::ColorF(D2D1::ColorF::Black, 1 - animation_progress)
                            );
                        }
                        break;
                    }
                }
                else {
                    draw_arrow_fn(
                        coord.first * 1.0f, coord.second * 1.0f,
                        facing * 1.0f,
                        D2D1::ColorF(D2D1::ColorF::Black)
                    );
                }
            }
            // Draw successful path (if exists)
            if (!next_op && !m_walk_stack.empty()) {
                std::vector<D2D1_POINT_2F> pts;
                pts.reserve(m_walk_stack.size());
                for (const auto& i : m_walk_stack) {
                    auto pt_x = m_actual_offset.x + (i.first.first + 0.5f) * block_size;
                    auto pt_y = m_actual_offset.y + (i.first.second + 0.5f) * block_size;
                    pts.emplace_back(pt_x, pt_y);
                }
                session.draw_polyline(pts, D2D1::ColorF(D2D1::ColorF::Green), board_scale * 3);
            }
        }
        void on_pointer_pressed(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) override {
            if (ptr_pt.Properties().IsLeftButtonPressed()) {
                m_pressed = true;
            }
        }
        void on_pointer_released(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) override {
            auto ptr_pos = ptr_pt.Position();
            auto pt = D2D1::Point2F(ptr_pos.X, ptr_pos.Y);
            float fblock_x = (pt.x - m_actual_offset.x) / (m_actual_size.width / m_width);
            float fblock_y = (pt.y - m_actual_offset.y) / (m_actual_size.height / m_height);
            if (fblock_x < 0 || fblock_x >= m_width) { return; }
            if (fblock_y < 0 || fblock_y >= m_height) { return; }
            auto iblock_x = static_cast<size_t>(std::floor(fblock_x));
            auto iblock_y = static_cast<size_t>(std::floor(fblock_y));
            if (m_pressed && is_halted()) {
                // Show context menu
                auto root_hwnd = m_that->get_underlying_handle();
                auto dpi_scale = GetDpiForWindow(root_hwnd) / 96.0f;
                POINT cursor_pt{ std::lround(pt.x * dpi_scale), std::lround(pt.y * dpi_scale) };
                ClientToScreen(root_hwnd, &cursor_pt);
                HMENU menu_handle = CreatePopupMenu();
                winrt::check_pointer(menu_handle);
                winrt::check_bool(AppendMenuW(menu_handle, 0, 1, L"设为起点"));
                winrt::check_bool(AppendMenuW(menu_handle, 0, 2, L"设为终点"));
                winrt::check_bool(AppendMenuW(menu_handle, MF_SEPARATOR, 0, nullptr));
                winrt::check_bool(AppendMenuW(menu_handle, 0, 3, L"设为障碍"));
                winrt::check_bool(AppendMenuW(menu_handle, 0, 4, L"设为通路"));
                auto menu_ret = TrackPopupMenu(
                    menu_handle,
                    TPM_NONOTIFY | TPM_RETURNCMD,
                    cursor_pt.x, cursor_pt.y,
                    0,
                    root_hwnd,
                    nullptr
                );
                DestroyMenu(menu_handle);
                coord_t coord{ iblock_x, iblock_y };
                if (menu_ret == 1) {
                    // Set start point
                    if (m_board[helper_coord_to_addr(coord)] == 0) {
                        m_start_pos = coord;
                        if (m_end_pos == m_start_pos) { m_end_pos = std::nullopt; }
                    }
                }
                else if (menu_ret == 2) {
                    // Set end point
                    if (m_board[helper_coord_to_addr(coord)] == 0) {
                        m_end_pos = coord;
                        if (m_start_pos == m_end_pos) { m_start_pos = std::nullopt; }
                    }
                }
                else if (menu_ret == 3) {
                    // Set as wall
                    m_board[helper_coord_to_addr(coord)] = 1;
                    if (m_start_pos == coord) { m_start_pos = std::nullopt; }
                    if (m_end_pos == coord) { m_end_pos = std::nullopt; }
                }
                else if (menu_ret == 4) {
                    // Set as path
                    m_board[helper_coord_to_addr(coord)] = 0;
                }
                m_res_mgr->request_redraw();
            }
            m_pressed = false;
        }
        void on_pointer_canceled(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) override {
            m_pressed = false;
        }
    private:
        MazeProblemApp* m_that;
        bool m_is_playing = false;
        double m_run_speed = 1;
        winrt::event<winrt::delegate<bool>> m_ev_work_state_changed;
        std::chrono::steady_clock::time_point m_last_ts;
        double m_cur_step_progress = 0;
        bool m_show_intermediate_frames{};
        bool m_pressed{};

        size_t m_width{}, m_height{};
        std::vector<uint8_t> m_board;
        // elem((x, y), facing)
        std::vector<std::pair<coord_t, size_t>> m_walk_stack;
        std::vector<bool> m_visited;
        std::optional<coord_t> m_start_pos, m_end_pos;

        enum NextOpType {
            Initialize, SpinAndAdvance, Spin, Return,
        };
        struct NextOp {
            NextOpType type;
        };

        size_t helper_coord_to_addr(const coord_t& coord) const {
            return coord.second * m_width + coord.first;
        }
        void update_is_playing(bool new_value) {
            if (m_is_playing != new_value) {
                m_is_playing = new_value;
                if (new_value) { m_res_mgr->request_redraw(); }
                m_ev_work_state_changed(new_value);
            }
        }
        // NOTE: Returns whether further advancements can be made
        std::optional<NextOp> update_progress(void) {
            if (!m_start_pos || !m_end_pos) {
                // Impossible to run without start & end
                return std::nullopt;
            }
            if (m_walk_stack.empty()) {
                // Not started or already failed
                if (m_visited[helper_coord_to_addr(*m_start_pos)]) {
                    // Already failed
                    return std::nullopt;
                }
                // Not started; init variables
                if (!m_is_playing) { return NextOp{ .type{ NextOpType::Initialize } }; }
                m_walk_stack.push_back({ *m_start_pos, 0 });
                m_visited[helper_coord_to_addr(*m_start_pos)] = true;
            }
            if (m_walk_stack.back().first == *m_end_pos) {
                // Already succeeded
                return std::nullopt;
            }

            // Process skipped operations
            auto cur_ts = std::chrono::steady_clock::now();
            auto passed_dur = m_is_playing ? std::chrono::duration<double>(cur_ts - m_last_ts).count() : 0;
            m_last_ts = cur_ts;
            m_cur_step_progress += passed_dur / DEFAULT_STEP_DURATION * m_run_speed;
            for (; m_cur_step_progress >= 1; m_cur_step_progress--) {
                if (m_walk_stack.empty()) {
                    // Already done
                    return std::nullopt;
                }
                auto [coord, facing] = m_walk_stack.back();
                if (coord == *m_end_pos) {
                    // Succeeded
                    return std::nullopt;
                }
                m_walk_stack.pop_back();
                if (facing == 4) {
                    // All directions have been searched
                    continue;
                }
                m_walk_stack.push_back({ coord, facing + 1 });
                // NOTE: Unsigned arithmetic underflow is modulo
                coord_t next_coord{ coord };
                switch (facing) {
                // Up
                case 0:     next_coord.second--;    break;
                // Right
                case 1:     next_coord.first++;     break;
                // Down
                case 2:     next_coord.second++;    break;
                // Left
                case 3:     next_coord.first--;     break;
                }
                if (next_coord.first < 0 || next_coord.first >= m_width) { continue; }
                if (next_coord.second < 0 || next_coord.second >= m_height) { continue; }
                if (m_board[helper_coord_to_addr(next_coord)] != 0) { continue; }
                if (m_visited[helper_coord_to_addr(next_coord)]) { continue; }
                m_walk_stack.push_back({ next_coord, 0 });
                m_visited[helper_coord_to_addr(next_coord)] = true;
            }

            // Dry run and return the next operations
            NextOp next_op;
            if (m_walk_stack.empty()) {
                // Already done
                return std::nullopt;
            }
            else do {
                auto [coord, facing] = m_walk_stack.back();
                if (coord == *m_end_pos) {
                    // Succeeded
                    return std::nullopt;
                }
                next_op = { NextOpType::Return };
                if (facing == 4) {
                    break;
                }
                next_op = { NextOpType::Spin };
                coord_t next_coord{ coord };
                switch (facing) {
                // Up
                case 0:     next_coord.second--;    break;
                // Right
                case 1:     next_coord.first++;     break;
                // Down
                case 2:     next_coord.second++;    break;
                // Left
                case 3:     next_coord.first--;     break;
                }
                if (next_coord.first < 0 || next_coord.first >= m_width) { break; }
                if (next_coord.second < 0 || next_coord.second >= m_height) { break; }
                if (m_board[helper_coord_to_addr(next_coord)] != 0) { break; }
                if (m_visited[helper_coord_to_addr(next_coord)]) { break; }
                next_op = { NextOpType::SpinAndAdvance };
            } while (false);
            return next_op;
        }
        void update_show_intermediate_frames(bool new_value) {
            if (m_show_intermediate_frames != new_value) {
                m_show_intermediate_frames = new_value;
                m_res_mgr->request_redraw();
            }
        }
    };

    MazeProblemApp(HINSTANCE hInst) : D2D1SimpleWindow(hInst, L"迷宫问题") {
        auto res_mgr = get_resource_manager();

        auto grid_root = std::make_shared<controls::D2D1Grid>(res_mgr);
        grid_root->column_definitions().append(
            { .width = { .value = 1, .type = controls::GridUnitType::Star } });
        grid_root->column_definitions().append(
            { .width = { .value = 240, .type = controls::GridUnitType::Pixel } });
        auto board_ctrl = std::make_shared<MazeBoardControl>(res_mgr, this);
        board_ctrl->margin({ 4, 4, 4, 4 });
        board_ctrl->horizontal_alignment(controls::HorizontalAlignment::Center);
        board_ctrl->vertical_alignment(controls::VerticalAlignment::Center);
        grid_root->children().append(board_ctrl);
        auto grid_sidebar = std::make_shared<controls::D2D1Grid>(res_mgr);
        grid_sidebar->row_definitions().append(
            { .height = { .value = 1, .type = controls::GridUnitType::Star } });
        grid_sidebar->row_definitions().append(
            { .height = { .value = 1, .type = controls::GridUnitType::Star } });
        grid_sidebar->row_definitions().append(
            { .height = { .value = 1, .type = controls::GridUnitType::Star } });
        auto sp_ops_pane = std::make_shared<controls::D2D1StackPanel>(res_mgr);
        sp_ops_pane->horizontal_alignment(controls::HorizontalAlignment::Center);
        sp_ops_pane->vertical_alignment(controls::VerticalAlignment::Center);
        sp_ops_pane->spacing(4);
        sp_ops_pane->margin({ 20, 0, 20, 0 });
        sp_ops_pane->orientation(controls::Orientation::Vertical);
        auto btn_open_file = std::make_shared<controls::D2D1Button>(res_mgr);
        auto tb_btn_open_file = std::make_shared<controls::D2D1TextBlock>(res_mgr);
        tb_btn_open_file->text(L"打开迷宫文件");
        btn_open_file->content(tb_btn_open_file);
        btn_open_file->horizontal_alignment(controls::HorizontalAlignment::Center);
        btn_open_file->vertical_alignment(controls::VerticalAlignment::Center);
        sp_ops_pane->children().append(btn_open_file);
        auto btn_random_maze = std::make_shared<controls::D2D1Button>(res_mgr);
        auto tb_btn_random_maze = std::make_shared<controls::D2D1TextBlock>(res_mgr);
        tb_btn_random_maze->text(L"生成随机迷宫");
        btn_random_maze->content(tb_btn_random_maze);
        btn_random_maze->horizontal_alignment(controls::HorizontalAlignment::Center);
        btn_random_maze->vertical_alignment(controls::VerticalAlignment::Center);
        sp_ops_pane->children().append(btn_random_maze);
        grid_sidebar->children().append(sp_ops_pane);
        auto sp_playback_pane = std::make_shared<controls::D2D1StackPanel>(res_mgr);
        sp_playback_pane->spacing(4);
        sp_playback_pane->margin({ 20, 0, 20, 0 });
        sp_playback_pane->orientation(controls::Orientation::Vertical);
        auto sp_playback_pane_buttons = std::make_shared<controls::D2D1StackPanel>(res_mgr);
        sp_playback_pane_buttons->orientation(controls::Orientation::Horizontal);
        sp_playback_pane_buttons->spacing(4);
        auto btn_play_pause = std::make_shared<controls::D2D1Button>(res_mgr);
        auto tb_btn_play_pause = std::make_shared<controls::D2D1TextBlock>(res_mgr);
        tb_btn_play_pause->horizontal_alignment(controls::HorizontalAlignment::Center);
        tb_btn_play_pause->vertical_alignment(controls::VerticalAlignment::Center);
        tb_btn_play_pause->text(STR_PLAY);
        btn_play_pause->content(tb_btn_play_pause);
        btn_play_pause->horizontal_alignment(controls::HorizontalAlignment::Center);
        btn_play_pause->vertical_alignment(controls::VerticalAlignment::Stretch);
        sp_playback_pane_buttons->children().append(btn_play_pause);
        auto btn_stop = std::make_shared<controls::D2D1Button>(res_mgr);
        auto tb_btn_stop = std::make_shared<controls::D2D1TextBlock>(res_mgr);
        tb_btn_stop->text(STR_STOP);
        btn_stop->content(tb_btn_stop);
        btn_stop->horizontal_alignment(controls::HorizontalAlignment::Center);
        btn_stop->vertical_alignment(controls::VerticalAlignment::Stretch);
        sp_playback_pane_buttons->children().append(btn_stop);
        auto btn_skip = std::make_shared<controls::D2D1Button>(res_mgr);
        auto tb_btn_skip = std::make_shared<controls::D2D1TextBlock>(res_mgr);
        tb_btn_skip->text(STR_SKIP);
        btn_skip->content(tb_btn_skip);
        btn_skip->horizontal_alignment(controls::HorizontalAlignment::Center);
        btn_skip->vertical_alignment(controls::VerticalAlignment::Stretch);
        sp_playback_pane_buttons->children().append(btn_skip);
        sp_playback_pane_buttons->horizontal_alignment(controls::HorizontalAlignment::Center);
        sp_playback_pane_buttons->vertical_alignment(controls::VerticalAlignment::Center);
        sp_playback_pane->children().append(sp_playback_pane_buttons);
        auto slider_playback_speed = std::make_shared<controls::D2D1Slider>(res_mgr);
        slider_playback_speed->minimum(1);
        slider_playback_speed->maximum(25);
        sp_playback_pane->children().append(slider_playback_speed);
        auto tb_playback_speed = std::make_shared<controls::D2D1TextBlock>(res_mgr);
        // tb_playback_speed->text(L"运行速度: ?.??x");
        sp_playback_pane->children().append(tb_playback_speed);
        sp_playback_pane->horizontal_alignment(controls::HorizontalAlignment::Center);
        sp_playback_pane->vertical_alignment(controls::VerticalAlignment::Center);
        controls::D2D1Grid::set_row(sp_playback_pane, 1);
        grid_sidebar->children().append(sp_playback_pane);
        auto sp_cfg_pane = std::make_shared<controls::D2D1StackPanel>(res_mgr);
        sp_cfg_pane->vertical_alignment(controls::VerticalAlignment::Center);
        sp_cfg_pane->margin({ 20, 0, 20, 0 });
        sp_cfg_pane->orientation(controls::Orientation::Vertical);
        auto cb_show_intermediate_frames = std::make_shared<controls::D2D1CheckBox>(res_mgr);
        auto tb_cb_show_intermediate_frames = std::make_shared<controls::D2D1TextBlock>(res_mgr);
        tb_cb_show_intermediate_frames->text(L"显示中间帧");
        cb_show_intermediate_frames->content(tb_cb_show_intermediate_frames);
        sp_cfg_pane->children().append(cb_show_intermediate_frames);
        controls::D2D1Grid::set_row(sp_cfg_pane, 2);
        grid_sidebar->children().append(sp_cfg_pane);
        controls::D2D1Grid::set_column(grid_sidebar, 1);
        grid_root->children().append(grid_sidebar);

        m_sidebar_ctrl = grid_sidebar;
        m_tb_btn_play_pause = tb_btn_play_pause;
        m_root_ctrl = grid_root;
        m_board_ctrl = board_ctrl;

        btn_open_file->click([this]() -> winrt::fire_and_forget {
            auto that = this;
            // Use WinRT APIs to open file dialog (much simpler than traditional methods)
            auto picker = winrt::Windows::Storage::Pickers::FileOpenPicker();
            winrt::check_hresult(picker.as<IInitializeWithWindow>()->Initialize(that->m_hwnd));
            picker.FileTypeFilter().Append(L".txt");
            auto file = co_await picker.PickSingleFileAsync();
            if (!file) { co_return; }
            // NOTE: Return control back to main thread to avoid data race
            co_await that->m_hwnd;
            std::ifstream fs(file.Path().c_str());
            if (!fs) {
                MessageBoxW(that->m_hwnd, L"请检查文件是否存在，且是否有足够的访问权限", L"无法打开文件", MB_ICONERROR);
                co_return;
            }
            try {
                fs.exceptions(std::ios::failbit);
                int width, height;
                fs >> width >> height;
                if (width <= 0 || height <= 0) { throw std::runtime_error("Size should be positive"); }
                std::vector<uint8_t> board;
                board.reserve(width * height);
                for (size_t i = 0; i < width * height; i++) {
                    int temp;
                    fs >> temp;
                    if (temp < 0 || temp > 1) { throw std::runtime_error("Invalid maze data"); }
                    *std::back_inserter(board) = temp;
                }
                that->m_board_ctrl->load_board(width, height, std::move(board));
                that->update_layout();
            }
            catch (const winrt::hresult_error& e) {
                MessageBoxW(that->m_hwnd,
                    std::format(
                        L"读取文件时发生了无法处理的异常，过程被中止。\n"
                        "技术信息:\nHRESULT: 0x{:08x}\n{}",
                        static_cast<uint32_t>(e.code()), e.message()).c_str(),
                    L"严重错误",
                    MB_ICONERROR
                );
            }
            catch (const std::exception& e) {
                MessageBoxA(that->m_hwnd,
                    std::format(
                        "读取文件时发生了无法处理的异常，过程被中止。\n"
                        "技术信息:\nstd::exception\n{}", e.what()).c_str(),
                    "严重错误",
                    MB_ICONERROR
                );
            }
            catch (...) {
                MessageBoxW(that->m_hwnd,
                    L"读取文件时发生了无法处理的异常，过程被中止。",
                    L"严重错误",
                    MB_ICONERROR
                );
            }
        });
        btn_random_maze->click([this] {
            auto maze_w = static_cast<size_t>(gen_random_number(10, 20) * 2 + 1);
            auto maze_h = static_cast<size_t>(gen_random_number(10, 20) * 2 + 1);
            auto addr_fn = [&](size_t x, size_t y) { return y * maze_w + x; };
            auto coord_fn = [&](size_t addr) { return std::pair{ addr % maze_w, addr / maze_w }; };
            auto is_path_fn = [&](size_t x, size_t y) { return x % 2 == 0 && y % 2 == 0; };
            // Kruskal
            DisjointSet ds(maze_w * maze_h);
            std::vector<uint8_t> board(maze_w * maze_h, 1);
            std::vector<size_t> edges;
            for (size_t y = 0; y < maze_h; y++) {
                for (size_t x = 0; x < maze_w; x++) {
                    if (is_path_fn(x, y)) {
                        // Vertices (paths)
                        board[addr_fn(x, y)] = 0;
                    }
                    else if (x % 2 == 1 && y % 2 == 1) {
                        // Walls
                    }
                    else {
                        // Edges (initially walls)
                        edges.push_back(addr_fn(x, y));
                    }
                }
            }
            std::shuffle(edges.begin(), edges.end(), std::mt19937(std::random_device{}()));
            for (auto [x, y] : edges | std::views::transform(coord_fn)) {
                auto cur_addr = addr_fn(x, y);
                auto try_link_fn = [&](size_t addr_a, size_t addr_b) {
                    if (ds.find(addr_a) != ds.find(addr_b)) {
                        board[cur_addr] = 0;
                        ds.unite(addr_a, addr_b);
                    }
                };
                if (x % 2 == 1) {
                    // Horizontal
                    if (x + 1 >= maze_w) { continue; }
                    try_link_fn(addr_fn(x - 1, y), addr_fn(x + 1, y));
                }
                else {  // y % 2 == 1
                    // Vertical
                    if (y + 1 >= maze_h) { continue; }
                    try_link_fn(addr_fn(x, y - 1), addr_fn(x, y + 1));
                }
            }
            m_board_ctrl->load_board(maze_w, maze_h, std::move(board));
            update_layout();
        });
        btn_play_pause->click([this] {
            if (m_board_ctrl->is_playing()) {
                m_board_ctrl->pause();
            }
            else {
                m_board_ctrl->play();
            }
        });
        btn_stop->click([this] {
            m_board_ctrl->stop();
        });
        btn_skip->click([this] {
            m_board_ctrl->skip();
        });
        auto slider_playback_speed_value_changed_fn = [=](double value) {
            tb_playback_speed->text(winrt::hstring(std::format(L"运行速度: {:.2f}x", value)));
            m_board_ctrl->run_speed(value);
            update_layout();
        };
        slider_playback_speed_value_changed_fn(slider_playback_speed->value());
        slider_playback_speed->value_changed(std::move(slider_playback_speed_value_changed_fn));
        board_ctrl->work_state_changed([&](bool is_playing) {
            m_tb_btn_play_pause->text(is_playing ? STR_PAUSE : STR_PLAY);
            update_layout();
        });
        cb_show_intermediate_frames->checked([this] {
            m_board_ctrl->show_intermediate_frames(true);
        });
        cb_show_intermediate_frames->unchecked([this] {
            m_board_ctrl->show_intermediate_frames(false);
        });

        update_layout();
    }

    void on_render(void) override {
        auto viewport_size = m_d2d1_dev_ctx->GetSize();
        m_d2d1_dev_ctx->Clear(D2D1::ColorF(D2D1::ColorF::LightGray));
        m_d2d1_dev_ctx->SetTransform(D2D1::Matrix3x2F::Identity());
        auto draw_sess = get_drawing_session();
        auto res_mgr = get_resource_manager();

        auto sidebar_ctrl_rt = D2D1::RectF(
            m_sidebar_ctrl->actual_offset.x,
            m_sidebar_ctrl->actual_offset.y,
            m_sidebar_ctrl->actual_offset.x + m_sidebar_ctrl->actual_size.width,
            m_sidebar_ctrl->actual_offset.y + m_sidebar_ctrl->actual_size.height
        );
        draw_sess.fill_rectangle(sidebar_ctrl_rt, D2D1::ColorF(D2D1::ColorF::White));
        m_root_ctrl->draw(draw_sess);
    }

    void on_resize(uint32_t width, uint32_t height) override {
        update_layout();
    }
    void on_pointer_update(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) override {
        if (m_capturing_ctrl) {
            m_capturing_ctrl->trigger_pointer_moved(ptr_pt);
        }
        else {
            auto ptr_pos = ptr_pt.Position();
            auto ctrl = m_root_ctrl->hit_test(D2D1::Point2F(ptr_pos.X, ptr_pos.Y));
            update_last_pointed_control(ptr_pt, ctrl);
            if (ctrl) {
                ctrl->trigger_pointer_moved(ptr_pt);
            }
        }
    }
    void on_pointer_capture_changed(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) override {
        if (m_capturing_ctrl) {
            m_capturing_ctrl->trigger_pointer_canceled(ptr_pt);
            m_capturing_ctrl = nullptr;
        }
        update_last_pointed_control(ptr_pt, nullptr);
    }
    void on_pointer_down(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) override {
        auto ptr_pos = ptr_pt.Position();
        auto ctrl = m_root_ctrl->hit_test(D2D1::Point2F(ptr_pos.X, ptr_pos.Y));
        update_last_pointed_control(ptr_pt, ctrl);
        if (ctrl) {
            m_capturing_ctrl = ctrl;
            m_capturing_ctrl->trigger_pointer_pressed(ptr_pt);
        }
    }
    void on_pointer_up(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) override {
        if (m_capturing_ctrl) {
            m_capturing_ctrl->trigger_pointer_released(ptr_pt);
            m_capturing_ctrl = nullptr;
        }
    }
    void on_pointer_enter(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) override {
        // Do nothing
    }
    void on_pointer_leave(const winrt::Windows::UI::Input::PointerPoint& ptr_pt) override {
        update_last_pointed_control(ptr_pt, nullptr);
    }
private:
    std::shared_ptr<controls::D2D1Control> m_capturing_ctrl, m_last_pointed_ctrl;
    std::shared_ptr<controls::D2D1Control> m_root_ctrl, m_sidebar_ctrl;
    std::shared_ptr<controls::D2D1TextBlock> m_tb_btn_play_pause;
    std::shared_ptr<MazeBoardControl> m_board_ctrl;

    void update_last_pointed_control(
        const winrt::Windows::UI::Input::PointerPoint& ptr_pt,
        const std::shared_ptr<controls::D2D1Control> ctrl
    ) {
        if (ctrl != m_last_pointed_ctrl) {
            if (m_last_pointed_ctrl) {
                m_last_pointed_ctrl->trigger_pointer_exited(ptr_pt);
            }
            m_last_pointed_ctrl = ctrl;
            if (ctrl) {
                ctrl->trigger_pointer_entered(ptr_pt);
            }
        }
    }
    void update_layout(void) {
        if (!m_root_ctrl) { return; }
        auto viewport_size = m_d2d1_dev_ctx->GetSize();
        m_root_ctrl->measure(viewport_size);
        m_root_ctrl->arrange(D2D1::RectF(0, 0, viewport_size.width, viewport_size.height));
#if true
        request_redraw();
#endif
    }
};

HWND g_hwnd;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) try {
    HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0);
    winrt::init_apartment();
    MazeProblemApp app{ hInstance };
    g_hwnd = app.get_underlying_handle();
    return app.main_loop();
}
catch (const winrt::hresult_error& e) {
    MessageBoxW(g_hwnd,
        std::format(L"发生了无法处理的异常，程序将会退出。\n技术信息:\nHRESULT: 0x{:08x}\n{}",
            static_cast<uint32_t>(e.code()), e.message()).c_str(),
        L"严重错误",
        MB_ICONERROR
    );
    return EXIT_FAILURE;
}
catch (const std::exception& e) {
    MessageBoxA(g_hwnd,
        std::format("发生了无法处理的异常，程序将会退出。\n技术信息:\nstd::exception\n{}", e.what()).c_str(),
        "严重错误",
        MB_ICONERROR
    );
    return EXIT_FAILURE;
}
catch (...) {
    MessageBoxW(g_hwnd,
        L"发生了无法处理的异常，程序将会退出。",
        L"严重错误",
        MB_ICONERROR
    );
    return EXIT_FAILURE;
}
