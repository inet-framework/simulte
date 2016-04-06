// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 

package lte.stack.compManager;

import lte.x2.X2User;

//
// Interface for the Comp Manager
//
moduleinterface LteCompManager extends X2User {
    parameters:
        @display("i=block/cogwheel");
}

simple LteCompManagerBase like LteCompManager
{
    parameters:
        @display("i=block/cogwheel");
        @class("LteCompManagerBase");
        string compNodeType = default("COMP_CLIENT");  // one of COMP_CLIENT, COMP_CLIENT_COORDINATOR, COMP_COORDINATOR

        @statistic[compReservedBlocks](title="CoMP Reserved Blocks"; unit=""; source="compReservedBlocks"; record=mean, vector);
    gates:
        //# connections to the X2 Manager
        input x2ManagerIn;
        output x2ManagerOut;
}

//
// LteCompManagerFlex
//
// This module implements a the CoMP/CS algorithm of Flex5Gware
//
//simple LteCompManagerFlex extends LteCompManagerBase
//{
//    parameters:
//        @display("i=block/cogwheel");
//        @class("LteCompManagerFlex");
//        
//}
