//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

//
//  @author Angelo Buono
//

package lte.corenetwork.nodes.mec.MEPlatform.MEClusterizeService;

import lte.corenetwork.nodes.mec.MEPlatform.IMEService;

//
// TODO auto-generated module
//

simple MEClusterizeService like IMEService
{
    parameters:
        
        double period @unit("s") = default(0.2s);
        double startTime @unit("s") = default(0s);
        
        double proximityThreshold @unit("m");                  // meter: threshold within two cars can communicate v2v
    	double directionDelimiterThreshold @unit("rad");         // radiant: threshold within two cars are going in the same direction

        
    gates:
        
        input meClusterizeAppIn[];
        output meClusterizeAppOut[];
}
