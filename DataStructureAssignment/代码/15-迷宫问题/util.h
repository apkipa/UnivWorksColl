#pragma once

#define NOMINMAX

#include <Windows.h>

#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

#include <Unknwn.h>
#include <Windows.h>
#include <shcore.h>
#include <Shlwapi.h>
#include <d2d1_3.h>
#include <d2d1_3helper.h>
#include <dwrite_3.h>
#include <wincodec.h>
#include <dwmapi.h>
#include <dxgi1_6.h>
#include <d3d11_4.h>
#include <shobjidl.h>
#include <winrt/base.h>
#include <winrt/windows.foundation.h>
#include <winrt/windows.foundation.collections.h>
#include <winrt/windows.ui.h>
#include <winrt/windows.ui.input.h>
#include <winrt/windows.ui.viewmanagement.h>
#include <winrt/windows.storage.h>
#include <winrt/windows.storage.pickers.h>

#include <string_view>
#include <algorithm>
#include <optional>
#include <iostream>
#include <variant>
#include <fstream>
#include <numeric>
#include <numbers>
#include <random>
#include <format>
#include <vector>
#include <ranges>
#include <queue>
#include <stack>
#include <any>

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

inline int gen_random_number(int start, int end) {
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
inline constexpr auto get_guid(const T& /* v_interface */) {
    return winrt::guid_of<T>();
}

template<typename T>
inline constexpr auto width_of(const T& v) -> decltype(v.left) {
    return v.right - v.left;
}
template<typename T>
inline constexpr auto height_of(const T& v) -> decltype(v.top) {
    return v.bottom - v.top;
}
inline D2D1_SIZE_F size_of(const D2D1_RECT_F& v) {
    return D2D1::SizeF(width_of(v), height_of(v));
}
inline D2D1_SIZE_U size_of(const D2D1_RECT_U& v) {
    return D2D1::SizeU(width_of(v), height_of(v));
}
template<typename T>
inline constexpr auto hsum_of(const T& v) -> decltype(v.left) {
    return v.left + v.right;
}
template<typename T>
inline constexpr auto vsum_of(const T& v) -> decltype(v.top) {
    return v.top + v.bottom;
}
template<typename T>
inline constexpr auto havg_of(const T& v) -> decltype(v.left) {
    return hsum_of(v) / 2;
}
template<typename T>
inline constexpr auto vavg_of(const T& v) -> decltype(v.top) {
    return vsum_of(v) / 2;
}

inline bool hit_test(const D2D1_POINT_2F& pt, const D2D1_RECT_F& v) {
    return (v.left <= pt.x && pt.x < v.right) && (v.top <= pt.y && pt.y < v.bottom);
}

enum CollectionChange {
    Reset = 0, ItemInserted, ItemRemoved, ItemChanged,
};
struct VectorChangedEventArgs {
    CollectionChange collection_change;
    size_t index;
};
// NOTE: A simplified version of Windows.Foundation.Collections.IObservableVector<T>,
//       where T is not limited to WinRT types
template<typename T>
struct ObservableVector {
    ObservableVector() {}
    ObservableVector(ObservableVector&& other) = delete;

    using vector_changed_t = winrt::delegate<ObservableVector*, VectorChangedEventArgs*>;
    winrt::event_token vector_changed(vector_changed_t delegate) {
        return m_ev_vec_changed.add(delegate);
    }
    void vector_changed(winrt::event_token token) {
        return m_ev_vec_changed.remove(token);
    }
    void append(const T& value) {
        auto index = m_vec.size();
        m_vec.push_back(value);
        VectorChangedEventArgs e{ CollectionChange::ItemInserted, index };
        m_ev_vec_changed(this, &e);
    }
    void append(T&& value) {
        auto index = m_vec.size();
        m_vec.push_back(std::move(value));
        VectorChangedEventArgs e{ CollectionChange::ItemInserted, index };
        m_ev_vec_changed(this, &e);
    }
    void clear(void) {
        m_vec.clear();
        VectorChangedEventArgs e{ CollectionChange::Reset, 0 };
        m_ev_vec_changed(this, &e);
    }
    T get_at(size_t index) const {
        return m_vec.at(index);
    }
    void set_at(size_t index, const T& value) {
        m_vec.at(index) = value;
        VectorChangedEventArgs e{ CollectionChange::ItemChanged, index };
        m_ev_vec_changed(this, &e);
    }
    void set_at(size_t index, T&& value) {
        m_vec.at(index) = std::move(value);
        VectorChangedEventArgs e{ CollectionChange::ItemChanged, index };
        m_ev_vec_changed(this, &e);
    }
    size_t size(void) const { return m_vec.size(); }

    struct iterator {
        using difference_type = std::ptrdiff_t;
        using value_type = T;

        iterator() : iterator(0, nullptr) {}
        iterator(size_t idx, const ObservableVector* vec) : m_index(idx), m_vec(vec) {}
        bool operator==(const iterator& other) const { return m_index == other.m_index; }
        iterator& operator++() {
            m_index++;
            return *this;
        }
        iterator operator++(int) {
            auto temp = *this;
            operator++();
            return temp;
        }
        iterator& operator--() {
            m_index--;
            return *this;
        }
        iterator operator--(int) {
            auto temp = *this;
            operator--();
            return temp;
        }
        value_type operator*(void) const {
            return m_vec->get_at(m_index);
        }
    private:
        size_t m_index;
        const ObservableVector* m_vec;
    };
    iterator begin(void) { return { 0, this }; }
    iterator end(void) { return { m_vec.size(), this }; }

private:
    std::vector<T> m_vec;
    winrt::event<vector_changed_t> m_ev_vec_changed;
};

inline D2D1_COLOR_F get_system_accent_color(void) {
    using namespace winrt::Windows::UI::ViewManagement;
    auto color = UISettings().GetColorValue(UIColorType::Accent);
    return D2D1::ColorF(color.R / 255.0f, color.G / 255.0f, color.B / 255.0f, color.A / 255.0f);
}
inline D2D1_COLOR_F get_system_accent_light1_color(void) {
    using namespace winrt::Windows::UI::ViewManagement;
    auto color = UISettings().GetColorValue(UIColorType::AccentLight1);
    return D2D1::ColorF(color.R / 255.0f, color.G / 255.0f, color.B / 255.0f, color.A / 255.0f);
}
inline D2D1_COLOR_F get_system_accent_dark1_color(void) {
    using namespace winrt::Windows::UI::ViewManagement;
    auto color = UISettings().GetColorValue(UIColorType::AccentDark1);
    return D2D1::ColorF(color.R / 255.0f, color.G / 255.0f, color.B / 255.0f, color.A / 255.0f);
}

inline D2D1_COLOR_F lerp_color(const D2D1_COLOR_F& clr1, const D2D1_COLOR_F& clr2, float t) {
    return D2D1::ColorF(
        std::lerp(clr1.r, clr2.r, t),
        std::lerp(clr1.g, clr2.g, t),
        std::lerp(clr1.b, clr2.b, t),
        std::lerp(clr1.a, clr2.a, t)
    );
}
