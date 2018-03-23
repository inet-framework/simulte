// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 

package lte.apps.d2dMultihop.eventGenerator;

//
// EventGenerator module
//
// This module generate periodic events (e.g. a collision between 
// vehicles) in random positions of the network (or randomly 
// selecting a node) and notifies a MultihopD2D application 
// via direct method call
//
simple EventGenerator 
{
    parameters:      
        double startTime @unit(s) = default(0.1s);
        double eventPeriod @unit(s) = default(1s);
        bool singleEventSource = default(true);
        
        bool forceStartingCoord = default(false);
        double startingCoordX @unit(m) = default(-1.0m);
        double startingCoordY @unit(m) = default(-1.0m);
        
        int startingUe = default(0);
        
        @display("i=block/cogwheel");
}