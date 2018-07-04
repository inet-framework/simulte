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

//
//	Clustering Algorithm based on Geometrical Shapes
//

simple MEPlatooningService extends MEClusterizeService
{
    parameters:
        @class("MEPlatooningService");
        
        string shape = default("rectangle");		//shape supported "rectangle" and "triangle"
        
        double proximityThreshold @unit("m");                  // meter: threshold within two cars can communicate v2v
    	double directionDelimiterThreshold @unit("rad");         // radiant: threshold within two cars are going in the same direction

        double roadLaneSize @unit("m");
        double triangleAngle @unit("rad");
        
        double desiredVelocity @unit("mps");
        double desiredDistance @unit("m");
}