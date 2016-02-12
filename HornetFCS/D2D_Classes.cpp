#include "stdafx.h"

#include "D2D_Classes.h"

#include <cassert>
#include <process.h>

#include "Utils.h"

namespace Gauge
{

//-----------------------------------------------------------------------------

D2DGeo2::D2DGeo2(ID2D1Factory*& srcfactory, bool open)
    :
    m_geo(nullptr),
    m_sink(nullptr),
    m_transformedGeo(nullptr)
{
    srcfactory->CreatePathGeometry(&m_geo);
    if (open)
    {
        m_geo->Open(&m_sink);
    }
}

D2DGeo2::~D2DGeo2()
{
    if (m_transformedGeo)
    {
        m_transformedGeo->Release();
    }
    if (m_geo)
    {
        m_geo->Release();
    }
    if (m_sink)
    {
        m_sink->Release();
    }
}

void D2DGeo2::open_sink()
{
    m_geo->Open(&m_sink);
}

void D2DGeo2::close_sink()
{
    if (m_sink)
    {
        m_sink->Close();
        m_sink->Release();
        m_sink = nullptr;
    }
}

void D2DGeo2::transform(ID2D1Factory*& srcfactory, const D2D1_MATRIX_3X2_F mat)
{
    if (m_transformedGeo)
    {
        m_transformedGeo->Release();
        m_transformedGeo = nullptr;
    }
    transform(srcfactory, m_transformedGeo, mat);
}

void D2DGeo2::transform(ID2D1Factory*& srcfactory, ID2D1TransformedGeometry*& dest_t_geo, const D2D1_MATRIX_3X2_F mat)
{
    srcfactory->CreateTransformedGeometry(m_geo, mat, &dest_t_geo);
}

//-----------------------------------------------------------------------------

ID2D1PathGeometry* combine_multiple_transformed_geometries(ID2D1Factory*& srcFactory, int geo_count, ID2D1TransformedGeometry* geos[])
{
    ID2D1PathGeometry* path_geo_1 = nullptr;
    ID2D1PathGeometry* path_geo_2 = nullptr;

    srcFactory->CreatePathGeometry(&path_geo_1);
    srcFactory->CreatePathGeometry(&path_geo_2);

    for (int i = 0; i < geo_count; i++)
    {
        ID2D1GeometrySink* cmpl_s1 = nullptr;
        ID2D1GeometrySink* cmpl_s2 = nullptr;

        if (i % 2 == 0)
        {
            //copying into 1
            path_geo_1->Open(&cmpl_s1);

            if (i == 0)
            {
                geos[i]->CombineWithGeometry(geos[i], D2D1_COMBINE_MODE_UNION, nullptr, cmpl_s1);
            }
            else
            {
                geos[i]->CombineWithGeometry(path_geo_2, D2D1_COMBINE_MODE_UNION, nullptr, 0.0f, cmpl_s1);
            }

            cmpl_s1->Close();
            cmpl_s1->Release();
            if (i != 0)
            {
                path_geo_2->Release();
                srcFactory->CreatePathGeometry(&path_geo_2);
            }
            //cmpl_g1 now contains the geometry so far
        }
        else
        {
            //copying into 2
            path_geo_2->Open(&cmpl_s2);

            geos[i]->CombineWithGeometry(path_geo_1, D2D1_COMBINE_MODE_UNION, nullptr, cmpl_s2);

            cmpl_s2->Close();
            cmpl_s2->Release();
            path_geo_1->Release();
            srcFactory->CreatePathGeometry(&path_geo_1);
            //cmpl_g2 now contains the geometry so far
        }
    }

    if (geo_count % 2 == 0)
    {
        if (path_geo_1)
        {
            path_geo_1->Release();
        }
        return path_geo_2;
    }
    else
    {
        if (path_geo_2)
        {
            path_geo_2->Release();
        }
        return path_geo_1;
    }
}

//-----------------------------------------------------------------------------

D2DGauge::D2DGauge(FCS::FBW::Ptr const & fbw)
    :
    m_draw(false),
    m_end(false),
    m_sizeX(0),
    m_sizeY(0),
    m_textFormatSegoe(nullptr),
    m_solidBrushWhite(nullptr)
{
    m_fcs = fbw;
    m_drawThread = INVALID_HANDLE_VALUE;
    D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &m_d2dFactory);
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&m_directWriteFactory));
    D2D1_RENDER_TARGET_PROPERTIES prop = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        0,
        0,
        D2D1_RENDER_TARGET_USAGE_NONE,
        D2D1_FEATURE_LEVEL_DEFAULT
        );
    m_d2dFactory->CreateDCRenderTarget(&prop, &m_d2dTarget);
}

