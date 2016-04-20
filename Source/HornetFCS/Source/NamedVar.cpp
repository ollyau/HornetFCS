/**********************************************************
*                                                         *
*   Copyright (C) Orion Lyau 2016. All Rights Reserved.   *
*                                                         *
**********************************************************/

#include "NamedVar.h"

#include <gauges.h>

namespace Gauge
{

//-----------------------------------------------------------------------------

NamedVar::NamedVar(const char *varName, const double defaultValue) :
    m_VarName(varName),
    m_Value(defaultValue),
    m_set(false),
    m_ID(-1)
{
    m_ID = register_named_variable(m_VarName);
    if (m_ID >= 0)
    {
        set_named_variable_value(m_ID, m_Value);
    }
}

//-----------------------------------------------------------------------------

double NamedVar::Get() const
{
    return m_Value;
}

//-----------------------------------------------------------------------------

std::pair<bool, double> NamedVar::Update()
{
    auto changed = m_set;
    m_set = false;
    auto oldVal = m_Value;
    if (m_ID >= 0)
    {
        m_Value = get_named_variable_value(m_ID);
    }
    return std::make_pair(m_Value != oldVal || changed, m_Value);
}

//-----------------------------------------------------------------------------

void NamedVar::Set(double val)
{
    if (m_ID >= 0)
    {
        m_Value = val;
        set_named_variable_value(m_ID, m_Value);
        m_set = true;
    }
}

//-----------------------------------------------------------------------------

void NamedVar::Toggle()
{
    if (m_ID >= 0)
    {
        m_Value = !get_named_variable_value(m_ID);
        set_named_variable_value(m_ID, m_Value);
        m_set = true;
    }
}

//-----------------------------------------------------------------------------

} // namespace Gauge