#define NOMINMAX

#include <string_view>
#include <algorithm>
#include <optional>
#include <iostream>
#include <variant>
#include <fstream>
#include <numbers>
#include <random>
#include <format>
#include <vector>
#include <ranges>
#include <queue>
#include <mutex>
#include <stack>

#include "res.h"

// Backport std::format support for winrt::hstring (https://github.com/microsoft/cppwinrt/pull/1025)
#ifdef __cpp_lib_format
constexpr bool winrt_has_builtin_formatter = CPPWINRT_VERSION >= std::string_view("2.0.210922.5");
struct winrt_hstring_fmt_stub {
    operator std::wstring_view() { return L""; }
};
template<>
struct std::formatter<std::conditional_t<winrt_has_builtin_formatter, winrt_hstring_fmt_stub, winrt::hstring>, wchar_t> :
    std::formatter<std::wstring_view, wchar_t> {};
#endif

int gen_random_number(int start, int end) {
    static std::mt19937 gen{ std::random_device{}() };
    return std::uniform_int_distribution<int>{ start, end }(gen);
}

template<typename>
inline constexpr bool always_false_v = false;

#define fmt_print(...) (::std::cout << ::std::format(__VA_ARGS__))
#define fmt_println(...) (::std::cout << ::std::format(__VA_ARGS__) << ::std::endl)
#define fmt_write(fs, ...) ((fs) << ::std::format(__VA_ARGS__))
#define fmt_writeln(fs, ...) ((fs) << ::std::format(__VA_ARGS__) << ::std::endl)

template<typename T>
constexpr auto get_guid(const T& /* v_interface */) {
    return winrt::guid_of<T>();
}

winrt::com_ptr<IStream> stream_from_str(std::string_view sv) {
    winrt::com_ptr<IStream> result;
    result.attach(SHCreateMemStream(
        reinterpret_cast<const BYTE*>(sv.data()), static_cast<UINT>(sv.size())
    ));
    return result;
}

template<typename T>
constexpr auto width_of(const T& v) -> decltype(v.left) {
    return v.right - v.left;
}
template<typename T>
constexpr auto height_of(const T& v) -> decltype(v.top) {
    return v.bottom - v.top;
}
D2D1_SIZE_F size_of(const D2D1_RECT_F& v) {
    return D2D1::SizeF(width_of(v), height_of(v));
}
D2D1_SIZE_U size_of(const D2D1_RECT_U& v) {
    return D2D1::SizeU(width_of(v), height_of(v));
}
template<typename T>
constexpr auto hsum_of(const T& v) -> decltype(v.left) {
    return v.left + v.right;
}
template<typename T>
constexpr auto vsum_of(const T& v) -> decltype(v.top) {
    return v.top + v.bottom;
}
template<typename T>
constexpr auto havg_of(const T& v) -> decltype(v.left) {
    return hsum_of(v) / 2;
}
template<typename T>
constexpr auto vavg_of(const T& v) -> decltype(v.top) {
    return vsum_of(v) / 2;
}

bool hit_test(const D2D1_POINT_2F& pt, const D2D1_RECT_F& v) {
    return (v.left <= pt.x && pt.x < v.right) && (v.top <= pt.y && pt.y < v.bottom);
}

// NOTE: Time unit: seconds
struct AnimationKeyFrame : std::enable_shared_from_this<AnimationKeyFrame> {
    virtual double get_time(void) = 0;
    virtual double get_value(void) = 0;
    // NOTE: Range of t: [0, 1]
    virtual double calculate(double s, double e, double t) = 0;
};
struct LinearAnimationKeyFrame : AnimationKeyFrame {
    LinearAnimationKeyFrame(double t, double v) : m_time(t), m_value(v) {}
    double get_time(void) override { return m_time; }
    double get_value(void) override { return m_value; }
    double calculate(double s, double e, double t) override {
        return s + (e - s) * t;
    }
private:
    double m_time, m_value;
};
struct DiscreteAnimationKeyFrame : AnimationKeyFrame {
    DiscreteAnimationKeyFrame(double t, double v) : m_time(t), m_value(v) {}
    double get_time(void) override { return m_time; }
    double get_value(void) override { return m_value; }
    double calculate(double s, double e, double t) override {
        return t < 1 ? s : e;
    }
private:
    double m_time, m_value;
};
struct CubicEaseInAnimationKeyFrame : AnimationKeyFrame {
    CubicEaseInAnimationKeyFrame(double t, double v) : m_time(t), m_value(v) {}
    double get_time(void) override { return m_time; }
    double get_value(void) override { return m_value; }
    double calculate(double s, double e, double t) override {
        return s + (e - s) * (t * t * t);
    }
private:
    double m_time, m_value;
};
struct CubicEaseOutAnimationKeyFrame : AnimationKeyFrame {
    CubicEaseOutAnimationKeyFrame(double t, double v) : m_time(t), m_value(v) {}
    double get_time(void) override { return m_time; }
    double get_value(void) override { return m_value; }
    double calculate(double s, double e, double t) override {
        return s + (e - s) * (1 - std::pow(1 - t, 3));
    }
private:
    double m_time, m_value;
};
struct ExponentialEaseOutAnimationKeyFrame : AnimationKeyFrame {
    ExponentialEaseOutAnimationKeyFrame(double t, double v) : m_time(t), m_value(v) {}
    double get_time(void) override { return m_time; }
    double get_value(void) override { return m_value; }
    double calculate(double s, double e, double t) override {
        return s + (e - s) * (t >= 1 ? 1 : 1 - std::pow(2, -10 * t));
    }
private:
    double m_time, m_value;
};

// Helper object to ease animation manipulation
struct AnimationVariable : std::enable_shared_from_this<AnimationVariable> {
    double get_value(void) {
        if (m_keyframes.empty()) {
            throw winrt::hresult_error(E_FAIL,
                L"AnimationVariable does not support interpolation without keyframes");
        }
        auto it = std::upper_bound(m_keyframes.begin(), m_keyframes.end(), m_cur_duration,
            [](double a, const std::shared_ptr<AnimationKeyFrame>& b) {
                return a < b->get_time();
            }
        );
        if (it == m_keyframes.begin()) {
            return m_keyframes.front()->get_value();
        }
        if (it == m_keyframes.end()) {
            return m_keyframes.back()->get_value();
        }
        // Begin & end range of time & value
        double tb, te, vb, ve;
        --it;
        tb = (*it)->get_time();
        vb = (*it)->get_value();
        ++it;
        te = (*it)->get_time();
        ve = (*it)->get_value();
        return (*it)->calculate(vb, ve, (m_cur_duration - tb) / (te - tb));
    }
    bool is_animating(void) {
        return m_cur_duration < m_keyframes.back()->get_time();
    }

private:
    friend struct AnimationManager;

    struct use_the_make_method_t {
        explicit use_the_make_method_t() {}
    };
    static std::shared_ptr<AnimationVariable> make(void) {
        return std::make_shared<AnimationVariable>(use_the_make_method_t{});
    }

    void start_with_time(const std::chrono::steady_clock::time_point& ts) {
        m_start_ts = m_cur_ts = ts;
        m_cur_duration = 0;
    }
    void update_with_time(const std::chrono::steady_clock::time_point& ts) {
        m_cur_ts = ts;
        m_cur_duration = std::chrono::duration<double>(m_cur_ts - m_start_ts).count();
    }

    std::vector<std::shared_ptr<AnimationKeyFrame>> m_keyframes;
    std::chrono::steady_clock::time_point m_start_ts, m_cur_ts;
    double m_cur_duration;

public:
    AnimationVariable(use_the_make_method_t) {}
};

// TODO: Introduce storyboard
struct AnimationManager {
    std::shared_ptr<AnimationVariable> create_variable(
        std::vector<std::shared_ptr<AnimationKeyFrame>> keyframes
    ) {
        auto var = AnimationVariable::make();
        std::sort(keyframes.begin(), keyframes.end(), [](const auto& a, const auto& b) {
            return a->get_time() < b->get_time();
        });
        var->m_keyframes = std::move(keyframes);
        m_var_refs.emplace_back(var);
        return var;
    }
    // WARN: Call this method before starting / checking animation state,
    //       or the animation variables may become unpredictable
    void update_state(void) {
        m_cur_ts = std::chrono::steady_clock::now();
        iterate_variables([&](const std::shared_ptr<AnimationVariable>& v) {
            v->update_with_time(m_cur_ts);
            return true;
        });
    }
    // Checks whether any managed animations are running
    bool is_animating(void) {
        bool result = false;
        iterate_variables([&](const std::shared_ptr<AnimationVariable>& v) {
            if (v->is_animating()) {
                result = true;
                return false;
            }
            return true;
        });
        return result;
    }
    void play(const std::shared_ptr<AnimationVariable>& var) {
        var->start_with_time(m_cur_ts);
    }

private:
    // NOTE: If functor returns false, the iteration will be stopped immediately
    template<typename Functor>
    void iterate_variables(Functor&& functor) {
        // Iterate and perform GC at the same time
        size_t i = 0;
        while (i < m_var_refs.size()) {
            if (auto var = m_var_refs[i].lock()) {
                if (!functor(var)) { break; }
                i++;
            }
            else {
                m_var_refs.erase(m_var_refs.begin() + i);
            }
        }
    }

