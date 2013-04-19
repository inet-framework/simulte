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


#include "ExtCell.h"

ExtCell::ExtCell(double pwr , int i , int num , int distance)
{
	double angle = (360/num)*i;

	double c = cos(angle*PI/180);
	double s = sin(angle*PI/180);

	position_.x = (c*distance)+distance;
	position_.y = (s*distance)+distance;

	EV << "ExtCell::ExtCell - Creating an External Cell with \n\t[x="<< position_.x << ",y=" << position_.y << "]\n\t" <<
																"[angle=" << angle << "]\n\t" <<
																"[distance=" << distance << "]\n\t" <<
																"[index=" << i << "/" << num-1 << "]\n\t" <<
																"[pwr="<< pwr <<"]" << endl;
	txPower_ = 	pwr;
	//position_ = pos;
}

ExtCell::~ExtCell() {
	// TODO Auto-generated destructor stub
}

