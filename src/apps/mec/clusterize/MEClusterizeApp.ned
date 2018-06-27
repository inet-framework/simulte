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
//		1) Sending ClusterizeCommand about cluster creation and in-cluster communication policies
//
//#	Receiver Side:
//		1) Receiving car-local infos and communicating to the ME ClusterizeService
//
simple MEClusterizeApp like IMEApp
{
    parameters:
        @display("i=block/app;is=s");
        
        double ram @unit("B") = default(10MB);
        double disk @unit("B") = default(0.1GB);
        double cpu = default(0.001);
        
        string v2vAppName = default(""); 					//to differentiate the instance based on the V2V Sender Class Name
                
        string sourceAddress = default("");				//meHost.virtualisationInfrastructure
        string destAddress = default("");					//car[x]
        int packetSize = default(10);
        
        //double period @unit("s") = default(0.02s);
        //double startTime @unit("s") = default(0s);
        
        string interfaceTableModule = default("");
               
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
