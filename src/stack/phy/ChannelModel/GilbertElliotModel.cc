//
// Created by seba on 09.07.19.
//

#include <stdexcept>
#include <iostream>
#include "GilbertElliotModel.h"
#include "stack/phy/ChannelModel/LteGilbertElliotChannelModel.h"

GilbertElliotModel::GilbertElliotModel() {}

void GilbertElliotModel::setStartingState() {
    double steady_state_good = getSteadyStateProbability(ChannelState::good);
    double random_number = parent->getRandomNumber();
    if (random_number <= steady_state_good)
        current_channel_state = ChannelState::good;
    else
        current_channel_state = ChannelState::bad;
}

double GilbertElliotModel::update() {
	double random_number = parent->getRandomNumber();
	double current_transition_probability = current_channel_state == ChannelState::good ? good_state_transition_prob : bad_state_transition_prob;
	if (random_number <= current_transition_probability)
		current_channel_state = current_channel_state == ChannelState::good ? ChannelState::bad : ChannelState::good;
	current_channel_state == ChannelState::good ? num_times_good_state_visited++ : num_times_bad_state_visited++;
	return getCurrentErrorProbability();
}

void GilbertElliotModel::setErrorProbability(const int channel_state, double error_probability) {
	if (channel_state == ChannelState::good)
		this->good_state_packet_error_prob = error_probability;
	else if (channel_state == ChannelState::bad)
		this->bad_state_packet_error_prob = error_probability;
	else
		throw std::invalid_argument("Channel state is neither good nor bad.");
}

void GilbertElliotModel::setTransitionProbability(const int channel_state, double transition_probability) {
	if (channel_state == ChannelState::good)
		this->good_state_transition_prob = transition_probability;
	else if (channel_state == ChannelState::bad)
		this->bad_state_transition_prob = transition_probability;
	else
		throw std::invalid_argument("Channel state is neither good nor bad.");
}

double GilbertElliotModel::getErrorProbability(const int channel_state) const {
	if (channel_state == ChannelState::good)
		return this->good_state_packet_error_prob;
	else if (channel_state == ChannelState::bad)
		return this->bad_state_packet_error_prob;
	else
		throw std::invalid_argument("Channel state is neither good nor bad.");
}

double GilbertElliotModel::getTransitionProbability(const int channel_state) const {
	if (channel_state == ChannelState::good)
		return this->good_state_transition_prob;
	else if (channel_state == ChannelState::bad)
		return this->bad_state_transition_prob;
	else
		throw std::invalid_argument("Channel state is neither good nor bad.");
}

double GilbertElliotModel::getCurrentErrorProbability() const {
	return current_channel_state == ChannelState::good ? good_state_packet_error_prob : bad_state_packet_error_prob;
}

double GilbertElliotModel::getSteadyStateProbability(const int channel_state) const {
	const double& p12 = good_state_transition_prob;
	const double& p21 = bad_state_transition_prob;
	if (channel_state == ChannelState::good)
		return p21 / (p12 + p21);
	else if (channel_state == ChannelState::bad)
		return p12 / (p21 + p12);
	else
		throw std::invalid_argument("Channel state is neither good nor bad.");
}

const GilbertElliotModel::ChannelState& GilbertElliotModel::getCurrentChannelState() const {
	return this->current_channel_state;
}

void GilbertElliotModel::setParent(LteGilbertElliotChannelModel* parent) {
    this->parent = parent;
}

unsigned long GilbertElliotModel::getNumTimesStateVisited(const int channel_state) const {
    if (channel_state == ChannelState::good)
        return num_times_good_state_visited;
    else if (channel_state == ChannelState::bad)
        return num_times_bad_state_visited;
    else
        throw std::invalid_argument("Channel state is neither good nor bad.");
}
