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
#include "LteMcs.h"

const unsigned int* itbs2tbs(LteMod mod, TxMode txMode, unsigned char layers, unsigned char itbs)
{
    const unsigned int* res;

    if(layers == 1 || (txMode != OL_SPATIAL_MULTIPLEXING && txMode != CL_SPATIAL_MULTIPLEXING)) {

        switch(mod) {
            case _QPSK: res = itbs2tbs_qpsk_1[itbs]; break;
            case _16QAM: res = itbs2tbs_16qam_1[itbs]; break;
            case _64QAM: res = itbs2tbs_64qam_1[itbs]; break;
            default: {
                EV << "Unknown MCS in LteAmc::itbs2tbs, Aborting." << endl;
                abort();
            }
        }

    }

    // Here we are sure to use Spatial Multiplexing with more than 1 layer (2 or 4)
    else if(layers == 2) {

        switch(mod) {
            case _QPSK: res = itbs2tbs_qpsk_2[itbs]; break;
            case _16QAM: res = itbs2tbs_16qam_2[itbs]; break;
            case _64QAM: res = itbs2tbs_64qam_2[itbs]; break;
            default: {
                EV << "Unknown MCS in LteAmc::itbs2tbs, Aborting." << endl;
                abort();
            }
        }

    }
    else if(layers == 4) {

        switch(mod) {
            case _QPSK: res = itbs2tbs_qpsk_4[itbs]; break;
            case _16QAM: res = itbs2tbs_16qam_4[itbs]; break;
            case _64QAM: res = itbs2tbs_64qam_4[itbs]; break;
            default: {
                EV << "Unknown MCS in LteAmc::itbs2tbs, Aborting." << endl;
                abort();
            }
        }

    }

    return res;
}

std::vector<unsigned char> cwMapping(const TxMode& txMode, const Rank& ri,const unsigned int antennaPorts)
{
    std::vector<unsigned char> res;

    if(ri <= 1) {
        res.push_back(1);
    } else {
        switch(txMode) {

            // SISO and MU-MIMO supports only rank 1 transmission (1 layer)
            case SINGLE_ANTENNA_PORT0:
            case SINGLE_ANTENNA_PORT5:
            case MULTI_USER:{
                res.push_back(1);
                break;
            }

            // TX Diversity uses a number of layers equal to antennaPorts
            case TRANSMIT_DIVERSITY: {
                res.push_back(antennaPorts);
                break;
            }

            // Spatial MUX uses MIN(RI, antennaPorts) layers
            case OL_SPATIAL_MULTIPLEXING:
            case CL_SPATIAL_MULTIPLEXING: {
                int usedRi = (antennaPorts < ri ) ? antennaPorts : ri;
                if(usedRi == 2) {res.push_back(1); res.push_back(1);}
                if(usedRi == 3) {res.push_back(1); res.push_back(2);}
                if(usedRi == 4) {res.push_back(2); res.push_back(2);}
                if(usedRi == 8) {res.push_back(4); res.push_back(4);}
                break;
            }

            default: {
                res.push_back(1);
                break;
            }
        }
    }
    return res;
}
