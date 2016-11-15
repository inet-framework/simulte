// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 

package lte.world.radio;

import lte.world.radio.ChannelControl;

simple LteChannelControl extends ChannelControl
{
    parameters:       
        @display("i=misc/sun");
        @labels(node);
        @class(LteChannelControl);
}