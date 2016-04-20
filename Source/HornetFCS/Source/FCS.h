/**********************************************************
*                                                         *
*   Copyright (C) Orion Lyau 2016. All Rights Reserved.   *
*                                                         *
**********************************************************/

#pragma once

#include <Optional.h>
#include <string>
#include <utility>

#include "AircraftVar.h"
#include "NamedVar.h"
#include "PIDController.h"
#include "SimConnectData.h"

#define DATA_GAUGE_ENABLED

namespace FCS
{

//-----------------------------------------------------------------------------

enum class State
{
    Disabled,
    Autopilot,
    PassThrough,
    Enabled
};

//-----------------------------------------------------------------------------

enum class RudderState
{
    Disabled,
    Autopilot,
    PassThrough,
    OnGround,
    Enabled
};

//-----------------------------------------------------------------------------

enum class Mode
{
    OnGround,
    Mechanical,
    PoweredApproach,
    UpAndAway
};

//-----------------------------------------------------------------------------

enum class ATCMode
{
    Disabled,
    Approach,
    Cruise
};

//-----------------------------------------------------------------------------

class FBW
{
public:
    typedef std::shared_ptr<FBW> Ptr;

    FBW();
    ~FBW();

    bool InitializeData(std::string const& cfgPath);

    bool SetElevator(long stickY);
    bool SetAileron(long stickX);
    std::pair<bool, long> SetRudder(long stickZ);
    bool SetThrottle(long slider, uint8_t throttleIdx);
    void SetFlapSelection(int flapSelection);

    void DisableAutoThrottle();
    void ToggleAutoThrottle();

    std::pair<bool, bool> SetState(FlightData *fd);
    std::pair<bool, double> SetMode();
    std::pair<bool, double> SetAutoThrottle();
    void Update6Hz();

    bool GetCfgValid() const { return m_cfgValid; }
    std::string GetCfgPath() const { return m_cfgPath; }

    double GetCurrentElevator();
    double GetCurrentAileron();
    double GetCurrentRudder();
    std::shared_ptr<Flaps> GetCurrentFlaps();

    double deltaTime;

#ifdef DATA_GAUGE_ENABLED
    std::string ToString() const;
#endif

private:
    std::experimental::optional<FlightData> m_flightData;
    std::shared_ptr<Flaps> m_transientFlaps;
    std::shared_ptr<Flaps> m_desiredFlaps;

    PIDController::IController::Ptr m_cStar;
    PIDController::IController::Ptr m_levelFlight;
    PIDController::IController::Ptr m_roll;
    PIDController::IController::Ptr m_sideslip;
    PIDController::IController::Ptr m_throttleApproach;
    PIDController::IController::Ptr m_throttleCruise;

    State m_mainState;
    RudderState m_yawState;
    Mode m_mode;

    ATCMode m_atcMode;
    double m_atcSpeed;
    long m_atcSlider[3];

    std::string m_cfgPath;
    bool m_cfgValid;
    bool m_takeoffTrimEnabled;

    double m_gScalar;
    double m_pitchScalar;
    double m_aoaScalar;
    double m_highAoAScalar;

    long m_stickX;
    long m_stickY;
    long m_stickZ;
    long m_slider[3];
    int m_flapSelection;

    Gauge::NamedVar::Ptr m_spinSwitch;
    Gauge::NamedVar::Ptr m_atcSwitch;
    Gauge::NamedVar::Ptr m_takeoffTrim;
    Gauge::NamedVar::Ptr m_fcsInitialized;
    Gauge::NamedVar::Ptr m_throttleCutoffLeft;
    Gauge::NamedVar::Ptr m_throttleCutoffRight;

    Gauge::AircraftVar::Ptr m_surfaceRelativeGroundSpeed;
    Gauge::AircraftVar::Ptr m_throttlePosition;
    Gauge::AircraftVar::Ptr m_fuelValve;

    const double FLAP_PER_SEC = 0.2;
    const double MIN_TARGET_DELTA = 0.05;
};

}; // namespace FCS