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

simple AlertSender like IUDPApp
{
    parameters:
        int localPort = default(-1);
        int destPort = default(3000);
        string destAddress;
        int packetSize = default(10);
        double period @unit("s") = default(0.02s);
        double startTime @unit("s") = default(0s);
        string interfaceTableModule;
               
        @signal[alertSentMsg];
        @statistic[alertSentMsg](title="Alert messages sent"; unit=""; source="alertSentMsg"; record=sum,vector);

        @display("i=block/source");
    gates:
        output udpOut;
        input udpIn;
}
