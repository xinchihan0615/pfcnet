#ifndef _ETHERENCAPPFC_H_
#define _ETHERENCAPPFC_H_

#include "inet/linklayer/ethernet/EtherEncap.h"

#include <iostream>
using namespace omnetpp;
using namespace inet;

class EtherEncapPFC : public EtherEncap
{
protected:
    virtual void handleMessageWhenUp(cMessage *msg) override;
};

#endif
