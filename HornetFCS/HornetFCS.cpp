// HornetFCS.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include "gauges.h"
#include "SimConnect.h"

#include "FCS.h"
#include "GaugeRes.h"
#include "PIDController.h"
#include "SimConnectData.h"
#include "Utils.h"

#include <memory>
#include <string>

#ifdef DATA_GAUGE_ENABLED
#include "D2D_Classes.h"
#include "GaugeRes.h"

#include <assert.h>
#include <process.h>
#endif

HRESULT hr = NULL;
HANDLE hSimConnect = NULL;

/////////////////////////////////////////////////////////////////////////////
//  FCS Gauge Declaration
/////////////////////////////////////////////////////////////////////////////

#define		GAUGE_NAME          "FCS"
#define		GAUGEHDR_VAR_NAME   gaugehdr_FCS
#define		GAUGE_W				1

char fcs_gauge_name[] = GAUGE_NAME;
extern PELEMENT_HEADER fcs_gauge_element_list;
extern MOUSERECT fcs_gauge_mouse_rect[];

// {8E69EBE3-34E9-4E67-81FA-3BD1A00905C8}
static const GUID fcs_gauge_guid = { 0x8e69ebe3, 0x34e9, 0x4e67, { 0x81, 0xfa, 0x3b, 0xd1, 0xa0, 0x9, 0x5, 0xc8 } };

GAUGE_HEADER_FS1000(
    GAUGEHDR_VAR_NAME,
    GAUGE_W,
    fcs_gauge_name,
    &fcs_gauge_element_list,
    fcs_gauge_mouse_rect,
    0,
    0L, 0L,
    fcs_gauge_guid,
    0, 0, 0, 0, 0);

MAKE_STATIC(gaugeTransparent_image,
    BMP_BACKGROUND,
    NULL,
    NULL,
    IMAGE_CREATE_DIBSECTION | IMAGE_USE_TRANSPARENCY | IMAGE_USE_ALPHA | IMAGE_USE_ERASE | IMAGE_ERASE_ALWAYS,
    0,
    0, 0)
    PELEMENT_HEADER		fcs_gauge_element_list = &gaugeTransparent_image.header;

MOUSE_BEGIN(fcs_gauge_mouse_rect, HELP_NONE, 0, 0)
MOUSE_END

#undef GAUGE_NAME
#undef GAUGEHDR_VAR_NAME
#undef GAUGE_W

#ifdef DATA_GAUGE_ENABLED

/////////////////////////////////////////////////////////////////////////////
//  Data Gauge Declaration
/////////////////////////////////////////////////////////////////////////////

void FSAPI gcallback(GAUGEHDR* gauge, int serviceId, unsigned);

#define	GAUGE_NAME	"Data"
#define	GAUGEHDR_VAR_NAME	gaugehdr_Data
#define	GAUGE_W	1

char data_gauge_name[] = GAUGE_NAME;
extern PELEMENT_HEADER		data_gauge_element_list;
extern MOUSERECT			data_gauge_mouse_rect[];

// {FFC00355-CBCB-4840-AED5-2D8E3DFA7339}
static const GUID data_gauge_guid = { 0xffc00355, 0xcbcb, 0x4840, { 0xae, 0xd5, 0x2d, 0x8e, 0x3d, 0xfa, 0x73, 0x39 } };

GAUGE_HEADER_FS1000(
    GAUGEHDR_VAR_NAME,
    GAUGE_W,
    data_gauge_name,
    &data_gauge_element_list,
    data_gauge_mouse_rect,
    gcallback,
    0L,
    0L,
    data_gauge_guid,
    0, 0, 0, 0, 0);

MAKE_STATIC(data_gauge_background,
    BMP_BACKGROUND,
    NULL,
    NULL,
    IMAGE_CREATE_DIBSECTION | IMAGE_USE_TRANSPARENCY | IMAGE_USE_ALPHA | IMAGE_USE_ERASE | IMAGE_ERASE_ALWAYS | IMAGE_USE_BRIGHT,
    0,
    0,
    0
    )
    PELEMENT_HEADER data_gauge_element_list = &data_gauge_background.header;

