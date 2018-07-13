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


class SimpleVelocityController{

    double A, B, C, D, x, u;

    public:
        SimpleVelocityController(){
            A = 0;
            B = 0;
            C = 0;
            D = 0.1;
            x = 0;
        }

        double getOutput(double e){
            u = D*e;
            u = ceil( (int)(u*1000)) / 1000.00;
            return u;
        }
        void updateNextState(double e){
            x = A*x + B*e;
            x = ceil( (int)(x*1000)) / 1000.00;
        }
};

class SimpleDistanceController{

    double A, B, C, D, x, u;
    bool initialized;

    public:
        SimpleDistanceController(){

            A = 0;
            B = 0.5;
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
            u = ceil( (int)(u*1000)) / 1000.00;
            //testing
            EV << "SimpleDistanceController::getOutput - x = [" << x <<"] u = "<< u << endl;
            return u;
        }
        void updateNextState(double e){
            x = A*x + B*e;
            x = ceil( (int)(x*1000)) / 1000.00;
        }
};

#endif /* CORENETWORK_NODES_MEC_MEPLATFORM_MECLUSTERIZESERVICE_SIMPLECONTROLLERS_H_ */
