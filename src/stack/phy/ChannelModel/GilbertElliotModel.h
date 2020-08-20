//
// Created by seba on 09.07.19.
//

#ifndef LTEGILBERTELLIOTCHANNELMODEL_GILBERTELLIOTMODEL_H
#define LTEGILBERTELLIOTCHANNELMODEL_GILBERTELLIOTMODEL_H

#include <random>

class LteGilbertElliotChannelModel;

/**
 * Implements a two-state Gilbert-Elliot channel model.
 * This is a discrete two-state Markov chain, with specific transition probabilities.
 * The two states correspond to the channel taking on a specific state, e.g. a 'good' state and a 'bad' state.
 * Each state may exhibit a certain packet error probability, so that checking whether a transmission succeeds boils down to evaluating which state the model is currently in, and applying the corresponding packet error probability.
 */
class GilbertElliotModel {
	public:
		/** The states the model can be in. */
		enum ChannelState {
			good = 0,
			bad = 1
		};

		GilbertElliotModel();
		virtual ~GilbertElliotModel() = default;

		void setStartingState();

		/**
		 * Updates the model. Possibly transitions to another state, or remains.
		 * @return The current packet error probability after updating.
		 */
		double update();

		double getCurrentErrorProbability() const;

		double getSteadyStateProbability(const int channel_state) const;

		void setErrorProbability(const int channel_state, double error_probability);

		void setTransitionProbability(const int channel_state, double transition_probability);

		double getErrorProbability(const int channel_state) const;

		double getTransitionProbability(const int channel_state) const;

		const GilbertElliotModel::ChannelState& getCurrentChannelState() const;

		void setParent(LteGilbertElliotChannelModel* parent);

		unsigned long getNumTimesStateVisited(const int channel_state) const;

	private:
		double good_state_packet_error_prob = 0.0,
				bad_state_packet_error_prob = 1.0;

		double good_state_transition_prob = 0.5,
				bad_state_transition_prob = 0.5;

		unsigned long num_times_good_state_visited = 0,
		              num_times_bad_state_visited = 0;

		ChannelState current_channel_state = ChannelState::good;

		LteGilbertElliotChannelModel* parent = nullptr;
};


#endif //LTEGILBERTELLIOTCHANNELMODEL_GILBERTELLIOTMODEL_H
