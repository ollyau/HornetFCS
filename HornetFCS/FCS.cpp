#include "stdafx.h"

#include "FCS.h"
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
    case State::PassThrough: return "PassThrough";
    case State::Enabled: return "Enabled";
    default: return "Unknown FCS::State value";
    }
}

char* ModeLookup(Mode mode)
{
    switch (mode)
    {
    case Mode::OnGround: return "OnGround";
    case Mode::PoweredApproach: return "PoweredApproach";
    case Mode::UpAndAway: return "UpAndAway";
    default: return "Unknown FCS::Mode value";
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
    // 3.552713678800501*^-15 - 0.2375 x + 0.000125 x^2
    auto val = static_cast<double>(elevatorPos) / 163.83;
    return 3.552713678800501e-15 + offset - (0.2375 * val) + (0.000125 * val * val);
}

double ElevatorGForce(long elevatorPos, bool limitG, double offset)
{
    // 1 - 0.065 x + 0.00025 x^2
    auto val = static_cast<double>(elevatorPos) / 163.83;
    auto result = 1.0 + offset - (0.065 * val) + (0.00025 * val * val);
    return (limitG && result > 5.5) ? 5.5 : result;
}

double AileronRollRate(long aileronPos, double offset)
{
    // quintic fit {{-100,-240},{-50,-60},{0,0},{50,60},{100,240}}
    // 0.8 x + 0.00016 x^3
    auto val = static_cast<double>(aileronPos) / 163.83;
    return (0.8 * val) + (0.00016 * val * val * val) + offset;
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
    auto airspeedBias = (airspeed - 230.0) / (390.0 - 230.0);
    double value = 0.0;
    if (airspeedBias >= 1.0)
    {
        value = GForce * GScalar;
    }
    else if (airspeedBias <= 0.0)
    {
        value = pitchRate * pitchScalar;
    }
    else
    {
        value = (pitchRate * pitchScalar * (1.0 - airspeedBias)) + (GForce * GScalar * airspeedBias);
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

FBW::FBW()
:
m_transientFlaps(std::make_shared<Flaps>()),
m_desiredFlaps(std::make_shared<Flaps>()),
m_cStar(std::make_shared<PIDController>(0, 0, 0, -100, 100)),
m_levelFlight(std::make_shared<PIDController>(0, 0, 0, -100, 100)),
m_roll(std::make_shared<PIDController>(0, 0, 0, -100, 100)),
m_sideslip(std::make_shared<PIDController>(0, 0, 0, -100, 100)),
m_gScalar(0.0),
m_pitchScalar(0.0),
m_aoaScalar(0.0),
m_highAoAScalar(0.0),
m_mainState(State::PassThrough),
m_yawState(State::PassThrough),
m_mode(Mode::OnGround),
m_stickX(0),
m_stickY(0),
m_stickZ(0),
m_spinSwitch(std::make_shared<NamedVar>("switch_spin")),
m_cfgValid(false),
frameRate(-1.0f),
m_flapSelection(0)
{}

FBW::~FBW() {}

bool FBW::InitializeData(std::string const& cfgPath)
{
    auto szCStar = Utils::ReadIni(cfgPath, "HornetFCS", "CStar");
    auto szLevelFlight = Utils::ReadIni(cfgPath, "HornetFCS", "LevelFlight");
    auto szRoll = Utils::ReadIni(cfgPath, "HornetFCS", "Aileron");
    auto szSideslip = Utils::ReadIni(cfgPath, "HornetFCS", "Sideslip");

    auto pitchRate = Utils::ReadIni(cfgPath, "HornetFCS", "PitchRateScalar");
    auto GForce = Utils::ReadIni(cfgPath, "HornetFCS", "GForceScalar");
    auto aoa = Utils::ReadIni(cfgPath, "HornetFCS", "AoAScalar");
    auto highAoA = Utils::ReadIni(cfgPath, "HornetFCS", "HighAoAScalar");

    auto cStarVec = Utils::SplitAndParse(szCStar, std::wstring(L","));
    auto levelFlightVec = Utils::SplitAndParse(szLevelFlight, std::wstring(L","));
    auto rollVec = Utils::SplitAndParse(szRoll, std::wstring(L","));
    auto sideslipVec = Utils::SplitAndParse(szSideslip, std::wstring(L","));

    if (cStarVec.size() == 3 && sideslipVec.size() == 3 && rollVec.size() == 3 && levelFlightVec.size() == 3)
    {
        m_cStar = std::make_shared<PIDController>(cStarVec[0], cStarVec[1], cStarVec[2], -100.0, 100.0);
        m_levelFlight = std::make_shared<PIDController>(levelFlightVec[0], levelFlightVec[1], levelFlightVec[2], -100.0, 100.0);
        m_roll = std::make_shared<PIDController>(rollVec[0], rollVec[1], rollVec[2], -100.0, 100.0);
        m_sideslip = std::make_shared<PIDController>(sideslipVec[0], sideslipVec[1], sideslipVec[2], -100.0, 100.0);
        m_gScalar = std::stod(GForce);
        m_pitchScalar = std::stod(pitchRate);
        m_aoaScalar = std::stod(aoa);
        m_highAoAScalar = std::stod(highAoA);
        m_cfgValid = true;
        return true;
    }
    else
    {
        return false;
    }
}

bool FBW::SetElevator(long stickY)
{
    m_stickY = stickY;
    switch (m_mainState)
    {
    case State::Disabled:
        return false;
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

bool FBW::SetRudder(long stickZ)
{
    m_stickZ = stickZ;
    switch (m_yawState)
    {
    case State::Disabled:
        return false;
    case State::PassThrough:
        m_sideslip->ResetError();
        return true;
    case State::Enabled:
        return false;
    default:
        return false;
    }
}

void FBW::SetFlapSelection(int flapSelection)
{
    m_flapSelection = flapSelection;
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
    }
}

std::pair<State, State> FBW::SetState(FlightData* fd)
{
    m_flightData = fd;

    if (fd->HydraulicPressure1 < 1500.0f && fd->HydraulicPressure2 < 1500.0f && fd->ApuPercent < 0.7f)
    {
        m_mainState = State::Disabled;
        m_yawState = State::Disabled;
    }
    else if (!!fd->SimOnGround || fd->AirspeedTrue < 50.0 || !!fd->AutopilotMaster || !!m_spinSwitch->Get())
    {
        m_mainState = State::PassThrough;
        m_yawState = State::PassThrough;
    }
    else
    {
        m_mainState = State::Enabled;
        m_yawState = m_stickZ == 0 ? State::Enabled : State::PassThrough;
    }

    return std::make_pair(m_mainState, m_yawState);
}

std::pair<bool, double> FBW::SetMode()
{
    if (!!m_flightData->SimOnGround)
    {
        if (m_mode != Mode::OnGround)
        {
            m_mode = Mode::OnGround;
            return std::make_pair(true, 0.0);
        }
    }
    else if (m_flapSelection > 0 && m_flightData->AirspeedTrue < 240.0)
    {
        if (m_mode != Mode::PoweredApproach)
        {
            m_mode = Mode::PoweredApproach;
            return std::make_pair(true, m_flightData->AngleOfAttack - 5.8);
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

        return m_cStar->Calculate(currentValue, desiredValue, 1.0 / frameRate) * 163.83;
    }
    case Mode::UpAndAway:
    {
        auto offsetVal = m_stickY == 0 ? m_levelFlight->Calculate(m_flightData->GForce, 1.0 + (m_flightData->ElevatorTrimPosition / 2.0), 1.0 / frameRate) : (m_flightData->ElevatorTrimPosition / 2.0);

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

        return m_cStar->Calculate(currentValue, desiredValue, 1.0 / frameRate) * 163.83;
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
    return m_roll->Calculate(m_flightData->RollRate, AileronRollRate(m_stickX, m_flightData->AileronTrimPercent * 60.0), 1.0 / frameRate) * 163.83;
}

double FBW::GetCurrentRudder()
{
    return m_sideslip->Calculate(m_flightData->SideslipAngle, 0.0, 1.0 / frameRate) * 163.83;
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

    auto dFlap = FLAP_PER_SEC / static_cast<double>(frameRate);
    //if (dFlap < MIN_FLAP_DELTA) { dFlap = MIN_FLAP_DELTA; }

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

auto fmt = R"(CStar (P=%f, I=%f, D=%f)
previous error %f
total error %f

Level Flight (P=%f, I=%f, D=%f)
previous error %f
total error %f

Rudder (P=%f, I=%f, D=%f)
previous error %f
total error %f

Aileron (P=%f, I=%f, D=%f)
previous error %f
total error %f

FCS State %s
FCS Rudder State %s
FCS Mode %s

Stick X %d
Stick Y %d
Stick Z %d
Flaps %d
Spin Switch %f

TAS (knots) %f
AoA (degrees) %f
G force %f)";

std::string FBW::ToString()
{
    char buf[2048];
    sprintf_s(buf, sizeof(buf), fmt,
        m_cStar->GetKp(),
        m_cStar->GetKi(),
        m_cStar->GetKd(),
        m_cStar->GetPreviousError(),
        m_cStar->GetTotalError(),

        m_levelFlight->GetKp(),
        m_levelFlight->GetKi(),
        m_levelFlight->GetKd(),
        m_levelFlight->GetPreviousError(),
        m_levelFlight->GetTotalError(),

        m_sideslip->GetKp(),
        m_sideslip->GetKi(),
        m_sideslip->GetKd(),
        m_sideslip->GetPreviousError(),
        m_sideslip->GetTotalError(),

        m_roll->GetKp(),
        m_roll->GetKi(),
        m_roll->GetKd(),
        m_roll->GetPreviousError(),
        m_roll->GetTotalError(),

        StateLookup(m_mainState),
        StateLookup(m_yawState),
        ModeLookup(m_mode),

        m_stickX,
        m_stickY,
        m_stickZ,
        m_flapSelection,
        m_spinSwitch->Get(),

        m_flightData->AirspeedTrue,
        m_flightData->AngleOfAttack,
        m_flightData->GForce
        );

    return std::string(buf);
}

#endif

}; // namespace FCS