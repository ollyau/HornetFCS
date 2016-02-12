#pragma once

#include <Windows.h>
#include <d2d1.h>
#include <DWrite.h>
#include <gauges.h>
#include <memory>

#include "FCS.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "dwrite.lib")

namespace Gauge
{

//-----------------------------------------------------------------------------

class D2DGeo2
{
public:
    D2DGeo2(ID2D1Factory*& srcfactory, bool open = true);
    ~D2DGeo2();
    void open_sink();
    void close_sink();
    void transform(ID2D1Factory*& srcfactory, const D2D1_MATRIX_3X2_F mat);
private:
    void transform(ID2D1Factory*& srcfactory, ID2D1TransformedGeometry*& dest_t_geo, const D2D1_MATRIX_3X2_F mat);
    ID2D1PathGeometry* m_geo;
    ID2D1GeometrySink* m_sink;
    ID2D1TransformedGeometry* m_transformedGeo;
};

//-----------------------------------------------------------------------------

extern ID2D1PathGeometry* combine_multiple_transformed_geometries(ID2D1Factory*& srcfactory, int geo_count, ID2D1TransformedGeometry* geos[]);

//-----------------------------------------------------------------------------

class D2DGauge
{
public:
    D2DGauge(FCS::FBW::Ptr const& fbw);
    ~D2DGauge();

    void SetCanvas(ELEMENT_STATIC_IMAGE *canvas);
    void Callback(GAUGEHDR *pgauge, int service_id);
private:
    static unsigned int __stdcall Draw(void* args);

    ELEMENT_STATIC_IMAGE *m_canvas;
    ID2D1Factory *m_d2dFactory;
    ID2D1DCRenderTarget *m_d2dTarget;
    IDWriteFactory *m_directWriteFactory;
    HANDLE m_drawThread;

    bool m_end;
    bool m_draw;
    int m_sizeX;
    int m_sizeY;

    ID2D1SolidColorBrush *m_solidBrushWhite;
    IDWriteTextFormat *m_textFormatSegoe;
    D2D1_RECT_F m_rectMain;

    std::weak_ptr<FCS::FBW> m_fcs;
};

//-----------------------------------------------------------------------------

} // namespace Gauge