#include "stdafx.h"

#include "D2D_Classes.h"

D2DGeo2::D2DGeo2(ID2D1Factory*& srcfactory)
{
    sink = NULL;
    geo = NULL;
    t_geo = NULL;

    srcfactory->CreatePathGeometry(&geo);
    geo->Open(&sink);
}

D2DGeo2::D2DGeo2(ID2D1Factory*& srcfactory, bool open)
{
    sink = NULL;
    geo = NULL;
    t_geo = NULL;

    srcfactory->CreatePathGeometry(&geo);
    if (open)
    {
        geo->Open(&sink);
    }
}

void D2DGeo2::open_sink()
{
    geo->Open(&sink);
}

void D2DGeo2::close_sink()
{
    sink->Close();
    sink->Release();
    sink = NULL;
}

void D2DGeo2::transform(ID2D1Factory*& srcfactory, const D2D1_MATRIX_3X2_F mat)
{
    if (t_geo)
    {
        t_geo->Release();
        t_geo = NULL;
    }
    this->transform(srcfactory, t_geo, mat);
}

void D2DGeo2::transform(ID2D1Factory*& srcfactory, ID2D1TransformedGeometry*& dest_t_geo, const D2D1_MATRIX_3X2_F mat)
{
    srcfactory->CreateTransformedGeometry(geo, mat, &dest_t_geo);
}

D2DGeo2::~D2DGeo2()
{
    if (t_geo)
        t_geo->Release();
    if (geo)
        geo->Release();
    if (sink)
        sink->Release();
}

ID2D1PathGeometry* combine_multiple_transformed_geometries(ID2D1Factory*& srcfactory, int geo_count, ID2D1TransformedGeometry* geos[])
{
    ID2D1PathGeometry* path_geo_1 = NULL;
    ID2D1PathGeometry* path_geo_2 = NULL;

    srcfactory->CreatePathGeometry(&path_geo_1);
    srcfactory->CreatePathGeometry(&path_geo_2);

    for (short i = 0; i < geo_count; i++)
    {
        ID2D1GeometrySink* cmpl_s1 = NULL;
        ID2D1GeometrySink* cmpl_s2 = NULL;

        if (i % 2 == 0) {
            //copying into 1
            path_geo_1->Open(&cmpl_s1);

            if (i == 0)
                geos[i]->CombineWithGeometry(geos[i], D2D1_COMBINE_MODE_UNION, NULL, cmpl_s1);
            else
                geos[i]->CombineWithGeometry(path_geo_2, D2D1_COMBINE_MODE_UNION, NULL, NULL, cmpl_s1);

            cmpl_s1->Close();
            cmpl_s1->Release();
            if (i != 0) {
                path_geo_2->Release();
                srcfactory->CreatePathGeometry(&path_geo_2);
            }
            //cmpl_g1 now contains the geometry so far
        }
        else
        {
            //copying into 2
            path_geo_2->Open(&cmpl_s2);

            geos[i]->CombineWithGeometry(path_geo_1, D2D1_COMBINE_MODE_UNION, NULL, cmpl_s2);

            cmpl_s2->Close();
            cmpl_s2->Release();
            path_geo_1->Release();
            srcfactory->CreatePathGeometry(&path_geo_1);
            //cmpl_g2 now contains the geometry so far
        }
    }

    if (geo_count % 2 == 0)
    {
        if (path_geo_1) { path_geo_1->Release(); }
        return path_geo_2;
    }
    else
    {
        if (path_geo_2) { path_geo_2->Release(); }
        return path_geo_1;
    }
}

D2DGauge::D2DGauge()
{
    drawthread = INVALID_HANDLE_VALUE;
    draw = FALSE;
    end = FALSE;
    sz_x = 0;
    sz_y = 0;
    D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &pfactory);
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&pwfactory));
    D2D1_RENDER_TARGET_PROPERTIES prop = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        0,
        0,
        D2D1_RENDER_TARGET_USAGE_NONE,
        D2D1_FEATURE_LEVEL_DEFAULT
        );
    pfactory->CreateDCRenderTarget(&prop, &ptarget);
}

D2DGauge::~D2DGauge()
{
    end = TRUE;
    if (drawthread != INVALID_HANDLE_VALUE) { WaitForSingleObject(drawthread, INFINITE); }
    if (pfactory) { pfactory->Release(); }
    if (pwfactory) { pwfactory->Release(); }
}