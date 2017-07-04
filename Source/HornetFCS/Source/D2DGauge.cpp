/**********************************************************
*                                                         *
*   Copyright (C) Orion Lyau 2016. All Rights Reserved.   *
*                                                         *
**********************************************************/

#include "D2DGauge.h"

#include <cassert>
#include <process.h>

#include "Utils.h"

namespace Gauge
{

//-----------------------------------------------------------------------------

D2DGauge::D2DGauge(FCS::FBW::Ptr const& fbw) :
    m_draw(false),
    m_end(false),
    m_sizeX(0),
    m_sizeY(0),
    m_d2dFactory(nullptr),
    m_d2dTarget(nullptr),
    m_directWriteFactory(nullptr),
    m_drawThread(INVALID_HANDLE_VALUE),
    m_fcs(fbw)
{
    D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &m_d2dFactory);
    D2D1_RENDER_TARGET_PROPERTIES prop = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        0,
        0,
        D2D1_RENDER_TARGET_USAGE_NONE,
        D2D1_FEATURE_LEVEL_DEFAULT
    );
    m_d2dFactory->CreateDCRenderTarget(&prop, &m_d2dTarget);
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&m_directWriteFactory));
}

//-----------------------------------------------------------------------------

D2DGauge::~D2DGauge()
{
    m_end = true;

    if (m_drawThread != INVALID_HANDLE_VALUE)
    {
        WaitForSingleObject(m_drawThread, INFINITE);
    }

    CloseHandle(m_drawThread);

    if (m_directWriteFactory)
    {
        m_directWriteFactory->Release();
    }

    if (m_d2dTarget)
    {
        m_d2dTarget->Release();
    }

    if (m_d2dFactory)
    {
        m_d2dFactory->Release();
    }
}

//-----------------------------------------------------------------------------

void D2DGauge::SetCanvas(ELEMENT_STATIC_IMAGE *canvas)
{
    m_canvas = canvas;
}

//-----------------------------------------------------------------------------

void D2DGauge::Callback(GAUGEHDR *pgauge, int service_id)
{
    switch (service_id)
    {
    case PANEL_SERVICE_PRE_DRAW:
    {
        if (m_canvas)
        {
            if (pgauge->elements_list[0])
            {
                m_draw = true;
            }
        }
        break;
    }
    case PANEL_SERVICE_POST_INSTALL:
    {
        m_canvas = reinterpret_cast<ELEMENT_STATIC_IMAGE*>(pgauge->elements_list[0]);
        if (m_canvas->hdc || m_canvas->hdc != INVALID_HANDLE_VALUE)
        {
            m_end = true;

            if (m_drawThread != INVALID_HANDLE_VALUE)
            {
                WaitForSingleObject(m_drawThread, INFINITE);
            }

            CloseHandle(m_drawThread);

            m_sizeX = m_canvas->image_data.final->dim.x;
            m_sizeY = m_canvas->image_data.final->dim.y;

            RECT rect;
            rect.top = 0;
            rect.left = 0;
            rect.bottom = static_cast<long>(m_sizeY);
            rect.right = static_cast<long>(m_sizeX);
            m_d2dTarget->BindDC(m_canvas->hdc, &rect);

            m_end = false;
            m_drawThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, Draw, static_cast<void*>(this), 0, nullptr));
        }
        break;
    }
    }
}

//-----------------------------------------------------------------------------

unsigned int __stdcall D2DGauge::Draw(void *args)
{
    auto gau = static_cast<D2DGauge*>(args);

    if (auto fcs = gau->m_fcs.lock())
    {
        D2D1_RECT_F rectMain = D2D1_RECT_F(D2D1::RectF(10.0f, 10.0f, static_cast<float>(gau->m_sizeX) - 20.0f, static_cast<float>(gau->m_sizeY) - 20.0f));
        ID2D1SolidColorBrush *solidBrushWhite = nullptr;
        IDWriteTextFormat *textFormatSegoe = nullptr;

        HRESULT resultText;
        HRESULT resultBrush;

        resultBrush = gau->m_d2dTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &solidBrushWhite);

        if (gau->m_sizeY < 1000)
        {
            resultText = gau->m_directWriteFactory->CreateTextFormat(L"Segoe UI", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"en-US", &textFormatSegoe);
        }
        else
        {
            resultText = gau->m_directWriteFactory->CreateTextFormat(L"Segoe UI", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 16.0f, L"en-US", &textFormatSegoe);
        }

        if (resultText == S_OK && resultBrush == S_OK)
        {
            textFormatSegoe->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
            textFormatSegoe->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
            textFormatSegoe->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_DEFAULT, 0.0f, 0.0f);

            assert(gau);
            while (!gau->m_end)
            {
                if (gau->m_draw)
                {
                    gau->m_d2dTarget->BeginDraw();
                    gau->m_d2dTarget->Clear();

                    auto text = Utils::s2ws(fcs->ToString());
                    gau->m_d2dTarget->DrawTextW(text.c_str(), static_cast<uint32_t>(text.size()), textFormatSegoe, rectMain, solidBrushWhite);

                    gau->m_d2dTarget->EndDraw();
                    SET_OFF_SCREEN(gau->m_canvas);
                    gau->m_draw = false;
                }
                Sleep(20);
            }
        }

        if (textFormatSegoe)
        {
            textFormatSegoe->Release();
        }

        if (solidBrushWhite)
        {
            solidBrushWhite->Release();
        }
    }

    return 0;
}

//-----------------------------------------------------------------------------

} // namespace Gauge