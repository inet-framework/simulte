//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

//
//	@author	Angelo Buono
//


package lte.corenetwork.nodes.mec;

import lte.corenetwork.nodes.mec.VirtualisationInfrastructure.VirtualisationInfrastructure;
import lte.corenetwork.nodes.mec.MEPlatform.MEPlatform;

//lte tunneling endpoint to connect externally with an eNodeB
import lte.epc.PgwStandardSimplified;
import inet.node.ethernet.Eth10G;

// 
//# MEHost module
//
//

module MEHost
{
    parameters:
        @display("i=device/pc2;bgb=352.17334,262.70667");

        //# Node specs
        string nodeType = "MEHost";						// DO NOT CHANGE

        //#	ME Host Resources
        int maxMEApps = default(0);
        double maxRam @unit("B") = default(32GB);
        double maxDisk @unit("B") = default(100TB);
        double maxCpu = default(1);						// percentage of available CPU

        //# Supported ME Service Paths
        //
        // adding one new string for each new service & configure in the ini file!
        //
        string MEClusterizeServicePath;					// i.e. ".mePlatform.udpService[0]"


    gates:

        inout ppp[];

    submodules:

        virtualisationInfrastructure: VirtualisationInfrastructure {
            parameters:
                @display("p=243.18668,169.98668;is=s");
        }

        mePlatform: MEPlatform {
            parameters:
                @display("p=243.18668,82.14667;is=s");
        }

        //gtp endpoint to connect directly with an eNodeB

    connections allowunconnected:

		ppp++ <--> virtualisationInfrastructure.ppp++;
        //virtualisationInfrastructure.ppp++ <--> Eth10G <--> gtpEndpoint.filterGate;
		//allowing external connection with eNodeB
        //ppp++ <--> gtpEndpoint.pppg++;
}
