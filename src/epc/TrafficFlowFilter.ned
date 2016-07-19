//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

package lte.epc;

//
// TODO auto-generated module
//
simple TrafficFlowFilter
{
    parameters:
        @display("i=block/filter");

        string filterFileName;
        string ownerType; // must be one between ENODEB or PGW
    gates:
        input internetFilterGateIn;

        output gtpUserGateOut;
}

simple TrafficFlowFilterSimplified
{
    parameters:
        @display("i=block/filter");

        string ownerType; // must be one between ENODEB or GW
        bool fastForwarding = default(true);
    gates:
        input internetFilterGateIn;
        output gtpUserGateOut;    
}