#pragma once

#include "gauges.h"

#include <cassert>
#include <memory>

class AircraftVar
{
public:
    typedef std::shared_ptr<AircraftVar> Ptr;

    AircraftVar(const char* varName, ENUM unitsEnum)
        : m_unitsEnum(unitsEnum)
    {
        m_varEnum = get_aircraft_var_enum(varName);
        assert(m_varEnum >= 0);
    }

    AircraftVar(const char* varName, const char* unitName)
    {
        m_varEnum = get_aircraft_var_enum(varName);
        m_unitsEnum = get_units_enum(unitName);
        assert(m_varEnum >= 0);
        assert(m_unitsEnum >= 0);
    }

    double Value() const { return m_value; }

    double Get(int index = 0)
    {
        m_value = aircraft_varget(m_varEnum, m_unitsEnum, index);
        return m_value;
    }

    double Get(const char* unitName, int index = 0)
    {
        auto unit = get_units_enum(unitName);
        assert(unit >= 0);
        m_value = aircraft_varget(m_varEnum, unit, index);
        return m_value;
    }

private:
    ENUM m_varEnum;
    ENUM m_unitsEnum;
    double m_value;
};