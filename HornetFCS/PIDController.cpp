#include "stdafx.h"

#include "PIDController.h"

namespace PID
{

double ClipValue(double input, double min, double max)
{
    if (input > max) { return max; }
    else if (input < min) { return min; }
    else { return input; }
}

}

double PIDController::Calculate(double processVariable, double setPoint, double deltaTime)
{
    double currentError = setPoint - processVariable;
    double integral = (currentError + m_previousError) * deltaTime / 2.0;

    double P = m_Kp * currentError;
    double I = m_Ki * (integral + m_totalError); // trapezoidal
    double D = m_Kd * (currentError - m_previousError) / deltaTime;

    if (std::abs(I) > m_integralMax) {
        I = m_integralMax;
    }
    else if (I < m_integralMin) {
        I = m_integralMin;
    }
    else {
        m_totalError += integral;
    }

    double output = P + I + D;

    m_previousError = currentError;

    if (m_constrainOutputValues)
    {
        output = PID::ClipValue(output, m_clipMinVal, m_clipMaxVal);
    }

    return output;
}

void PIDController::ResetError()
{
    m_previousError = 0.0;
    m_totalError = 0.0;
}