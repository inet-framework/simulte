// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 

package lte.corenetwork.nodes;

// 
// ExtCell Module
//
simple ExtCell
{
    parameters:
        @display("i=device/antennatower;bgb=300,300");

        // coordinates
        int position_x @unit("m") = default(0m);
        int position_y @unit("m") = default(0m);
        
        // transmission power
        double txPower = default(46);
        		
        // transmission direction
        string txDirection = default("OMNI");
        
        // transmission angle
        double txAngle = default(0);
        
        // number of logical bands
        int numBands = default(1);
        
        // --- ALLOCATION MANAGEMENT --- //
        string bandAllocationType = default("FULL_ALLOC");
        double bandUtilization = default(0.5);    
        int startingOffset = default(0);
        // ----------------------------- //
}
