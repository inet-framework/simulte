SimuLTE
=======

LTE user plane simulation model, compatible with the INET Framework.

Features
--------

- eNodeB and UE models
- ...

Limitations
-----------

- only the radio access network is modeled, the EPC network is not 
  (in the model eNodeB directly connects to the internet, there's no 
  explicit S-GW and P-GW, no GTP tunneling etc.)
- User Plane only (Control Plane not modeled)
- FDD only (TDD not supported)
- no EPS bearer support – note: a similar concept, "connections", has 
  been implemented, but they are neither dynamic nor statically 
  configurable via some config file
- radio bearers not implemented, not even statically configured radio 
  bearers (dynamically allocating bearers would need the RRC protocol, 
  which is Control Plane so not implemented)
- handovers not implemented (no X2-based handover, that is; S1-based 
  handover would require an S-GW model)

You might also have a look at the 4GSim project (http://github.com/4gsim/4Gsim)
which concentrates on simulating other aspects of LTE, namely,
control plane and backhaul.
