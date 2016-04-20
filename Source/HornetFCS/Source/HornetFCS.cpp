/**********************************************************
*                                                         *
*   Copyright (C) Orion Lyau 2016. All Rights Reserved.   *
*                                                         *
**********************************************************/

#include <gauges.h>
#include <memory>
#include <SimConnect.h>
#include <sstream>
#include <string>

#include "../Resources/GaugeRes.h"
#include "FCS.h"
#include "PIDController.h"
#include "SimConnectData.h"
#include "Utils.h"

#ifdef DATA_GAUGE_ENABLED
#include "D2DGauge.h"
#endif

namespace FCS
{

//-----------------------------------------------------------------------------

HANDLE g_simConnect = nullptr;
FCS::FBW::Ptr g_fbw = nullptr;

//-----------------------------------------------------------------------------

typedef double(__stdcall *GetElapsedSimTimeSec)();
GetElapsedSimTimeSec g_getSimTime = nullptr;
double g_currentSimTime = 0.0;
double g_previousSimTime = 0.0;

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

void DisplayText(SIMCONNECT_TEXT_TYPE textType, float durationSeconds, std::string const& text)
{
    SimConnect_Text(g_simConnect, textType, durationSeconds, EVENT_DISPLAY_TEXT, text.size() + 1, (void*)text.c_str());
}

//-----------------------------------------------------------------------------

void Open(SIMCONNECT_RECV_OPEN *data)
{
    if (!g_getSimTime)
    {
        HMODULE simschedulerDll = GetModuleHandle(L"simscheduler.dll");
        if (simschedulerDll)
        {
            if (data->dwApplicationVersionMajor == 10UL)
            {
                if (data->dwApplicationBuildMajor == 61472UL)
                {
                    g_getSimTime = (GetElapsedSimTimeSec)GetProcAddress(simschedulerDll, MAKEINTRESOURCEA(4));
                }
                else
                {
                    g_getSimTime = (GetElapsedSimTimeSec)GetProcAddress(simschedulerDll, MAKEINTRESOURCEA(6));
                }
            }
            else if (data->dwApplicationVersionMajor == 3UL || (data->dwApplicationVersionMajor == 2UL && data->dwApplicationVersionMinor == 5UL))
            {
                g_getSimTime = (GetElapsedSimTimeSec)GetProcAddress(simschedulerDll, "?GetElapsedSimTimeSec@@YGNXZ");
            }
        }
    }

#ifndef NDEBUG
    SimConnect_MapClientEventToSimEvent(g_simConnect, EVENT_RESERVED_KEY_REQUEST);
    SimConnect_RequestReservedKey(g_simConnect, EVENT_RESERVED_KEY_REQUEST, "q", "a", "z");
    SimConnect_MapClientEventToSimEvent(g_simConnect, EVENT_RELOAD_USER_AIRCRAFT, "RELOAD_USER_AIRCRAFT");
#endif

    SimConnect_AddToDataDefinition(g_simConnect, DEFINITION_FLIGHT_DATA, "AIRSPEED MACH", "Mach");
    SimConnect_AddToDataDefinition(g_simConnect, DEFINITION_FLIGHT_DATA, "AIRSPEED TRUE", "Knots");
    SimConnect_AddToDataDefinition(g_simConnect, DEFINITION_FLIGHT_DATA, "INCIDENCE ALPHA", "degrees");
    SimConnect_AddToDataDefinition(g_simConnect, DEFINITION_FLIGHT_DATA, "INCIDENCE BETA", "degrees");
    SimConnect_AddToDataDefinition(g_simConnect, DEFINITION_FLIGHT_DATA, "G FORCE", "GForce");
    SimConnect_AddToDataDefinition(g_simConnect, DEFINITION_FLIGHT_DATA, "ROTATION VELOCITY BODY X", "degrees per second");
    SimConnect_AddToDataDefinition(g_simConnect, DEFINITION_FLIGHT_DATA, "ROTATION VELOCITY BODY Z", "degrees per second");
    SimConnect_AddToDataDefinition(g_simConnect, DEFINITION_FLIGHT_DATA, "ELEVATOR TRIM POSITION", "degrees");
    SimConnect_AddToDataDefinition(g_simConnect, DEFINITION_FLIGHT_DATA, "TRAILING EDGE FLAPS LEFT PERCENT", "percent over 100");
    SimConnect_AddToDataDefinition(g_simConnect, DEFINITION_FLIGHT_DATA, "TRAILING EDGE FLAPS RIGHT PERCENT", "percent over 100");
    SimConnect_AddToDataDefinition(g_simConnect, DEFINITION_FLIGHT_DATA, "LEADING EDGE FLAPS LEFT PERCENT", "percent over 100");
    SimConnect_AddToDataDefinition(g_simConnect, DEFINITION_FLIGHT_DATA, "LEADING EDGE FLAPS RIGHT PERCENT", "percent over 100");
    SimConnect_AddToDataDefinition(g_simConnect, DEFINITION_FLIGHT_DATA, "AILERON TRIM PCT", "percent over 100");
    SimConnect_AddToDataDefinition(g_simConnect, DEFINITION_FLIGHT_DATA, "PLANE BANK DEGREES", "degrees", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(g_simConnect, DEFINITION_FLIGHT_DATA, "HYDRAULIC PRESSURE:1", "psi", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(g_simConnect, DEFINITION_FLIGHT_DATA, "HYDRAULIC PRESSURE:2", "psi", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(g_simConnect, DEFINITION_FLIGHT_DATA, "APU PCT RPM", "percent over 100", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(g_simConnect, DEFINITION_FLIGHT_DATA, "TOTAL WEIGHT", "pounds", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(g_simConnect, DEFINITION_FLIGHT_DATA, "SIM ON GROUND", "Bool", SIMCONNECT_DATATYPE_INT32);
    SimConnect_AddToDataDefinition(g_simConnect, DEFINITION_FLIGHT_DATA, "AUTOPILOT MASTER", "Bool", SIMCONNECT_DATATYPE_INT32);

    SimConnect_AddToDataDefinition(g_simConnect, DEFINITION_FLAPS, "TRAILING EDGE FLAPS LEFT PERCENT", "percent over 100");
    SimConnect_AddToDataDefinition(g_simConnect, DEFINITION_FLAPS, "TRAILING EDGE FLAPS RIGHT PERCENT", "percent over 100");
    SimConnect_AddToDataDefinition(g_simConnect, DEFINITION_FLAPS, "LEADING EDGE FLAPS LEFT PERCENT", "percent over 100");
    SimConnect_AddToDataDefinition(g_simConnect, DEFINITION_FLAPS, "LEADING EDGE FLAPS RIGHT PERCENT", "percent over 100");

    SimConnect_AddToDataDefinition(g_simConnect, DEFINITION_FLAP_HANDLE, "FLAPS HANDLE INDEX", "Number", SIMCONNECT_DATATYPE_INT32);

    SimConnect_AddToDataDefinition(g_simConnect, DEFINITION_ELEVATOR_TRIM, "ELEVATOR TRIM POSITION", "degrees");

    SimConnect_MapClientEventToSimEvent(g_simConnect, EVENT_AXIS_ELEVATOR_SET, "AXIS_ELEVATOR_SET");
    SimConnect_MapClientEventToSimEvent(g_simConnect, EVENT_AXIS_RUDDER_SET, "AXIS_RUDDER_SET");
    SimConnect_MapClientEventToSimEvent(g_simConnect, EVENT_AXIS_AILERONS_SET, "AXIS_AILERONS_SET");
    SimConnect_MapClientEventToSimEvent(g_simConnect, EVENT_AXIS_THROTTLE_SET, "AXIS_THROTTLE_SET");
    SimConnect_MapClientEventToSimEvent(g_simConnect, EVENT_AUTO_THROTTLE_ARM, "AUTO_THROTTLE_ARM");
    SimConnect_MapClientEventToSimEvent(g_simConnect, EVENT_AXIS_THROTTLE1_SET, "AXIS_THROTTLE1_SET");
    SimConnect_MapClientEventToSimEvent(g_simConnect, EVENT_AXIS_THROTTLE2_SET, "AXIS_THROTTLE2_SET");
    SimConnect_AddClientEventToNotificationGroup(g_simConnect, GROUP_FLIGHT_CONTROLS, EVENT_AXIS_ELEVATOR_SET, true);
    SimConnect_AddClientEventToNotificationGroup(g_simConnect, GROUP_FLIGHT_CONTROLS, EVENT_AXIS_RUDDER_SET, true);
    SimConnect_AddClientEventToNotificationGroup(g_simConnect, GROUP_FLIGHT_CONTROLS, EVENT_AXIS_AILERONS_SET, true);
    SimConnect_AddClientEventToNotificationGroup(g_simConnect, GROUP_FLIGHT_CONTROLS, EVENT_AXIS_THROTTLE_SET, true);
    SimConnect_AddClientEventToNotificationGroup(g_simConnect, GROUP_FLIGHT_CONTROLS, EVENT_AUTO_THROTTLE_ARM, true);
    SimConnect_AddClientEventToNotificationGroup(g_simConnect, GROUP_FLIGHT_CONTROLS, EVENT_AXIS_THROTTLE1_SET, true);
    SimConnect_AddClientEventToNotificationGroup(g_simConnect, GROUP_FLIGHT_CONTROLS, EVENT_AXIS_THROTTLE2_SET, true);
    SimConnect_SetNotificationGroupPriority(g_simConnect, GROUP_FLIGHT_CONTROLS, SIMCONNECT_GROUP_PRIORITY_HIGHEST_MASKABLE);

    SimConnect_MapClientEventToSimEvent(g_simConnect, EVENT_THROTTLE_CUT, "THROTTLE_CUT");
    SimConnect_MapClientEventToSimEvent(g_simConnect, EVENT_THROTTLE_DECR, "THROTTLE_DECR");
    SimConnect_MapClientEventToSimEvent(g_simConnect, EVENT_THROTTLE_DECR_SMALL, "THROTTLE_DECR_SMALL");
    SimConnect_MapClientEventToSimEvent(g_simConnect, EVENT_THROTTLE_INCR, "THROTTLE_INCR");
    SimConnect_MapClientEventToSimEvent(g_simConnect, EVENT_THROTTLE_INCR_SMALL, "THROTTLE_INCR_SMALL");
    SimConnect_MapClientEventToSimEvent(g_simConnect, EVENT_THROTTLE_FULL, "THROTTLE_FULL");
    SimConnect_MapClientEventToSimEvent(g_simConnect, EVENT_INCREASE_THROTTLE, "INCREASE_THROTTLE");
    SimConnect_MapClientEventToSimEvent(g_simConnect, EVENT_DECREASE_THROTTLE, "DECREASE_THROTTLE");
    SimConnect_AddClientEventToNotificationGroup(g_simConnect, GROUP_THROTTLE, EVENT_THROTTLE_CUT);
    SimConnect_AddClientEventToNotificationGroup(g_simConnect, GROUP_THROTTLE, EVENT_THROTTLE_DECR);
    SimConnect_AddClientEventToNotificationGroup(g_simConnect, GROUP_THROTTLE, EVENT_THROTTLE_DECR_SMALL);
    SimConnect_AddClientEventToNotificationGroup(g_simConnect, GROUP_THROTTLE, EVENT_THROTTLE_INCR);
    SimConnect_AddClientEventToNotificationGroup(g_simConnect, GROUP_THROTTLE, EVENT_THROTTLE_INCR_SMALL);
    SimConnect_AddClientEventToNotificationGroup(g_simConnect, GROUP_THROTTLE, EVENT_THROTTLE_FULL);
    SimConnect_AddClientEventToNotificationGroup(g_simConnect, GROUP_THROTTLE, EVENT_INCREASE_THROTTLE);
    SimConnect_AddClientEventToNotificationGroup(g_simConnect, GROUP_THROTTLE, EVENT_DECREASE_THROTTLE);
    SimConnect_SetNotificationGroupPriority(g_simConnect, GROUP_THROTTLE, SIMCONNECT_GROUP_PRIORITY_HIGHEST);

    SimConnect_MapClientEventToSimEvent(g_simConnect, EVENT_MENU);
    SimConnect_MenuAddItem(g_simConnect, "HornetFCS", EVENT_MENU, 12345);
    SimConnect_AddClientEventToNotificationGroup(g_simConnect, GROUP_MENU, EVENT_MENU);
    SimConnect_SetNotificationGroupPriority(g_simConnect, GROUP_MENU, SIMCONNECT_GROUP_PRIORITY_HIGHEST);

    SimConnect_SubscribeToSystemEvent(g_simConnect, EVENT_SIM_START, "SimStart");
}

//-----------------------------------------------------------------------------

void SimStart()
{
    SimConnect_RequestSystemState(g_simConnect, REQUEST_AIR_FILE, "AircraftLoaded");
}

//-----------------------------------------------------------------------------

#ifndef NDEBUG
void RecvReservedKey(SIMCONNECT_RECV_RESERVED_KEY *rkey)
{
    char buf[64];
    sprintf_s(buf, sizeof(buf), "Hornet FCS Reserved Key: %s %s", rkey->szChoiceReserved, rkey->szReservedKey);
    DisplayText(SIMCONNECT_TEXT_TYPE_PRINT_WHITE, 60.0f, buf);
}
#endif

//-----------------------------------------------------------------------------

void RecvMenuEvent(SIMCONNECT_RECV_EVENT *evt)
{
    std::ostringstream ss;
    ss << "Hornet FCS Build Timestamp: " << Utils::compile_time_str() << std::endl;
    ss << "CFG Path: " << g_fbw->GetCfgPath() << std::endl;
    ss << "FCS Initialized: " << (g_fbw->GetCfgValid() ? "True" : "False") << std::endl;
    DisplayText(SIMCONNECT_TEXT_TYPE_PRINT_WHITE, 15.0f, ss.str());
}

//-----------------------------------------------------------------------------

void RecvException(SIMCONNECT_RECV_EXCEPTION *ex)
{
    char buf[128];
    sprintf_s(buf, sizeof(buf), "Hornet FCS SimConnect Exception %u: %s", ex->dwException, SimConnectExceptionLookup(ex->dwException));
    DisplayText(SIMCONNECT_TEXT_TYPE_PRINT_RED, 15.0f, buf);
}

//-----------------------------------------------------------------------------

void AirFileRequest(SIMCONNECT_RECV_SYSTEM_STATE *evt)
{
    if (!g_fbw->GetCfgValid())
    {
        auto path = std::string(evt->szString);
        path = path.substr(0, path.find_last_of('\\') + 1) + "aircraft.cfg";
        auto result = g_fbw->InitializeData(path);
        if (result)
        {
            SimConnect_RequestSystemState(g_simConnect, REQUEST_FLIGHT_PLAN, "FlightPlan");
            SimConnect_SubscribeToSystemEvent(g_simConnect, EVENT_6HZ, "6Hz");
            if (!g_getSimTime)
            {
                SimConnect_SubscribeToSystemEvent(g_simConnect, EVENT_FRAME, "Frame");
            }
            SimConnect_RequestDataOnSimObject(g_simConnect, REQUEST_FLIGHT_DATA, DEFINITION_FLIGHT_DATA, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_SIM_FRAME, SIMCONNECT_DATA_REQUEST_FLAG_DEFAULT);
            SimConnect_RequestDataOnSimObject(g_simConnect, REQUEST_FLAP_HANDLE, DEFINITION_FLAP_HANDLE, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_SIM_FRAME, SIMCONNECT_DATA_REQUEST_FLAG_CHANGED);
        }
        else
        {
            DisplayText(SIMCONNECT_TEXT_TYPE_PRINT_RED, 15.0f, "Hornet FCS: missing or unexpected configuration");
        }
    }
}

//-----------------------------------------------------------------------------

void ElevatorSet(SIMCONNECT_RECV_EVENT *evt)
{
    auto result = g_fbw->SetElevator(static_cast<long>(evt->dwData));
    if (result)
    {
        SimConnect_TransmitClientEvent(g_simConnect, SIMCONNECT_OBJECT_ID_USER, evt->uEventID, evt->dwData, SIMCONNECT_GROUP_PRIORITY_STANDARD, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
    }
}

//-----------------------------------------------------------------------------

void AileronSet(SIMCONNECT_RECV_EVENT *evt)
{
    auto result = g_fbw->SetAileron(static_cast<long>(evt->dwData));
    if (result)
    {
        SimConnect_TransmitClientEvent(g_simConnect, SIMCONNECT_OBJECT_ID_USER, evt->uEventID, evt->dwData, SIMCONNECT_GROUP_PRIORITY_STANDARD, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
    }
}

//-----------------------------------------------------------------------------

void RudderSet(SIMCONNECT_RECV_EVENT *evt)
{
    auto result = g_fbw->SetRudder(static_cast<long>(evt->dwData));
    if (result.first)
    {
        SimConnect_TransmitClientEvent(g_simConnect, SIMCONNECT_OBJECT_ID_USER, evt->uEventID, static_cast<unsigned long>(result.second), SIMCONNECT_GROUP_PRIORITY_STANDARD, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
    }
}

//-----------------------------------------------------------------------------

void ThrottleSet(SIMCONNECT_RECV_EVENT *evt, uint8_t throttleIdx = 0U)
{
    auto result = g_fbw->SetThrottle(static_cast<long>(evt->dwData), throttleIdx);
    if (result)
    {
        SimConnect_TransmitClientEvent(g_simConnect, SIMCONNECT_OBJECT_ID_USER, evt->uEventID, evt->dwData, SIMCONNECT_GROUP_PRIORITY_STANDARD, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
    }
}

//-----------------------------------------------------------------------------

void FlightDataRequest(SIMCONNECT_RECV_SIMOBJECT_DATA *pObjData)
{
    auto flightData = (FlightData*)&pObjData->dwData;
    if (g_getSimTime)
    {
        g_previousSimTime = g_currentSimTime == 0.0 ? g_getSimTime() : g_currentSimTime;
        g_currentSimTime = g_getSimTime();
        g_fbw->deltaTime = g_currentSimTime - g_previousSimTime;
    }
    if (g_fbw->deltaTime > 0.0)
    {
        auto fbwState = g_fbw->SetState(flightData);
        auto fbwTrim = g_fbw->SetMode();

        if (fbwTrim.first)
        {
            SimConnect_SetDataOnSimObject(g_simConnect, DEFINITION_ELEVATOR_TRIM, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(double), &fbwTrim.second);
        }

        if (fbwState.first && g_fbw->GetCfgValid())
        {
            auto elevatorVal = g_fbw->GetCurrentElevator();
            SimConnect_TransmitClientEvent(g_simConnect, SIMCONNECT_OBJECT_ID_USER, EVENT_AXIS_ELEVATOR_SET, static_cast<DWORD>(elevatorVal), SIMCONNECT_GROUP_PRIORITY_STANDARD, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);

            if (fbwState.second)
            {
                auto rudderVal = g_fbw->GetCurrentRudder();
                SimConnect_TransmitClientEvent(g_simConnect, SIMCONNECT_OBJECT_ID_USER, EVENT_AXIS_RUDDER_SET, static_cast<DWORD>(rudderVal), SIMCONNECT_GROUP_PRIORITY_STANDARD, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
            }

            auto aileronVal = g_fbw->GetCurrentAileron();
            SimConnect_TransmitClientEvent(g_simConnect, SIMCONNECT_OBJECT_ID_USER, EVENT_AXIS_AILERONS_SET, static_cast<DWORD>(aileronVal), SIMCONNECT_GROUP_PRIORITY_STANDARD, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
        }

        auto pFlaps = g_fbw->GetCurrentFlaps();
        SimConnect_SetDataOnSimObject(g_simConnect, DEFINITION_FLAPS, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(Flaps), pFlaps.get());

        auto autoThrottle = g_fbw->SetAutoThrottle();
        if (autoThrottle.first)
        {
            SimConnect_TransmitClientEvent(g_simConnect, SIMCONNECT_OBJECT_ID_USER, EVENT_AXIS_THROTTLE_SET, static_cast<DWORD>(autoThrottle.second), SIMCONNECT_GROUP_PRIORITY_STANDARD, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
        }
    }
}

//-----------------------------------------------------------------------------

void CALLBACK FCS_DispatchProcDLL(SIMCONNECT_RECV *pData, DWORD cbData, void *pContext)
{
    switch (pData->dwID)
    {
#ifndef NDEBUG
    case SIMCONNECT_RECV_ID_RESERVED_KEY:
    {
        RecvReservedKey((SIMCONNECT_RECV_RESERVED_KEY*)pData);
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
            SimConnect_TransmitClientEvent(g_simConnect, SIMCONNECT_OBJECT_ID_USER, EVENT_RELOAD_USER_AIRCRAFT, 1, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
            break;
#endif
        case EVENT_SIM_START:
            SimStart();
            break;
        case EVENT_6HZ:
            g_fbw->Update6Hz();
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
            g_fbw->ToggleAutoThrottle();
            return;
        case EVENT_THROTTLE_CUT:
        case EVENT_THROTTLE_DECR:
        case EVENT_THROTTLE_DECR_SMALL:
        case EVENT_THROTTLE_INCR:
        case EVENT_THROTTLE_INCR_SMALL:
        case EVENT_THROTTLE_FULL:
        case EVENT_INCREASE_THROTTLE:
        case EVENT_DECREASE_THROTTLE:
            g_fbw->DisableAutoThrottle();
            return;
        case EVENT_MENU:
            RecvMenuEvent(evt);
            return;
            //default:
            //    //SimConnect_TransmitClientEvent(g_simConnect, SIMCONNECT_OBJECT_ID_USER, evt->uEventID, evt->dwData, SIMCONNECT_GROUP_PRIORITY_STANDARD, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
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
                SimConnect_FlightPlanLoad(g_simConnect, evt->szString);
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
                g_fbw->deltaTime = 1.0 / static_cast<double>(evt->fFrameRate);
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
            g_fbw->SetFlapSelection(static_cast<int>(pObjData->dwData));
            break;
        case REQUEST_FLIGHT_DATA:
            FlightDataRequest(pObjData);
            break;
        }
        break;
    }
    case SIMCONNECT_RECV_ID_OPEN:
    {
        Open((SIMCONNECT_RECV_OPEN*)pData);
        break;
    }
    case SIMCONNECT_RECV_ID_EXCEPTION:
    {
        RecvException((SIMCONNECT_RECV_EXCEPTION*)pData);
        break;
    }
    }
}

} // namespace FCS

//-----------------------------------------------------------------------------

#ifdef DATA_GAUGE_ENABLED
template <class T>
void __stdcall GaugeCallback(GAUGEHDR *pgauge, int service_id, unsigned int extra_data)
{
    switch (service_id)
    {
    case PANEL_SERVICE_CONNECT_TO_WINDOW:
        // when the panel service starts, instantiate T and store pointer to T in user_data
        assert(pgauge->user_data == 0U);
        pgauge->user_data = reinterpret_cast<unsigned int>(new T(FCS::g_fbw));
        reinterpret_cast<T*>(pgauge->user_data)->SetCanvas(reinterpret_cast<PELEMENT_STATIC_IMAGE>(pgauge->elements_list[0]));
        break;
    case PANEL_SERVICE_DISCONNECT:
        // when the panel service ends, we destroy the C++ gauge object freeing the heap.
        assert(pgauge->user_data);
        delete reinterpret_cast<T*>(pgauge->user_data);
        pgauge->user_data = 0U;
        break;
    default:
        // in all other cases, we use the C++ pointer to jump to the generic Callback function of
        // the framework that will then call the appropriate Virtual function defined in the actual gauges.
        assert(pgauge->user_data);
        reinterpret_cast<T*>(pgauge->user_data)->Callback(pgauge, service_id);
        break;
    }
}
#endif

//-----------------------------------------------------------------------------

#define		GAUGE_NAME          "FCS"
#define		GAUGEHDR_VAR_NAME   gaugehdr_FCS
#define		GAUGE_W				1

char fcs_gauge_name[] = GAUGE_NAME;
extern PELEMENT_HEADER fcs_gauge_element_list;
extern MOUSERECT fcs_gauge_mouse_rect[];

// {8E69EBE3-34E9-4E67-81FA-3BD1A00905C8}
static const GUID fcs_gauge_guid = { 0x8e69ebe3, 0x34e9, 0x4e67,{ 0x81, 0xfa, 0x3b, 0xd1, 0xa0, 0x9, 0x5, 0xc8 } };

GAUGE_HEADER_FS1000(GAUGEHDR_VAR_NAME, GAUGE_W, fcs_gauge_name, &fcs_gauge_element_list, fcs_gauge_mouse_rect, 0, 0L, 0L, fcs_gauge_guid, 0, 0, 0, 0, 0);
MAKE_STATIC(gaugeTransparent_image, BMP_BACKGROUND, NULL, NULL, IMAGE_CREATE_DIBSECTION | IMAGE_USE_TRANSPARENCY | IMAGE_USE_ALPHA | IMAGE_USE_ERASE | IMAGE_ERASE_ALWAYS, 0, 0, 0)
PELEMENT_HEADER fcs_gauge_element_list = &gaugeTransparent_image.header;

MOUSE_BEGIN(fcs_gauge_mouse_rect, HELP_NONE, 0, 0)
MOUSE_END

#undef GAUGE_NAME
#undef GAUGEHDR_VAR_NAME
#undef GAUGE_W

//-----------------------------------------------------------------------------

#ifdef DATA_GAUGE_ENABLED

#define	GAUGE_NAME	"Data"
#define	GAUGEHDR_VAR_NAME	gaugehdr_Data
#define	GAUGE_W	1

char data_gauge_name[] = GAUGE_NAME;
extern PELEMENT_HEADER		data_gauge_element_list;
extern MOUSERECT			data_gauge_mouse_rect[];

// {FFC00355-CBCB-4840-AED5-2D8E3DFA7339}
static const GUID data_gauge_guid = { 0xffc00355, 0xcbcb, 0x4840,{ 0xae, 0xd5, 0x2d, 0x8e, 0x3d, 0xfa, 0x73, 0x39 } };

GAUGE_HEADER_FS1000(GAUGEHDR_VAR_NAME, GAUGE_W, data_gauge_name, &data_gauge_element_list, data_gauge_mouse_rect, GaugeCallback<Gauge::D2DGauge>, 0L, 0L, data_gauge_guid, 0, 0, 0, 0, 0);

MAKE_STATIC(data_gauge_background, BMP_BACKGROUND, NULL, NULL, IMAGE_CREATE_DIBSECTION | IMAGE_USE_TRANSPARENCY | IMAGE_USE_ALPHA | IMAGE_USE_ERASE | IMAGE_ERASE_ALWAYS | IMAGE_USE_BRIGHT, 0, 0, 0)
PELEMENT_HEADER data_gauge_element_list = &data_gauge_background.header;

MOUSE_BEGIN(data_gauge_mouse_rect, HELP_NONE, 0, 0)
MOUSE_END

#undef	GAUGE_NAME
#undef	GAUGEHDR_VAR_NAME
#undef	GAUGE_W

#endif

//-----------------------------------------------------------------------------

GAUGESIMPORT ImportTable = {
    { 0x0000000F, (PPANELS)nullptr },
    { 0x00000000, nullptr }
};

//-----------------------------------------------------------------------------

void FSAPI module_init(void)
{
#ifdef DATA_GAUGE_ENABLED
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif

    FCS::g_fbw = std::make_shared<FCS::FBW>();

    // Instantiate SimConnect
    auto hr = SimConnect_Open(&FCS::g_simConnect, "Hornet FCS", nullptr, 0, 0, 0);

    // Define a callback in this dll so that the simulation can be notified of SimConnect events
    if (hr == S_OK)
    {
        SimConnect_CallDispatch(FCS::g_simConnect, FCS::FCS_DispatchProcDLL, nullptr);
    }
}

//-----------------------------------------------------------------------------

void FSAPI module_deinit(void)
{
#ifdef DATA_GAUGE_ENABLED
    CoUninitialize();
#endif
    SimConnect_MenuDeleteItem(FCS::g_simConnect, FCS::EVENT_MENU);
    SimConnect_Close(FCS::g_simConnect);
}

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------