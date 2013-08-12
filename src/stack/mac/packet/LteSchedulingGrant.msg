

//
// TODO generated message class
//
packet LteSchedulingGrant 
{
    @customize(true);  // see the generated C++ header for more info
    bool periodic;
       unsigned int period;
       unsigned int expiration;
       // blocks granted on all Remotes, all Bands
    unsigned int totalGrantedBlocks;
    // number of codewords
    unsigned int codewords;
    // granted bytes per codeword
    abstract unsigned int grantedCwBytes[];
    
    //#  in LteSchedulingGrant.h
    //# const UserTxParams* userTxParams;
    //# RbMap grantedBlocks;
    //# 
}
