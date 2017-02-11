/**********************************************************
*                                                         *
*   Copyright (C) Orion Lyau 2016. All Rights Reserved.   *
*                                                         *
**********************************************************/

#include "PIDController.h"

#include <memory>
#include <sstream>

namespace PIDController
{

//-----------------------------------------------------------------------------

double ClipValue(double input, double min, double max)
{
    if (input > max) { return max; }
    else if (input < min) { return min; }
    else { return input; }
}

//-----------------------------------------------------------------------------

class PIDController : public IController
{
public:
    typedef std::shared_ptr<PIDController> Ptr;

    PIDController() {}
    PIDController(double gain_Kp, double gain_Ki, double gain_Kd);
    PIDController(double gain_Kp, double gain_Ki, double gain_Kd, double _minVal, double _maxVal);

    virtual double GetPreviousError() const;
    virtual double GetTotalError() const;
    virtual double GetKp() const;
    virtual double GetKi() const;
    virtual double GetKd() const;

    virtual double Calculate(double processVariable, double setPoint, double deltaTime);
    virtual double CalculateCustom(double processVariable, double setPoint, double deltaTime, double ki);
    virtual void ResetError();

    virtual std::string ToString() const;
private:
    double m_previousError = 0.0;
    double m_cumulativeError = 0.0;
    double m_Kp = 1.0;
    double m_Ki = 1.0;
    double m_Kd = 1.0;
    double m_clipMinVal = -100.0;
    double m_clipMaxVal = 100.0;
    const double m_integralMax = 100.0;
    const double m_integralMin = -100.0;
    bool m_constrainOutputValues = false;
};

//-----------------------------------------------------------------------------

PIDController::PIDController(double gain_Kp, double gain_Ki, double gain_Kd) :
    m_Kp(gain_Kp),
    m_Ki(gain_Ki),
    m_Kd(gain_Kd)
{}

//-----------------------------------------------------------------------------

PIDController::PIDController(double gain_Kp, double gain_Ki, double gain_Kd, double _minVal, double _maxVal) :
    m_Kp(gain_Kp),
    m_Ki(gain_Ki),
    m_Kd(gain_Kd),
    m_clipMinVal(_minVal),
    m_clipMaxVal(_maxVal),
    m_constrainOutputValues(true)
{}

//-----------------------------------------------------------------------------

double PIDController::GetPreviousError() const
{
    return m_previousError;
}

//-----------------------------------------------------------------------------

double PIDController::GetTotalError() const
{
    return m_cumulativeError;
}

//-----------------------------------------------------------------------------

double PIDController::GetKp() const
{
    return m_Kp;
}

//-----------------------------------------------------------------------------

double PIDController::GetKi() const
{
    return m_Ki;
}

//-----------------------------------------------------------------------------

double PIDController::GetKd() const
{
    return m_Kd;
}

//-----------------------------------------------------------------------------

double PIDController::Calculate(double processVariable, double setPoint, double deltaTime)
{
    double currentError = setPoint - processVariable;
    double dt = (currentError + m_previousError) * deltaTime / 2.0;

    double P = m_Kp * currentError;
    double I = m_Ki * (m_cumulativeError + dt);
    double D = m_Kd * (currentError - m_previousError) / deltaTime;

    if (I > m_integralMax)
    {
        I = m_integralMax;
    }
    else if (I < m_integralMin)
    {
        I = m_integralMin;
    }
    else
    {
        m_cumulativeError += dt;
    }

    double output = P + I + D;

    m_previousError = currentError;

    if (m_constrainOutputValues)
    {
        output = ClipValue(output, m_clipMinVal, m_clipMaxVal);
    }

    return output;
}

//-----------------------------------------------------------------------------

double PIDController::CalculateCustom(double processVariable, double setPoint, double deltaTime, double ki)
{
    double currentError = setPoint - processVariable;
    double dt = (currentError + m_previousError) * deltaTime / 2.0;

    double P = m_Kp * currentError;
    double I = m_Ki * (m_cumulativeError + dt) * ki;
    double D = m_Kd * (currentError - m_previousError) / deltaTime;

    if (I > m_integralMax)
    {
        I = m_integralMax;
    }
    else if (I < m_integralMin)
    {
        I = m_integralMin;
    }
    else
    {
        m_cumulativeError += dt;
    }

    double output = P + I + D;

    m_previousError = currentError;

    if (m_constrainOutputValues)
    {
        output = ClipValue(output, m_clipMinVal, m_clipMaxVal);
    }

    return output;
}

//-----------------------------------------------------------------------------

void PIDController::ResetError()
{
    m_previousError = 0.0;
    m_cumulativeError = 0.0;
}

//-----------------------------------------------------------------------------

std::string PIDController::ToString() const
{
    std::ostringstream ss;
    ss << "P=" << m_Kp << ", I=" << m_Ki << ", D=" << m_Kd << std::endl;
    ss << "Previous Error: " << m_previousError << std::endl;
    ss << "Cumulative Error: " << m_cumulativeError << std::endl;
    return ss.str();
}

//-----------------------------------------------------------------------------

IController::Ptr Factory::Make()
{
    return std::make_shared<PIDController>();
}

//-----------------------------------------------------------------------------

IController::Ptr Factory::Make(double gain_Kp, double gain_Ki, double gain_Kd)
{
    return std::make_shared<PIDController>(gain_Kp, gain_Ki, gain_Kd);
}

//-----------------------------------------------------------------------------

IController::Ptr Factory::Make(double gain_Kp, double gain_Ki, double gain_Kd, double minVal, double maxVal)
{
    return std::make_shared<PIDController>(gain_Kp, gain_Ki, gain_Kd, minVal, maxVal);
}

//-----------------------------------------------------------------------------

} // namespace PID