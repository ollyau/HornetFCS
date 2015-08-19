#pragma once

#include <Windows.h>
#include <d2d1.h>
#include <DWrite.h>
#include "gauges.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "dwrite.lib")

class D2DGeo2
{
public:
    //Initializes a D2D1Geo1 class and automatically opens the ID2D1GeometrySink
    D2DGeo2(ID2D1Factory*& srcfactory);
    //Initializes a D2D1Geo class and allows you to decide if it opens the ID2D1GeometrySink
    D2DGeo2(ID2D1Factory*& srcfactory, bool auto_open);
    //Opens the ID2D1GeometrySink
    void open_sink();
    //Closes the ID2D1GeometrySink
    void close_sink();
    //Applies a transformation to the geometry and stores the transformed geometry in t_geo, releasing the previously stored transformed geometry, if any
    void transform(ID2D1Factory*& srcfactory, const D2D1_MATRIX_3X2_F mat);
    //Applies a transformation to the geometry and stores the value in the specified ID2D1TransformedGeometry
    void transform(ID2D1Factory*& srcfactory, ID2D1TransformedGeometry*& dest_t_geo, const D2D1_MATRIX_3X2_F mat);
    ~D2DGeo2();
    ID2D1PathGeometry* geo;
    ID2D1GeometrySink* sink;
    ID2D1TransformedGeometry* t_geo;
};

extern ID2D1PathGeometry* combine_multiple_transformed_geometries(ID2D1Factory*& srcfactory, int geo_count, ID2D1TransformedGeometry* geos[]);

class D2DGauge
{
public:
    PELEMENT_STATIC_IMAGE canvas;
    double sz_x;
    double sz_y;
    HANDLE drawthread;
    ID2D1Factory* pfactory;
    ID2D1DCRenderTarget* ptarget;
    IDWriteFactory* pwfactory;
    //unique_ptr<Gdiplus::Graphics> gfx;
    volatile short end;
    volatile short draw;
    short wgauge;
    D2DGauge();
    ~D2DGauge();
};