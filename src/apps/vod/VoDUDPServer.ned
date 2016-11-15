//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

package lte.apps.vod;
import inet.applications.contract.IUDPApp;


simple VoDUDPServer like IUDPApp
{
    parameters:
        int localPort; // port to listen on
        string vod_trace_file;
        int fps = default(25);
        string traceType = default("NS2");

        // clients parameters
        string destAddresses;
        int destPort;
        volatile double clientsStartStreamTime = default(0);
        string clientsReqTime = default("");
        double startTime @unit("s") = default(0);

        @display("i=block/app");
    gates:
        input udpIn @labels(UDPControlInfo/up);
        output udpOut @labels(UDPControlInfo/down);
}
