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

package lte.corenetwork.nodes.mec.MEPlatform.MEWarningAlertService;

import lte.corenetwork.nodes.mec.MEPlatform.IMEService;

//
// MEWarningAlertService connected to the MEWarningAlertApp
//
// When receives the INFO_UEAPP from the UEWarningAlertApp checks if the UE is within the danger area 
// defined by the quadrilateral dangerEdgeA - dangerEdgeB - dangerEdgeC - dangerEdgeD
// if UE IS INSIDE then REPLAY with INFO_MEAPP WarningAlertPacket with danger == TRUE else danger == FALSE

simple MEWarningAlertService like IMEService
{
    parameters:
        
        double dangerEdgeAx @unit("m");
        double dangerEdgeAy @unit("m");
        double dangerEdgeAz @unit("m") = default(0);
        double dangerEdgeBx @unit("m");
        double dangerEdgeBy @unit("m");
        double dangerEdgeBz @unit("m") = default(0);
        double dangerEdgeCx @unit("m");
        double dangerEdgeCy @unit("m");
        double dangerEdgeCz @unit("m") = default(0);
        double dangerEdgeDx @unit("m");
        double dangerEdgeDy @unit("m");
        double dangerEdgeDz @unit("m") = default(0);
        
    gates:
       
        input meAppIn[];
        output meAppOut[];     
}
