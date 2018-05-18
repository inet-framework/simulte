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


package lte.apps.vehicular.mec.clusterize;

import inet.applications.contract.IUDPApp;

//
//#	UEClusterizeApp
//
//	Simple Module for an UE App communicating with a ME App on a ME HOST.
//	The task of this module is:
//#	Sender Side:
//		1) Instantiating the correspondent ME App on the ME HOST (through the ME Orchestrator)
//		2) Reporting periodically local information to the correspondent ME App instance (User App)
//
//#	Receiver Side:
//		1) Receiving commands to join/leave clusters and transmisison policies, from the correspondent ME App (User App)
//		2) Configuring the V2V-Supported App to communicate using the transmission policies defined on the ME HOST!
//
simple UEClusterizeApp like IUDPApp
{
        parameters:
        
        string v2vAppPath = default("");		//specify module path in the Car (parent module) of the v2vAppSender UDP App!  --> i.e. ".udpApp[1]"
    
        int localPort = default(4000);
        int destPort = default(4000);
        string destAddress;
        int packetSize = default(10);
        
        string multicastGoupAddress = default("224.0.0.10");
        
        
        double period @unit("s") = default(0.02s);
        double startTime @unit("s") = default(0s); 
        double stopTime @unit("s") = default(20s);
        
        string interfaceTableModule;
        
        //#Sender Side
        @signal[clusterizeInfoSentMsg];
        @statistic[clusterizeInfoSentMsg](title="ClusterizeInfo messages sent"; unit=""; source="clusterizeInfoSentMsg"; record=sum,vector);

		//#Receiver Side          
        @signal[clusterizeConfigRcvdMsg]; 
        @statistic[clusterizeConfigRcvdMsg](title="ClusterizeConfig Messages Received"; unit="s"; source="clusterizeConfigRcvdMsg"; record=sum,vector);
                        
        @signal[clusterizeConfigDelay]; 
        @statistic[clusterizeConfigDelay](title="ClusterizeConfig Delay"; unit="s"; source="clusterizeConfigDelay"; record=mean,vector);
              
        @display("i=block/source");    
            
    gates:
        
        output udpOut;
        input udpIn;
}
