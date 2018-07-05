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
// COMPUTE OUTPUT -->   u_k = c*x_k + D*e_k
//
// COMPUTE NEXT STATE -->   x_k+1 = A*x_k + B*e_k


class SimpleVelocityController{

    double A, B, C, D, x, u;

    public:
        SimpleVelocityController(){
            A=0;
            B=0;
            C=0;
            D=0.165;
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

            for(int i=0; i<2; i++)
                for(int j=0; j<2; j++)
                    A[i][j] = 0;

            for(int j=0; j<2; j++)
                B[j] = 0;

            for(int j=0; j<2; j++)
                C[j] = 0;

            D=0.5;

            for(int j=0; j<2; j++)
                x[j] = 0;
        }

        double getOutput(double e){
            u = D*e;
            return u;
        }
        void updateNextState(double e){

        }
};

#endif /* CORENETWORK_NODES_MEC_MEPLATFORM_MECLUSTERIZESERVICE_SIMPLECONTROLLERS_H_ */
