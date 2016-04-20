/**********************************************************
*                                                         *
*   Copyright (C) Orion Lyau 2016. All Rights Reserved.   *
*                                                         *
**********************************************************/

#include "FCS.h"

#include <gauges.h>
#include <sstream>

#include "Utils.h"

namespace FCS
{

//-----------------------------------------------------------------------------

#ifdef DATA_GAUGE_ENABLED

char* StateLookup(State state)
{
    switch (state)
    {
    case State::Disabled: return "Disabled";
    case State::Autopilot: return "PassThrough - Autopilot";
    case State::PassThrough: return "PassThrough";
    case State::Enabled: return "Enabled";
    default: return "Unknown FCS::State value";
    }
}

char* RudderStateLookup(RudderState state)
{
    switch (state)
    {
    case RudderState::Disabled: return "Disabled";
    case RudderState::Autopilot: return "PassThrough - Autopilot";
    case RudderState::PassThrough: return "PassThrough";
    case RudderState::OnGround: return "OnGround";
    case RudderState::Enabled: return "Enabled";
    default: return "Unknown FCS::RudderState value";
    }
}

char* ModeLookup(Mode mode)
{
    switch (mode)
    {
    case Mode::OnGround: return "OnGround";
    case Mode::Mechanical: return "Mechanical";
    case Mode::PoweredApproach: return "PoweredApproach";
    case Mode::UpAndAway: return "UpAndAway";
    default: return "Unknown FCS::Mode value";
    }
}

char* ATCModeLookup(ATCMode mode)
{
    switch (mode)
    {
    case ATCMode::Disabled: return "Disabled";
    case ATCMode::Approach: return "Approach";
    case ATCMode::Cruise: return "Cruise";
    default: return "Unknown FCS::ATCMode value";
    }
}

#endif

//-----------------------------------------------------------------------------

double ClipValue(double input, double min, double max)
{
    if (input > max) { return max; }
    else if (input < min) { return min; }
    else { return input; }
}

double LeadingEdgeFlaps(double mach, double aoa)
{
    if (mach < 0.76 && aoa > 0.0)
    {
        return aoa * 4.0 / 3.0;
    }
    else
    {
        return 0.0;
    }
}

double tefKias(double kias)
{
    return (250.0 - kias) * 9.0 / 20.0;
}

double tefAoA(double aoa)
{
    if (aoa > 0.0 && aoa < 11.0)
    {
        return aoa * 1.5;
    }
    else if (aoa >= 11.0 && aoa <= 16.0)
    {
        return 16.5;
    }
    else if (aoa > 16.0 && aoa <= 25.0)
    {
        return (25.0 - aoa) * 11.0 / 6.0;
    }
    else
    {
        return 0.0;
    }
}

double TrailingEdgeFlaps(double mach, double aoa)
{
    if (mach < 0.9 && aoa > 0.0)
    {
        return tefAoA(aoa);
    }
    else if (mach >= 0.9 && mach < 1.05 && aoa > 0.0)
    {
        return ClipValue(tefAoA(aoa), 0.0, 8.0);
    }
    else
    {
        return 0.0;
    }
}

double ElevatorAoA(long elevatorPos, double offset)
{
    // quintic fit {{-100,20},{0,5.8},{100,-3}}
    // 5.8 - 0.115 x + 0.00027 x^2
    auto val = static_cast<double>(elevatorPos) / 163.83;
    return 5.8 + offset - (0.115 * val) + (0.00027 * val * val);
}

double ElevatorPitchRate(long elevatorPos, double offset = 0.0)
{
    // quintic fit {{-100,25},{0,0},{100,-22.5}}
    // - 0.2375 x + 0.000125 x^2
    auto val = static_cast<double>(elevatorPos) / 163.83;
    return offset - (0.2375 * val) + (0.000125 * val * val);
}

double ElevatorGForce(long elevatorPos, bool limitG, double offset)
{
    // quintic fit {{-100,7.5},{0,1},{100,-3}}
    // 1 - 0.0525 x + 0.000125 x^2
    // quintic fit {{-100,10},{0,1},{100,-5}} => 1 - 0.075 x + 0.00015 x^2
    auto val = static_cast<double>(elevatorPos) / 163.83;
    auto result = 1.0 + offset - (0.075 * val) + (0.00015 * val * val);
    return (limitG && result > 5.5) ? 5.5 : result;
}

double AileronRollRate(long aileronPos, double offset)
{
    // quintic fit {{-100,-240},{-50,-45},{0,0},{50,45},{100,240}}
    // 0.4 x + 0.0002 x^3
    auto val = static_cast<double>(aileronPos) / 163.83;
    return (0.4 * val) + (0.0002 * val * val * val) + offset;
}

double HighAoA(long elevatorPos, double offset)
{
    // quintic fit {{-100,55},{0,22},{100,-10}}
    // 22. - 0.325 x + 0.00005 x^2
    auto val = static_cast<double>(elevatorPos) / 163.83;
    return 22 + offset - (0.325 * val) + (0.00005 * val * val);
}

double PoweredApproach(double pitchRate, double aoa, double flapPosition, double pitchScalar, double aoaScalar)
{
    return (pitchRate * pitchScalar * (1.0 - flapPosition)) + (aoa * aoaScalar * flapPosition);
}

double UpAndAway(double pitchRate, double GForce, double aoa, double currentAoA, double airspeed, double GScalar, double pitchScalar, double aoaScalar)
{
    auto aoaBias = (currentAoA - 22.0) / (35.0 - 22.0);
    auto airspeedBias = airspeed / 310.0;
    double value = 0.0;
    if (airspeedBias >= 1.0)
    {
        value = GForce * GScalar;
    }
    else
    {
        value = (pitchRate * pitchScalar * (1.0 - airspeedBias)) + (GForce * GScalar);
    }

    if (aoaBias <= 0.0)
    {
        return value;
    }
    else if (aoaBias >= 1.0)
    {
        return aoa * aoaScalar;
    }
    else
    {
        return (value * (1.0 - aoaBias)) + (aoa * aoaScalar * aoaBias);
    }
}

//-----------------------------------------------------------------------------

FBW::FBW() :
    m_transientFlaps(std::make_shared<Flaps>()),
    m_desiredFlaps(std::make_shared<Flaps>()),
    m_cStar(PIDController::Factory::Make(0, 0, 0, -100, 100)),
    m_levelFlight(PIDController::Factory::Make(0, 0, 0, -100, 100)),
    m_roll(PIDController::Factory::Make(0, 0, 0, -100, 100)),
    m_sideslip(PIDController::Factory::Make(0, 0, 0, -100, 100)),
    m_throttleApproach(PIDController::Factory::Make(0, 0, 0, -100, 60)),
    m_throttleCruise(PIDController::Factory::Make(0, 0, 0, -100, 60)),
    m_gScalar(0.0),
    m_pitchScalar(0.0),
    m_aoaScalar(0.0),
    m_highAoAScalar(0.0),
    m_mainState(State::PassThrough),
    m_yawState(RudderState::PassThrough),
    m_mode(Mode::OnGround),
    m_atcMode(ATCMode::Disabled),
    m_stickX(0),
    m_stickY(0),
    m_stickZ(0),
    m_slider{ 0L, 0L, 0L },
    m_spinSwitch(std::make_shared<Gauge::NamedVar>("switch_spin")),
    m_atcSwitch(std::make_shared<Gauge::NamedVar>("ATC_INIT")),
    m_takeoffTrim(std::make_shared<Gauge::NamedVar>("Take_Off_Trim")),
    m_fcsInitialized(std::make_shared<Gauge::NamedVar>("fcs_initialized")),
    m_throttleCutoffLeft(std::make_shared<Gauge::NamedVar>("Throttle_cutoff0")),
    m_throttleCutoffRight(std::make_shared<Gauge::NamedVar>("Throttle_cutoff1")),
    m_throttlePosition(std::make_shared<Gauge::AircraftVar>("GENERAL ENG THROTTLE LEVER POSITION", "percent")),
    m_fuelValve(std::make_shared<Gauge::AircraftVar>("GENERAL ENG FUEL VALVE", "Bool")),
    m_surfaceRelativeGroundSpeed(std::make_shared<Gauge::AircraftVar>("SURFACE RELATIVE GROUND SPEED", "knots")),
    m_takeoffTrimEnabled(false),
    m_cfgValid(false),
    m_cfgPath(""),
    deltaTime(-1.0),
    m_flapSelection(0),
    m_atcSpeed(0.0),
    m_atcSlider{ 0L, 0L, 0L }
{}

FBW::~FBW() {}

bool FBW::InitializeData(std::string const& cfgPath)
{
    m_cfgPath = cfgPath;

    auto szCStar = Utils::ReadIni(cfgPath, "HornetFCS", "CStar");
    auto szLevelFlight = Utils::ReadIni(cfgPath, "HornetFCS", "LevelFlight");
    auto szRoll = Utils::ReadIni(cfgPath, "HornetFCS", "Aileron");
    auto szSideslip = Utils::ReadIni(cfgPath, "HornetFCS", "Sideslip");
    auto szThrottleApproach = Utils::ReadIni(cfgPath, "HornetFCS", "ThrottleApproach");
    auto szThrottleCruise = Utils::ReadIni(cfgPath, "HornetFCS", "ThrottleCruise");

    auto pitchRate = Utils::ReadIni(cfgPath, "HornetFCS", "PitchRateScalar");
    auto GForce = Utils::ReadIni(cfgPath, "HornetFCS", "GForceScalar");
    auto aoa = Utils::ReadIni(cfgPath, "HornetFCS", "AoAScalar");
    auto highAoA = Utils::ReadIni(cfgPath, "HornetFCS", "HighAoAScalar");

    auto afterburnerThreshold = Utils::ReadIni(cfgPath, "TurbineEngineData", "afterburner_throttle_threshold");

    try
    {
        auto cStarVec = Utils::SplitAndParse(szCStar, std::wstring(L","));
        auto levelFlightVec = Utils::SplitAndParse(szLevelFlight, std::wstring(L","));
        auto rollVec = Utils::SplitAndParse(szRoll, std::wstring(L","));
        auto sideslipVec = Utils::SplitAndParse(szSideslip, std::wstring(L","));
        auto throttleApproachVec = Utils::SplitAndParse(szThrottleApproach, std::wstring(L","));
        auto throttleCruiseVec = Utils::SplitAndParse(szThrottleCruise, std::wstring(L","));

        auto throttleLimit = !afterburnerThreshold.empty() ? std::stod(afterburnerThreshold) : 1.0;
        auto throttlePidMax = (200.0 * throttleLimit) - 100.0;

        if (cStarVec.size() == 3 && levelFlightVec.size() == 3 && rollVec.size() == 3 && sideslipVec.size() == 3 && throttleApproachVec.size() == 3 && throttleCruiseVec.size() == 3)
        {
            m_cStar = PIDController::Factory::Make(cStarVec[0], cStarVec[1], cStarVec[2], -100.0, 100.0);
            m_levelFlight = PIDController::Factory::Make(levelFlightVec[0], levelFlightVec[1], levelFlightVec[2], -100.0, 100.0);
            m_roll = PIDController::Factory::Make(rollVec[0], rollVec[1], rollVec[2], -100.0, 100.0);
            m_sideslip = PIDController::Factory::Make(sideslipVec[0], sideslipVec[1], sideslipVec[2], -100.0, 100.0);
            m_throttleApproach = PIDController::Factory::Make(throttleApproachVec[0], throttleApproachVec[1], throttleApproachVec[2], -100.0, throttlePidMax);
            m_throttleCruise = PIDController::Factory::Make(throttleCruiseVec[0], throttleCruiseVec[1], throttleCruiseVec[2], -100.0, throttlePidMax);
            m_gScalar = std::stod(GForce);
            m_pitchScalar = std::stod(pitchRate);
            m_aoaScalar = std::stod(aoa);
            m_highAoAScalar = std::stod(highAoA);
            m_cfgValid = true;
            m_fcsInitialized->Set(1.0);
            return true;
        }

        return false;
    }
    catch (std::invalid_argument const&)
    {
        return false;
    }

    return false;
}

bool FBW::SetElevator(long stickY)
{
    m_stickY = stickY;
    switch (m_mainState)
    {
    case State::Disabled:
        return false;
    case State::Autopilot:
    case State::PassThrough:
        return true;
    case State::Enabled:
        if (stickY == 0)
        {
            m_cStar->ResetError();
        }
        else
        {
            m_levelFlight->ResetError();
        }
        return false;
    default:
        return false;
    }
}

bool FBW::SetAileron(long stickX)
{
    m_stickX = stickX;
    switch (m_mainState)
    {
    case State::Disabled:
        return false;
    case State::Autopilot:
    case State::PassThrough:
        return true;
    case State::Enabled:
        if (stickX == 0)
        {
            m_roll->ResetError();
        }
        return false;
    default:
        return false;
    }
}

std::pair<bool, long> FBW::SetRudder(long stickZ)
{
    m_stickZ = stickZ;
    switch (m_yawState)
    {
    case RudderState::Disabled:
        return std::make_pair(false, 0L);
    case RudderState::Autopilot:
    case RudderState::PassThrough:
    {
        m_sideslip->ResetError();
        auto rudderVal = stickZ;
        if (m_flightData && m_flightData->AngleOfAttack < 22.0)
        {
            rudderVal = static_cast<long>(stickZ / 2.0);
        }
        return std::make_pair(true, rudderVal);
    }
    case RudderState::OnGround:
    {
        auto rudderVal = stickZ;
        if (m_surfaceRelativeGroundSpeed->Get() > 10.0 && abs(rudderVal) > 4369L)
        {
            rudderVal = rudderVal > 0 ? 4369L : -4369L;
        }
        return std::make_pair(true, rudderVal);
    }
    case RudderState::Enabled:
        return std::make_pair(false, 0L);
    default:
        return std::make_pair(false, 0L);
    }
}

bool FBW::SetThrottle(long slider, uint8_t throttleIdx)
{
    m_slider[throttleIdx] = slider;
    if (m_atcMode == ATCMode::Disabled)
    {
        return true;
    }
    else if (abs(slider - m_atcSlider[throttleIdx]) > 3277L || abs(m_slider[1] - m_slider[2]) > 3277L) // 3276.6 is 10% of -16383 +16383 range
    {
        m_atcSwitch->Set(0.0);
        return true;
    }
    return false;
}

void FBW::SetFlapSelection(int flapSelection)
{
    m_flapSelection = flapSelection;
}

void FBW::DisableAutoThrottle()
{
    if (m_atcMode != ATCMode::Disabled)
    {
        m_atcSwitch->Set(0.0);
    }
}

void FBW::ToggleAutoThrottle()
{
    m_atcSwitch->Toggle();
}

void FBW::Update6Hz()
{
    auto spinResult = m_spinSwitch->Update();
    if (spinResult.first && !!spinResult.second)
    {
        m_cStar->ResetError();
        m_levelFlight->ResetError();
        m_roll->ResetError();
        m_sideslip->ResetError();
        m_throttleApproach->ResetError();
        m_throttleCruise->ResetError();
    }

    auto atcResult = m_atcSwitch->Update();
    if (atcResult.first && !atcResult.second)
    {
        m_atcMode = ATCMode::Disabled;
    }

    auto trimResult = m_takeoffTrim->Update();
    if (trimResult.first)
    {
        if (!!trimResult.second && !!m_flightData->SimOnGround)
        {
            m_takeoffTrimEnabled = true;
        }
        else
        {
            m_takeoffTrimEnabled = false;
        }
    }

    auto throttleCutoffLeft = m_throttleCutoffLeft->Update();
    if (throttleCutoffLeft.first)
    {
        if (!!throttleCutoffLeft.second)
        {
            if (m_throttlePosition->Get(1) < 3.0)
            {
                if (!!m_fuelValve->Get(1))
                {
                    send_key_event(KEY_TOGGLE_FUEL_VALVE_ENG1, UINT32(0));
                }
            }
            else
            {
                m_throttleCutoffLeft->Set(0.0);
            }
        }
        else
        {
            if (!m_fuelValve->Get(1))
            {
                send_key_event(KEY_TOGGLE_FUEL_VALVE_ENG1, UINT32(0));
            }
        }
    }

    auto throttleCutoffRight = m_throttleCutoffRight->Update();
    if (throttleCutoffRight.first)
    {
        if (!!throttleCutoffRight.second)
        {
            if (m_throttlePosition->Get(2) < 3.0)
            {
                if (!!m_fuelValve->Get(2))
                {
                    send_key_event(KEY_TOGGLE_FUEL_VALVE_ENG2, UINT32(0));
                }
            }
            else
            {
                m_throttleCutoffRight->Set(0.0);
            }
        }
        else
        {
            if (!m_fuelValve->Get(2))
            {
                send_key_event(KEY_TOGGLE_FUEL_VALVE_ENG2, UINT32(0));
            }
        }
    }
}

std::pair<bool, bool> FBW::SetState(FlightData* fd)
{
    m_flightData = *fd;

    if (fd->HydraulicPressure1 < 1500.0f && fd->HydraulicPressure2 < 1500.0f && fd->ApuPercent < 0.7f)
    {
        m_mainState = State::Enabled;
        m_yawState = RudderState::Enabled;
    }
    else if (!!fd->SimOnGround || fd->AirspeedTrue < 50.0 || !!m_spinSwitch->Get() || m_takeoffTrimEnabled)
    {
        m_mainState = State::PassThrough;
        m_yawState = !!fd->SimOnGround ? RudderState::OnGround : RudderState::PassThrough;
    }
    else if (!!fd->AutopilotMaster)
    {
        m_mainState = State::Autopilot;
        m_yawState = RudderState::Autopilot;
    }
    else
    {
        m_mainState = State::Enabled;
        m_yawState = m_stickZ == 0 ? RudderState::Enabled : RudderState::PassThrough;
    }

    return std::make_pair(m_mainState == State::Enabled, m_yawState == RudderState::Enabled);
}

std::pair<bool, double> FBW::SetMode()
{
    if (m_flightData->HydraulicPressure1 < 1500.0f && m_flightData->HydraulicPressure2 < 1500.0f && m_flightData->ApuPercent < 0.7f)
    {
        if (m_mode != Mode::Mechanical)
        {
            m_mode = Mode::Mechanical;
            return std::make_pair(true, 0.0);
        }
    }
    else if (!!m_flightData->SimOnGround)
    {
        if (m_mode != Mode::OnGround)
        {
            auto setTrim = true;
            if (m_mode == Mode::PoweredApproach)
            {
                setTrim = false;
            }
            m_mode = Mode::OnGround;
            return std::make_pair(setTrim, 0.0);
        }
    }
    else if (m_flapSelection > 0 && m_flightData->AirspeedTrue < 240.0)
    {
        if (m_mode != Mode::PoweredApproach)
        {
            auto setTrim = true;
            if (m_mode == Mode::OnGround)
            {
                setTrim = false;
            }
            m_mode = Mode::PoweredApproach;
            return std::make_pair(setTrim, m_flightData->AngleOfAttack - 5.8);
        }
    }
    else
    {
        if (m_mode != Mode::UpAndAway)
        {
            m_mode = Mode::UpAndAway;
            return std::make_pair(true, 0.0);
        }
    }

    return std::make_pair(false, 0.0);
}

std::pair<bool, double> FBW::SetAutoThrottle()
{
    if (!m_atcSwitch->Get())
    {
        return std::make_pair(false, 0.0);
    }
    switch (m_atcMode)
    {
    case ATCMode::Disabled:
    {
        // switch is on; check for conditions to turn on autothrottle
        if (m_flapSelection == 0 && !m_flightData->SimOnGround)
        {
            m_throttleCruise->ResetError();
            m_atcMode = ATCMode::Cruise;
            for (int i = 0; i < 3; i++)
            {
                m_atcSlider[i] = m_slider[i];
            }
            m_atcSpeed = m_flightData->AirspeedTrue;
        }
        else if (m_flapSelection > 0 && m_flightData->TrailingFlapsLeft >= 0.6 && !m_flightData->SimOnGround)
        {
            m_throttleApproach->ResetError();
            m_atcMode = ATCMode::Approach;
            for (int i = 0; i < 3; i++)
            {
                m_atcSlider[i] = m_slider[i];
            }
        }
        else
        {
            m_atcSwitch->Set(0.0);
        }
        return std::make_pair(false, 0.0);
    }
    case ATCMode::Cruise:
    {
        if (m_flapSelection > 0 || m_mainState == State::PassThrough || m_mode == Mode::Mechanical || !!m_flightData->SimOnGround)
        {
            m_atcSwitch->Set(0.0);
            return std::make_pair(false, 0.0);
        }
        else
        {
            auto result = m_throttleCruise->Calculate(m_flightData->AirspeedTrue, m_atcSpeed, deltaTime) * 163.83;
            return std::make_pair(true, result);
        }
    }
    case ATCMode::Approach:
    {
        if (m_flapSelection == 0 || m_flightData->TrailingFlapsLeft < 0.6 || !!m_flightData->SimOnGround || abs(m_flightData->BankDegrees) > 70.0f || m_mainState == State::PassThrough || m_mode == Mode::Mechanical)
        {
            m_atcSwitch->Set(0.0);
            return std::make_pair(false, 0.0);
        }
        else
        {
            auto result = m_throttleApproach->Calculate(8.1, m_flightData->AngleOfAttack, deltaTime) * 163.83;
            return std::make_pair(true, result);
        }
    }
    }
    return std::make_pair(false, 0.0);
}

double FBW::GetCurrentElevator()
{
    switch (m_mode)
    {
    case Mode::PoweredApproach:
    {
        auto desiredValue = PoweredApproach(
            m_flightData->PitchRate,
            m_flightData->AngleOfAttack,
            m_flightData->TrailingFlapsLeft,
            m_pitchScalar,
            m_aoaScalar
            );

        auto currentValue = PoweredApproach(
            ElevatorPitchRate(m_stickY),
            ElevatorAoA(m_stickY, m_flightData->ElevatorTrimPosition),
            m_flightData->TrailingFlapsLeft,
            m_pitchScalar,
            m_aoaScalar
            );

        return m_cStar->CalculateCustom(currentValue, desiredValue, deltaTime, 1.0) * 163.83;
    }
    case Mode::UpAndAway:
    {
        auto offsetVal = (m_stickY == 0 && abs(m_flightData->PitchRate) < 1.0) ? m_levelFlight->Calculate(m_flightData->GForce - m_flightData->PitchRate, 1.0 + (m_flightData->ElevatorTrimPosition / 2.0), deltaTime) : (m_flightData->ElevatorTrimPosition / 2.0);

        auto desiredValue = UpAndAway(
            m_flightData->PitchRate,
            m_flightData->GForce,
            m_flightData->AngleOfAttack,
            m_flightData->AngleOfAttack,
            m_flightData->AirspeedTrue,
            m_gScalar,
            m_pitchScalar,
            m_highAoAScalar
            );

        auto currentValue = UpAndAway(
            ElevatorPitchRate(m_stickY, offsetVal),
            ElevatorGForce(m_stickY, m_flightData->TotalWeight > 44000.0f, offsetVal),
            HighAoA(m_stickY, offsetVal),
            m_flightData->AngleOfAttack,
            m_flightData->AirspeedTrue,
            m_gScalar,
            m_pitchScalar,
            m_highAoAScalar
            );

        auto val = abs(static_cast<double>(m_stickY) / 163.83);
        auto ki = 1.0;
        if (val < 10.0)
        {
            ki = val / 10.0;
        }

        return m_cStar->CalculateCustom(currentValue, desiredValue, deltaTime, ki) * 163.83;
    }
    case Mode::Mechanical:
    {
        return m_stickY / (m_flapSelection > 0 ? 1.5 : 2.0);
    }
    default:
    {
        // this should not be triggered
        return m_stickY;
    }
    }
}

double FBW::GetCurrentAileron()
{
    if (m_mode == Mode::Mechanical)
    {
        return m_stickX / 2.0;
    }

    auto rollRate = AileronRollRate(m_stickX, m_flightData->AileronTrimPercent * 60.0);

    if (m_mode == Mode::PoweredApproach)
    {
        if (rollRate > 180.0) { rollRate = 180.0; }
        else if (rollRate < -180.0) { rollRate = -180.0; }
    }

    return m_roll->Calculate(m_flightData->RollRate, rollRate, deltaTime) * 163.83;
}

double FBW::GetCurrentRudder()
{
    if (m_mode == Mode::Mechanical)
    {
        return m_stickZ / 4.0;
    }

    auto val = m_sideslip->Calculate(m_flightData->SideslipAngle, 0.0, deltaTime) * 163.83;
    if (m_flightData && m_flightData->AngleOfAttack < 22.0)
    {
        val = val / 2.0;
    }
    return val;
}

std::shared_ptr<Flaps> FBW::GetCurrentFlaps()
{
    double lef = 0.0;
    double tef = 0.0;
    if (!!m_flightData->SimOnGround)
    {
        switch (m_flapSelection)
        {
        case 0:
            break;
        case 1:
            lef = 12.0;
            tef = 30.0;
            break;
        case 2:
            lef = 12.0;
            tef = 45.0;
            break;
        }
    }
    else
    {
        lef = ClipValue(LeadingEdgeFlaps(m_flightData->AirspeedMach, m_flightData->AngleOfAttack), 0.0, 33.0);
        switch (m_flapSelection)
        {
        case 0:
            tef = TrailingEdgeFlaps(m_flightData->AirspeedMach, m_flightData->AngleOfAttack);
            break;
        case 1:
            if (m_flightData->AirspeedTrue < 250.0)
            {
                tef = ClipValue(tefKias(m_flightData->AirspeedTrue), 0.0, 30.0);
            }
            else
            {
                tef = TrailingEdgeFlaps(m_flightData->AirspeedMach, m_flightData->AngleOfAttack);
            }
            break;
        case 2:
            if (m_flightData->AirspeedTrue < 250.0)
            {
                tef = ClipValue(tefKias(m_flightData->AirspeedTrue), 0.0, 45.0);
            }
            else
            {
                tef = TrailingEdgeFlaps(m_flightData->AirspeedMach, m_flightData->AngleOfAttack);
            }
            break;
        }
    }
    m_desiredFlaps->LeadingLeft = lef / 33.0;
    m_desiredFlaps->LeadingRight = lef / 33.0;
    m_desiredFlaps->TrailingLeft = tef / 45.0;
    m_desiredFlaps->TrailingRight = tef / 45.0;

    auto dFlap = FLAP_PER_SEC * deltaTime;

    auto dLeadingLeft = m_desiredFlaps->LeadingLeft - m_flightData->LeadingFlapsLeft;
    if (std::abs(dLeadingLeft) < MIN_TARGET_DELTA)
    {
        m_transientFlaps->LeadingLeft = m_desiredFlaps->LeadingLeft;
    }
    else
    {
        m_transientFlaps->LeadingLeft = dLeadingLeft > 0.0 ? m_flightData->LeadingFlapsLeft + dFlap : m_flightData->LeadingFlapsLeft - dFlap;
    }

    auto dLeadingRight = m_desiredFlaps->LeadingRight - m_flightData->LeadingFlapsRight;
    if (std::abs(dLeadingRight) < MIN_TARGET_DELTA)
    {
        m_transientFlaps->LeadingRight = m_desiredFlaps->LeadingRight;
    }
    else
    {
        m_transientFlaps->LeadingRight = dLeadingRight > 0.0 ? m_flightData->LeadingFlapsRight + dFlap : m_flightData->LeadingFlapsRight - dFlap;
    }

    auto dTrailingLeft = m_desiredFlaps->TrailingLeft - m_flightData->TrailingFlapsLeft;
    if (std::abs(dTrailingLeft) < MIN_TARGET_DELTA)
    {
        m_transientFlaps->TrailingLeft = m_desiredFlaps->TrailingLeft;
    }
    else
    {
        m_transientFlaps->TrailingLeft = dTrailingLeft > 0.0 ? m_flightData->TrailingFlapsLeft + dFlap : m_flightData->TrailingFlapsLeft - dFlap;
    }

    auto dTrailingRight = m_desiredFlaps->TrailingRight - m_flightData->TrailingFlapsRight;
    if (std::abs(dTrailingRight) < MIN_TARGET_DELTA)
    {
        m_transientFlaps->TrailingRight = m_desiredFlaps->TrailingRight;
    }
    else
    {
        m_transientFlaps->TrailingRight = dTrailingRight > 0.0 ? m_flightData->TrailingFlapsRight + dFlap : m_flightData->TrailingFlapsRight - dFlap;
    }

    return m_transientFlaps;
}

#ifdef DATA_GAUGE_ENABLED

std::string FBW::ToString() const
{
    std::ostringstream ss;
    ss << "CStar " << m_cStar->ToString() << std::endl;
    ss << "Level Flight " << m_levelFlight->ToString() << std::endl;
    ss << "Rudder " << m_sideslip->ToString() << std::endl;
    ss << "Aileron " << m_roll->ToString() << std::endl;
    ss << "Throttle Approach " << m_throttleApproach->ToString() << std::endl;
    ss << "Throttle Cruise " << m_throttleCruise->ToString() << std::endl;

    ss << "FCS State " << StateLookup(m_mainState) << std::endl;
    ss << "FCS Rudder State " << RudderStateLookup(m_yawState) << std::endl;
    ss << "FCS Mode " << ModeLookup(m_mode) << std::endl;
    ss << "ATC Mode " << ATCModeLookup(m_atcMode) << std::endl;

    ss << "Stick X " << m_stickX << std::endl;
    ss << "Stick Y " << m_stickY << std::endl;
    ss << "Stick Z " << m_stickZ << std::endl;
    ss << "Slider " << m_slider[0] << std::endl;
    ss << "Slider 1 " << m_slider[1] << std::endl;
    ss << "Slider 2 " << m_slider[2] << std::endl;
    ss << "Flaps " << m_flapSelection << std::endl;
    ss << "Spin Switch " << m_spinSwitch->Get() << std::endl;
    ss << "ATC Switch " << m_atcSwitch->Get() << std::endl;
    ss << "Takeoff Trim Switch " << m_takeoffTrim->Get() << std::endl;

    ss << "TAS (knots) " << (!m_flightData ? 0.0 : m_flightData->AirspeedTrue) << std::endl;
    ss << "AoA (degrees) " << (!m_flightData ? 0.0 : m_flightData->AngleOfAttack) << std::endl;
    ss << "G force " << (!m_flightData ? 0.0 : m_flightData->GForce) << std::endl;
    return ss.str();
}

#endif

}; // namespace FCS