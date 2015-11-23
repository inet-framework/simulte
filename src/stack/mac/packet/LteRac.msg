//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

packet LteRac
{
   // meaningful only for DL (response) RAC packets : if true RAC request has been admitted.
    bool success;
}