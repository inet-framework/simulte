// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 

package lte.stack.handoverManager;

import lte.x2.X2User;

//
// Interface for the Handover Manager
//
moduleinterface ILteHandoverManager extends X2User {
    parameters:
        @display("i=block/cogwheel");
}

simple LteHandoverManager like ILteHandoverManager
{
    parameters:
        @display("i=block/cogwheel");
        @class("LteHandoverManager");
        
        bool losslessHandover = default(false);
        
    gates:
        //# connections to the X2 Manager
        input x2ManagerIn;
        output x2ManagerOut;
}
