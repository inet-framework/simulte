/*
 * LteGilbertElliotChannelModel.h
 *
 *  Created on: Jul 9, 2019
 *      Author: Sebastian Lindner, sebastian.lindner@tuhh.de
 */

#ifndef STACK_PHY_CHANNELMODEL_LTEGILBERTELLIOTCHANNELMODEL_H_
#define STACK_PHY_CHANNELMODEL_LTEGILBERTELLIOTCHANNELMODEL_H_

#include "stack/phy/ChannelModel/LteRealisticChannelModel.h"
#include "stack/phy/ChannelModel/GilbertElliotModel.h"

/**
 * Implements a two-state Gilbert-Elliot channel model.
 * This is a discrete two-state Markov chain, with specific transition probabilities.
 * The two states correspond to the channel taking on a specific state, e.g. a 'good' state and a 'bad' state.
 * Each state may exhibit a certain packet error probability, so that checking whether a transmission succeeds boils down to evaluating which state the model is currently in, and applying the corresponding packet error probability.
 */
class LteGilbertElliotChannelModel : public LteRealisticChannelModel {
    friend class GilbertElliotModel;

public:
    LteGilbertElliotChannelModel();
    virtual ~LteGilbertElliotChannelModel() = default;

    virtual bool error(LteAirFrame *frame, UserControlInfo* lteI) override;
    virtual bool error_D2D(LteAirFrame *frame, UserControlInfo* lteI, const std::vector<double>& rsrpVector) override;

    virtual void initialize();

protected:
    double getRandomNumber();

private:
    GilbertElliotModel channel_model;
    cRNG* rng_error, *rng_model;
};

#endif /* STACK_PHY_CHANNELMODEL_LTEGILBERTELLIOTCHANNELMODEL_H_ */
