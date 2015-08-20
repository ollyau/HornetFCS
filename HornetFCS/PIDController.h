#pragma once

#include <memory>

class PIDController
{
public:
    typedef std::shared_ptr<PIDController> Ptr;

    PIDController() { }
    PIDController(double gain_Kp, double gain_Ki, double gain_Kd) :
        m_Kp(gain_Kp), m_Ki(gain_Ki), m_Kd(gain_Kd) { }
    PIDController(double gain_Kp, double gain_Ki, double gain_Kd, double _minVal, double _maxVal) :
        m_Kp(gain_Kp), m_Ki(gain_Ki), m_Kd(gain_Kd), m_clipMinVal(_minVal), m_clipMaxVal(_maxVal), m_constrainOutputValues(true) { }
    
    double GetPreviousError() const { return m_previousError; };
    double GetTotalError() const { return m_cumulativeError; };
    double GetKp() const { return m_Kp; };
    double GetKi() const { return m_Ki; };
    double GetKd() const { return m_Kd; };

    double Calculate(double processVariable, double setPoint, double deltaTime);
    void ResetError();
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