MOUSE_BEGIN(data_gauge_mouse_rect, HELP_NONE, 0, 0)
MOUSE_END

#undef	GAUGE_NAME
#undef	GAUGEHDR_VAR_NAME
#undef	GAUGE_W

#endif

/////////////////////////////////////////////////////////////////////////////
// Gauge table entries
/////////////////////////////////////////////////////////////////////////////
void FSAPI module_init(void);
void FSAPI module_deinit(void);

GAUGESIMPORT ImportTable = {
    { 0x0000000F, (PPANELS)NULL },
    { 0x00000000, NULL }
};

GAUGESLINKAGE Linkage =
{
    0x00000013,
    module_init,
    module_deinit,
    0,
    0,
    FS9LINK_VERSION,
    {
        &gaugehdr_FCS,
#ifdef DATA_GAUGE_ENABLED
        &gaugehdr_Data,
#endif
        0
    }
};

/////////////////////////////////////////////////////////////////////////////
//  Gauge DLL Implementation
/////////////////////////////////////////////////////////////////////////////

FCS::FBW::Ptr fbw;

void FSAPI module_init(void)
{
#ifdef DATA_GAUGE_ENABLED
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
#endif

    fbw = std::make_shared<FCS::FBW>();

    // Instantiate SimConnect
    hr = SimConnect_Open(&hSimConnect, "Hornet FCS", NULL, 0, 0, 0);

    // Define a callback in this dll so that the simulation can be notified of SimConnect events
    if (hr == S_OK)
        hr = SimConnect_CallDispatch(hSimConnect, FCS_DispatchProcDLL, NULL);
}

void FSAPI module_deinit(void)
{
#ifdef DATA_GAUGE_ENABLED
    CoUninitialize();
#endif
    hr = SimConnect_MenuDeleteItem(hSimConnect, EVENT_MENU);
    hr = SimConnect_Close(hSimConnect);
};

#ifdef DATA_GAUGE_ENABLED

unsigned int CALLBACK d2d_gauge_drawcb(void* args)
{
    D2DGauge* gau = reinterpret_cast<D2DGauge*>(args);

    ID2D1SolidColorBrush* solidbrush_white = NULL;
    D2D1_RECT_F* rect_main = NULL;
    IDWriteTextFormat* txtfmt_segoe = NULL;

    gau->ptarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &solidbrush_white);
    rect_main = new D2D1_RECT_F(D2D1::RectF(10.0f, 10.0f, static_cast<float>(gau->sz_x) - 20.0f, static_cast<float>(gau->sz_y) - 20.0f));
    gau->pwfactory->CreateTextFormat(L"Segoe UI", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 16.0f, L"en-US", &txtfmt_segoe);
    txtfmt_segoe->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    txtfmt_segoe->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    txtfmt_segoe->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_DEFAULT, 0.00f, 0.00f);

    assert(gau);
    while (gau->end == FALSE)
    {
        if (gau->draw == TRUE)
        {
            gau->ptarget->BeginDraw();
            gau->ptarget->Clear();

            auto text = Utils::s2ws(fbw->ToString());
            gau->ptarget->DrawTextW(text.c_str(), text.size(), txtfmt_segoe, rect_main, solidbrush_white);

            gau->ptarget->EndDraw();
            SET_OFF_SCREEN(gau->canvas);
            gau->draw = FALSE;
        }
        Sleep(20);
    }

    if (solidbrush_white)
        solidbrush_white->Release();
    if (txtfmt_segoe)
        txtfmt_segoe->Release();
    delete rect_main;

    return 0;
}

