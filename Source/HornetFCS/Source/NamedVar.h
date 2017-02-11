/**********************************************************
*                                                         *
*   Copyright (C) Orion Lyau 2016. All Rights Reserved.   *
*                                                         *
**********************************************************/

#pragma once

#include <memory>
#include <utility>

namespace Gauge
{

//-----------------------------------------------------------------------------

class NamedVar
{
public:
    typedef std::shared_ptr<NamedVar> Ptr;

    NamedVar(const char *varName, const double defaultValue = 0.0);

    double Get() const;
    std::pair<bool, double> Update();

    void Set(double val);
    void Toggle();

private:
    int m_ID;
    bool m_set;
    double m_Value;
    const char *m_VarName;
};

//-----------------------------------------------------------------------------

} // namespace Gauge