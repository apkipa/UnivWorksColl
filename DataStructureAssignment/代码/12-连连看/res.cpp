#define RES_H_INCLUDE_RES

#include "res.h"

namespace resources {
    namespace imaging {
        winrt::com_ptr<IWICBitmapSource> load_wic_bitmap_from_file(
            const winrt::com_ptr<IWICImagingFactory>& wic_factory,
            const wchar_t* path
        ) {
            winrt::com_ptr<IWICBitmapDecoder> decoder;
            winrt::com_ptr<IWICBitmapFrameDecode> frame_src;
            winrt::com_ptr<IWICFormatConverter> converter;

            winrt::check_hresult(wic_factory->CreateDecoderFromFilename(
                path,
                nullptr,
                GENERIC_READ,
                WICDecodeMetadataCacheOnLoad,
                decoder.put()
            ));
            winrt::check_hresult(decoder->GetFrame(0, frame_src.put()));
            winrt::check_hresult(wic_factory->CreateFormatConverter(converter.put()));
            winrt::check_hresult(converter->Initialize(
                frame_src.get(),
                GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone,
                nullptr,
                0,
                WICBitmapPaletteTypeMedianCut
            ));
            return converter;
        }
        winrt::com_ptr<ID2D1Bitmap> load_d2d1_bitmap_from_file(
            const winrt::com_ptr<ID2D1RenderTarget>& render_target,
            const winrt::com_ptr<IWICImagingFactory>& wic_factory,
            const wchar_t* path
        ) {
            winrt::com_ptr<ID2D1Bitmap> bitmap;
            winrt::com_ptr<IWICBitmapDecoder> decoder;
            winrt::com_ptr<IWICBitmapFrameDecode> frame_src;
            winrt::com_ptr<IWICFormatConverter> converter;

            winrt::check_hresult(wic_factory->CreateDecoderFromFilename(
                path,
                nullptr,
                GENERIC_READ,
                WICDecodeMetadataCacheOnLoad,
                decoder.put()
            ));
            winrt::check_hresult(decoder->GetFrame(0, frame_src.put()));
            winrt::check_hresult(wic_factory->CreateFormatConverter(converter.put()));
            winrt::check_hresult(converter->Initialize(
                frame_src.get(),
                GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone,
                nullptr,
                0,
                WICBitmapPaletteTypeMedianCut
            ));
            winrt::check_hresult(render_target->CreateBitmapFromWicBitmap(
                converter.get(),
                nullptr,
                bitmap.put()
            ));
            return bitmap;
        }
    }
}
