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
//#	V2vAlertReceiver
//
//	Simple Module for a Receiver v2v App:
//	The task of this module is:
//		1) Receiving the Alert message
//

simple V2vAlertReceiver like IUDPApp
{
    parameters:
        
        int localPort = default(3000);
        string interfaceTableModule;
        
        @signal[v2vAlertDelay];
        @statistic[v2vAlertDelay](title="V2vAlert Message Delay"; unit="s"; source="v2vAlertDelay"; record=mean,vector);
           
        @signal[v2vAlertRcvdMsg]; 
        @statistic[v2vAlertRcvdMsg](title="V2vAlert Messages Received"; unit="s"; source="v2vAlertRcvdMsg"; record=sum,vector);
              
        @display("i=block/source");
        
    gates:
        
        output udpOut;
        input udpIn;
}