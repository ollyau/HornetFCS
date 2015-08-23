#pragma once

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

#ifdef DATA_GAUGE_ENABLED

char* StateLookup(State state);
char* ModeLookup(Mode mode);

#endif

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
    void SetFlapSelection(int flapSelection);

    std::pair<State, State> SetState(FlightData* fd);
    std::pair<bool, double> SetMode();
    void Update6Hz();

    //State GetMainState() const { return m_mainState; }
    //State GetYawState() const { return m_yawState; }
    //Mode GetMode() const { return m_mode; }
    bool GetCfgValid() const { return m_cfgValid; }

    double GetCurrentElevator();
    double GetCurrentAileron();
    double GetCurrentRudder();
    std::shared_ptr<Flaps> GetCurrentFlaps();

    float frameRate;
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

    State m_mainState;
    State m_yawState;
    Mode m_mode;

    bool m_cfgValid;
    bool m_takeoffTrimEnabled;

    double m_gScalar;
    double m_pitchScalar;
    double m_aoaScalar;
    double m_highAoAScalar;

    long m_stickX;
    long m_stickY;
    long m_stickZ;
    int m_flapSelection;

    NamedVar::Ptr m_spinSwitch;
    NamedVar::Ptr m_takeoffTrim;

    const double FLAP_PER_SEC = 2.0 / 15.0;
    const double MIN_TARGET_DELTA = 0.01;
    //const double MIN_FLAP_DELTA = .025;
};

}; // namespace FCS