D2DGauge::~D2DGauge()
{
    m_end = true;
    if (m_drawThread != INVALID_HANDLE_VALUE) { WaitForSingleObject(m_drawThread, INFINITE); }
    if (m_d2dFactory) { m_d2dFactory->Release(); }
    if (m_directWriteFactory) { m_directWriteFactory->Release(); }
}

void D2DGauge::SetCanvas(ELEMENT_STATIC_IMAGE * canvas)
{
    m_canvas = canvas;
}

void D2DGauge::Callback(GAUGEHDR * pgauge, int service_id)
{
    switch (service_id)
    {
    case PANEL_SERVICE_PRE_DRAW:
    {
        // Called each time the gauge is to be re-drawn each frame
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
        // Called on init and on each gauge resize
        m_canvas = reinterpret_cast<ELEMENT_STATIC_IMAGE*>(pgauge->elements_list[0]);
        if (m_canvas->hdc || m_canvas->hdc != INVALID_HANDLE_VALUE)
        {
            m_end = true;
            if (m_drawThread != INVALID_HANDLE_VALUE)
            {
                WaitForSingleObject(m_drawThread, INFINITE);
            }

            m_sizeX = m_canvas->image_data.final->dim.x;
            m_sizeY = m_canvas->image_data.final->dim.y;

            RECT re;
            re.top = 0;
            re.left = 0;
            re.bottom = static_cast<long>(m_sizeY);
            re.right = static_cast<long>(m_sizeX);

            m_rectMain = D2D1_RECT_F(D2D1::RectF(10.0f, 10.0f, static_cast<float>(m_sizeX) - 20.0f, static_cast<float>(m_sizeY) - 20.0f));

            m_d2dTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_solidBrushWhite);

            if (m_sizeY < 1000)
            {
                m_directWriteFactory->CreateTextFormat(L"Segoe UI", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"en-US", &m_textFormatSegoe);
            }
            else
            {
                m_directWriteFactory->CreateTextFormat(L"Segoe UI", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 16.0f, L"en-US", &m_textFormatSegoe);
            }

            m_textFormatSegoe->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
            m_textFormatSegoe->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
            m_textFormatSegoe->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_DEFAULT, 0.0f, 0.0f);

            m_d2dTarget->BindDC(m_canvas->hdc, &re);
            m_end = false;
            m_drawThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, Draw, static_cast<void*>(this), 0, nullptr));
        }
        break;
    }
    }
}

unsigned int __stdcall D2DGauge::Draw(void *args)
{
    auto gau = static_cast<D2DGauge*>(args);
    
    assert(gau);
    while (!gau->m_end)
    {
        if (gau->m_draw)
        {
            if (auto p = gau->m_fcs.lock())
            {
                gau->m_d2dTarget->BeginDraw();
                gau->m_d2dTarget->Clear();

                auto text = Utils::s2ws(p->ToString());
                gau->m_d2dTarget->DrawTextW(text.c_str(), text.size(), gau->m_textFormatSegoe, gau->m_rectMain, gau->m_solidBrushWhite);

                gau->m_d2dTarget->EndDraw();
                SET_OFF_SCREEN(gau->m_canvas);
            }
            gau->m_draw = false;
        }
        Sleep(20);
    }

    return 0;
}

//-----------------------------------------------------------------------------

} // namespace Gauge