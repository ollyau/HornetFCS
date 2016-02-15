#pragma once

#include <memory>
#include <string>

namespace PIDController
{

//-----------------------------------------------------------------------------

class IController
{
public:
    typedef std::shared_ptr<IController> Ptr;

    virtual ~IController() {}

    virtual double GetPreviousError() const = 0;
    virtual double GetTotalError() const = 0;
    virtual double GetKp() const = 0;
    virtual double GetKi() const = 0;
    virtual double GetKd() const = 0;

    virtual double Calculate(double processVariable, double setPoint, double deltaTime) = 0;
    virtual void ResetError() = 0;

    virtual std::string ToString() const = 0;
};

//-----------------------------------------------------------------------------

class PIDControllerCustom
{
public:
    typedef std::shared_ptr<PIDControllerCustom> Ptr;

    PIDControllerCustom() { }
    PIDControllerCustom(double gain_Kp, double gain_Ki, double gain_Kd) :
        m_Kp(gain_Kp), m_Ki(gain_Ki), m_Kd(gain_Kd) { }
    PIDControllerCustom(double gain_Kp, double gain_Ki, double gain_Kd, double _minVal, double _maxVal) :
        m_Kp(gain_Kp), m_Ki(gain_Ki), m_Kd(gain_Kd), m_clipMinVal(_minVal), m_clipMaxVal(_maxVal), m_constrainOutputValues(true) { }

    double GetPreviousError() const { return m_previousError; };
    double GetTotalError() const { return m_cumulativeError; };
    double GetKp() const { return m_Kp; };
    double GetKi() const { return m_Ki; };
    double GetKd() const { return m_Kd; };

    double Calculate(double processVariable, double setPoint, double deltaTime, double ki);
    void ResetError();

    std::string ToString() const;
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

class Factory
{
public:
    static IController::Ptr Make();
    static IController::Ptr Make(double gain_Kp, double gain_Ki, double gain_Kd);
    static IController::Ptr Make(double gain_Kp, double gain_Ki, double gain_Kd, double _minVal, double _maxVal);
};

//-----------------------------------------------------------------------------

} // namespace PIDController