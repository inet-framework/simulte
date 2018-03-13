// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 


package lte.corenetwork.lteCellInfo;

simple LteCellInfo {
    parameters:
        @display("i=block/control");
            
        @class("LteCellInfo");
        //#
        //# General properties
        //#
        
        double constraintAreaMinX @unit(m) = default(-1m/0); // min x position of the constraint area, unconstrained by 
        double constraintAreaMinY @unit(m) = default(-1m/0); // min y position of the constraint area, unconstrained by 
        double constraintAreaMaxX @unit(m) = default(1m/0); // max x position of the constraint area, unconstrained by d
        double constraintAreaMaxY @unit(m) = default(1m/0); // max y position of the constraint area, unconstrained by d
        
        // speed for CircleMobility and ConstSpeedSpeedMobility
        double speed @unit(mps) = default(0mps);
        
        // position update interval for CircleMobility and ConstSpeedMobility
        double positionUpdateInterval @unit(s) = default(0.001s);
          
        // broadcast messages interval (used for handover or das)
        double broadcastMessageInterval @unit(s) = default(1s);
          
        // eNodeB type (MICRO / MACRO) - valid only for eNodeBs
        bool microCell = default(false);  
          
        //#
        //# Remote Units (DAS)
        //#
        // number of remote Units
        int numRus = default(0);
        
        // distance range of remote units
        int ruRange = default(0);
        
        // ru starting angle (only graphical meaning since rus are virtuals)
        double ruStartingAngle @unit(deg) = default(0deg);
        
        // CSV string supplying TX power for i-th ru (integer values)
        string ruTxPower = default("0;");
        
        // cws for each antenna, note that first position refers to the MACRO
        string antennaCws = default("1;");

        //#
        //# Physical layer properties
        //#
        
        /// number of RB, DL
        int numRbDl=default(100);
        
        /// number of RB, UL
        int numRbUl=default(100);
                
        // number of sub-carriers per RB, DL
        int rbyDl = default(12);
        
        // number of sub-carriers per RB, UL
        int rbyUl = default(12);
        
        // number of OFDM symbols per slot, DL
        int rbxDl = default(7);
        
        // number of OFDM symbols per slot, UL
        int rbxUl = default(7);
        
        // number of pilot REs per RB, DL
        int rbPilotDl = default(3);
        
        // number of pilot REs per RB, UL
        int rbPilotUl = default(0);
        
        // number of signaling symbols for RB, DL
        int signalDl = default(1);
        
        // number of signaling symbols for RB, UL
        int signalUl = default(1);
        
        // number of preferred bands to use (meaningful only in PREFERRED mode)
        int numPreferredBands = default(1);
}
