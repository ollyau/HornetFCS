#pragma once

#include "AircraftVar.h"
#include "NamedVar.h"
#include "SimConnectData.h"
#include "PIDController.h"

#include <string>
#include <utility>

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

enum class Mode
{
    OnGround,
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
    bool SetRudder(long stickZ);
    bool SetThrottle(long slider, uint8_t throttleIdx);
    void SetFlapSelection(int flapSelection);

    void DisableAutoThrottle();
    void ToggleAutoThrottle();

    std::pair<bool, bool> SetState(FlightData* fd);
    std::pair<bool, double> SetMode();
    std::pair<bool, double> SetAutoThrottle();
    void Update6Hz();

    //State GetMainState() const { return m_mainState; }
    //State GetYawState() const { return m_yawState; }
    //Mode GetMode() const { return m_mode; }
    bool GetCfgValid() const { return m_cfgValid; }

    double GetCurrentElevator();
    double GetCurrentAileron();
    double GetCurrentRudder();
    std::shared_ptr<Flaps> GetCurrentFlaps();

    double deltaTime;
#ifdef DATA_GAUGE_ENABLED
    std::string ToString() const;
#endif

private:
    FlightData* m_flightData;
    std::shared_ptr<Flaps> m_transientFlaps;
    std::shared_ptr<Flaps> m_desiredFlaps;

    PIDController::Ptr m_cStar;
    PIDController::Ptr m_levelFlight;
    PIDController::Ptr m_roll;
    PIDController::Ptr m_sideslip;
    PIDController::Ptr m_throttleApproach;
    PIDController::Ptr m_throttleCruise;

    State m_mainState;
    State m_yawState;
    Mode m_mode;

    ATCMode m_atcMode;
    double m_atcSpeed;
    long m_atcSlider[3];

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

    NamedVar::Ptr m_spinSwitch;
    NamedVar::Ptr m_atcSwitch;
    NamedVar::Ptr m_takeoffTrim;
	NamedVar::Ptr m_fcsInitialized;
    NamedVar::Ptr m_throttleCutoffLeft;
    NamedVar::Ptr m_throttleCutoffRight;

    AircraftVar::Ptr m_throttlePosition;
    AircraftVar::Ptr m_fuelValve;

    const double FLAP_PER_SEC = 0.2;
    const double MIN_TARGET_DELTA = 0.05;
    //const double MIN_FLAP_DELTA = .025;
};

}; // namespace FCS