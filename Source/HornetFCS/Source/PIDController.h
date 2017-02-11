/**********************************************************
*                                                         *
*   Copyright (C) Orion Lyau 2016. All Rights Reserved.   *
*                                                         *
**********************************************************/

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
    virtual double CalculateCustom(double processVariable, double setPoint, double deltaTime, double ki) = 0;
    virtual void ResetError() = 0;

    virtual std::string ToString() const = 0;
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