void FSAPI gcallback(GAUGEHDR* gauge, int serviceId, unsigned) {
    switch (serviceId) {
    case PANEL_SERVICE_CONNECT_TO_WINDOW:
        assert(gauge->user_data == 0);
        gauge->user_data = reinterpret_cast<unsigned int>(new D2DGauge());
        break;
    case PANEL_SERVICE_DISCONNECT:
        assert(gauge->user_data);
        delete reinterpret_cast<D2DGauge*>(gauge->user_data);
        gauge->user_data = NULL;
        break;
    case PANEL_SERVICE_PRE_DRAW:
    {
        assert(gauge->user_data);
        D2DGauge* g = reinterpret_cast<D2DGauge*>(gauge->user_data);
        if (g->canvas)
        {
            if (gauge->elements_list[0])
            {
                g->draw = TRUE;
            }
        }
        break;
    }
    case PANEL_SERVICE_POST_INSTALL:
    {
        assert(gauge->user_data);
        D2DGauge* g = reinterpret_cast<D2DGauge*>(gauge->user_data);
        PELEMENT_STATIC_IMAGE element = reinterpret_cast<PELEMENT_STATIC_IMAGE>(gauge->elements_list[0]);
        g->canvas = element;
        if (element->hdc || element->hdc != INVALID_HANDLE_VALUE)
        {
            g->end = TRUE;
            if (g->drawthread != INVALID_HANDLE_VALUE)
            {
                WaitForSingleObject(g->drawthread, INFINITE);
            }

            g->sz_x = element->image_data.final->dim.x;
            g->sz_y = element->image_data.final->dim.y;

            RECT re;
            re.top = 0;
            re.left = 0;
            re.bottom = static_cast<long>(g->sz_y);
            re.right = static_cast<long>(g->sz_x);

            g->ptarget->BindDC(element->hdc, &re);
            g->end = FALSE;
            g->drawthread = (HANDLE)_beginthreadex(NULL, NULL, d2d_gauge_drawcb, g, NULL, NULL);
        }
        break;
    }
    }
}

#endif

char* SimConnectExceptionLookup(unsigned long i)
{
    switch (i)
    {
    case 0: return "SIMCONNECT_EXCEPTION_NONE";
    case 1: return "SIMCONNECT_EXCEPTION_ERROR";
    case 2: return "SIMCONNECT_EXCEPTION_SIZE_MISMATCH";
    case 3: return "SIMCONNECT_EXCEPTION_UNRECOGNIZED_ID";
    case 4: return "SIMCONNECT_EXCEPTION_UNOPENED";
    case 5: return "SIMCONNECT_EXCEPTION_VERSION_MISMATCH";
    case 6: return "SIMCONNECT_EXCEPTION_TOO_MANY_GROUPS";
    case 7: return "SIMCONNECT_EXCEPTION_NAME_UNRECOGNIZED";
    case 8: return "SIMCONNECT_EXCEPTION_TOO_MANY_EVENT_NAMES";
    case 9: return "SIMCONNECT_EXCEPTION_EVENT_ID_DUPLICATE";
    case 10: return "SIMCONNECT_EXCEPTION_TOO_MANY_MAPS";
    case 11: return "SIMCONNECT_EXCEPTION_TOO_MANY_OBJECTS";
    case 12: return "SIMCONNECT_EXCEPTION_TOO_MANY_REQUESTS";
    case 13: return "SIMCONNECT_EXCEPTION_WEATHER_INVALID_PORT";
    case 14: return "SIMCONNECT_EXCEPTION_WEATHER_INVALID_METAR";
    case 15: return "SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_GET_OBSERVATION";
    case 16: return "SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_CREATE_STATION";
    case 17: return "SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_REMOVE_STATION";
    case 18: return "SIMCONNECT_EXCEPTION_INVALID_DATA_TYPE";
    case 19: return "SIMCONNECT_EXCEPTION_INVALID_DATA_SIZE";
    case 20: return "SIMCONNECT_EXCEPTION_DATA_ERROR";
    case 21: return "SIMCONNECT_EXCEPTION_INVALID_ARRAY";
    case 22: return "SIMCONNECT_EXCEPTION_CREATE_OBJECT_FAILED";
    case 23: return "SIMCONNECT_EXCEPTION_LOAD_FLIGHTPLAN_FAILED";
    case 24: return "SIMCONNECT_EXCEPTION_OPERATION_INVALID_FOR_OJBECT_TYPE";
    case 25: return "SIMCONNECT_EXCEPTION_ILLEGAL_OPERATION";
    case 26: return "SIMCONNECT_EXCEPTION_ALREADY_SUBSCRIBED";
    case 27: return "SIMCONNECT_EXCEPTION_INVALID_ENUM";
    case 28: return "SIMCONNECT_EXCEPTION_DEFINITION_ERROR";
    case 29: return "SIMCONNECT_EXCEPTION_DUPLICATE_ID";
    case 30: return "SIMCONNECT_EXCEPTION_DATUM_ID";
    case 31: return "SIMCONNECT_EXCEPTION_OUT_OF_BOUNDS";
    case 32: return "SIMCONNECT_EXCEPTION_ALREADY_CREATED";
    case 33: return "SIMCONNECT_EXCEPTION_OBJECT_OUTSIDE_REALITY_BUBBLE";
    case 34: return "SIMCONNECT_EXCEPTION_OBJECT_CONTAINER";
    case 35: return "SIMCONNECT_EXCEPTION_OBJECT_AI";
    case 36: return "SIMCONNECT_EXCEPTION_OBJECT_ATC";
    case 37: return "SIMCONNECT_EXCEPTION_OBJECT_SCHEDULE";
    default: return "Unknown exception.";
    }
}

