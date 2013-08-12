

cplusplus {{
    #include "AirFrame_m.h"
}}
packet AirFrame;

// 
// This is LteAirFrame.
//
packet LteAirFrame extends AirFrame {
    @customize(true);
    //TODO: add txmode for MIMO operations
}
