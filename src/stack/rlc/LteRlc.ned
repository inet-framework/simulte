// 
//                           SimuLTE
// Copyright (C) 2012 Antonio Virdis, Daniele Migliorini, Giovanni
// Accongiagioco, Generoso Pagano, Vincenzo Pii.
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 


package lte.stack.rlc;


// 
// Compound module for the RLC layer of LTE Stack.
//
module LteRlc {
    parameters:
        @display("i=block/transport");
        @statistic[rlcDelayDl](title="Delay at the rlc layer UL"; unit="s"; source="rlcDelayDl"; record=lteAvg);
        @statistic[rlcThroughputDl](title="Throughput at the rlc layer DL"; unit="Bps"; source="rlcThroughputDl"; record=lteRate);
        @statistic[rlcDelayUl](title="Delay at the rlc layer UL"; unit="s"; source="rlcDelayUl"; record=lteAvg);
        @statistic[rlcThroughputUl](title="Throughput at the rlc layer UL"; unit="Bps"; source="rlcThroughputUl"; record=lteRate);
        @statistic[rlcPduDelayDl](title="Delay at the rlc layer UL"; unit="s"; source="rlcPduDelayDl"; record=lteAvg);
        @statistic[rlcPduThroughputDl](title="Throughput at the rlc layer DL"; unit="Bps"; source="rlcPduThroughputDl"; record=lteRate);
        @statistic[rlcPduDelayUl](title="Delay at the rlc layer UL"; unit="s"; source="rlcPduDelayUl"; record=lteAvg);
        @statistic[rlcPduThroughputUl](title="Throughput at the rlc layer UL"; unit="Bps"; source="rlcPduThroughputUl"; record=lteRate);
        @statistic[rlcCellThroughputUl](title="Cell Throughput at the rlc layer UL"; unit="Bps"; source="rlcCellThroughputUl"; record=lteRate);
        @statistic[rlcCellThroughputDl](title="Cell Throughput at the rlc layer DL"; unit="Bps"; source="rlcCellThroughputDl"; record=lteRate);
        @statistic[rlcCellPacketLossDl](title="rlc Cell Packet Loss"; unit=""; source="rlcCellPacketLossDl"; record=lteAvg);
        @statistic[rlcCellPacketLossUl](title="rlc Cell Packet Loss"; unit=""; source="rlcCellPacketLossUl"; record=lteAvg);
        @statistic[rlcPacketLossUl](title="rlc Packet Loss"; unit=""; source="rlcPacketLossUl"; record=lteAvg);
        @statistic[rlcPacketLossDl](title="rlc Packet Loss"; unit=""; source="rlcPacketLossDl"; record=lteAvg);
        @statistic[rlcPduPacketLossUl](title="rlc Packet Loss"; unit=""; source="rlcPduPacketLossUl"; record=lteAvg);
        @statistic[rlcPduPacketLossDl](title="rlc Packet Loss"; unit=""; source="rlcPduPacketLossDl"; record=lteAvg);

    gates:
        //# 
        //# Gates connecting PDCP/RRC and RLC Layers
        //# 

        inout TM_Sap;    // Transparent Mode SAP
        inout UM_Sap;    // Unacknowledged Mode SAP
        inout AM_Sap;    // Acknowledged Mode SAP

        //# 
        //# Gates connecting RLC and MAC Layers
        //#

        input MAC_to_RLC;    // MAC to RLC
        output RLC_to_MAC;    // RLC to MAC

    submodules:
        // TM Module
        tm: LteRlcTm {
            @display("p=100,100;");
        }
        
        // UM Module
        um: LteRlcUm {
            @display("p=200,100;");
        }
        
        // AM Module
        am: LteRlcAm {
            @display("p=300,100;");
        }
        
           // Muxer Module
        mux: LteRlcMux {
            @display("p=200,200");
        }

    connections:
        //#
        //# Connections from RLC (up)
        //# to TM/UM/AM modules
        //#
        tm.TM_Sap_up <--> TM_Sap;
        um.UM_Sap_up <--> UM_Sap;
        am.AM_Sap_up <--> AM_Sap;

        //#
        //# Connections from TM/UM/AM 
        //# modules to the RLC Mux
        //#
        tm.TM_Sap_down <--> mux.TM_Sap;
        um.UM_Sap_down <--> mux.UM_Sap;
        am.AM_Sap_down <--> mux.AM_Sap;

        //#
        //# Connections between Mux
        //# and RLC (down)
        //#
        mux.MAC_to_RLC <-- MAC_to_RLC;
        mux.RLC_to_MAC --> RLC_to_MAC;
}

// 
// TM Module for the RLC layer of LTE Stack.
//
simple LteRlcTm {
    parameters:
        @display("i=block/wheelbarrow");

    gates:
        //# 
        //# Gates connecting RLC and TM
        //# 
        inout TM_Sap_up;    // Gate to PDCP
        inout TM_Sap_down;    // Gate to RLC Mux
}

// 
// UM Module for the RLC layer of LTE Stack.
//
simple LteRlcUm {
    parameters:
        @display("i=block/wheelbarrow");
        
    gates:
        //# 
        //# Gates connecting RLC and UM
        //# 
        inout UM_Sap_up;    // Gate to PDCP
        inout UM_Sap_down;    // Gate to RLC Mux
}

// 
// AM Module for the RLC layer of LTE Stack.
//
simple LteRlcAm {
    parameters:
        @display("i=block/wheelbarrow");
    gates:
        //# 
        //# Gates connecting RLC and AM
        //# 
        inout AM_Sap_up;    // Gate to PDCP
        inout AM_Sap_down;    // Gate to RLC Mux
}

// 
// Muxer for the RLC layer of LTE Stack.
// Mux/Demux of data to/from MAC layer
//
simple LteRlcMux {
    parameters:
        @display("i=block/classifier");                        // Display preferences

    gates:
        //# 
        //# Gates connecting TM/UM/AM modules and RLC Mux
        //# 
        inout TM_Sap;    // Transparent Mode SAP
        inout UM_Sap;    // Unacknowledged Mode SAP
        inout AM_Sap;    // Acknowledged Mode SAP

        //# 
        //# Gates connecting RLC and MAC Layers
        //#
        input MAC_to_RLC;    // MAC to RLC
        output RLC_to_MAC;    // RLC to MAC
}

// 
// Transmit Buffer of RLC UM
//
simple UmTxQueue {
    parameters:
        @dynamic(true);
        @display("i=block/segm");
        int fragmentSize @unit(B) = default(30B);        // Size of fragments
}

// 
// Receive Buffer of RLC UM
//
simple UmRxQueue {
    parameters:
        @dynamic(true);
        @display("i=block/segm");
        double timeout @unit(s) = default(1s);            // Timeout for RX Buffer
}

// 
// Transmit Buffer of RLC AM
//
simple AmTxQueue {
    parameters:
        @dynamic(true);
        @display("i=block/segm");
        int fragmentSize @unit(B) = default(30B);        // Size of fragments
        int maxRtx = default(3);
        double pduRtxTimeout @unit(s) = default(2.0s);
        double ctrlPduRtxTimeout @unit(s) = default (2.0s);
        double bufferStatusTimeout @unit(s) = default (2.0s);
        int txWindowSize = default (200);
}

// 
// Receive Buffer of RLC AM
//
simple AmRxQueue {
    parameters:
        @dynamic(true);
        @display("i=block/segm");
        int rxWindowSize = default(20);
        double ackReportInterval @unit(s) = 0.10s;
        double statusReportInterval @unit(s) = 0.20s;
        double timeout @unit(s) = default(1s);            // Timeout for RX Buffer
}
