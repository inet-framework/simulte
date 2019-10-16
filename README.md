SimuLTE
=======

LTE/LTE-A user-plane simulation model, compatible with the INET Framework.

Dependencies
------------

The current master/head version requires either of

- OMNeT++ 5.5.1 and INET 3.6.6

Features
--------

General

- eNodeB and UE models
- Full LTE protocol stack

PDCP-RRC

- Header compression/decompression
- Logical connection establishment  and maintenance 

RLC

- Multiplexing/Demultiplexing of MAC SDUs
- UM, (AM and TM testing) modes

MAC

- RLC PDUs buffering
- HARQ functionalities (with multi-codeword support)
- Allocation management
- AMC
- Scheduling Policies (MAX C/I, Proportional Fair, DRR)

PHY

- Heterogeneous Net (HetNets) support: Macro, micro, pico eNbs
- Channel Feedback management
- Dummy channel model
- Realistic channel model with
  - inter-cell interference
  - path-loss
  - fast fading
  - shadowing 
  - (an)isotropic antennas

Other

- X2 communication support
- X2-based handover
- Device-to-device communications
- Support for vehicular mobility

Applications

- Voice-over-IP (VoIP)
- Constant Bit Rate (CBR)
- Trace-based Video-on-demand traffic


Limitations
-----------

- User Plane only (Control Plane not modeled)
- FDD only (TDD not supported)
- no EPS bearer support – note: a similar concept, "connections", has 
  been implemented, but they are neither dynamic nor statically 
  configurable via some config file
- radio bearers not implemented, not even statically configured radio 
  bearers (dynamically allocating bearers would need the RRC protocol, 
  which is Control Plane so not implemented)


