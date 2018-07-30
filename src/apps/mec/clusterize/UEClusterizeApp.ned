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

import inet.applications.contract.IUDPApp;

//
//#	UEClusterizeApp
//
//	Simple Module for an UE App communicating with a ME App on a ME HOST.
//	The task of this module is:
//#	Sender Side:
//		1) Request the Instantiation (START_MEAPP) of the correspondent MEClusterizeApp on the ME HOST 
//		2) Reporting periodically local information (INFO_UEAPP) to the correspondent MEClusterizeApp instance (User App)
//		3) Request the Termination (STOP_MEAPP) of the correspondent MEClusterizeApp instance
//#	Receiver Side:
//		1) Handling the ACK_START_MEAPP message: starting the Periodic transmission of INFO_UEAPP messages 
//		2) Receiving periodically cluster info (INFO_MEAPP) from the correspondent MEClusterizeApp (User App)
//		and Eventually handling the cluster info (INFO_MEAPP) propagation to follower (when txMode is V2V_UNICAST or V2V_MULTICAST)
//		3) Handling the ACK_STOP_MEAPP message

simple UEClusterizeApp like IUDPApp
{
    parameters:
            @display("i=block/source");    
        
        //packet info
        int packetSize = default(100);
        
        //communication with ME Host
        int localPort = default(4000);
        int destPort = default(4000);
        string meHostAddress;
        
        //sidelink communication with cars
        int multicastPort = default(5000);
        string multicastGoupAddress = default("224.0.0.10");
        
        //resource required infos
        int requiredRam @unit("B") = default(10MB);
        int requiredDisk @unit("B") = default(100MB);
        double requiredCpu = default(0.01);
        
        //autoscheduling timers
        double period @unit("s") = default(0.02s);
        double startTime @unit("s") = default(0s); 
        double stopTime @unit("s") = default(20s);
        
        string interfaceTableModule;
        
        //statstics
        //#Sender Side
        @signal[clusterizeInfoSentMsg];
        @statistic[clusterizeInfoSentMsg](title="ClusterizeInfo messages sent"; unit=""; source="clusterizeInfoSentMsg"; record=sum,vector);

		//#Receiver Side          
        @signal[clusterizeConfigRcvdMsg]; 
        @statistic[clusterizeConfigRcvdMsg](title="ClusterizeConfig Messages Received"; unit="s"; source="clusterizeConfigRcvdMsg"; record=sum,vector);
        
        @signal[clusterizeConfigDelay]; 
        @statistic[clusterizeConfigDelay](title="ClusterizeConfig Delay"; unit="s"; source="clusterizeConfigDelay"; record=mean,vector);
        
        //#platooning formation values
        @signal[clusterizeAccelerations];
        @statistic[clusterizeAccelerations]("Clusterize Accelerations"; unit=""; source="clusterizeAccelerations"; record=vector);
        
        @signal[clusterizeVelocities];
        @statistic[clusterizeVelocities]("Clusterize Velocities"; unit="mps"; source="clusterizeVelocities"; record=vector);
        
        @signal[clusterizeDistancies];
        @statistic[clusterizeDistancies]("Clusterize Distancies"; unit="m"; source="clusterizeDistancies"; record=vector);
                  
    gates:
        
        output udpOut;
        input udpIn;
}
