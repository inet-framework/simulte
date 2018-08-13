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

package lte.apps.mec.clusterize;

import lte.corenetwork.nodes.mec.MEPlatform.IMEApp;

//
//#	MEClusterizeApp
//
//	Simple Module for an ME App communicating with a UE App on a Car.
//	The task of this module is:
//#	Sender Side:
//		1) Sending INFO_MEAPP packet with cluster informations computed by the MEClusterizeService
//		2) Forwarding STOP_MEAPP packet to the MEClusterizeService to clean its data structures
//#	Receiver Side:
//		1) Receiving INFO_UEAPP packet with car-local informations and forwarding to the MEClusterizeService
//
simple MEClusterizeApp like IMEApp
{
    parameters:
        @display("i=block/app;is=s");
        
        //packet informations
        int packetSize = default(10);
        
        //communication with UEClusterizeApp
        string ueSimbolicAddress; 		// i.e. UE[x] or car[x]
        string meHostSimbolicAddress;	// i.e meHost.virtualisationInfrastructure
        
        string interfaceTableModule = default("");
        
        //statistics
        //#Sender Side       
        @signal[clusterizeConfigSentMsg];
        @statistic[clusterizeConfigSentMsg](title="ClusterizeConfig messages sent"; unit=""; source="clusterizeConfigSentMsg"; record=sum,vector);
        
		//#Receiver Side
        @signal[clusterizeInfoRcvdMsg]; 
        @statistic[clusterizeInfoRcvdMsg](title="ClusterizeInfo Messages Received"; unit="s"; source="clusterizeInfoRcvdMsg"; record=sum,vector);
        @signal[clusterizeInfoDelay]; 
        @statistic[clusterizeInfoDelay](title="ClusterizeInfo Delay"; unit="s"; source="clusterizeInfoDelay"; record=mean,vector);
        
    gates:
        
        input virtualisationInfrastructureIn;
        output virtualisationInfrastructureOut;
        
        input mePlatformIn;
        output mePlatformOut;
}
