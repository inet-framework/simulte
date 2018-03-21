// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 

package lte.apps.vehicular.v2v.v2vAlert;

import inet.applications.contract.IUDPApp;

//
//#	V2vAlertSender
//
//	Simple Module for a Sender v2v App:
//	The task of this module is:
//		1) Advertising, using the txMode defined, the v2v-receivers assigned by the UEClusterizeReceiver
//

simple V2vAlertSender like IUDPApp
{
    parameters:
                        
        int localPort = default(-1);
        int destPort = default(3000);
        string destAddress = default("");					
        int packetSize = default(10);
        
        double period @unit("s") = default(0.02s);
        double startTime @unit("s") = default(0s);
        string interfaceTableModule;
               
        @signal[v2vAlertSentMsg];
        @statistic[v2vAlertSentMsg](title="V2vAlert messages sent"; unit=""; source="v2vAlertSentMsg"; record=sum,vector);

        @display("i=block/source");
        
    gates:
        
        output udpOut;
        input udpIn;
}