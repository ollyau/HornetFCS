#pragma once

#include "gauges.h"

#include <memory>
#include <utility>

class NamedVar
{
public:
    typedef std::shared_ptr<NamedVar> Ptr;

    NamedVar(const char* varName, const double defaultValue = 0.0)
        : m_VarName(varName), m_Value(defaultValue), m_set(false), m_ID(-1)
    {
        m_ID = register_named_variable(m_VarName);
        if (m_ID >= 0)
        {
            set_named_variable_value(m_ID, m_Value);
        }
    }

    void Set(double val)
    {
        if (m_ID >= 0)
        {
            m_Value = val;
            set_named_variable_value(m_ID, m_Value);
            m_set = true;
        }
    }

    std::pair<bool, double> Update()
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

    double Get() const
    {
        return m_Value;
    }

    void Toggle()
    {
        if (m_ID >= 0)
        {
            m_Value = !get_named_variable_value(m_ID);
            set_named_variable_value(m_ID, m_Value);
            m_set = true;
        }
    }

private:
    ID m_ID;
    bool m_set;
    double m_Value;
    const char* m_VarName;
};