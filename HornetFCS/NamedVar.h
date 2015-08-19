#pragma once

#include "gauges.h"

#include <memory>
#include <utility>

class NamedVar
{
public:
    typedef std::shared_ptr<NamedVar> Ptr;

    NamedVar(const char* varName, const double defaultValue = 0)
        : m_VarName(varName), m_Value(defaultValue), m_ID(-1)
    {
        m_ID = register_named_variable(m_VarName);
        Set(defaultValue);
    }

    void Set(double val)
    {
        if (m_ID >= 0)
        {
            m_Value = val;
            set_named_variable_value(m_ID, m_Value);
        }
    }

    std::pair<bool, double> Update()
    {
        auto oldVal = m_Value;
        if (m_ID >= 0)
        {
            m_Value = get_named_variable_value(m_ID);
        }
        return std::make_pair(m_Value != oldVal, m_Value);
    }

    double Get()
    {
        return m_Value;
    }

private:
    ID m_ID;
    double m_Value;
    const char* m_VarName;
};