void DisplayText(SIMCONNECT_TEXT_TYPE textType, float durationSeconds, std::string const& text)
{
    hr = SimConnect_Text(hSimConnect, textType, durationSeconds, EVENT_DISPLAY_TEXT, text.size() + 1, (void*)text.c_str());
}

void Open()
{
#ifndef NDEBUG
    hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_RESERVED_KEY_REQUEST);
    hr = SimConnect_RequestReservedKey(hSimConnect, EVENT_RESERVED_KEY_REQUEST, "q", "a", "z");
    hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_RELOAD_USER_AIRCRAFT, "RELOAD_USER_AIRCRAFT");
#endif

    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_FLIGHT_DATA, "AIRSPEED MACH", "Mach");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_FLIGHT_DATA, "AIRSPEED TRUE", "Knots");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_FLIGHT_DATA, "INCIDENCE ALPHA", "degrees");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_FLIGHT_DATA, "INCIDENCE BETA", "degrees");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_FLIGHT_DATA, "G FORCE", "GForce");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_FLIGHT_DATA, "ROTATION VELOCITY BODY X", "degrees per second");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_FLIGHT_DATA, "ROTATION VELOCITY BODY Z", "degrees per second");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_FLIGHT_DATA, "ELEVATOR TRIM POSITION", "degrees");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_FLIGHT_DATA, "TRAILING EDGE FLAPS LEFT PERCENT", "percent over 100");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_FLIGHT_DATA, "TRAILING EDGE FLAPS RIGHT PERCENT", "percent over 100");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_FLIGHT_DATA, "LEADING EDGE FLAPS LEFT PERCENT", "percent over 100");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_FLIGHT_DATA, "LEADING EDGE FLAPS RIGHT PERCENT", "percent over 100");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_FLIGHT_DATA, "AILERON TRIM PCT", "percent over 100");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_FLIGHT_DATA, "PLANE BANK DEGREES", "degrees", SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_FLIGHT_DATA, "HYDRAULIC PRESSURE:1", "psi", SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_FLIGHT_DATA, "HYDRAULIC PRESSURE:2", "psi", SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_FLIGHT_DATA, "APU PCT RPM", "percent over 100", SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_FLIGHT_DATA, "TOTAL WEIGHT", "pounds", SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_FLIGHT_DATA, "SIM ON GROUND", "Bool", SIMCONNECT_DATATYPE_INT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_FLIGHT_DATA, "AUTOPILOT MASTER", "Bool", SIMCONNECT_DATATYPE_INT32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_FLAPS, "TRAILING EDGE FLAPS LEFT PERCENT", "percent over 100");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_FLAPS, "TRAILING EDGE FLAPS RIGHT PERCENT", "percent over 100");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_FLAPS, "LEADING EDGE FLAPS LEFT PERCENT", "percent over 100");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_FLAPS, "LEADING EDGE FLAPS RIGHT PERCENT", "percent over 100");

    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_FLAP_HANDLE, "FLAPS HANDLE INDEX", "Number", SIMCONNECT_DATATYPE_INT32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_ELEVATOR_TRIM, "ELEVATOR TRIM POSITION", "degrees");

    hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_AXIS_ELEVATOR_SET, "AXIS_ELEVATOR_SET");
    hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_AXIS_RUDDER_SET, "AXIS_RUDDER_SET");
    hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_AXIS_AILERONS_SET, "AXIS_AILERONS_SET");
    hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_AXIS_THROTTLE_SET, "AXIS_THROTTLE_SET");
    hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_AUTO_THROTTLE_ARM, "AUTO_THROTTLE_ARM");
    hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_AXIS_THROTTLE1_SET, "AXIS_THROTTLE1_SET");
    hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_AXIS_THROTTLE2_SET, "AXIS_THROTTLE2_SET");
    hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_FLIGHT_CONTROLS, EVENT_AXIS_ELEVATOR_SET, true);
    hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_FLIGHT_CONTROLS, EVENT_AXIS_RUDDER_SET, true);
    hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_FLIGHT_CONTROLS, EVENT_AXIS_AILERONS_SET, true);
    hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_FLIGHT_CONTROLS, EVENT_AXIS_THROTTLE_SET, true);
    hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_FLIGHT_CONTROLS, EVENT_AUTO_THROTTLE_ARM, true);
    hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_FLIGHT_CONTROLS, EVENT_AXIS_THROTTLE1_SET, true);
    hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_FLIGHT_CONTROLS, EVENT_AXIS_THROTTLE2_SET, true);
    hr = SimConnect_SetNotificationGroupPriority(hSimConnect, GROUP_FLIGHT_CONTROLS, SIMCONNECT_GROUP_PRIORITY_HIGHEST_MASKABLE);

    hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_THROTTLE_CUT, "THROTTLE_CUT");
    hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_THROTTLE_DECR, "THROTTLE_DECR");
    hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_THROTTLE_DECR_SMALL, "THROTTLE_DECR_SMALL");
    hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_THROTTLE_INCR, "THROTTLE_INCR");
    hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_THROTTLE_INCR_SMALL, "THROTTLE_INCR_SMALL");
    hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_THROTTLE_FULL, "THROTTLE_FULL");
    hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_INCREASE_THROTTLE, "INCREASE_THROTTLE");
    hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_DECREASE_THROTTLE, "DECREASE_THROTTLE");
    hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_THROTTLE, EVENT_THROTTLE_CUT);
    hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_THROTTLE, EVENT_THROTTLE_DECR);
    hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_THROTTLE, EVENT_THROTTLE_DECR_SMALL);
    hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_THROTTLE, EVENT_THROTTLE_INCR);
    hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_THROTTLE, EVENT_THROTTLE_INCR_SMALL);
    hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_THROTTLE, EVENT_THROTTLE_FULL);
    hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_THROTTLE, EVENT_INCREASE_THROTTLE);
    hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_THROTTLE, EVENT_DECREASE_THROTTLE);
    hr = SimConnect_SetNotificationGroupPriority(hSimConnect, GROUP_THROTTLE, SIMCONNECT_GROUP_PRIORITY_HIGHEST);

    hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_MENU);
    hr = SimConnect_MenuAddItem(hSimConnect, "HornetFCS", EVENT_MENU, 12345);
    hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_MENU, EVENT_MENU);
    hr = SimConnect_SetNotificationGroupPriority(hSimConnect, GROUP_MENU, SIMCONNECT_GROUP_PRIORITY_HIGHEST);

    hr = SimConnect_SubscribeToSystemEvent(hSimConnect, EVENT_SIM_START, "SimStart");
}

