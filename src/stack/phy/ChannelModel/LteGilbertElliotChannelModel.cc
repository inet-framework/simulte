/*
 * LteGilbertElliotChannelModel.cc
 *
 *  Created on: Jul 9, 2019
 *      Author: Sebastian Lindner, sebastian.lindner@tuhh.de
 */

#include "LteGilbertElliotChannelModel.h"
#include <iostream>

Define_Module(LteGilbertElliotChannelModel);

LteGilbertElliotChannelModel::LteGilbertElliotChannelModel() : channel_model(), rng_error(getRNG(0)), rng_model(getRNG(0)) {
    channel_model.setParent(this);
}

bool LteGilbertElliotChannelModel::error(LteAirFrame *frame, UserControlInfo* lteI) {
    // Update the channel model.
    GilbertElliotModel::ChannelState state_before = channel_model.getCurrentChannelState();
    double packet_error_prob = channel_model.update();
    // Print update.
    EV << "Channel update: " << state_before << " -> " << channel_model.getCurrentChannelState() << std::endl;
    // Draw a random number.
    double random_number = rng_error->doubleRand();
    // Apply HARQ reduction.
    double nTx = (double) lteI->getTxNumber();
    packet_error_prob = pow(packet_error_prob, nTx); // Assuming exponential decrease.
    // Compare to the current model state's error probability.
    if (random_number > packet_error_prob) {// e.g. packet_error_prob=0.3, then for random_number>0.3 no error occurs, which is an 'area' of 0.7.
        EV << "Channel result: no error" << std::endl;
        return false;
    } else {
        EV << "Channel result: an error" << std::endl;
        return true;
    }
}

bool LteGilbertElliotChannelModel::error_D2D(LteAirFrame *frame, UserControlInfo* lteI, const std::vector<double>& rsrpVector) {
    return error(frame, lteI);
}

void LteGilbertElliotChannelModel::initialize() {
    LteRealisticChannelModel::initialize();
    channel_model.setErrorProbability(GilbertElliotModel::ChannelState::good, par("error_prob_good_state").doubleValue());
    channel_model.setErrorProbability(GilbertElliotModel::ChannelState::bad, par("error_prob_bad_state").doubleValue());
    channel_model.setTransitionProbability(GilbertElliotModel::ChannelState::good, par("trans_prob_good_state").doubleValue());
    channel_model.setTransitionProbability(GilbertElliotModel::ChannelState::bad, par("trans_prob_bad_state").doubleValue());
}

double LteGilbertElliotChannelModel::getRandomNumber() {
    return rng_model->doubleRand();
}
