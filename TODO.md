## Design flaws
### Inheritance hierarchy
- changing the inheritance hierarchy in PgwStandard.ned. inet3 used the
NodeBase up until the network layer. Inet4 is separated into nodes for four of
the five layers (Link, Network, Transport, Application), where upper layers
derive from the consecutive lower layer. LinkLayerNodeBase derives from
NodeBase.
- use the new packet api from inet
    
    
## Fixing the simulations
### Knowns
- Udp has no vector gate anymore. 
- neither appIn and appOut in Udp.ned nor transportIn and transportOut in 
INetworkLayer are vectorgates anymore

### Unknowns
- which x2-related parts actually work and are covered by working simulations?
- does TCP work? (not implemented everywhere)

## Restructuring code
### removing redundancies
- a lot of code is commented out. Has to be either used or removed.
- is it possible to get rid of Lteip.cc?
