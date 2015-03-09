// 
//                           SimuLTE
// Copyright (C) 2015 Antonio Virdis, Giovanni Nardini, Daniele Migliorini, Giovanni
// Accongiagioco, Generoso Pagano, Vincenzo Pii.
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 

package lte.stack.compManager;

//
// LteCompManager
//
// This module implements a simple CoMP Coordinated Scheduling algorithm.
// A CompManager module inside an eNodeB can be either a master or a slave. 
// Slave nodes send to the master node the amount of RBs required to satisfy
// their users' load. The master node partitions the bandwidth based on these
// requests and sends back the indication of which RBs are available for 
// transmission and which not.
simple LteCompManager
{
    parameters:
        @display("i=block/cogwheel");
        string nodeType = default("COMP_SLAVE");  // one of COMP_MASTER, COMP_SLAVE
        
        //# COMP_SLAVE parameters
        int masterId;							  // id of the master node
        
        //# COMP_MASTER-parameters
        string slavesList = default("");          // list of ids of slave nodes, separated by blank spaces
 
        @statistic[compReservedBlocks](title="CoMP Reserved Blocks"; unit=""; source="compReservedBlocks"; record=mean, vector);
        
    gates:
        //# connections to the X2 Manager
        input x2ManagerIn;
        output x2ManagerOut;
}