void SimStart()
{
    hr = SimConnect_RequestSystemState(hSimConnect, REQUEST_AIR_FILE, "AircraftLoaded");
}

#ifndef NDEBUG
void RecvReservedKey(SIMCONNECT_RECV_RESERVED_KEY *rkey)
{
    char buf[64];
    sprintf_s(buf, sizeof(buf), "Hornet FCS Reserved Key: %s %s", rkey->szChoiceReserved, rkey->szReservedKey);
    DisplayText(SIMCONNECT_TEXT_TYPE_PRINT_WHITE, 60.0f, buf);
}
#endif

void RecvMenuEvent(SIMCONNECT_RECV_EVENT *evt)
{
    char buf[2048];
    sprintf_s(buf, sizeof(buf), "Hornet FCS Build Timestamp: %s\r\nFCS Initialized: %s", Utils::compile_time_str().c_str(), fbw->GetCfgValid() ? "True" : "False");
    DisplayText(SIMCONNECT_TEXT_TYPE_PRINT_WHITE, 15.0f, buf);
}

void RecvException(SIMCONNECT_RECV_EXCEPTION *ex)
{
    char buf[128];
    sprintf_s(buf, sizeof(buf), "Hornet FCS SimConnect Exception %u: %s", ex->dwException, SimConnectExceptionLookup(ex->dwException));
    DisplayText(SIMCONNECT_TEXT_TYPE_PRINT_RED, 15.0f, buf);
}

