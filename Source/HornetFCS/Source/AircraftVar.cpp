/**********************************************************
*                                                         *
*   Copyright (C) Orion Lyau 2016. All Rights Reserved.   *
*                                                         *
**********************************************************/

#include "AircraftVar.h"

#include <cassert>
#include <gauges.h>

namespace Gauge
{

//-----------------------------------------------------------------------------

AircraftVar::AircraftVar(const char* varName, int unitsEnum) :
    m_unitsEnum(unitsEnum)
{
    m_varEnum = get_aircraft_var_enum(varName);
    assert(m_varEnum >= 0);
}

//-----------------------------------------------------------------------------

AircraftVar::AircraftVar(const char* varName, const char* unitName)
{
    m_varEnum = get_aircraft_var_enum(varName);
    m_unitsEnum = get_units_enum(unitName);
    assert(m_varEnum >= 0);
    assert(m_unitsEnum >= 0);
}

//-----------------------------------------------------------------------------

double AircraftVar::Value() const
{
    return m_value;
}

//-----------------------------------------------------------------------------

double AircraftVar::Get(int index)
{
    m_value = aircraft_varget(m_varEnum, m_unitsEnum, index);
    return m_value;
}

//-----------------------------------------------------------------------------

double AircraftVar::GetUnits(int unitEnum, int index)
{
    m_value = aircraft_varget(m_varEnum, unitEnum, index);
    return m_value;
}

//-----------------------------------------------------------------------------

double AircraftVar::GetUnits(const char* unitName, int index)
{
    auto unit = get_units_enum(unitName);
    assert(unit >= 0);
    m_value = aircraft_varget(m_varEnum, unit, index);
    return m_value;
}

//-----------------------------------------------------------------------------

} // namespace Gauge