    std::vector<std::weak_ptr<AnimationVariable>> m_var_refs;
    std::chrono::steady_clock::time_point m_cur_ts;
};

// Core game data & logic
struct MahjongGame {
    static constexpr unsigned MAHJONG_TYPES_COUNT = 10;
    static constexpr unsigned BOARD_WIDTH = 20, BOARD_HEIGHT = 10;
    MahjongGame() { rebuild_board(); }
    // Randomly builds a new board
    void rebuild_board() {
        unsigned* board_ptr = reinterpret_cast<unsigned*>(m_board);
        // Fisher-Yates shuffle
        for (unsigned i = 0; i < BOARD_HEIGHT * BOARD_WIDTH; i++) {
            board_ptr[i] = i / (BOARD_HEIGHT * BOARD_WIDTH / MAHJONG_TYPES_COUNT) + 1;
        }
        for (unsigned i = 0; i < BOARD_HEIGHT * BOARD_WIDTH - 1; i++) {
            std::iter_swap(board_ptr + i, board_ptr + gen_random_number(i, BOARD_HEIGHT * BOARD_WIDTH - 1));
        }
    }
    // NOTE: Range of return value: [0, MAHJONG_TYPES_COUNT]
    // NOTE: Return value of 0 represents empty
    unsigned read_board(unsigned x, unsigned y) {
        if (x >= BOARD_WIDTH || y >= BOARD_HEIGHT) {
            throw winrt::hresult_error(E_FAIL, L"棋盘位置无效");
        }
        return m_board[y][x];
    }
    void write_board(unsigned x, unsigned y, unsigned v) {
        if (x >= BOARD_WIDTH || y >= BOARD_HEIGHT) {
            throw winrt::hresult_error(E_FAIL, L"棋盘位置无效");
        }
        if (v < 0 || v > MAHJONG_TYPES_COUNT) {
            throw winrt::hresult_error(E_FAIL, L"新的棋盘值无效");
        }
        m_board[y][x] = v;
    }
    bool is_board_empty(void) {
        unsigned* board_ptr = reinterpret_cast<unsigned*>(m_board);
        for (size_t i = 0; i < BOARD_HEIGHT * BOARD_WIDTH; i++) {
            if (board_ptr[i] != 0) { return false; }
        }
        return true;
    }
    // NOTE: Returns all key points in path (including corners, start & end)
    //       if path exists, otherwise empty
    // NOTE: User may choose whether to treat out-of-bounds slots (such as (-1, 0))
    //       as searching candidates
    std::vector<std::pair<int, int>> test_connectivity(
        unsigned x1, unsigned y1, unsigned x2, unsigned y2,
        bool consider_out_of_bounds = false
    ) {
        // BFS
        using coord_t = std::pair<int, int>;
        if (x1 >= BOARD_WIDTH || y1 >= BOARD_HEIGHT || x2 >= BOARD_WIDTH || y2 >= BOARD_HEIGHT) {
            throw winrt::hresult_error(E_FAIL, L"棋盘位置无效");
        }
        // const unsigned max_lines = 3;
        const unsigned max_lines = 9999999;
        // NOTE: +2 to account for out-of-bounds accessing
        bool visited[BOARD_HEIGHT + 2][BOARD_WIDTH + 2]{};
        coord_t path[BOARD_HEIGHT + 2][BOARD_WIDTH + 2];
        std::queue<coord_t> search_queue;
        visited[y1 + 1][x1 + 1] = true;
        path[y1 + 1][x1 + 1] = { x1, y1 };
        search_queue.emplace(x1, y1);
        unsigned depth = 0;
        while (!visited[y2 + 1][x2 + 1] && !search_queue.empty() && depth < max_lines) {
            size_t iter_cnt = search_queue.size();
            for (size_t i = 0; i < iter_cnt; i++) {
                auto [curx, cury] = search_queue.front(); search_queue.pop();
                // NOTE: Returns whether iteration should stop
                //       (true => continue; false => stop)
                auto visit_fn = [&](int x, int y) {
                    unsigned mahjong_value;
                    // Get mahjong value at specified position
                    if (x < 0 || x >= BOARD_WIDTH) { mahjong_value = 0; }
                    else if (y < 0 || y >= BOARD_HEIGHT) { mahjong_value = 0; }
                    else { mahjong_value = m_board[y][x]; }
                    // Check mahjong
                    if (visited[y + 1][x + 1]) {
                        if (mahjong_value != 0) { return false; }
                    }
                    else {
                        visited[y + 1][x + 1] = true;
                        path[y + 1][x + 1] = { curx, cury };
                        // If there is mahjong, stop searching
                        if (mahjong_value != 0) { return false; }
                        // Else append current position to queue
                        search_queue.emplace(x, y);
                    }
                    return true;
                };
                // Left
                for (int x = curx; x > 0 - consider_out_of_bounds; x--) {
                    if (!visit_fn(x - 1, cury)) { break; }
                }
                // Right
                for (int x = curx + 1; x < static_cast<int>(BOARD_WIDTH + consider_out_of_bounds); x++) {
                    if (!visit_fn(x, cury)) { break; }
                }
                // Up
                for (int y = cury; y > 0 - consider_out_of_bounds; y--) {
                    if (!visit_fn(curx, y - 1)) { break; }
                }
                // Down
                for (int y = cury + 1; y < static_cast<int>(BOARD_HEIGHT + consider_out_of_bounds); y++) {
                    if (!visit_fn(curx, y)) { break; }
                }
            }
            depth++;
        }
        if (!visited[y2 + 1][x2 + 1]) { return {}; }
        std::vector<coord_t> result;
        // Build path
        result.emplace_back(x2, y2);
        while (true) {
            auto lastpos = result.back();
            auto curpos = path[lastpos.second + 1][lastpos.first + 1];
            if (lastpos == curpos) { break; }
            result.emplace_back(curpos.first, curpos.second);
        }
        std::reverse(result.begin(), result.end());
        return result;
    }

private:
    unsigned m_board[BOARD_HEIGHT][BOARD_WIDTH];
};

// Game window (frontend)
struct MahjongApp {
    static constexpr float BOARD_WIDTH = 960, BOARD_HEIGHT = 640;
    static constexpr float MAHJONG_WIDTH = 24 * 2, MAHJONG_HEIGHT = 32 * 2;
    static constexpr float BOARD_MARGIN = 16;
    // static constexpr float BOARD_MARGIN = MAHJONG_HEIGHT / 2;
    // static constexpr float MAHJONG_IMG_WIDTH = 16 * 2, MAHJONG_IMG_HEIGHT = 16 * 2;
    static constexpr float MAHJONG_IMG_MARGIN = 4 * 2;
    static constexpr float SIDEBAR_WIDTH = 240;
    static constexpr unsigned REDRAW_TIMER_ID = 1;
    static constexpr unsigned MENU_SYS_ID_RUN_AT_FULL_SPEED = 0x0001;

