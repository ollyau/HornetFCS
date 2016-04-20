/**********************************************************
*                                                         *
*   Copyright (C) Orion Lyau 2016. All Rights Reserved.   *
*                                                         *
**********************************************************/

#pragma once

#include <memory>

namespace Gauge
{

//-----------------------------------------------------------------------------

class AircraftVar
{
public:
    typedef std::shared_ptr<AircraftVar> Ptr;

    AircraftVar(const char *varName, int unitsEnum);
    AircraftVar(const char *varName, const char *unitName);

    double Value() const;

    double Get(int index = 0);
    double GetUnits(int unitEnum, int index = 0);
    double GetUnits(const char *unitName, int index = 0);

private:
    int m_varEnum;
    int m_unitsEnum;
    double m_value;
};

//-----------------------------------------------------------------------------

} // namespace Gauge