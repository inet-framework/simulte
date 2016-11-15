// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 


package lte.stack.mac;

// 
// Interface for the MAC layer of LTE Stack.
//
moduleinterface LteMac {
    parameters:
        @display("i=block/mac");
        string interfaceTableModule;
   
    gates:
        input RLC_to_MAC;    // RLC to MAC
        output MAC_to_RLC;    // MAC to RLC
        input PHY_to_MAC;    // PHY to MAC
        output MAC_to_PHY;    // MAC to PHY
}

// 
// Base module for the MAC layer of LTE Stack.
//
simple LteMacBase like LteMac {
    parameters:
        @display("i=block/mac");
        string interfaceTableModule;

        //# Mac Queues
        int queueSize @unit(B) = default(2MiB);              // MAC Buffers queue size
        
        //# Mac Scheduler
        int    maxBytesPerTti @unit(B) = default(3000KiB);          // Schedulable Bytes
        
        //# Mac MIB
        
        bool muMimo = default(true);
        
                
        //# H-ARQ
        int harqProcesses = default(8);
        int maxHarqRtx = default(4);
         
        //#
        //# Statistic recording: end2end delay and throughput at the mac layer
        //#
        @signal[macDelayDl];
        @statistic[macDelayDl](title="Delay at the MAC layer UL"; unit="s"; source="macDelayDl"; record=lteAvg);
        @signal[macThroughputDl];
        @statistic[macThroughputDl](title="Throughput at the MAC layer DL"; unit="Bps"; source="macThroughputDl"; record=lteRate);
        @signal[macDelayUl];
        @statistic[macDelayUl](title="Delay at the MAC layer UL"; unit="s"; source="macDelayUl"; record=lteAvg);
        @signal[macThroughputUl];
        @statistic[macThroughputUl](title="Throughput at the MAC layer UL"; unit="Bps"; source="macThroughputUl"; record=lteRate);
        @signal[macDelayD2D];
        @statistic[macDelayD2D](title="Delay at the MAC layer D2D"; unit="s"; source="macDelayD2D"; record=lteAvg);
        @signal[macThroughputD2D];
        @statistic[macThroughputD2D](title="Throughput at the MAC layer D2D"; unit="Bps"; source="macThroughputD2D"; record=lteRate);
        @signal[macCellThroughputUl];
        @statistic[macCellThroughputUl](title="Cell Throughput at the MAC layer UL"; unit="Bps"; source="macCellThroughputUl"; record=lteRate);
        @signal[macCellThroughputDl];
        @statistic[macCellThroughputDl](title="Cell Throughput at the MAC layer DL"; unit="Bps"; source="macCellThroughputDl"; record=lteRate);
        @signal[macCellThroughputD2D];
        @statistic[macCellThroughputD2D](title="Cell Throughput at the MAC layer D2D"; unit="Bps"; source="macCellThroughputD2D"; record=lteRate);  
        @signal[macCellPacketLossDl];
        @statistic[macCellPacketLossDl](title="Mac Cell Packet Loss"; unit=""; source="macCellPacketLossDl"; record=mean);
        @signal[macCellPacketLossUl];
        @statistic[macCellPacketLossUl](title="Mac Cell Packet Loss"; unit=""; source="macCellPacketLossUl"; record=mean);
        @signal[macCellPacketLossD2D];
        @statistic[macCellPacketLossD2D](title="Mac Cell Packet Loss D2D"; unit=""; source="macCellPacketLossD2D"; record=mean);
        @signal[macPacketLossUl];
        @statistic[macPacketLossUl](title="Mac Packet Loss"; unit=""; source="macPacketLossUl"; record=mean);
        @signal[macPacketLossDl];
        @statistic[macPacketLossDl](title="Mac Packet Loss"; unit=""; source="macPacketLossDl"; record=mean);
        @signal[macPacketLossD2D];
        @statistic[macPacketLossD2D](title="Mac Packet Loss D2D"; unit=""; source="macPacketLossD2D"; record=mean);
        @signal[macBufferOverFlowDl];
        @statistic[macBufferOverFlowDl](title="Mac buffer overflow as function of time"; unit="Byte/s"; source="macBufferOverFlowDl"; record=lteRate);
        @signal[macBufferOverFlowUl];
        @statistic[macBufferOverFlowUl](title="Mac buffer overflow as function of time"; unit="Byte/s"; source="macBufferOverFlowUl"; record=lteRate);
        @signal[macBufferOverFlowD2D];
    	@statistic[macBufferOverFlowD2D](title="Mac buffer overflow as function of time"; unit="Byte/s"; source="macBufferOverFlowD2D"; record=lteRate);
        @signal[harqErrorRateUl];
        @statistic[harqErrorRateUl](title="Harq Error Rate Dl"; unit=""; source="harqErrorRateUl"; record=mean);
        @signal[harqErrorRateDl];
        @statistic[harqErrorRateDl](title="Harq Error Rate Dl"; unit=""; source="harqErrorRateDl"; record=mean);
        @signal[harqErrorRateD2D];
        @statistic[harqErrorRateD2D](title="Harq Error Rate D2D"; unit=""; source="harqErrorRateD2D"; record=mean);
        @signal[harqErrorRate_1st_Ul];
        @statistic[harqErrorRate_1st_Ul](title="Harq Error Rate Dl"; unit=""; source="harqErrorRate_1st_Ul"; record=mean);
        @signal[harqErrorRate_1st_Dl];
        @statistic[harqErrorRate_1st_Dl](title="Harq Error Rate Dl"; unit=""; source="harqErrorRate_1st_Dl"; record=mean);
        @signal[harqErrorRate_1st_D2D];
        @statistic[harqErrorRate_1st_D2D](title="Harq Error Rate D2D"; unit=""; source="harqErrorRate_1st_D2D"; record=mean);
        @signal[harqErrorRate_2nd_Ul];
        @statistic[harqErrorRate_2nd_Ul](title="Harq Error Rate Dl"; unit=""; source="harqErrorRate_2nd_Ul"; record=mean);
        @signal[harqErrorRate_2nd_Dl];
        @statistic[harqErrorRate_2nd_Dl](title="Harq Error Rate Dl"; unit=""; source="harqErrorRate_2nd_Dl"; record=mean);
        @signal[harqErrorRate_2nd_D2D];
        @statistic[harqErrorRate_2nd_D2D](title="Harq Error Rate D2D"; unit=""; source="harqErrorRate_2nd_D2D"; record=mean);
        @signal[harqErrorRate_3rd_Ul];
        @statistic[harqErrorRate_3rd_Ul](title="Harq Error Rate Dl"; unit=""; source="harqErrorRate_3rd_Ul"; record=mean);
        @signal[harqErrorRate_3rd_Dl];
        @statistic[harqErrorRate_3rd_Dl](title="Harq Error Rate Dl"; unit=""; source="harqErrorRate_3rd_Dl"; record=mean);
        @signal[harqErrorRate_3rd_D2D];
        @statistic[harqErrorRate_3rd_D2D](title="Harq Error Rate D2D"; unit=""; source="harqErrorRate_3rd_D2D"; record=mean);
        @signal[harqErrorRate_4th_Ul];
        @statistic[harqErrorRate_4th_Ul](title="Harq Error Rate Dl"; unit=""; source="harqErrorRate_4th_Ul"; record=mean);
        @signal[harqErrorRate_4th_Dl];
        @statistic[harqErrorRate_4th_Dl](title="Harq Error Rate Dl"; unit=""; source="harqErrorRate_4th_Dl"; record=mean);
        @signal[harqErrorRate_4th_D2D];
        @statistic[harqErrorRate_4th_D2D](title="Harq Error Rate D2D"; unit=""; source="harqErrorRate_4th_D2D"; record=mean);
        @signal[receivedPacketFromUpperLayer];
        @statistic[receivedPacketFromUpperLayer](source="receivedPacketFromUpperLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @signal[receivedPacketFromLowerLayer];
        @statistic[receivedPacketFromLowerLayer](source="receivedPacketFromLowerLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @signal[sentPacketToUpperLayer];
        @statistic[sentPacketToUpperLayer](source="sentPacketToUpperLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @signal[sentPacketToLowerLayer];
        @statistic[sentPacketToLowerLayer](source="sentPacketToLowerLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @signal[measuredItbs];
        @statistic[measuredItbs](title="TBS index"; unit=""; source="measuredItbs"; record=mean,vector);
        @signal[measuredItbs_lte];
        @statistic[measuredItbs_lte](title="TBS index"; unit=""; source="measuredItbs_lte"; record=lteAvg);

        //# signals for indicating buffer overflow
        @signal[macBufferOverflowUl];
        @signal[macBufferOverflowDl];

    gates:
        //# 
        //# Gates connecting RLC and MAC Layers
        //# 
        
        input RLC_to_MAC;    // RLC to MAC
        output MAC_to_RLC;    // MAC to RLC
        
        //# 
        //# Gates connecting MAC and PHY Layers
        //# 
        
        input PHY_to_MAC;    // PHY to MAC
        output MAC_to_PHY;    // MAC to PHY
}

//
// User Equipment MAC layer of LTE stack
//
simple LteMacUe extends LteMacBase {
    @class("LteMacUe");
}

//
// D2D-capable User Equipment MAC layer of LTE stack
//
simple LteMacUeD2D extends LteMacUe {
    parameters:
        @class("LteMacUeD2D");
    
        bool usePreconfiguredTxParams = default(true);
        int d2dCqi = default(7);
}

//
// User Equipment MAC layer of LTE stack (Realistic)
//
simple LteMacUeRealistic extends LteMacUe {
    @class("LteMacUeRealistic");
}

//
// D2D-capable User Equipment MAC layer of LTE stack (Realistic)
//
simple LteMacUeRealisticD2D extends LteMacUeRealistic
{
  parameters:
    
    @class("LteMacUeRealisticD2D"); 
    
    bool usePreconfiguredTxParams = default(false);
    int d2dCqi = default(7);
}

//
// eNodeB MAC layer of LTE stack
//
simple LteMacEnb extends LteMacBase {
    
    parameters:
        
        @class("LteMacEnb");    
        
        volatile xml optSolution = default(xmldoc("solution.sol"));
        
        //#
        //# AMC Parameters
        //#
        
        // AMC Mode:  "auto", "piloted", "multi", "das"
        string amcMode = default("AUTO");
        
        //Min disance used to pair two MuMimo UE
        int muMimoMinDistance =default(50);
        
        // resource allocation type ("distributed" or "localized")
        string rbAllocationType = default("distributed");

        // summary feedback confidence function lower bound
        double summaryLowerBound @unit(s) = default(5ms);

        // summary feedback confidence function upper bound
        double summaryUpperBound @unit(s) = default(20ms);

        // FeedBack Historical Base capacity in DL (number of stored feedback samples per UE) 
        int fbhbCapacityDl = default(5);

        // FeedBack Historical Base capacity in UL (number of stored feedback samples per UE)
        int fbhbCapacityUl = default(5);

        // FeedBack Historical Base capacity in D2D (number of stored feedback samples per UE)
        int fbhbCapacityD2D = default(5);
        
        // wideband PMI generation parameter (0.0 means "use the mean value" )        
        double pmiWeight = default(0.0);

        // wideband CQI generation parameter (0.0 means "use the mean value" )
        double cqiWeight = default(0.0);

        // AMC k CQI: used in DAS AMC mode
        int kCqi = default(0);
       
        // number of eNodeBs - set to 0 if unknown
        int eNodeBCount = default(0);
        //#
        //# eNb Scheduler Parameters
        //#    
        // Scheduling discipline. See LteCommon.h for discipline meaning.
        string schedulingDisciplineDl = default("MAXCI");
        string schedulingDisciplineUl = default("MAXCI");

        // Grant type DL
        string grantTypeConversationalDl = default("FITALL");
        string grantTypeStreamingDl      = default("FITALL");
        string grantTypeInteractiveDl    = default("FITALL");
        string grantTypeBackgroundDl     = default("FITALL");
        
        // Grant Size Dl in bytes (-1 means max grant, i.e. 4GB)
        int grantSizeConversationalDl = default(-1);
        int grantSizeStreamingDl      = default(-1);
        int grantSizeInteractiveDl    = default(-1);
        int grantSizeBackgroundDl     = default(-1);

        // Grant type Ul
        string grantTypeConversationalUl = default("FITALL");
        string grantTypeStreamingUl      = default("FITALL");
        string grantTypeInteractiveUl    = default("FITALL");
        string grantTypeBackgroundUl     = default("FITALL");

        // Grant Size Ul in bytes (-1 means max grant, i.e. 4GB)
        int grantSizeConversationalUl = default(-1);
        int grantSizeStreamingUl      = default(-1);
        int grantSizeInteractiveUl    = default(-1);
        int grantSizeBackgroundUl     = default(-1);
        
        // Proportional Fair parameters
        double pfAlpha    = default(0.95);
        
        // LTE Advanced Scheduler general parameters - DL
        int lteAallocationRbsDl = default(1);
        int lteAhistorySizeDL = default(20);
        int lteAgainHistoryThCONVERSATIONALDL = default(15);
        int lteAgainHistoryThSTREAMINGDL = default(15);
        int lteAgainHistoryThINTERACTIVEDL = default(15);
        int lteAgainHistoryThBACKGROUNDDL = default(15);
        
        // LTE Advanced Scheduler general parameters - UL 
        int lteAallocationRbsUl = default(1);
        int lteAhistorySizeUL = default(20);
        int lteAgainHistoryThUL = default(5);
            
        // LTE Advanced Scheduler QOS Parameters - DL
        double lteAdeadlineCONVERSATIONALDL @unit(s) = default(200ms);
        double lteAdeadlineSTREAMINGDL @unit(s) = default(1000ms);
        double lteAdeadlineINTERACTIVEDL @unit(s) = default(50ms);
        double lteAdeadlineBACKGROUNDDL @unit(s) = default(-1ms);
        
        double lteAslackTermCONVERSATIONALDL @unit(s) = default(50ms);
        double lteAslackTermSTREAMINGDL @unit(s) = default(100ms);
        double lteAslackTermINTERACTIVEDL @unit(s) = default(10ms);
        double lteAslackTermBACKGROUNDDL @unit(s) = default(0ms);
        
        int lteAmaxUrgentBurstCONVERSATIONALDL @unit(byte) = default(0);
        int lteAmaxUrgentBurstSTREAMINGDL @unit(byte)     = default(0);
        int lteAmaxUrgentBurstINTERACTIVEDL @unit(byte)    = default(0);
        int lteAmaxUrgentBurstBACKGROUNDDL   @unit(byte)  = default(200B);
        
        int lteAmaxFairnessBurstCONVERSATIONALDL @unit(byte) = default(0);
        int lteAmaxFairnessBurstSTREAMINGDL @unit(byte)     = default(0);
        int lteAmaxFairnessBurstINTERACTIVEDL @unit(byte)    = default(0);
        int lteAmaxFairnessBurstBACKGROUNDDL   @unit(byte)  = default(200B);
        
        // LTE Advanced Scheduler QOS Parameters - UL
        double lteAdeadlineCONVERSATIONALUL @unit(s) = default(250ms);
        double lteAdeadlineSTREAMINGUL @unit(s) = default(1500ms);
        double lteAdeadlineINTERACTIVEUL @unit(s) = default(2500ms);
        double lteAdeadlineBACKGROUNDUL @unit(s) = default(250ms);
        
        double lteAslackTermCONVERSATIONALUL @unit(s) = default(50ms);
        double lteAslackTermSTREAMINGUL @unit(s) = default(500ms);
        double lteAslackTermINTERACTIVEUL @unit(s) = default(500ms);
        double lteAslackTermBACKGROUNDUL @unit(s) = default(0ms);
        
        int lteAmaxBurstCONVERSATIONALUL @unit(byte) = default(0);
        int lteAmaxBurstSTREAMINGUL     @unit(byte) = default(0);
        int lteAmaxBurstINTERACTIVEUL   @unit(byte) = default(0);
        int lteAmaxBurstBACKGROUNDUL    @unit(byte) = default(0);
        
        // LTE Advanced Scheduler PF Version - DL
        bool pfTmsAwareDL = default(false);
        // LTE Advanced Scheduler PF Version - UL
        bool pfTmsAwareUL = default(false);
        
        // algorithm activation thresholds. 
        int layer0th= default (10000);
        int layer1th= default (8000);
        int layer2th= default (5000);
        int layer3th= default (2000);
        

        
        // D2 class of service percentages
        double goldUsers = default(0.2);
        double silverUsers = default(0.4);
        
        
        // Power Model Parameters for MBSFN frame
        // minimum depleted power (W)
        double zeroLevelMBSFNDL @unit(W)= default(260W); //504
        // minimum depleted power (W)
        double zeroLevelMBSFNUL @unit(W)= default(260W); 
        // idle state depleted power (W)
        double idleLevelMBSFNDL @unit(W)= default(150W); //336
        // idle state depleted power (W)
        double idleLevelMBSFNUL @unit(W)= default(150W);
        // per-block depletion unit (W)
        double powerUnitMBSFNDL @unit(W)= default(3.76W); //6.72
        // per-block depletion unit (W)
        double powerUnitMBSFNUL @unit(W)= default(3.76W);
        // maximumum depletable power (W)
        double maxPowerMBSFNDL @unit(W)= default(448W); //840
        // maximumum depletable power (W)
        double maxPowerMBSFNUL @unit(W)= default(448W);
        
        // Power Model Parameters for PAGING FRAMES
        // minimum depleted power (W)
        double zeroLevelPAGINGDL @unit(W)= default(260W);
        // minimum depleted power (W)
        double zeroLevelPAGINGUL @unit(W)= default(260W);
        // idle state depleted power (W)
        double idleLevelPAGINGDL @unit(W)= default(150W);
        // idle state depleted power (W)
        double idleLevelPAGINGUL @unit(W)= default(150W);
        // per-block depletion unit (W)
        double powerUnitPAGINGDL @unit(W)= default(3.76W);
        // per-block depletion unit (W)
        double powerUnitPAGINGUL @unit(W)= default(3.76W);
        // maximumum depletable power (W)
        double maxPowerPAGINGDL @unit(W)= default(448W);
        // maximumum depletable power (W)
        double maxPowerPAGINGUL @unit(W)= default(448W);
        
        // Power Model Parameters for NORMAL frames
        // minimum depleted power (W)
        double zeroLevelNORMAL_FRAME_TYPEDL @unit(W)= default(260W);
        // minimum depleted power (W)
        double zeroLevelNORMAL_FRAME_TYPEUL @unit(W)= default(260W);
        // idle state depleted power (W)
        double idleLevelNORMAL_FRAME_TYPEDL @unit(W)= default(150W);
        // idle state depleted power (W)
        double idleLevelNORMAL_FRAME_TYPEUL @unit(W)= default(150W);
        // per-block depletion unit (W)
        double powerUnitNORMAL_FRAME_TYPEDL @unit(W)= default(3.76W);
        // per-block depletion unit (W)
        double powerUnitNORMAL_FRAME_TYPEUL @unit(W)= default(3.76W);
        // maximumum depletable power (W)
        double maxPowerNORMAL_FRAME_TYPEDL @unit(W)= default(448W);
        // maximumum depletable power (W)
        double maxPowerNORMAL_FRAME_TYPEUL @unit(W)= default(448W);
        
        // Power Model Parameters for BROADCAST frames
        // minimum depleted power (W)
        double zeroLevelBROADCASTDL @unit(W)= default(260W);
        // minimum depleted power (W)
        double zeroLevelBROADCASTUL @unit(W)= default(260W);
        // idle state depleted power (W)
        double idleLevelBROADCASTDL @unit(W)= default(150W);
        // idle state depleted power (W)
        double idleLevelBROADCASTUL @unit(W)= default(150W);
        // per-block depletion unit (W)
        double powerUnitBROADCASTDL @unit(W)= default(3.76W);
        // per-block depletion unit (W)
        double powerUnitBROADCASTUL @unit(W)= default(3.76W);
        // maximumum depletable power (W)
        double maxPowerBROADCASTDL @unit(W)= default(448W);
        // maximumum depletable power (W)
        double maxPowerBROADCASTUL @unit(W)= default(448W);
        
        // Power Model Parameters for SYNCRO frames
        // minimum depleted power (W)
        double zeroLevelSYNCRODL @unit(W)= default(260W);
        // minimum depleted power (W)
        double zeroLevelSYNCROUL @unit(W)= default(260W);
        // idle state depleted power (W)
        double idleLevelSYNCRODL @unit(W)= default(150W);
        // idle state depleted power (W)
        double idleLevelSYNCROUL @unit(W)= default(150W);
        // per-block depletion unit (W)
        double powerUnitSYNCRODL @unit(W)= default(3.76W);
        // per-block depletion unit (W)
        double powerUnitSYNCROUL @unit(W)= default(3.76W);
        // maximumum depletable power (W)
        double maxPowerSYNCRODL @unit(W)= default(448W);
        // maximumum depletable power (W)
        double maxPowerSYNCROUL @unit(W)= default(448W);
        
        double prfFixedSYNCRO @unit(W)= default(5.543W);
        double prfRBSYNCRO @unit(W)= default(0.689W);
        
        double prfFixedMBSFN @unit(W)=default(4.4444W);
        double prfRBMBSFN @unit(W)=default(0.711W);
        
        double prfFixedNORMAL_FRAME_TYPE @unit(W)= default(3.810W);
        double prfRBNORMAL_FRAME_TYPE @unit(W)= default(0.724W);
        
        double prfFixedPAGING @unit(W)= default(3.810W);
        double prfRBPAGING @unit(W)= default(0.724W);
        
        double prfFixedBROADCAST @unit(W)= default(4.4W);
        double prfRBBROADCAST @unit(W)= default(0.712W);
        
        
        //# Statistics for Lte Scheduler Enb
        @signal[tmsTxDiv];
        @statistic[tmsTxDiv](title="how many time TMS has coiche TxDIV respect to the other"; unit="ratio"; source="tmsTxDiv"; record=lteAvg);
        @signal[tmsSMux];
        @statistic[tmsSMux](title="how many time TMS has coiche SMUX respect to the other"; unit="ratio"; source="tmsSMux"; record=lteAvg);
        @signal[tmsMuMimo];
        @statistic[tmsMuMimo](title="how many time TMS has coiche MuMIMO respect to the other"; unit="ratio"; source="tmsMuMimo"; record=lteAvg);
        
        //#
        //# Statistics for Lte Scheduler Enb
        @signal[cellBlocksUtilizationDl];
        @statistic[cellBlocksUtilizationDl](title="LTE Cell Blocks Utilization Dl"; unit="blocks"; source="cellBlocksUtilizationDl"; record=lteAvg);
        @signal[cellBlocksUtilizationUl];
        @statistic[cellBlocksUtilizationUl](title="LTE Cell Blocks Utilization Ul"; unit="blocks"; source="cellBlocksUtilizationUl"; record=lteAvg);
        @signal[avgServedBlocksDl];
        @statistic[avgServedBlocksDl](title="LTE Avg Served Blocks Dl"; unit="blocks"; source="avgServedBlocksDl"; record=lteAvg);
        @signal[avgServedBlocksUl];
        @statistic[avgServedBlocksUl](title="LTE Avg Served Blocks Ul"; unit="blocks"; source="avgServedBlocksUl"; record=lteAvg);
        @signal[depletedPowerDl];
        @statistic[depletedPowerDl](title="LTE ENodeB depleted power Dl"; unit="watts"; source="depletedPowerDl"; record=lteAvg);
        @signal[depletedPowerUl];
        @statistic[depletedPowerUl](title="LTE ENodeB depleted power Ul"; unit="watts"; source="depletedPowerUl"; record=lteAvg);
        @signal[activatedFrames];
        @statistic[activatedFrames](title="D1 algo, ratio of activated frame"; unit="ratio"; source="activetedFrames"; record=lteAvg);
        @signal[sleepFrames];
        @statistic[sleepFrames](title="D1 algo, ratio of sleep frame"; unit="ratio"; source="sleepFrames"; record=lteAvg);
        @signal[wastedFrames];
        @statistic[wastedFrames](title="D1 algo, ratio of activated frame with no traffic to serve"; unit="ratio"; source="wastedFrames"; record=lteAvg);
        
        @signal[prf_0];
        @statistic[prf_0](unit="ratio"; source="prf_0"; record=lteAvg);
        @signal[prf_1a];
        @statistic[prf_1a](unit="ratio"; source="prf_1a"; record=lteAvg);
        @signal[prf_2a];
        @statistic[prf_2a](  unit="ratio"; source="prf_2a"; record=lteAvg);
        @signal[prf_3a];
        @statistic[prf_3a](  unit="ratio"; source="prf_3a"; record=lteAvg);
        @signal[prf_1b];
        @statistic[prf_1b](  unit="ratio"; source="prf_1b"; record=lteAvg);
        @signal[prf_2b];
        @statistic[prf_2b](  unit="ratio"; source="prf_2b"; record=lteAvg);
        @signal[prf_3b];
        @statistic[prf_3b](  unit="ratio"; source="prf_3b"; record=lteAvg);
        @signal[prf_3b];
        @statistic[prf_3b](  unit="ratio"; source="prf_1c"; record=lteAvg);
        @signal[prf_2c];
        @statistic[prf_2c](  unit="ratio"; source="prf_2c"; record=lteAvg);
        @signal[prf_3c];
        @statistic[prf_3c](  unit="ratio"; source="prf_3c"; record=lteAvg);
        @signal[prf_4];
        @statistic[prf_4](  unit="ratio"; source="prf_4"; record=lteAvg);
        @signal[prf_5];
        @statistic[prf_5](  unit="ratio"; source="prf_5"; record=lteAvg);
        @signal[prf_6a];
        @statistic[prf_6a](  unit="ratio"; source="prf_6a"; record=lteAvg);
        @signal[prf_7a];
        @statistic[prf_7a](  unit="ratio"; source="prf_7a"; record=lteAvg);
        @signal[prf_8a];
        @statistic[prf_8a](  unit="ratio"; source="prf_8a"; record=lteAvg);
        @signal[prf_6b];
        @statistic[prf_6b](  unit="ratio"; source="prf_6b"; record=lteAvg);
        @signal[prf_7b];
        @statistic[prf_7b](  unit="ratio"; source="prf_7b"; record=lteAvg);
        @signal[prf_8b];
        @statistic[prf_8b](  unit="ratio"; source="prf_8b"; record=lteAvg);
        @signal[prf_6c];
        @statistic[prf_6c](  unit="ratio"; source="prf_6c"; record=lteAvg);
        @signal[prf_7c];
        @statistic[prf_7c](  unit="ratio"; source="prf_7c"; record=lteAvg);
        @signal[prf_8c];
        @statistic[prf_8c](  unit="ratio"; source="prf_8c"; record=lteAvg);
        @signal[prf_9];
        @statistic[prf_9](  unit="ratio"; source="prf_9"; record=lteAvg);
        
        @signal[rb_0];
        @statistic[rb_0](  unit="ratio"; source="rb_0"; record=lteAvg);
        @signal[rb_1a];
        @statistic[rb_1a](  unit="ratio"; source="rb_1a"; record=lteAvg);
        @signal[rb_2a];
        @statistic[rb_2a](  unit="ratio"; source="rb_2a"; record=lteAvg);
        @signal[rb_3a];
        @statistic[rb_3a](  unit="ratio"; source="rb_3a"; record=lteAvg);
        @signal[rb_1b];
        @statistic[rb_1b](  unit="ratio"; source="rb_1b"; record=lteAvg);
        @signal[rb_2b];
        @statistic[rb_2b](  unit="ratio"; source="rb_2b"; record=lteAvg);
        @signal[rb_3b];
        @statistic[rb_3b](  unit="ratio"; source="rb_3b"; record=lteAvg);
        @signal[rb_1c];
        @statistic[rb_1c](  unit="ratio"; source="rb_1c"; record=lteAvg);
        @signal[rb_2c];
        @statistic[rb_2c](  unit="ratio"; source="rb_2c"; record=lteAvg);
        @signal[rb_3c];
        @statistic[rb_3c](  unit="ratio"; source="rb_3c"; record=lteAvg);
        @signal[rb_4];
        @statistic[rb_4](  unit="ratio"; source="rb_4"; record=lteAvg);
        @signal[rb_5];
        @statistic[rb_5](  unit="ratio"; source="rb_5"; record=lteAvg);
        @signal[rb_6a];
        @statistic[rb_6a](  unit="ratio"; source="rb_6a"; record=lteAvg);
        @signal[rb_7a];
        @statistic[rb_7a](  unit="ratio"; source="rb_7a"; record=lteAvg);
        @signal[rb_8a];
        @statistic[rb_8a](  unit="ratio"; source="rb_8a"; record=lteAvg);
        @signal[rb_6b];
        @statistic[rb_6b](  unit="ratio"; source="rb_6b"; record=lteAvg);
        @signal[rb_7b];
        @statistic[rb_7b](  unit="ratio"; source="rb_7b"; record=lteAvg);
        @signal[rb_8b];
        @statistic[rb_8b](  unit="ratio"; source="rb_8b"; record=lteAvg);
        @signal[rb_6c];
        @statistic[rb_6c](  unit="ratio"; source="rb_6c"; record=lteAvg);
        @signal[rb_7c];
        @statistic[rb_7c](  unit="ratio"; source="rb_7c"; record=lteAvg);
        @signal[rb_8c];
        @statistic[rb_8c](  unit="ratio"; source="rb_8c"; record=lteAvg);
        @signal[rb_9];
        @statistic[rb_9](  unit="ratio"; source="rb_9"; record=lteAvg);
        
        @signal[cqiDlSiso0];
        @statistic[cqiDlSiso0](title="Average cqi siso band 0"; unit="cqi"; source="cqiDlSiso0"; record=lteAvg);
        @signal[cqiDlSiso1];
        @statistic[cqiDlSiso1](title="Average cqi siso band 1"; unit="cqi"; source="cqiDlSiso1"; record=lteAvg);
        @signal[cqiDlSiso2];
        @statistic[cqiDlSiso2](title="Average cqi siso band 2"; unit="cqi"; source="cqiDlSiso2"; record=lteAvg);
        @signal[cqiDlSiso3];
        @statistic[cqiDlSiso3](title="Average cqi siso band 3"; unit="cqi"; source="cqiDlSiso3"; record=lteAvg);
        @signal[cqiDlSiso4];
        @statistic[cqiDlSiso4](title="Average cqi siso band 4"; unit="cqi"; source="cqiDlSiso4"; record=lteAvg);
    
        @signal[cqiDlSpmux0];
        @statistic[cqiDlSpmux0](title="Average cqi Spmux band 0"; unit="cqi"; source="cqiDlSpmux0"; record=lteAvg);
        @signal[cqiDlSpmux1];
        @statistic[cqiDlSpmux1](title="Average cqi Spmux band 1"; unit="cqi"; source="cqiDlSpmux1"; record=lteAvg);
        @signal[cqiDlSpmux2];
        @statistic[cqiDlSpmux2](title="Average cqi Spmux band 2"; unit="cqi"; source="cqiDlSpmux2"; record=lteAvg);
        @signal[cqiDlSpmux3];
        @statistic[cqiDlSpmux3](title="Average cqi Spmux band 3"; unit="cqi"; source="cqiDlSpmux3"; record=lteAvg);
        @signal[cqiDlSpmux4];
        @statistic[cqiDlSpmux4](title="Average cqi Spmux band 4"; unit="cqi"; source="cqiDlSpmux4"; record=lteAvg);
        
        @signal[cqiDlTxDiv0];
        @statistic[cqiDlTxDiv0](title="Average cqi TxDiv band 0"; unit="cqi"; source="cqiDlTxDiv0"; record=lteAvg);
        @signal[cqiDlTxDiv1];
        @statistic[cqiDlTxDiv1](title="Average cqi TxDiv band 1"; unit="cqi"; source="cqiDlTxDiv1"; record=lteAvg);
        @signal[cqiDlTxDiv2];
        @statistic[cqiDlTxDiv2](title="Average cqi TxDiv band 2"; unit="cqi"; source="cqiDlTxDiv2"; record=lteAvg);
        @signal[cqiDlTxDiv3];
        @statistic[cqiDlTxDiv3](title="Average cqi TxDiv band 3"; unit="cqi"; source="cqiDlTxDiv3"; record=lteAvg);
        @signal[cqiDlTxDiv4];
        @statistic[cqiDlTxDiv4](title="Average cqi TxDiv band 4"; unit="cqi"; source="cqiDlTxDiv4"; record=lteAvg);
        
        @signal[cqiDlMuMimo0];
        @statistic[cqiDlMuMimo0](title="Average cqi MuMimo band 0"; unit="cqi"; source="cqiDlMuMimo0"; record=lteAvg);
        @signal[cqiDlMuMimo1];
        @statistic[cqiDlMuMimo1](title="Average cqi MuMimo band 1"; unit="cqi"; source="cqiDlMuMimo1"; record=lteAvg);
        @signal[cqiDlMuMimo2];
        @statistic[cqiDlMuMimo2](title="Average cqi MuMimo band 2"; unit="cqi"; source="cqiDlMuMimo2"; record=lteAvg);
        @signal[cqiDlMuMimo3];
        @statistic[cqiDlMuMimo3](title="Average cqi MuMimo band 3"; unit="cqi"; source="cqiDlMuMimo3"; record=lteAvg);
        @signal[cqiDlMuMimo4];
        @statistic[cqiDlMuMimo4](title="Average cqi MuMimo band 4"; unit="cqi"; source="cqiDlMuMimo4"; record=lteAvg);    
}    

//
// eNodeB MAC layer of LTE stack with support for D2D-capable UEs
//
simple LteMacEnbD2D extends LteMacEnb 
{
    parameters:
        @class("LteMacEnbD2D"); 
        
        bool usePreconfiguredTxParams = default(true);
        int d2dCqi = default(7);
}
      
//
// eNodeB MAC layer of LTE stack (Realistic)
//
simple LteMacEnbRealistic extends LteMacEnb {
    @class("LteMacEnbRealistic");
}

//
// eNodeB MAC layer of LTE stack (Realistic) with support for D2D-capable UEs
//
simple LteMacEnbRealisticD2D extends LteMacEnbRealistic
{
    parameters:
        
	@class("LteMacEnbRealisticD2D");
	
	bool usePreconfiguredTxParams = default(false);
    int d2dCqi = default(7);
}

//
// Relay (UE side) MAC layer of LTE stack
// Connection: eNodeB <--> Relay
//
simple LteMacRelayUe extends LteMacUe 
{
    @class("LteMacRelayUe");
}

//
// Relay (eNB side) MAC layer of LTE stack
// Connection: Relay <--> UE
//
simple LteMacRelayEnb extends LteMacEnb 
{
    @class("LteMacRelayEnb");
}
