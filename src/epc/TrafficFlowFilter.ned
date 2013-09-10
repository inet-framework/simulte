//
//                           SimuLTE
// Copyright (C) 2013 Antonio Virdis
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
        string ownerType; // must be one between ENB or PGW
    gates:
        input internetFilterGateIn;

        output gtpUserGateOut;
}
