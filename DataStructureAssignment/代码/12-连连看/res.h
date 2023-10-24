#pragma once

#ifdef RES_H_INCLUDE_RES
#include <initguid.h>
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
#include <winrt/base.h>

namespace resources {
    namespace imaging {
        winrt::com_ptr<IWICBitmapSource> load_wic_bitmap_from_file(
            const winrt::com_ptr<IWICImagingFactory>& wic_factory,
            const wchar_t* path
        );
        winrt::com_ptr<ID2D1Bitmap> load_d2d1_bitmap_from_file(
            const winrt::com_ptr<ID2D1RenderTarget>& render_target,
            const winrt::com_ptr<IWICImagingFactory>& wic_factory,
            const wchar_t* path
        );
    }
}
