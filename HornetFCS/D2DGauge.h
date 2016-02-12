#pragma once

#include <d2d1.h>
#include <DWrite.h>
#include <gauges.h>
#include <memory>
#include <Windows.h>

#include "FCS.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

namespace Gauge
{

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

    ID2D1Factory *m_d2dFactory;
    ID2D1DCRenderTarget *m_d2dTarget;
    IDWriteFactory *m_directWriteFactory;

    HANDLE m_drawThread;
    bool m_end;
    bool m_draw;

    ELEMENT_STATIC_IMAGE *m_canvas;
    int m_sizeX;
    int m_sizeY;

    std::weak_ptr<FCS::FBW> m_fcs;
};

//-----------------------------------------------------------------------------

} // namespace Gauge