    MahjongApp(HINSTANCE hInst) {
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
        for (size_t i = 0; i < MahjongGame::MAHJONG_TYPES_COUNT; i++) {
            m_wic_img_tiles[i] = resources::imaging::load_wic_bitmap_from_file(
                m_wic_factory,
                std::format(L"res/tile{:02}.png", i + 1).c_str()
            );
        }

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
                    auto* that = reinterpret_cast<MahjongApp*>(pcs->lpCreateParams);
                    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(that));
                    return 1;
                }
                auto* that = reinterpret_cast<MahjongApp*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
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
            wc.lpszClassName = L"MahjongAppWindowClass";
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

        m_hwnd = create_window_fn(L"连连看");
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

        m_game_start_ts = std::chrono::system_clock::now();
    }
    ~MahjongApp() {
        DestroyWindow(m_hwnd);
    }

    void recreate_device_resources(void) {
        SetWindowTextW(m_hwnd, L"连连看 - 正在重新加载资源...");

        // Explicitly release resources
        m_dxgi_swapchain = nullptr;
        m_d2d1_dev_ctx = nullptr;
        m_board_bmp_render_target = nullptr;
        m_board_bmp_dev_ctx = nullptr;
        m_light_slate_gray_brush = nullptr;
        m_light_gray_brush = nullptr;
        m_corn_flower_blue_brush = nullptr;
        m_black_brush = nullptr;
        m_white_brush = nullptr;
        for (size_t i = 0; i < MahjongGame::MAHJONG_TYPES_COUNT; i++) {
            m_img_tiles[i] = nullptr;
        }
        m_img_atlas_tiles = nullptr;

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
            // m_d2d1_factory->GetDesktopDpi(&dpix, &dpiy);
            dpix = dpiy = GetDpiForWindow(m_hwnd) * 1.0f;
            m_d2d1_dev_ctx->SetDpi(dpix, dpiy);
        };
        init_d2d1_with_swapchain_fn();

        on_update_layout();

        auto rt_size = m_d2d1_dev_ctx->GetSize();
        winrt::check_hresult(m_d2d1_dev_ctx->CreateCompatibleRenderTarget(
            size_of(m_layout.board.outer_rt),
            m_board_bmp_render_target.put()
        ));
        m_board_bmp_render_target.as(m_board_bmp_dev_ctx);
        winrt::check_hresult(m_d2d1_dev_ctx->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::LightSlateGray),
            m_light_slate_gray_brush.put()
        ));
        winrt::check_hresult(m_d2d1_dev_ctx->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::LightGray),
            m_light_gray_brush.put()
        ));
        winrt::check_hresult(m_d2d1_dev_ctx->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::CornflowerBlue),
            m_corn_flower_blue_brush.put()
        ));
        winrt::check_hresult(m_d2d1_dev_ctx->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::Black),
            m_black_brush.put()
        ));
        winrt::check_hresult(m_d2d1_dev_ctx->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::White),
            m_white_brush.put()
        ));
        for (size_t i = 0; i < MahjongGame::MAHJONG_TYPES_COUNT; i++) {
            winrt::check_hresult(m_d2d1_dev_ctx->CreateBitmapFromWicBitmap(
                m_wic_img_tiles[i].get(), m_img_tiles[i].put()
            ));
        }

        recreate_device_size_sensitive_resources();

        SetWindowTextW(m_hwnd, L"连连看");
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
        // TODO: Maybe move m_board_bmp_render_target recreation logic inside
        // Recreate tiles atlas
        const auto dest_rect = m_layout.board.inner_rt;
        if (width_of(dest_rect) <= 0 || height_of(dest_rect) <= 0) { return; }
        const auto scale_factor = width_of(dest_rect) / BOARD_WIDTH;
        m_img_atlas_tiles = nullptr;
        winrt::com_ptr<ID2D1BitmapRenderTarget> bmp_target;
        winrt::com_ptr<ID2D1DeviceContext5> bmp_dev_ctx;
        unsigned col_cnt = static_cast<unsigned>(std::lround(std::ceil(
            std::sqrt(MahjongGame::MAHJONG_TYPES_COUNT))));
        unsigned row_cnt = (MahjongGame::MAHJONG_TYPES_COUNT + col_cnt - 1) / col_cnt;
        m_rows_img_atlas_tiles = row_cnt;
        m_cols_img_atlas_tiles = col_cnt;
        winrt::check_hresult(m_d2d1_dev_ctx->CreateCompatibleRenderTarget(
            D2D1::SizeF(
                scale_factor * MAHJONG_WIDTH * col_cnt,
                scale_factor * MAHJONG_HEIGHT * row_cnt),
            bmp_target.put()
        ));
        bmp_target.as(bmp_dev_ctx);
        bmp_dev_ctx->BeginDraw();
        auto base_transform = D2D1::Matrix3x2F::Scale(D2D1::SizeF(scale_factor, scale_factor));
        for (size_t i = 0; i < MahjongGame::MAHJONG_TYPES_COUNT; i++) {
            const auto& cur_tile_img = m_img_tiles[i];
            size_t x = i % col_cnt, y = i / col_cnt;
            auto mahjong_rect = D2D1::RectF(
                MAHJONG_WIDTH * x, MAHJONG_HEIGHT * y,
                MAHJONG_WIDTH * (x + 1), MAHJONG_HEIGHT * (y + 1)
            );
            auto tile_orig_size = cur_tile_img->GetSize();
            auto tile_avail_size = D2D1::SizeF(
                MAHJONG_WIDTH - MAHJONG_IMG_MARGIN,
                MAHJONG_HEIGHT - MAHJONG_IMG_MARGIN
            );
            auto tile_actual_size = D2D1::SizeF(
                std::min(tile_avail_size.width, tile_avail_size.height / tile_orig_size.height * tile_orig_size.width),
                std::min(tile_avail_size.height, tile_avail_size.width / tile_orig_size.width * tile_orig_size.height)
            );
            float tile_zoom_factor = tile_actual_size.width / tile_orig_size.width;
            auto tile_rect = D2D1::RectF(
                mahjong_rect.left + (MAHJONG_WIDTH - tile_actual_size.width) / 2,
                mahjong_rect.top + (MAHJONG_HEIGHT - tile_actual_size.height) / 2,
                mahjong_rect.left + (MAHJONG_WIDTH + tile_actual_size.width) / 2,
                mahjong_rect.top + (MAHJONG_HEIGHT + tile_actual_size.height) / 2
            );
            auto tile_local_transform =
                D2D1::Matrix3x2F::Scale(D2D1::SizeF(tile_zoom_factor, tile_zoom_factor)) *
                D2D1::Matrix3x2F::Translation(tile_rect.left, tile_rect.top);
            auto tile_transform = tile_local_transform * base_transform;
            bmp_dev_ctx->SetTransform(tile_transform);
            bmp_dev_ctx->DrawImage(
                cur_tile_img.get(),
                // D2D1_INTERPOLATION_MODE_MULTI_SAMPLE_LINEAR
                D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC
            );
        }
        bmp_dev_ctx->EndDraw();
        bmp_target->GetBitmap(m_img_atlas_tiles.put());
    }

    LRESULT window_proc(UINT msg, WPARAM wParam, LPARAM lParam) {
        bool handled = false;
        LRESULT result = 0;
        auto get_translated_mouse_pos = [&]() -> D2D1_POINT_2F {
            auto dpi_scale = GetDpiForWindow(m_hwnd) / 96.0f;
            POINT pos;
            GetCursorPos(&pos);
            ScreenToClient(m_hwnd, &pos);
            return { pos.x / dpi_scale, pos.y / dpi_scale };
        };
        if (msg == WM_DESTROY) {
            result = 1;
            handled = true;
        }
        else if (msg == WM_PAINT) {
            KillTimer(m_hwnd, REDRAW_TIMER_ID);
            ValidateRect(m_hwnd, nullptr);
            this->on_render();
            result = 0;
            handled = true;
        }
        else if (msg == WM_SIZE) {
            RECT rt;
            GetClientRect(m_hwnd, &rt);
            this->on_resize(rt.right, rt.bottom);
        }
        else if (msg == WM_DISPLAYCHANGE) {
            InvalidateRect(m_hwnd, nullptr, false);
            result = 0;
            handled = true;
        }
        else if (msg == WM_CAPTURECHANGED) {
            on_mouse_capture_changed();
            result = 0;
            handled = true;
        }
        else if (msg == WM_LBUTTONDOWN) {
            on_mouse_left_down(get_translated_mouse_pos());
            result = 0;
            handled = true;
        }
        else if (msg == WM_LBUTTONUP) {
            on_mouse_left_up(get_translated_mouse_pos());
            result = 0;
            handled = true;
        }
        else if (msg == WM_MOUSEMOVE) {
            on_mouse_move(get_translated_mouse_pos());
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

    HWND get_underlying_handle(void) {
        return m_hwnd;
    }

private:
    void on_render(void) {
        HRESULT hr;
        m_d2d1_dev_ctx->BeginDraw();

        /*  Pseudo UI layout:
         *  <Grid ColumnDefinitions="*,240">
         *  <Viewbox Grid.Column="0" Margin="16">
         *      <!-- Mahjong tile size is 1.26"H x 1"W x 0.63"D (3.2cm x 2.4cm x 1.6cm) -->
         *      {{ Game content of size 960x640 }}
         *  </Viewbox>
         *  <StackPanel Grid.Column="1" HorizontalAlignment="Center" VerticalAlignment="Center">
         *      <TextBlock Text="当前用时: {{ Spent time }}"/>
         *      <Button Content="重新开始"/>
         *  </StackPanel>
         *  </Grid>
        */

        auto draw_text_center_fn = [&](
            std::wstring_view str, float font_size,
            const D2D1_RECT_F& txt_rt, const winrt::com_ptr<ID2D1DeviceContext5>& devctx)
        {
            winrt::com_ptr<IDWriteTextLayout> txt_layout;
            txt_layout = make_text_layout(str, font_size, width_of(txt_rt), true);
            DWRITE_TEXT_METRICS txt_metrics;
            winrt::check_hresult(txt_layout->GetMetrics(&txt_metrics));
            devctx->DrawTextLayout(
                D2D1::Point2F(
                    txt_rt.left,
                    txt_rt.top + (height_of(txt_rt) - txt_metrics.height) / 2),
                txt_layout.get(),
                m_black_brush.get()
            );
        };

        m_animation_manager.update_state();
        if (m_animation_manager.is_animating()) {
            request_redraw();
        }

        m_d2d1_dev_ctx->SetTransform(D2D1::Matrix3x2F::Identity());
        m_d2d1_dev_ctx->Clear(D2D1::ColorF(D2D1::ColorF::LightGray));
        auto viewport_size = m_d2d1_dev_ctx->GetSize();
        float dpi_x, dpi_y;
        m_d2d1_dev_ctx->GetDpi(&dpi_x, &dpi_y);
        float dpi_x_scale = dpi_x / 96, dpi_y_scale = dpi_y / 96;
        // Draw mahjong board
        auto draw_mahjong_board_fn = [&] {
            const auto dest_rect = m_layout.board.inner_rt;
            if (width_of(dest_rect) <= 0 || height_of(dest_rect) <= 0) { return; }
            const auto scale_factor = width_of(dest_rect) / BOARD_WIDTH;
            auto base_transform =
                D2D1::Matrix3x2F::Scale(D2D1::SizeF(scale_factor, scale_factor)) *
                D2D1::Matrix3x2F::Translation(D2D1::SizeF(dest_rect.left, dest_rect.top));
            auto draw_single_base_fn = [&](float x, float y, unsigned type, const winrt::com_ptr<ID2D1DeviceContext5>& devctx) {
                std::ignore = type;
                auto mahjong_rect = D2D1::RectF(
                    MAHJONG_WIDTH * x, MAHJONG_HEIGHT * y,
                    MAHJONG_WIDTH * (x + 1), MAHJONG_HEIGHT * (y + 1)
                );
                auto mahjong_round_rect = D2D1::RoundedRect(mahjong_rect, 4, 4);
                devctx->FillRoundedRectangle(&mahjong_round_rect, m_white_brush.get());
                devctx->DrawRoundedRectangle(&mahjong_round_rect, m_black_brush.get(), 1.5f);
            };
            auto draw_single_img_fn = [&](float x, float y, unsigned type, const winrt::com_ptr<ID2D1DeviceContext5>& devctx, bool high_quality) {
                const auto& cur_tile_img = m_img_tiles[type];
                auto mahjong_rect = D2D1::RectF(
                    MAHJONG_WIDTH * x, MAHJONG_HEIGHT * y,
                    MAHJONG_WIDTH * (x + 1), MAHJONG_HEIGHT * (y + 1)
                );
                auto tile_orig_size = cur_tile_img->GetSize();
                auto tile_avail_size = D2D1::SizeF(
                    MAHJONG_WIDTH - MAHJONG_IMG_MARGIN,
                    MAHJONG_HEIGHT - MAHJONG_IMG_MARGIN
                );
                auto tile_actual_size = D2D1::SizeF(
                    std::min(tile_avail_size.width, tile_avail_size.height / tile_orig_size.height * tile_orig_size.width),
                    std::min(tile_avail_size.height, tile_avail_size.width / tile_orig_size.width * tile_orig_size.height)
                );
                float tile_zoom_factor = tile_actual_size.width / tile_orig_size.width;
                auto tile_rect = D2D1::RectF(
                    mahjong_rect.left + (MAHJONG_WIDTH - tile_actual_size.width) / 2,
                    mahjong_rect.top + (MAHJONG_HEIGHT - tile_actual_size.height) / 2,
                    mahjong_rect.left + (MAHJONG_WIDTH + tile_actual_size.width) / 2,
                    mahjong_rect.top + (MAHJONG_HEIGHT + tile_actual_size.height) / 2
                );
                auto tile_local_transform =
                    D2D1::Matrix3x2F::Scale(D2D1::SizeF(tile_zoom_factor, tile_zoom_factor)) *
                    D2D1::Matrix3x2F::Translation(tile_rect.left, tile_rect.top);
                auto tile_transform = tile_local_transform * base_transform;
                if (high_quality) {
                    devctx->SetTransform(tile_transform);
                    devctx->DrawImage(
                        cur_tile_img.get(),
                        // D2D1_INTERPOLATION_MODE_MULTI_SAMPLE_LINEAR
                        D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC
                    );
                    devctx->SetTransform(base_transform);
                }
                else {
                    // Faster drawing from atlas at the cost of quality
                    devctx->SetTransform(D2D1::Matrix3x2F::Identity());
                    size_t atlas_x = type % m_cols_img_atlas_tiles;
                    size_t atlas_y = type / m_cols_img_atlas_tiles;
                    auto mahjong_actual_size = D2D1::SizeF(
                        scale_factor * MAHJONG_WIDTH,
                        scale_factor * MAHJONG_HEIGHT
                    );
                    devctx->DrawImage(
                        m_img_atlas_tiles.get(),
                        D2D1::Point2F(
                            scale_factor * mahjong_rect.left + dest_rect.left,
                            scale_factor * mahjong_rect.top + dest_rect.top),
                        D2D1::RectF(
                            mahjong_actual_size.width * atlas_x,
                            mahjong_actual_size.height * atlas_y,
                            mahjong_actual_size.width * (atlas_x + 1),
                            mahjong_actual_size.height * (atlas_y + 1)),
                        D2D1_INTERPOLATION_MODE_LINEAR
                    );
                    devctx->SetTransform(base_transform);
                }
            };
            // NOTE: nodpi indicates this function should not be used
            //       in conjunction with Direct2D's automatic DPI scaling
            auto draw_single_img_batch_nodpi_fn = [&](float x, float y, unsigned type, const winrt::com_ptr<ID2D1SpriteBatch>& batch) {
                // NOTE: We handle DPI ourselves here to ensure pixel-perfect output
                size_t atlas_x = type % m_cols_img_atlas_tiles;
                size_t atlas_y = type / m_cols_img_atlas_tiles;
                auto mahjong_rect = D2D1::RectF(
                    MAHJONG_WIDTH * x, MAHJONG_HEIGHT * y,
                    MAHJONG_WIDTH * (x + 1), MAHJONG_HEIGHT * (y + 1)
                );
                auto mahjong_actual_size = D2D1::SizeF(
                    dpi_x_scale * scale_factor * MAHJONG_WIDTH,
                    dpi_y_scale * scale_factor * MAHJONG_HEIGHT
                );
                auto tile_src_rt = D2D1::RectU(
                    std::lround(mahjong_actual_size.width * atlas_x),
                    std::lround(mahjong_actual_size.height * atlas_y),
                    std::lround(mahjong_actual_size.width * (atlas_x + 1)),
                    std::lround(mahjong_actual_size.height * (atlas_y + 1))
                );
                auto tile_dest_rt = D2D1::RectF(
                    std::lround(dpi_x_scale * (scale_factor * mahjong_rect.left + dest_rect.left)) * 1.0f,
                    std::lround(dpi_y_scale * (scale_factor * mahjong_rect.top + dest_rect.top)) * 1.0f,
                    (std::lround(dpi_x_scale * (scale_factor * mahjong_rect.left + dest_rect.left)) + width_of(tile_src_rt)) * 1.0f,
                    (std::lround(dpi_y_scale * (scale_factor * mahjong_rect.top + dest_rect.top)) + height_of(tile_src_rt)) * 1.0f
                );
                batch->AddSprites(1, &tile_dest_rt, &tile_src_rt);
            };
            auto draw_single_glow_fn = [&](float x, float y, const D2D1_COLOR_F& clr, const winrt::com_ptr<ID2D1DeviceContext5>& devctx) {
                auto mahjong_rect = D2D1::RectF(
                    MAHJONG_WIDTH * x, MAHJONG_HEIGHT * y,
                    MAHJONG_WIDTH * (x + 1), MAHJONG_HEIGHT * (y + 1)
                );
                const float HIGHLIGHT_BLUR_RADIUS = 3.0f;
                winrt::com_ptr<ID2D1Effect> flood_effect;
                winrt::check_hresult(devctx->CreateEffect(CLSID_D2D1Flood, flood_effect.put()));
                winrt::check_hresult(flood_effect->SetValue(
                    D2D1_FLOOD_PROP_COLOR,
                    D2D1::Vector4F(clr.r, clr.g, clr.b, clr.a)
                ));
                winrt::com_ptr<ID2D1Effect> premul_effect;
                winrt::check_hresult(devctx->CreateEffect(CLSID_D2D1Premultiply, premul_effect.put()));
                premul_effect->SetInputEffect(0, flood_effect.get());
                winrt::com_ptr<ID2D1Effect> crop_effect;
                winrt::check_hresult(devctx->CreateEffect(CLSID_D2D1Crop, crop_effect.put()));
                winrt::check_hresult(crop_effect->SetValue(
                    D2D1_CROP_PROP_RECT,
                    D2D1::RectF(
                        -HIGHLIGHT_BLUR_RADIUS,
                        -HIGHLIGHT_BLUR_RADIUS,
                        width_of(mahjong_rect) + HIGHLIGHT_BLUR_RADIUS,
                        height_of(mahjong_rect) + HIGHLIGHT_BLUR_RADIUS)
                ));
                crop_effect->SetInputEffect(0, premul_effect.get());
                winrt::com_ptr<ID2D1Effect> blur_effect;
                winrt::check_hresult(devctx->CreateEffect(CLSID_D2D1GaussianBlur, blur_effect.put()));
                winrt::check_hresult(blur_effect->SetValue(
                    D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION,
                    HIGHLIGHT_BLUR_RADIUS
                ));
                blur_effect->SetInputEffect(0, crop_effect.get());
                devctx->DrawImage(
                    blur_effect.get(),
                    D2D1::Point2F(mahjong_rect.left, mahjong_rect.top)
                );
            };
            auto draw_polyline_fn = [&](const std::vector<D2D1_POINT_2F>& pts, const D2D1_COLOR_F& clr, const winrt::com_ptr<ID2D1DeviceContext5>& devctx) {
                if (pts.empty()) { return; }
                winrt::com_ptr<ID2D1PathGeometry> path;
                winrt::com_ptr<ID2D1GeometrySink> path_sink;
                winrt::check_hresult(m_d2d1_factory->CreatePathGeometry(path.put()));
                winrt::check_hresult(path->Open(path_sink.put()));
                path_sink->BeginFigure(pts.front(), D2D1_FIGURE_BEGIN_HOLLOW);
                path_sink->AddLines(pts.data() + 1, static_cast<UINT32>(pts.size() - 1));
                path_sink->EndFigure(D2D1_FIGURE_END_OPEN);
                winrt::check_hresult(path_sink->Close());
                winrt::com_ptr<ID2D1SolidColorBrush> brush;
                winrt::check_hresult(m_d2d1_dev_ctx->CreateSolidColorBrush(clr, brush.put()));
                devctx->DrawGeometry(path.get(), brush.get(), 2);
            };
            auto draw_connection_path_fn = [&](const std::vector<std::pair<int, int>>& path, const D2D1_COLOR_F& clr, const winrt::com_ptr<ID2D1DeviceContext5>& devctx) {
                std::vector<D2D1_POINT_2F> pts;
                pts.reserve(path.size());
                for (const auto& [x, y] : path) {
                    pts.emplace_back((x + 0.5f) * MAHJONG_WIDTH, (y + 0.5f) * MAHJONG_HEIGHT);
                }
                draw_polyline_fn(pts, clr, devctx);
            };
            auto draw_glow_ring_fn = [&](float x, float y, float inner_size, const D2D1_COLOR_F& clr, const winrt::com_ptr<ID2D1DeviceContext5>& devctx) {
                constexpr float RING_WIDTH = 30;
                const float full_size = inner_size + RING_WIDTH;
                auto clr_transparent = clr;
                clr_transparent.a = 0;
                auto center_pt = D2D1::Point2F(
                    x * MAHJONG_WIDTH,
                    y * MAHJONG_HEIGHT
                );
                winrt::com_ptr<ID2D1RadialGradientBrush> radial_brush;
                winrt::com_ptr<ID2D1GradientStopCollection> gradient_stop_collection;
                D2D1_GRADIENT_STOP gradient_stops_data[3] = {
                    { .position = inner_size / full_size, .color = clr_transparent },
                    { .position = (inner_size + RING_WIDTH / 2) / full_size, .color = clr },
                    { .position = 1, .color = clr_transparent },
                };
                winrt::check_hresult(devctx->CreateGradientStopCollection(
                    gradient_stops_data, 3, gradient_stop_collection.put()
                ));
                winrt::check_hresult(devctx->CreateRadialGradientBrush(
                    D2D1::RadialGradientBrushProperties(
                        center_pt, D2D1::Point2F(), full_size, full_size),
                    gradient_stop_collection.get(),
                    radial_brush.put()
                ));
                devctx->FillEllipse(
                    D2D1::Ellipse(center_pt, full_size, full_size),
                    radial_brush.get()
                );
            };
            // Draw base
            m_board_bmp_dev_ctx->BeginDraw();
            m_board_bmp_dev_ctx->SetTransform(base_transform);
            m_board_bmp_dev_ctx->Clear(D2D1::ColorF(D2D1::ColorF::White, 0));
            // NOTE: ID2D1SpriteBatch can reduce CPU usage
            const bool use_img_batch_draw = true;
            winrt::com_ptr<ID2D1SpriteBatch> img_batch;
            if (use_img_batch_draw) {
                winrt::check_hresult(m_board_bmp_dev_ctx->CreateSpriteBatch(img_batch.put()));
            }
            for (unsigned y = 0; y < MahjongGame::BOARD_HEIGHT; y++) {
                for (unsigned x = 0; x < MahjongGame::BOARD_WIDTH; x++) {
                    auto type = m_game.read_board(x, y);
                    if (type == 0) { continue; }
                    type--;
                    draw_single_base_fn(x * 1.0f, y * 1.0f, type, m_board_bmp_dev_ctx);
                    if (use_img_batch_draw) {
                        draw_single_img_batch_nodpi_fn(x * 1.0f, y * 1.0f, type, img_batch);
                    }
                    else {
                        draw_single_img_fn(x * 1.0f, y * 1.0f, type, m_board_bmp_dev_ctx, true);
                    }
                }
            }
            if (use_img_batch_draw) {
                m_board_bmp_dev_ctx->SetUnitMode(D2D1_UNIT_MODE_PIXELS);
                m_board_bmp_dev_ctx->SetTransform(D2D1::Matrix3x2F::Identity());
                m_board_bmp_dev_ctx->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
                m_board_bmp_dev_ctx->DrawSpriteBatch(
                    img_batch.get(),
                    m_img_atlas_tiles.get(),
                    D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR
                );
                m_board_bmp_dev_ctx->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                m_board_bmp_dev_ctx->SetTransform(base_transform);
                m_board_bmp_dev_ctx->SetUnitMode(D2D1_UNIT_MODE_DIPS);
            }
            m_board_bmp_dev_ctx->EndDraw();
            winrt::com_ptr<ID2D1Bitmap> bmp;
            winrt::check_hresult(m_board_bmp_render_target->GetBitmap(bmp.put()));
            winrt::com_ptr<ID2D1Effect> shadow_effect;
            winrt::check_hresult(m_board_bmp_dev_ctx->CreateEffect(CLSID_D2D1Shadow, shadow_effect.put()));
            winrt::check_hresult(shadow_effect->SetValue(D2D1_SHADOW_PROP_OPTIMIZATION, D2D1_SHADOW_OPTIMIZATION_SPEED));
            winrt::check_hresult(shadow_effect->SetValue(D2D1_SHADOW_PROP_BLUR_STANDARD_DEVIATION, scale_factor * 4));
            shadow_effect->SetInput(0, bmp.get());
            winrt::com_ptr<ID2D1Effect> composite_effect;
            winrt::check_hresult(m_board_bmp_dev_ctx->CreateEffect(CLSID_D2D1Composite, composite_effect.put()));
            composite_effect->SetInputEffect(0, shadow_effect.get());
            composite_effect->SetInput(1, bmp.get());
            m_d2d1_dev_ctx->DrawImage(composite_effect.get());
            // Draw overlay (glow effect, path, etc.)
            m_d2d1_dev_ctx->SetTransform(base_transform);
            // Draw selection effect
            if (m_selected_mahjong_pos) {
                draw_single_glow_fn(m_selected_mahjong_pos->x * 1.0f, m_selected_mahjong_pos->y * 1.0f,
                    D2D1::ColorF(D2D1::ColorF::Yellow, 0.5f), m_d2d1_dev_ctx);
            }
            // NOTE: Clear specific stale animations
            for (size_t i = 0; i < m_tracking_animations.size(); i++) {
                auto p = std::get_if<AppAnimationDesc_SuccessfulMatch>(&m_tracking_animations[i]);
                if (!p) { continue; }
                if (!p->veffect->is_animating()) {
                    m_tracking_animations.erase(m_tracking_animations.begin() + i);
                    i--;
                }
            }
            // Draw animations
            for (const auto& i : m_tracking_animations) {
                std::visit([&](auto&& v) {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, AppAnimationDesc_InvalidMatch>) {
                        if (!v.vopacity->is_animating()) { return; }
                        draw_connection_path_fn(
                            v.path,
                            D2D1::ColorF(D2D1::ColorF::Red, static_cast<float>(v.vopacity->get_value())),
                            m_d2d1_dev_ctx
                        );
                        draw_single_glow_fn(
                            v.dest.x * 1.0f, v.dest.y * 1.0f,
                            D2D1::ColorF(D2D1::ColorF::Red, static_cast<float>(v.vopacity->get_value() * 0.5f)),
                            m_d2d1_dev_ctx
                        );
                    }
                    else if constexpr (std::is_same_v<T, AppAnimationDesc_SuccessfulMatch>) {
                        if (v.voffx->is_animating()) {
                            draw_single_base_fn(
                                static_cast<float>(v.center.x - v.voffx->get_value()),
                                static_cast<float>(v.center.y - v.voffy->get_value()),
                                v.type,
                                m_d2d1_dev_ctx
                            );
                            draw_single_img_fn(
                                static_cast<float>(v.center.x - v.voffx->get_value()),
                                static_cast<float>(v.center.y - v.voffy->get_value()),
                                v.type,
                                m_d2d1_dev_ctx,
                                true
                            );
                            draw_single_base_fn(
                                static_cast<float>(v.center.x + v.voffx->get_value()),
                                static_cast<float>(v.center.y + v.voffy->get_value()),
                                v.type,
                                m_d2d1_dev_ctx
                            );
                            draw_single_img_fn(
                                static_cast<float>(v.center.x + v.voffx->get_value()),
                                static_cast<float>(v.center.y + v.voffy->get_value()),
                                v.type,
                                m_d2d1_dev_ctx,
                                true
                            );
                        }
                        draw_connection_path_fn(
                            v.path,
                            D2D1::ColorF(D2D1::ColorF::LimeGreen, static_cast<float>(v.vopacity->get_value())),
                            m_d2d1_dev_ctx
                        );
                        draw_glow_ring_fn(
                            v.center.x + 0.5f, v.center.y + 0.5f,
                            static_cast<float>((1 - v.veffect->get_value()) * 30),
                            D2D1::ColorF(D2D1::ColorF::HotPink, static_cast<float>(v.veffect->get_value())),
                            m_d2d1_dev_ctx
                        );
                    }
                    else if constexpr (std::is_same_v<T, AppAnimationDesc_WinGame>) {
                        m_d2d1_dev_ctx->SetTransform(D2D1::Matrix3x2F::Identity());
                        winrt::com_ptr<ID2D1SolidColorBrush> overlay_brush;
                        winrt::check_hresult(m_d2d1_dev_ctx->CreateSolidColorBrush(
                            D2D1::ColorF(D2D1::ColorF::WhiteSmoke, static_cast<float>(v.vprogress->get_value() * 0.75f)),
                            overlay_brush.put()
                        ));
                        m_d2d1_dev_ctx->FillRectangle(m_layout.board.outer_rt, overlay_brush.get());
                        auto dur = std::chrono::duration<double>(*m_game_stop_ts - m_game_start_ts);
                        draw_text_center_fn(
                            std::format(L"恭喜!\n你的用时: {} 秒", std::round(dur.count() * 100) / 100),
                            static_cast<float>(v.vprogress->get_value() * 30),
                            m_layout.board.outer_rt,
                            m_d2d1_dev_ctx
                        );
                        m_d2d1_dev_ctx->SetTransform(base_transform);
                    }
                    else {
                        static_assert(always_false_v<T>, "Non-exhaustive visitor!");
                    }
                }, i);
            }
            m_d2d1_dev_ctx->SetTransform(D2D1::Matrix3x2F::Identity());
        };
        draw_mahjong_board_fn();
        // Draw sidebar
        auto draw_sidebar_fn = [&] {
            m_d2d1_dev_ctx->FillRectangle(&m_layout.sidebar.outer_rt, m_white_brush.get());
            auto time_txt_rt = D2D1::RectF(
                m_layout.sidebar.inner_rt.left,
                m_layout.sidebar.inner_rt.top,
                m_layout.sidebar.inner_rt.right,
                (m_layout.sidebar.inner_rt.top + m_layout.sidebar.inner_rt.bottom) / 2
            );
            auto spent_time_dur_raw = m_game_stop_ts.value_or(std::chrono::system_clock::now()) - m_game_start_ts;
            if (!m_game_stop_ts) {
                // NOTE: Remember to update timer while the game is going on
                request_idle_redraw_until(m_game_start_ts + ceil<std::chrono::seconds>(spent_time_dur_raw));
            }
            auto spent_time_dur = std::chrono::duration_cast<std::chrono::seconds>(spent_time_dur_raw);
            auto spent_time_dur_mins = spent_time_dur.count() / 60;
            auto spent_time_dur_secs = spent_time_dur.count() % 60;
            draw_text_center_fn(
                std::format(L"当前用时:\n{:02}:{:02}", spent_time_dur_mins, spent_time_dur_secs),
                20, time_txt_rt, m_d2d1_dev_ctx);
            const auto& refresh_btn_rt = m_layout.sidebar.refresh_btn.frame_rt;
            bool refresh_btn_pressed = false;
            if (m_current_mouse_left_down && hit_test(m_last_pointer_down_pos, refresh_btn_rt) &&
                hit_test(m_last_pointer_pos, refresh_btn_rt))
            {
                refresh_btn_pressed = true;
            }
            m_d2d1_dev_ctx->FillRectangle(
                m_layout.sidebar.refresh_btn.frame_rt,
                refresh_btn_pressed ? m_corn_flower_blue_brush.get() : m_light_gray_brush.get()
            );
            m_d2d1_dev_ctx->DrawTextLayout(
                D2D1::Point2F(
                    m_layout.sidebar.refresh_btn.inner_rt.left,
                    m_layout.sidebar.refresh_btn.inner_rt.top),
                m_txt_layout_refresh_btn.get(),
                m_black_brush.get()
            );
        };
        draw_sidebar_fn();
        // Commit
        m_d2d1_dev_ctx->EndDraw();
        hr = m_dxgi_swapchain->Present(1, 0);
        if (hr != S_OK && hr != DXGI_STATUS_OCCLUDED) {
            recreate_device_resources();
            return on_render();
        }

        if (m_run_at_full_speed) {
            request_redraw();
        }
    }
    void on_resize(uint32_t width, uint32_t height) {
        std::ignore = width;
        std::ignore = height;
        {   // Resize swapchain
            m_d2d1_dev_ctx->SetTarget(nullptr);
            auto hr = m_dxgi_swapchain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
            if (hr == S_OK) {
                recreate_device_swapchain_bitmap();
            }
            else {
                recreate_device_resources();
            }
        }
        on_update_layout();
        m_board_bmp_render_target = nullptr;
        m_board_bmp_dev_ctx = nullptr;
        winrt::check_hresult(m_d2d1_dev_ctx->CreateCompatibleRenderTarget(
            D2D1::SizeF(
                std::max(1.0f, width_of(m_layout.board.outer_rt)),
                std::max(1.0f, height_of(m_layout.board.outer_rt))
            ),
            m_board_bmp_render_target.put()
        ));
        m_board_bmp_render_target.as(m_board_bmp_dev_ctx);
        recreate_device_size_sensitive_resources();
    }
    void on_update_layout(void) {
        const auto view_size = m_d2d1_dev_ctx->GetSize();
        D2D1_SIZE_F size_f1, size_f2;
        // Board
        m_layout.board.outer_rt = D2D1::RectF(
            0, 0,
            view_size.width - SIDEBAR_WIDTH, view_size.height
        );
        m_layout.board.frame_rt = D2D1::RectF(
            BOARD_MARGIN,
            BOARD_MARGIN,
            m_layout.board.outer_rt.right - BOARD_MARGIN,
            m_layout.board.outer_rt.bottom - BOARD_MARGIN
        );
        size_f1 = size_of(m_layout.board.frame_rt);
        size_f2 = D2D1::SizeF(
            std::min(size_f1.width, size_f1.height / BOARD_HEIGHT * BOARD_WIDTH),
            std::min(size_f1.height, size_f1.width / BOARD_WIDTH * BOARD_HEIGHT)
        );
        m_layout.board.scale_factor = size_f2.width / BOARD_WIDTH;
        m_layout.board.inner_rt = D2D1::RectF(
            m_layout.board.frame_rt.left + (size_f1.width - size_f2.width) / 2,
            m_layout.board.frame_rt.top + (size_f1.height - size_f2.height) / 2,
            m_layout.board.frame_rt.left + (size_f1.width + size_f2.width) / 2,
            m_layout.board.frame_rt.top + (size_f1.height + size_f2.height) / 2
        );
        // Sidebar
        m_layout.sidebar.outer_rt = D2D1::RectF(
            view_size.width - SIDEBAR_WIDTH, 0,
            view_size.width, view_size.height
        );
        m_layout.sidebar.frame_rt = m_layout.sidebar.outer_rt;
        m_layout.sidebar.inner_rt = m_layout.sidebar.outer_rt;
        m_layout.sidebar.refresh_btn.outer_rt = D2D1::RectF(
            m_layout.sidebar.inner_rt.left,
            vavg_of(m_layout.sidebar.inner_rt),
            m_layout.sidebar.inner_rt.right,
            m_layout.sidebar.inner_rt.bottom
        );
        m_txt_layout_refresh_btn = make_text_layout(L"重新开始", 16);
        DWRITE_TEXT_METRICS txt_metrics;
        winrt::check_hresult(m_txt_layout_refresh_btn->GetMetrics(&txt_metrics));
        size_f1 = D2D1::SizeF(txt_metrics.width, txt_metrics.height);
        m_layout.sidebar.refresh_btn.inner_rt = D2D1::RectF(
            havg_of(m_layout.sidebar.refresh_btn.outer_rt) - size_f1.width / 2,
            vavg_of(m_layout.sidebar.refresh_btn.outer_rt) - size_f1.height / 2,
            havg_of(m_layout.sidebar.refresh_btn.outer_rt) + size_f1.width / 2,
            vavg_of(m_layout.sidebar.refresh_btn.outer_rt) + size_f1.height / 2
        );
        m_layout.sidebar.refresh_btn.frame_rt = D2D1::RectF(
            m_layout.sidebar.refresh_btn.inner_rt.left - 10,
            m_layout.sidebar.refresh_btn.inner_rt.top - 6,
            m_layout.sidebar.refresh_btn.inner_rt.right + 10,
            m_layout.sidebar.refresh_btn.inner_rt.bottom + 6
        );
    }

    void on_mouse_capture_changed(void) {
        if (m_current_mouse_left_down) {
            // Fix broken state
            m_current_mouse_left_down = false;
            request_redraw();
        }
    }
    void on_mouse_left_down(D2D1_POINT_2F pt) {
        SetCapture(m_hwnd);

        if (hit_test(pt, m_layout.board.outer_rt)) {
            const auto scale_factor = width_of(m_layout.board.inner_rt) / BOARD_WIDTH;
            auto ix = std::lround(std::floor((
                pt.x - m_layout.board.inner_rt.left) / (scale_factor * MAHJONG_WIDTH)));
            auto iy = std::lround(std::floor((
                pt.y - m_layout.board.inner_rt.top) / (scale_factor * MAHJONG_HEIGHT)));
            std::optional<D2D1_POINT_2U> new_sel_pos;
            if (ix < 0 || ix >= MahjongGame::BOARD_WIDTH || iy < 0 || iy >= MahjongGame::BOARD_HEIGHT) {
                new_sel_pos = std::nullopt;
            }
            else {
                new_sel_pos = { static_cast<unsigned>(ix), static_cast<unsigned>(iy) };
                // Don't select if there is no mahjong
                if (m_game.read_board(new_sel_pos->x, new_sel_pos->y) == 0) {
                    new_sel_pos = std::nullopt;
                }
            }
            if (!new_sel_pos || !m_selected_mahjong_pos) {
                m_selected_mahjong_pos = new_sel_pos;
            }
            else {
                if (m_selected_mahjong_pos->x == new_sel_pos->x && m_selected_mahjong_pos->y == new_sel_pos->y) {
                    m_selected_mahjong_pos = std::nullopt;
                }
                else {
                    auto old_mahjong = m_game.read_board(m_selected_mahjong_pos->x, m_selected_mahjong_pos->y);
                    auto new_mahjong = m_game.read_board(new_sel_pos->x, new_sel_pos->y);
                    if (old_mahjong != new_mahjong) {
                        m_selected_mahjong_pos = new_sel_pos;
                    }
                    else {
                        auto path = m_game.test_connectivity(
                            m_selected_mahjong_pos->x, m_selected_mahjong_pos->y,
                            new_sel_pos->x, new_sel_pos->y
                        );
                        m_animation_manager.update_state();
                        if (path.size() == 0 || path.size() > 4) {
                            // Invalid match
                            play_app_animation(anim_invalid_match, std::move(path), *new_sel_pos);
                        }
                        else {
                            // Successful match
                            m_game.write_board(m_selected_mahjong_pos->x, m_selected_mahjong_pos->y, 0);
                            m_game.write_board(new_sel_pos->x, new_sel_pos->y, 0);
                            m_selected_mahjong_pos = std::nullopt;
                            play_app_animation(anim_successful_match, std::move(path), old_mahjong - 1);
                            if (m_game.is_board_empty()) {
                                m_game_stop_ts = std::chrono::system_clock::now();
                                play_app_animation(anim_win_game);
                            }
                        }
                    }
                }
            }
            request_redraw();
        }
        else if (hit_test(pt, m_layout.sidebar.refresh_btn.frame_rt)) {
            request_redraw();
        }
        m_last_pointer_pos = m_last_pointer_down_pos = pt;
        m_last_pointer_pos = pt;
        m_current_mouse_left_down = true;
    }
    void on_mouse_left_up(D2D1_POINT_2F pt) {
        const auto& refresh_btn_rt = m_layout.sidebar.refresh_btn.frame_rt;
        if (m_current_mouse_left_down && hit_test(m_last_pointer_down_pos, refresh_btn_rt) &&
            hit_test(pt, refresh_btn_rt))
        {
            m_game.rebuild_board();
            m_selected_mahjong_pos = std::nullopt;
            m_game_start_ts = std::chrono::system_clock::now();
            m_game_stop_ts = std::nullopt;
            m_tracking_animations.clear();
            request_redraw();
        }
        m_last_pointer_pos = pt;
        m_current_mouse_left_down = false;

        ReleaseCapture();
    }
    void on_mouse_move(D2D1_POINT_2F pt) {
        const auto& refresh_btn_rt = m_layout.sidebar.refresh_btn.frame_rt;
        if (m_current_mouse_left_down && hit_test(m_last_pointer_down_pos, refresh_btn_rt) &&
            hit_test(m_last_pointer_pos, refresh_btn_rt) != hit_test(pt, refresh_btn_rt))
        {
            request_redraw();
        }

        m_last_pointer_pos = pt;
    }

    // WARN: Remember to update animation manager before playing animations!
    struct anim_invalid_match_t {};
    static constexpr anim_invalid_match_t anim_invalid_match{};
    struct anim_successful_match_t {};
    static constexpr anim_successful_match_t anim_successful_match{};
    struct anim_win_game_t {};
    static constexpr anim_win_game_t anim_win_game{};
    void play_app_animation(anim_invalid_match_t,
        std::vector<std::pair<int, int>> path,
        D2D1_POINT_2U dest
    ) {
        // Find or create unused animation
        auto it = std::find_if(
            m_tracking_animations.begin(), m_tracking_animations.end(),
            [](const AppAnimationDesc& anim) {
                auto p = std::get_if<AppAnimationDesc_InvalidMatch>(&anim);
                if (!p) { return false; }
                if (p->vopacity->is_animating()) { return false; }
                return true;
            }
        );
        if (it == m_tracking_animations.end()) {
            m_tracking_animations.emplace_back(AppAnimationDesc_InvalidMatch{
                .vopacity = m_animation_manager.create_variable({
                    std::make_shared<CubicEaseOutAnimationKeyFrame>(0, 1),
                    std::make_shared<CubicEaseOutAnimationKeyFrame>(0.5, 0),
                })
            });
            it = m_tracking_animations.end();
            --it;
        }
        auto& anim = std::get<AppAnimationDesc_InvalidMatch>(*it);
        anim.path = std::move(path);
        anim.dest = dest;
        m_animation_manager.play(anim.vopacity);
    }
    void play_app_animation(anim_successful_match_t,
        std::vector<std::pair<int, int>> path,
        unsigned mahjong_type
    ) {
        if (path.size() < 2) {
            throw winrt::hresult_invalid_argument(L"动画参数 path 无效");
        }
        // NOTE: No need to find unused animation, they are unrecyclable (for the time being)
        //       and will be automatically removed in on_render()
        auto pos1 = path.front(), pos2 = path.back();
        int neg_fix = (pos1.first - pos2.first) * (pos1.second - pos2.second) < 0 ? -1 : 1;
        auto x_dist = std::abs(pos1.first - pos2.first), y_dist = std::abs(pos1.second - pos2.second);
        double x_backoff_dist = x_dist >= 10 ? 0 : std::pow((10 - x_dist) / 10.0, 3) * 2;
        auto calc_inv_cubic_ease_out_fn = [](double s, double e, double y) {
            double v = (y - s) / (e - s);
            return std::cbrt(v - 1) + 1;
        };
        double phase_0_x = x_dist / 2.0, phase_1_x = x_dist / 2.0 + x_backoff_dist, phase_2_x = 0.5;
        double phase_1_dur = 1 - calc_inv_cubic_ease_out_fn(std::min(phase_0_x, phase_2_x), phase_1_x, phase_0_x);
        double phase_2_dur = 1;
        double x_backoff_time = (phase_1_dur / (phase_1_dur + phase_2_dur)) * 0.5;
        AppAnimationDesc_SuccessfulMatch anim_desc{
            .path = std::move(path),
            .type = mahjong_type,
            .center = { (pos1.first + pos2.first) / 2.0f, (pos1.second + pos2.second) / 2.0f },
            .voffx = m_animation_manager.create_variable({
                // std::make_shared<LinearAnimationKeyFrame>(0, x_dist / 2.0),
                // std::make_shared<LinearAnimationKeyFrame>(0.5, 0.5),
                std::make_shared<CubicEaseOutAnimationKeyFrame>(0, phase_0_x),
                std::make_shared<CubicEaseOutAnimationKeyFrame>(x_backoff_time, phase_1_x),
                std::make_shared<CubicEaseInAnimationKeyFrame>(0.5, phase_2_x),
            }),
            .voffy = m_animation_manager.create_variable({
                std::make_shared<LinearAnimationKeyFrame>(0, neg_fix * y_dist / 2.0),
                std::make_shared<LinearAnimationKeyFrame>(0.5, 0),
            }),
            .vopacity = m_animation_manager.create_variable({
                std::make_shared<CubicEaseOutAnimationKeyFrame>(0, 1),
                std::make_shared<CubicEaseOutAnimationKeyFrame>(0.5, 0),
            }),
            .veffect = m_animation_manager.create_variable({
                std::make_shared<DiscreteAnimationKeyFrame>(0, 0),
                std::make_shared<DiscreteAnimationKeyFrame>(0.5, 1),
                std::make_shared<ExponentialEaseOutAnimationKeyFrame>(1.5, 0),
            }),
        };
        m_animation_manager.play(anim_desc.voffx);
        m_animation_manager.play(anim_desc.voffy);
        m_animation_manager.play(anim_desc.vopacity);
        m_animation_manager.play(anim_desc.veffect);
        m_tracking_animations.push_back(std::move(anim_desc));
    }
    void play_app_animation(anim_win_game_t) {
        AppAnimationDesc_WinGame anim_desc {
            .vprogress = m_animation_manager.create_variable({
                std::make_shared<DiscreteAnimationKeyFrame>(0, 0),
                std::make_shared<DiscreteAnimationKeyFrame>(1.5, 0),
                std::make_shared<ExponentialEaseOutAnimationKeyFrame>(2, 1),
            }),
        };
        m_animation_manager.play(anim_desc.vprogress);
        m_tracking_animations.push_back(std::move(anim_desc));
    }

    winrt::com_ptr<IDWriteTextLayout> make_text_layout(
        std::wstring_view str,
        float font_size,
        float max_width = 1e9,
        bool center = false,
        const wchar_t* font_family = L"Microsoft YaHei"
    ) {
        if (font_size <= 0) {
            font_size = static_cast<float>(1e-9);
        }
        winrt::com_ptr<IDWriteTextFormat> txt_fmt;
        winrt::com_ptr<IDWriteTextLayout> txt_layout;
        winrt::check_hresult(m_dwrite_factory->CreateTextFormat(
            font_family,
            nullptr,
            DWRITE_FONT_WEIGHT_REGULAR,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            font_size,
            L"zh-cn",
            txt_fmt.put()
        ));
        winrt::check_hresult(m_dwrite_factory->CreateTextLayout(
            str.data(), static_cast<UINT32>(str.size()),
            txt_fmt.get(),
            max_width,
            0,
            txt_layout.put()
        ));
        if (center) {
            winrt::check_hresult(txt_layout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER));
        }
        return txt_layout;
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
        SetTimer(m_hwnd, REDRAW_TIMER_ID, static_cast<UINT>(wait_milliseconds), [](HWND hwnd, UINT, UINT_PTR, DWORD) {
            KillTimer(hwnd, REDRAW_TIMER_ID);
            InvalidateRect(hwnd, nullptr, false);
            // DwmFlush();
        });
    }

    std::exception_ptr m_cur_exception = nullptr;
    AnimationManager m_animation_manager;
    struct AppAnimationDesc_InvalidMatch {
        std::vector<std::pair<int, int>> path;
        D2D1_POINT_2U dest;
        std::shared_ptr<AnimationVariable> vopacity;    // Shared opacity
    };
    struct AppAnimationDesc_SuccessfulMatch {
        // NOTE: Guaranteed to contain at least 2 points (start & end)
        std::vector<std::pair<int, int>> path;
        unsigned type;  // Type of mahjong {range: [0, MahjongGame::MAHJONG_TYPES_COUNT)}
        // NOTE: Get src & dest coordinates from path instead
        // D2D1_POINT_2U src, dest;
        D2D1_POINT_2F center;   // Center pos
        std::shared_ptr<AnimationVariable> voffx, voffy;    // Mahjong offset, unit: index in board
        std::shared_ptr<AnimationVariable> vopacity;    // Path opacity
        std::shared_ptr<AnimationVariable> veffect;     // Effect progress
    };
    struct AppAnimationDesc_WinGame {
        std::shared_ptr<AnimationVariable> vprogress;
    };
    using AppAnimationDesc = std::variant<
        AppAnimationDesc_InvalidMatch, AppAnimationDesc_SuccessfulMatch,
        AppAnimationDesc_WinGame>;
    std::vector<AppAnimationDesc> m_tracking_animations;

    MahjongGame m_game;
    std::chrono::system_clock::time_point m_game_start_ts;
    std::optional<std::chrono::system_clock::time_point> m_game_stop_ts;

    HWND m_hwnd{};
    bool m_run_at_full_speed{ false };
    winrt::com_ptr<ID2D1Factory7> m_d2d1_factory;
    winrt::com_ptr<IWICImagingFactory> m_wic_factory;
    winrt::com_ptr<IDWriteFactory> m_dwrite_factory;
    winrt::com_ptr<IDXGISwapChain1> m_dxgi_swapchain;
    winrt::com_ptr<ID2D1DeviceContext5> m_d2d1_dev_ctx;
    winrt::com_ptr<ID2D1BitmapRenderTarget> m_board_bmp_render_target;
    winrt::com_ptr<ID2D1DeviceContext5> m_board_bmp_dev_ctx;
    winrt::com_ptr<ID2D1SolidColorBrush> m_light_slate_gray_brush;
    winrt::com_ptr<ID2D1SolidColorBrush> m_light_gray_brush;
    winrt::com_ptr<ID2D1SolidColorBrush> m_corn_flower_blue_brush;
    winrt::com_ptr<ID2D1SolidColorBrush> m_black_brush;
    winrt::com_ptr<ID2D1SolidColorBrush> m_white_brush;
    winrt::com_ptr<IWICBitmapSource> m_wic_img_tiles[MahjongGame::MAHJONG_TYPES_COUNT];
    winrt::com_ptr<ID2D1Bitmap> m_img_tiles[MahjongGame::MAHJONG_TYPES_COUNT];
    winrt::com_ptr<ID2D1Bitmap> m_img_atlas_tiles;
    unsigned m_rows_img_atlas_tiles, m_cols_img_atlas_tiles;
    winrt::com_ptr<IDWriteTextLayout> m_txt_layout_refresh_btn;

    D2D1_POINT_2F m_last_pointer_pos{}, m_last_pointer_down_pos{};
    bool m_current_mouse_left_down{};
    std::optional<D2D1_POINT_2U> m_selected_mahjong_pos;

    struct {
        struct {
            D2D1_RECT_F outer_rt, frame_rt, inner_rt;
            float scale_factor;
        } board;
        struct {
            D2D1_RECT_F outer_rt, frame_rt, inner_rt;
            struct {
                D2D1_RECT_F outer_rt, frame_rt, inner_rt;
            } refresh_btn;
        } sidebar;
    } m_layout;
};

HWND g_hwnd;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) try {
    HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0);
    winrt::init_apartment();
    MahjongApp app{ hInstance };
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
