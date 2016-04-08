// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 

package lte.stack.compManager.compManagerProportional;

import lte.stack.compManager.LteCompManagerBase;

//
// LteCompManagerProportional
//
// This module implements a simple CoMP Coordinated Scheduling algorithm.
// A CompManager module inside an eNodeB can be either a master or a slave. 
// Slave nodes send to the master node the amount of RBs required to satisfy
// their users' load. The master node partitions the bandwidth based on these
// requests and sends back the indication of which RBs are available for 
// transmission and which not.
simple LteCompManagerProportional extends LteCompManagerBase
{
    parameters:
        @display("i=block/cogwheel");
        @class("LteCompManagerProportional");
}
