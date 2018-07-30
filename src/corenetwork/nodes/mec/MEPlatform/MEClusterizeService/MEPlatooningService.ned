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
//#	MEPlatooningService
//
//	Simple Module for a ME Service extending the MEClusterizeService
//
//		implementing the compute() with a platoon formation algorithm

simple MEPlatooningService extends MEClusterizeService
{
    parameters:
        @class("MEPlatooningService");
        
        //algorithm parameters
        string shape = default("rectangle");				//area shape for checking car adjacences: "rectangle" and "triangle"
        //direction
    	double directionDelimiterThreshold @unit("rad");	//threshold to consider two cars going on the same direction
		//area dimensions (depending on the shape)
        double proximityThreshold @unit("m");				//threshold to consider two cars adjacent on the same direction
        double roadLaneSize @unit("m");						//rectangle base ortogonal to the car direction
        double triangleAngle @unit("rad");					//triangle angle aroung car direction
        //cluster mobility
        double desiredVelocity @unit("mps");
        double criticalDistance @unit("m");
        double maxAcceleration;
        double minAcceleration;
}