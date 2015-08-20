#pragma once

static enum EVENT_ID
{
    EVENT_SIM_START,
    EVENT_DISPLAY_TEXT,
    EVENT_FRAME,
    EVENT_6HZ,
    EVENT_AXIS_ELEVATOR_SET,
    EVENT_AXIS_RUDDER_SET,
    EVENT_AXIS_AILERONS_SET,
    EVENT_AXIS_THROTTLE_SET,
    EVENT_AUTO_THROTTLE_ARM
#ifndef NDEBUG
    ,
    EVENT_RESERVED_KEY_REQUEST,
    EVENT_RELOAD_USER_AIRCRAFT
#endif
};

static enum GROUP_ID {
    GROUP_FLIGHT_CONTROLS
};

static enum DATA_DEFINE_ID
{
    DEFINITION_FLIGHT_DATA,
    DEFINITION_FLAP_HANDLE,
    DEFINITION_FLAPS,
    DEFINITION_ELEVATOR_TRIM,
    DEFINITION_AUTO_THROTTLE
};

static enum DATA_REQUEST_ID
{
    REQUEST_AIR_FILE,
    REQUEST_FLIGHT_DATA,
    REQUEST_FLAP_HANDLE,
    REQUEST_AUTO_THROTTLE
};

struct Flaps
{
    double TrailingLeft;
    double TrailingRight;
    double LeadingLeft;
    double LeadingRight;
};

struct FlightData
{
    double AirspeedMach;
    double AirspeedTrue;
    double AngleOfAttack;
    double SideslipAngle;
    double GForce;
    double PitchRate;
    double RollRate;
    double ElevatorTrimPosition;
    double TrailingFlapsLeft;
    double TrailingFlapsRight;
    double LeadingFlapsLeft;
    double LeadingFlapsRight;
    double AileronTrimPercent;
    float BankDegrees;
    float HydraulicPressure1;
    float HydraulicPressure2;
    float ApuPercent;
    float TotalWeight;
    int SimOnGround;
    int AutopilotMaster;
};