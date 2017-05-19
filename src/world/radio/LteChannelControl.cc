//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "world/radio/LteChannelControl.h"
#include "inet/common/INETMath.h"
#include <cassert>

#include "stack/phy/packet/AirFrame_m.h"

#define coreEV EV << "LteChannelControl: "

Define_Module(LteChannelControl);

LteChannelControl::LteChannelControl()
{
}

LteChannelControl::~LteChannelControl()
{
}

/**
 * Calculates maxInterferenceDistance.
 *
 * @ref calcInterfDist
 */
void LteChannelControl::initialize()
{
    coreEV << "initializing LteChannelControl\n";
    ChannelControl::initialize();
}

/**
 * Calculation of the interference distance based on the transmitter
 * power, wavelength, pathloss coefficient and a threshold for the
 * minimal receive Power
 *
 * You may want to overwrite this function in order to do your own
 * interference calculation
 *
 * TODO check interference model
 */
double LteChannelControl::calcInterfDist()
{
    double interfDistance;

    //the carrier frequency used
    double carrierFrequency = par("carrierFrequency");
    //maximum transmission power possible
    double pMax = par("pMax");
    //signal attenuation threshold
    double sat = par("sat");
    //path loss coefficient
    double alpha = par("alpha");

    double waveLength = (SPEED_OF_LIGHT / carrierFrequency);
    //minimum power level to be able to physically receive a signal
    double minReceivePower = pow(10.0, sat / 10.0);

    interfDistance = pow(waveLength * waveLength * pMax / (16.0 * M_PI * M_PI * minReceivePower), 1.0 / alpha);

    coreEV << "max interference distance:" << interfDistance << endl;

    return interfDistance;
}

void LteChannelControl::sendToChannel(RadioRef srcRadio, AirFrame *airFrame)
{
    // NOTE: no Enter_Method()! We pretend this method is part of ChannelAccess

    // loop through all radios in range
    const RadioRefVector& neighbors = getNeighbors(srcRadio);
    for (unsigned int i=0; i<neighbors.size(); i++)
    {
        RadioRef r = neighbors[i];
        coreEV << "sending message to radio\n";
        simtime_t delay = 0.0;
        check_and_cast<cSimpleModule*>(srcRadio->radioModule)->sendDirect(airFrame->dup(), delay, airFrame->getDuration(), r->radioInGate);
    }

    // the original frame can be deleted
    delete airFrame;
}
