// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 

package lte.x2;

//
// LteX2Manager
//
// The task of this module is to allow communications between eNodeBs via the X2 interface.
// It acts as a bridge between the X2 user modules and the X2Apps that are responsible 
// for communication with the X2Apps on the peering eNodeBs. 
// If you want to implement a new module that needs to exploit the X2 interface, you must
// connect the module to the dataPort gate of the LteX2Manager. 
//
simple LteX2Manager
{
    parameters:
        @display("i=block/cogwheel");
        
    gates:
        inout dataPort[]; // connection to X2 user modules
        inout x2[];       // connections to X2App modules
        inout x2Gtp;      // connections to GtpUserX2 module
}