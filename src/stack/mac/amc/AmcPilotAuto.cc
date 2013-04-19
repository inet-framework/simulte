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
#include "AmcPilotAuto.h"

const UserTxParams& AmcPilotAuto::computeTxParams(MacNodeId id, const Direction dir)
{
	EV<<NOW<<" AmcPilot"<<getName()<<"::computeTxParams for UE "<<id<<", direction "<<dirToA(dir)<<endl;

	// Check if user transmission parameters have been already allocated
	if(amc_->existTxParams(id, dir)) {
		EV<<NOW<<" AmcPilot"<<getName()<<"::computeTxParams The Information for this user have been already assigned \n";
		return amc_->getTxParams(id, dir);
	}

	// TODO make it configurable from NED
	// default transmission mode
	TxMode txMode = TRANSMIT_DIVERSITY;

	/**
	 *  Select the band which has the best summary
	 *  Note: this pilot is not DAS aware, so only MACRO antenna
	 *  is used.
	 */
	LteSummaryFeedback sfb = amc_->getFeedback(id, MACRO, txMode, dir);

	if (TxMode(txMode)==MULTI_USER) // Initialize MuMiMoMatrix
	    amc_->muMimoMatrixInit(dir,id);


	sfb.print(0,id,dir,txMode,"AmcPilotAuto::computeTxParams");

	// get a vector of  CQI over first CW
	std::vector<Cqi> summaryCqi = sfb.getCqi(0);

	// TODO Add MIN and MEAN cqi computation methods
	Band band = 0;
	double max = summaryCqi.at(band);
	BandSet b;
	unsigned int bands = summaryCqi.size(); // number of bands
	for(Band b = 1; b < bands; ++b)
	{	// For all LBs
		double s = (double)summaryCqi.at(b);
		if(max < s)
		{
			band = b;
			max = s;
		}
	}
	b.insert(band);
	// Set user transmission parameters only for the best band
	UserTxParams info;
	info.writeTxMode(txMode);
	info.writeRank(sfb.getRi());
	info.writeCqi(std::vector<Cqi>(1, sfb.getCqi(0, band)));
	info.writePmi(sfb.getPmi(band));
	info.writeBands(b);
	RemoteSet antennas;
	antennas.insert(MACRO);
	info.writeAntennas(antennas);

	// DEBUG
	EV<<NOW<<" AmcPilot"<<getName()<<"::computeTxParams NEW values assigned! - CQI MAX=" << max << "\n";
	info.print("AmcPilotAuto::computeTxParams");

	return amc_->setTxParams(id, dir, info);
}
