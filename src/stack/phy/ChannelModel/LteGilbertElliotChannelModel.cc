/*
 * LteGilbertElliotChannelModel.cc
 *
 *  Created on: Jul 9, 2019
 *      Author: Sebastian Lindner, sebastian.lindner@tuhh.de
 */

#include "LteGilbertElliotChannelModel.h"
#include <iostream>

Define_Module(LteGilbertElliotChannelModel);

LteGilbertElliotChannelModel::LteGilbertElliotChannelModel() : channel_model(), rng(std::mt19937(random_device())), distribution(std::uniform_real_distribution<double>(0.0, 1.0)) {
}

bool LteGilbertElliotChannelModel::error(LteAirFrame *frame, UserControlInfo* lteI) {
    // Update the channel model.
    double packet_error_prob = channel_model.update();
    // Draw a random number.
    double random_number = distribution(rng);
    // Compare to the current model state's error probability.
    if (random_number > packet_error_prob) // e.g. packet_error_prob=0.3, then for random_number>0.3 no error occurs, which is an 'area' of 0.7.
        return false; // No error occurred.
    else
        return true; // An error occurred.
}

bool LteGilbertElliotChannelModel::error_D2D(LteAirFrame *frame, UserControlInfo* lteI, const std::vector<double>& rsrpVector) {
    return error(frame, lteI);
}

void LteGilbertElliotChannelModel::initialize() {
    LteRealisticChannelModel::initialize();
    std::cout << "trans_prob_good_state=" << par("trans_prob_good_state").doubleValue() << std::endl;
}