void AirFileRequest(SIMCONNECT_RECV_SYSTEM_STATE *evt)
{
    if (!fbw->GetCfgValid())
    {
        auto path = std::string(evt->szString);
        path = path.substr(0, path.find_last_of('\\') + 1) + "aircraft.cfg";
        auto result = fbw->InitializeData(path);
        if (result)
        {
            hr = SimConnect_RequestSystemState(hSimConnect, REQUEST_FLIGHT_PLAN, "FlightPlan");
            hr = SimConnect_SubscribeToSystemEvent(hSimConnect, EVENT_6HZ, "6Hz");
            hr = SimConnect_SubscribeToSystemEvent(hSimConnect, EVENT_FRAME, "Frame");
            hr = SimConnect_RequestDataOnSimObject(hSimConnect, REQUEST_FLIGHT_DATA, DEFINITION_FLIGHT_DATA, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_SIM_FRAME, SIMCONNECT_DATA_REQUEST_FLAG_DEFAULT);
            hr = SimConnect_RequestDataOnSimObject(hSimConnect, REQUEST_FLAP_HANDLE, DEFINITION_FLAP_HANDLE, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_SIM_FRAME, SIMCONNECT_DATA_REQUEST_FLAG_CHANGED);
        }
        else
        {
            DisplayText(SIMCONNECT_TEXT_TYPE_PRINT_RED, 15.0f, "Hornet FCS: missing or unexpected configuration");
        }
    }
}

