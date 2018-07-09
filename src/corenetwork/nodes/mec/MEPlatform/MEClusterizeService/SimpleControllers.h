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
            A=0;
            B=0;
            C=0;
            D=0.1;
            x=0;
        }

        double getOutput(double e){
            u = D*e;
            return u;
        }
        void updateNextState(double e){
            x = A*x + B*e;
        }
};

class SimpleDistanceController{

    double A[2][2], B[2], C[2], D, x[2], u;

    public:
        SimpleDistanceController(){

            A[0][0] = 0;
            A[0][1] = -0.7348;
            A[1][0] = 0;
            A[1][1] = 0.4;

            B[0] = 1.897;
            B[1] = 1.549;

            C[0] = -1.423;
            C[1] = -1.162;

            D = 3;

            for(int j=0; j<2; j++)
                x[j] = 0;
        }

        double getOutput(double e){
            u = C[0]*x[0] + C[1]*x[1] +  D*e;
            return u;
        }
        void updateNextState(double e){
            x[0] = A[0][0]*x[0] + A[0][1]*x[1] + B[0]*e;
            x[1] = A[1][0]*x[0] + A[1][1]*x[1] + B[1]*e;
        }
};

#endif /* CORENETWORK_NODES_MEC_MEPLATFORM_MECLUSTERIZESERVICE_SIMPLECONTROLLERS_H_ */
