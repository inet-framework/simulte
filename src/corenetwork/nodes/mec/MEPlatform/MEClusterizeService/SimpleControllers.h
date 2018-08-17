//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

//
//  @author Angelo Buono
//

#ifndef CORENETWORK_NODES_MEC_MEPLATFORM_MECLUSTERIZESERVICE_SIMPLECONTROLLERS_H_
#define CORENETWORK_NODES_MEC_MEPLATFORM_MECLUSTERIZESERVICE_SIMPLECONTROLLERS_H_

//simple Controllers with 1 input and 1 output and n-states
//
// A = nxn, B = nx1, C = 1xn, D = 1x1
// at time k --> e_k = 1x1 (input), u_k = 1x1 (output), x_k = nx1 (state)
//
// COMPUTE OUTPUT -->   u_k = C*x_k + D*e_k
//
// COMPUTE NEXT STATE -->   x_k+1 = A*x_k + B*e_k


class SimpleVelocityController
{
    double A, B, C, D, x, u;

    public:
        SimpleVelocityController()
    {
            A = 0.78;
            B = 0.1732;
            C = 0.1219;
            D = 0.044;
            x = 0;
        }

        double getOutput(double e)
        {
            u = D*e;
            return u;
        }
        void updateNextState(double e)
        {
            x = A*x + B*e;
        }
};

class SimpleDistanceController
{
    /*
     * controller C(z) = 0.1 *(z-1)/z --> A=0   B=0.25  C=-0.4  D=0.1
     *                                  controller input max is 30 m to respect the output max of 3 m/s^2 (max acceleration supposed to be 3)
     *                                  settling time is 120s
     */
    double A, B, C, D, x, u;
    bool initialized;

    public:
        SimpleDistanceController()
    {
            A = 0;
            B = 0.25;
            C = -0.4;
            D = 0.1;
            x = 0;
            initialized = false;
        }

        bool isInitialized()
        {
            return initialized;
        }

        void initialize(double x)
        {
            this->x = x;
            initialized = true;
            //testing
            EV << "SimpleDistanceController::initialize - x = [" << x <<"]" << endl;
        }

        double getOutput(double e){
            u = C*x + D*e;
            //testing
            EV << "SimpleDistanceController::getOutput - x = [" << x <<"] u = "<< u << endl;
            return u;
        }
        void updateNextState(double e){
            x = A*x + B*e;
        }
};

class GeneralSpeedController
{
    double period;  //time period of the cycle of actions
    double a_min;   //minimum acceleration
    double a_max;   //maximum acceleration
    double gain;

public:
    GeneralSpeedController(){ this->gain = 0.2;}

    GeneralSpeedController(double period, double a_min, double a_max)
    {
     this->period = period;
     this->a_min = a_min;
     this->a_max = a_max;
     this->gain = 0.2;
    }
    double getAcceleration(double desiredSpeed, double currentSpeed)
    {
        double acceleration = (desiredSpeed - currentSpeed)*gain;
        //limiting the acceleration
        acceleration = (acceleration < a_min)? a_min : (acceleration > a_max)? a_max : acceleration;
        return acceleration;
    }
};

/*
 * Implementing Safe Longitudinal Platoon Formation Controller proposed by Scheuer, Simonin and Charpillet.
 *
 *  Algorithm assures that all vehicles within the platoon reach the desired speed and mantain the same distance;
 *   the inter-vehicle distance depends on the speed, and assures NO COLLISION in the WORST CASE! (when breaking at a_min)
 *
 *   SAFETY CRITERION developed by the algorithm:   lower_bound_next_distance = distanceToLeading - d_crit + min(0, (followerSpeed^2 - leadingSpeed^2)/(2*a_min)) >= 0
 *                                                                                                               |                  |
 *                                                                                                        no collision now!      enough distance to stop!
 *
 */
class SafePlatooningController
{
    double period;  //time period of the cycle of actions
    double a_min;   //minimum acceleration
    double a_max;   //maximum acceleration
    double d_crit;  //critical distance (>0)

public:
    SafePlatooningController(){}
    SafePlatooningController(double period, double a_min, double a_max, double d_crit)
    {
     this->period = period;
     this->a_min = a_min;
     this->a_max = a_max;
     this->d_crit = d_crit;
    }
    double getAcceleration(double distanceToLeading, double followerSpeed, double leadingSpeed)
    {
        double period_2 = period*period;
        //lower bound of next distance between leading and follower vehicles
        double lb_next_distance = distanceToLeading + (leadingSpeed - followerSpeed)*period + ((a_min - a_max)*period_2)/2;
        //lower bound of next leading vehicle speed
        double lb_next_leading_speed = leadingSpeed + a_min*period;
        //upper bound of next follower vehicle speed
        double ub_next_follower_speed = followerSpeed + a_max*period;
        //lower bound of next safety criterion
        double lb_next_safety_criterion = lb_next_distance - d_crit + (std::pow(ub_next_follower_speed, 2) - std::pow(lb_next_leading_speed, 2))/(2*a_min);
        //lower bound of next next safety criterion
        double lb_next_next_safety_criterion = std::max( 0.0, lb_next_safety_criterion - ((a_max - a_min)*(ub_next_follower_speed + (a_max*period)/2)*period)/(-a_min) ) + (a_max - a_min)*period_2;

        //possible accelerations
        //insures next_next_distance >= d_crit
        double a1 = a_min + 2*(lb_next_distance - d_crit + ((lb_next_leading_speed - ub_next_follower_speed)*period))/(2*period_2);
        //insures that next next safety criterion >= 0
        double a2 = (std::sqrt((ub_next_follower_speed - (a_min*period)/2) - 2*a_min*lb_next_safety_criterion) - (ub_next_follower_speed - 1.5*a_min*period))/period;
        //insures that lower bound of next safety criterion >= 0
        double a3 = (std::sqrt(std::pow(ub_next_follower_speed + (a_max - a_min/2)*period, 2) - 2*a_min*lb_next_next_safety_criterion))/period - (ub_next_follower_speed + (a_max - 1.5*a_min)*period)/period;
        //chosing the minimum
        double acceleration = std::min(std::min(a1, a2), a3);
        //limiting the acceleration
        acceleration = (acceleration < a_min)? a_min : (acceleration > a_max)? a_max : acceleration;
        return acceleration;
    }
};

#endif /* CORENETWORK_NODES_MEC_MEPLATFORM_MECLUSTERIZESERVICE_SIMPLECONTROLLERS_H_ */
