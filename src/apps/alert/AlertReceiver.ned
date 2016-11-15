// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 

package lte.apps.alert;

import inet.applications.contract.IUDPApp;


simple AlertReceiver like IUDPApp
{
    parameters:
        int localPort = default(3000);
        string interfaceTableModule;
        @signal[alertDelay];
        @statistic[alertDelay](title="Alert Message Delay"; unit="s"; source="alertDelay"; record=mean,vector);    
        @signal[alertRcvdMsg]; 
        @statistic[alertRcvdMsg](title="Alert Messages Received"; unit="s"; source="alertRcvdMsg"; record=sum,vector);         
        @display("i=block/source");
    gates:
        output udpOut;
        input udpIn;
}