void ElevatorSet(SIMCONNECT_RECV_EVENT *evt)
{
    auto result = fbw->SetElevator(static_cast<long>(evt->dwData));
    if (result)
    {
        SimConnect_TransmitClientEvent(hSimConnect, SIMCONNECT_OBJECT_ID_USER, evt->uEventID, evt->dwData, SIMCONNECT_GROUP_PRIORITY_STANDARD, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
    }
}

void AileronSet(SIMCONNECT_RECV_EVENT *evt)
{
    auto result = fbw->SetAileron(static_cast<long>(evt->dwData));
    if (result)
    {
        SimConnect_TransmitClientEvent(hSimConnect, SIMCONNECT_OBJECT_ID_USER, evt->uEventID, evt->dwData, SIMCONNECT_GROUP_PRIORITY_STANDARD, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
    }
}

void RudderSet(SIMCONNECT_RECV_EVENT *evt)
{
    auto result = fbw->SetRudder(static_cast<long>(evt->dwData));
    if (result.first)
    {
        SimConnect_TransmitClientEvent(hSimConnect, SIMCONNECT_OBJECT_ID_USER, evt->uEventID, static_cast<unsigned long>(result.second), SIMCONNECT_GROUP_PRIORITY_STANDARD, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
    }
}

void ThrottleSet(SIMCONNECT_RECV_EVENT *evt, uint8_t throttleIdx = 0U)
{
    auto result = fbw->SetThrottle(static_cast<long>(evt->dwData), throttleIdx);
    if (result)
    {
        SimConnect_TransmitClientEvent(hSimConnect, SIMCONNECT_OBJECT_ID_USER, evt->uEventID, evt->dwData, SIMCONNECT_GROUP_PRIORITY_STANDARD, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
    }
}

void FlightDataRequest(SIMCONNECT_RECV_SIMOBJECT_DATA *pObjData)
{
    auto flightData = (FlightData*)&pObjData->dwData;
    if (fbw->deltaTime > 0.0f)
    {
        auto fbwState = fbw->SetState(flightData);
        auto fbwTrim = fbw->SetMode();

        if (fbwTrim.first)
        {
            SimConnect_SetDataOnSimObject(hSimConnect, DEFINITION_ELEVATOR_TRIM, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(double), &fbwTrim.second);
        }

        if (fbwState.first && fbw->GetCfgValid())
        {
            auto elevatorVal = fbw->GetCurrentElevator();
            SimConnect_TransmitClientEvent(hSimConnect, SIMCONNECT_OBJECT_ID_USER, EVENT_AXIS_ELEVATOR_SET, static_cast<DWORD>(elevatorVal), SIMCONNECT_GROUP_PRIORITY_STANDARD, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);

            if (fbwState.second)
            {
                auto rudderVal = fbw->GetCurrentRudder();
                SimConnect_TransmitClientEvent(hSimConnect, SIMCONNECT_OBJECT_ID_USER, EVENT_AXIS_RUDDER_SET, static_cast<DWORD>(rudderVal), SIMCONNECT_GROUP_PRIORITY_STANDARD, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
            }

            auto aileronVal = fbw->GetCurrentAileron();
            SimConnect_TransmitClientEvent(hSimConnect, SIMCONNECT_OBJECT_ID_USER, EVENT_AXIS_AILERONS_SET, static_cast<DWORD>(aileronVal), SIMCONNECT_GROUP_PRIORITY_STANDARD, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
        }

        auto pFlaps = fbw->GetCurrentFlaps();
        SimConnect_SetDataOnSimObject(hSimConnect, DEFINITION_FLAPS, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(Flaps), pFlaps.get());

        auto autoThrottle = fbw->SetAutoThrottle();
        if (autoThrottle.first)
        {
            SimConnect_TransmitClientEvent(hSimConnect, SIMCONNECT_OBJECT_ID_USER, EVENT_AXIS_THROTTLE_SET, static_cast<DWORD>(autoThrottle.second), SIMCONNECT_GROUP_PRIORITY_STANDARD, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
        }
    }
}

void CALLBACK FCS_DispatchProcDLL(SIMCONNECT_RECV* pData, DWORD cbData, void *pContext)
{
    switch (pData->dwID)
    {
#ifndef NDEBUG
    case SIMCONNECT_RECV_ID_RESERVED_KEY:
    {
        SIMCONNECT_RECV_RESERVED_KEY *rkey = (SIMCONNECT_RECV_RESERVED_KEY*)pData;
        RecvReservedKey(rkey);
        break;
    }
#endif
    case SIMCONNECT_RECV_ID_EVENT:
    {
        SIMCONNECT_RECV_EVENT *evt = (SIMCONNECT_RECV_EVENT*)pData;
        switch (evt->uEventID)
        {
#ifndef NDEBUG
        case EVENT_RESERVED_KEY_REQUEST:
            SimConnect_TransmitClientEvent(hSimConnect, SIMCONNECT_OBJECT_ID_USER, EVENT_RELOAD_USER_AIRCRAFT, 1, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
            break;
#endif
        case EVENT_SIM_START:
            SimStart();
            break;
        case EVENT_6HZ:
            fbw->Update6Hz();
            break;
        case EVENT_AXIS_ELEVATOR_SET:
            ElevatorSet(evt);
            return;
        case EVENT_AXIS_AILERONS_SET:
            AileronSet(evt);
            return;
        case EVENT_AXIS_RUDDER_SET:
            RudderSet(evt);
            return;
        case EVENT_AXIS_THROTTLE_SET:
            ThrottleSet(evt);
            return;
        case EVENT_AXIS_THROTTLE1_SET:
            ThrottleSet(evt, 1U);
            return;
        case EVENT_AXIS_THROTTLE2_SET:
            ThrottleSet(evt, 2U);
            return;
        case EVENT_AUTO_THROTTLE_ARM:
            fbw->ToggleAutoThrottle();
            return;
        case EVENT_THROTTLE_CUT:
        case EVENT_THROTTLE_DECR:
        case EVENT_THROTTLE_DECR_SMALL:
        case EVENT_THROTTLE_INCR:
        case EVENT_THROTTLE_INCR_SMALL:
        case EVENT_THROTTLE_FULL:
        case EVENT_INCREASE_THROTTLE:
        case EVENT_DECREASE_THROTTLE:
            fbw->DisableAutoThrottle();
            return;
        case EVENT_MENU:
            RecvMenuEvent(evt);
            return;
            //default:
            //    //SimConnect_TransmitClientEvent(hSimConnect, SIMCONNECT_OBJECT_ID_USER, evt->uEventID, evt->dwData, SIMCONNECT_GROUP_PRIORITY_STANDARD, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
            //    break;
        }
        break;
    }
    case SIMCONNECT_RECV_ID_SYSTEM_STATE:
    {
        SIMCONNECT_RECV_SYSTEM_STATE *evt = (SIMCONNECT_RECV_SYSTEM_STATE*)pData;
        switch (evt->dwRequestID)
        {
        case REQUEST_AIR_FILE:
            AirFileRequest(evt);
            break;
        case REQUEST_FLIGHT_PLAN:
            if (evt->szString[0] != '\0')
            {
                hr = SimConnect_FlightPlanLoad(hSimConnect, evt->szString);
            }
            break;
        }
        break;
    }
    case SIMCONNECT_RECV_ID_EVENT_FRAME:
    {
        SIMCONNECT_RECV_EVENT_FRAME *evt = (SIMCONNECT_RECV_EVENT_FRAME*)pData;
        switch (evt->uEventID)
        {
        case EVENT_FRAME:
            if (evt->fFrameRate > 0.0f)
            {
                fbw->deltaTime = 1.0 / static_cast<double>(evt->fFrameRate);
            }
            break;
        }
        break;
    }
    case SIMCONNECT_RECV_ID_SIMOBJECT_DATA:
    {
        SIMCONNECT_RECV_SIMOBJECT_DATA *pObjData = (SIMCONNECT_RECV_SIMOBJECT_DATA*)pData;
        switch (pObjData->dwRequestID)
        {
        case REQUEST_FLAP_HANDLE:
            fbw->SetFlapSelection(static_cast<int>(pObjData->dwData));
            break;
        case REQUEST_FLIGHT_DATA:
            FlightDataRequest(pObjData);
            break;
        }
        break;
    }
    case SIMCONNECT_RECV_ID_OPEN:
    {
        Open();
        break;
    }
    case SIMCONNECT_RECV_ID_EXCEPTION:
    {
        SIMCONNECT_RECV_EXCEPTION *ex = (SIMCONNECT_RECV_EXCEPTION*)pData;
        RecvException(ex);
        break;
    }
    }
}