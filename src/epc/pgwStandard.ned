//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package lte.epc;

import lte.epc.gtp.GtpUser;
import inet.linklayer.ppp.PPPInterface;
import inet.nodes.inet.NodeBase;
import lte.epc.TrafficFlowFilter;
import inet.transport.udp.UDP;


module pgwStandard extends NodeBase
{
    parameters:
        string nodeType; // must be one between ENB and PGW
    	@display("bgb=920,462;i=device/mainframe");
    gates:
        inout filterGate @labels(PPPFrame-conn);

    submodules:
        trafficFlowFilter: TrafficFlowFilter {
			ownerType = nodeType;
            @display("p=813,206");
        }
        pppInterface: PPPInterface {
            @display("p=727,386");
        }
        udp: UDP {
            @display("p=329,206");
        }
        gtp_user: GtpUser {
            @display("p=591,206");
        }
    connections:
        udp.ipOut --> networkLayer.udpIn;
        udp.ipIn <-- networkLayer.udpOut;

        pppInterface.upperLayerOut --> trafficFlowFilter.internetFilterGateIn;
        pppInterface.upperLayerIn <-- gtp_user.pppGate; 
        pppInterface.phys <--> filterGate;
        
        udp.appOut++ --> gtp_user.udpIn;
        gtp_user.udpOut --> udp.appIn++;
        trafficFlowFilter.gtpUserGateOut --> gtp_user.trafficFlowFilterGate;             
}
