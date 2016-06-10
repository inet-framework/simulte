// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 

package lte.stack.d2dModeSelection;

//
// Interface for D2D Mode Selection module
//
moduleinterface D2DModeSelection {
    parameters:
        @display("i=block/cogwheel");
}

//
// Base module for the D2D Mode Selection
//
// Extends this module for implementing new mode selection policies.
// It runs periodically the mode selection algorithm and, for flows 
// that must be switched, sends a notification to the transmitting 
// endpoint of those flows
//
simple D2DModeSelectionBase like D2DModeSelection
{
    parameters:
        double modeSelectionPeriod @unit(s) = default(1s);
}

//
// D2DModeSelectionBestCqi module
// 
// For each D2D pair, selects the mode having the best CQI
//
simple D2DModeSelectionBestCqi extends D2DModeSelectionBase
{
    parameters:
        @class("D2DModeSelectionBestCqi");
}