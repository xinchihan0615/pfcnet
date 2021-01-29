#ifndef _ETHERQUEUEPFC_H_
#define _ETHERQUEUEPFC_H_

#include <omnetpp.h>
#include <iostream>
#include "inet/queueing/queue/PacketQueue.h"
using namespace omnetpp;
using namespace inet;
using namespace queueing;


class EtherQueuePFC : public PacketQueue
{
public:
    EtherQueuePFC(){
        EV_INFO << "EtherQueueRaise \n ";
    }
    virtual void initialize(int stage) override;
    virtual Packet *popPacket(cGate *gate) override;
private:
    bool is_host_;
};

#endif
