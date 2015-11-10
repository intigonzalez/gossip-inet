//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "GossipPush.h"
#include "Gossip_m.h"
#include "GossipHello_m.h"

#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/contract/udp/UDPControlInfo.h"

#include <algorithm>

namespace inet {

Define_Module(GossipPush);

enum ControlMessageTypes {
    IDLE,
    START,
    NEW_GOSSIP,
    GOSSIP,
    SAY_HELLO
};

void GossipPush::initialize(int stage)
{
    ApplicationBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {

        isSource = par("isSource").boolValue();

        EV_TRACE << "Initialized as source : " << isSource << "\n";

        ctrlMsg0 = new cMessage("controlMSG", IDLE);
        ctrlMsg1 = new cMessage("controlMSG2", IDLE);
        ctrlHello = new cMessage("controlHello", IDLE);
    }
}

void GossipPush::handleMessageWhenUp(cMessage *msg)
{
    cPacket* pkt = nullptr;



    if (msg->isSelfMessage()) {

        switch (msg->getKind()) {
            case START:
                processStart();
                break;
            case NEW_GOSSIP:
                EV_TRACE << "Message Received\n";
                if (isSource) {
                    /* let's create the infection */
                    GossipPush::GossipInfection infection;
                    infection.idMsg = lastIdMsg++;
                    infection.roundsLeft = roundRatio;
                    infection.source = myAddress.str();
                    infection.text = "A message is nice";
                    infections.push_back(infection);

                    /* reduce the number of future infections */
                    numMessages--;

                    cancelEvent(ctrlMsg1);
                    ctrlMsg1->setKind(GOSSIP);
                    scheduleAt(simTime() + gossipInterval, ctrlMsg1);

                    if (numMessages) {
                        ctrlMsg0->setKind(NEW_GOSSIP);
                        scheduleAt(simTime() + intervalAmongNewMessages, ctrlMsg0);
                    }
                }
                break;
            case GOSSIP:
                if (gossiping() && !ctrlMsg1->isScheduled()) {
                    ctrlMsg1->setKind(GOSSIP);
                    scheduleAt(simTime() + gossipInterval, ctrlMsg1);
                }
                break;
            case SAY_HELLO:
                if (sayHello() && !ctrlHello->isScheduled()) {
                    ctrlHello->setKind(SAY_HELLO);
                    scheduleAt(simTime() + HELLO_INTERVAL, ctrlHello);
                }
                break;
            default:
                break;
        }

    }
    else if (msg->getKind() == UDP_I_DATA) {
        pkt = PK(msg);
        UDPDataIndication *ctrl = check_and_cast<UDPDataIndication *>(pkt->getControlInfo());
        Gossip* g = check_and_cast_nullable<Gossip*>(dynamic_cast<Gossip*>(pkt));

        if (g) {
            bool exists = std::any_of(infections.begin(), infections.end(), [&](GossipInfection t) {

                return (g->getId() == t.idMsg) && (g->getSource() == t.source);
            });

            if (!exists) {

                GossipInfection t;
                t.idMsg = g->getId();
                t.roundsLeft = roundRatio;
                t.source = g->getSource();
                t.text = g->getMsg();
                infections.push_back(t);

                EV_TRACE << "A new foreign message : '"  <<  t.text << "' from " << t.source << " through "<< ctrl->getSrcAddr() << "\n";
            }

            if (!ctrlMsg1->isScheduled()) {
                ctrlMsg1->setKind(GOSSIP);
                scheduleAt(simTime() + gossipInterval, ctrlMsg1);
            }
        }
        else {

            GossipHello * gh = check_and_cast_nullable<GossipHello*>(dynamic_cast<GossipHello*>(pkt));
            if (gh) {

                L3Address result;
                L3AddressResolver().tryResolve(gh->getId(), result);
                if (result.isUnspecified())
                    EV_ERROR << "cannot resolve destination address: " << ((gh->getId())?gh->getId():"NULL") << endl;
                else if (myself != gh->getId()) {
                    string sss = gh->getId();
                    map<string, L3Address>::iterator it = addresses.find(sss);
                    if (it == addresses.end()) {
                        EV_TRACE << "Hello from " << sss << "\n";
                        addresses.insert(std::pair<string,L3Address>(sss, result));
                    }
                }
            }

        }

        delete pkt;
    }

}

bool GossipPush::sayHello()
{
    // EV_TRACE << myself << " is saying hello" << endl;
    // FIXME: Figure out how to do a real broadcast
    for ( L3Address& addr : possibleNeighbors ) {
        GossipHello* pkt = new GossipHello("Hello");
        pkt->setId(myself.c_str());
        socket.sendTo(pkt, addr, destinationPort);
    }

    return true;
}

bool GossipPush::gossiping()
{
    bool r = false;
    std::vector<GossipInfection>::iterator it;

    it = std::find_if(infections.begin(), infections.end(), [] (GossipInfection f) {
       return f.roundsLeft > 0;
    });

    while (it != infections.end()) {

        for (std::map<string,L3Address>::iterator it2=addresses.begin(); it2!=addresses.end(); ++it2) {
            L3Address addr = it2->second;
            Gossip* pkt = new Gossip("");
            pkt->setId(it->idMsg);
            pkt->setSource(it->source.c_str());
            pkt->setMsg(it->text.c_str());
            socket.sendTo(pkt, addr, destinationPort);
        }

        r = true;
        it->roundsLeft--;

        it = std::find_if(it, infections.end(), [] (GossipInfection f) {
           return f.roundsLeft > 0;
        });
    }

    return r;
}

void GossipPush::finish()
{
    if (ctrlMsg0)
        cancelAndDelete(ctrlMsg0);

    if (ctrlMsg1)
        cancelAndDelete(ctrlMsg1);
    ctrlMsg1 = nullptr;

    ctrlMsg0 = nullptr;

    if (ctrlHello)
        cancelAndDelete(ctrlHello);
    ctrlHello = nullptr;
}

bool GossipPush::handleNodeStart(IDoneCallback *doneCallback)
{
    ctrlMsg0->setKind(START);
    scheduleAt(simTime() + 0.01, ctrlMsg0);
    ctrlHello->setKind(SAY_HELLO);
    scheduleAt(simTime() + HELLO_INTERVAL, ctrlHello);
    return true;
}

bool GossipPush::handleNodeShutdown(IDoneCallback *doneCallback)
{
    if (ctrlMsg0)
        cancelAndDelete(ctrlMsg0);
    ctrlMsg0 = nullptr;

    if (ctrlMsg1)
        cancelAndDelete(ctrlMsg1);
    ctrlMsg1 = nullptr;

    if (ctrlHello)
        cancelAndDelete(ctrlHello);
    ctrlHello = nullptr;

    return true;
}

void GossipPush::handleNodeCrash()
{
    if (ctrlMsg0)
        cancelAndDelete(ctrlMsg0);
    ctrlMsg0 = nullptr;

    if (ctrlMsg1)
        cancelAndDelete(ctrlMsg1);
    ctrlMsg1 = nullptr;

    if (ctrlHello)
        cancelAndDelete(ctrlHello);
    ctrlHello = nullptr;
}

void GossipPush::processStart()
{
    myself = this->getParentModule()->getFullName();
    L3AddressResolver().tryResolve(myself.c_str(), myAddress);
    EV_TRACE << "Starting the process in module " << myself << " (" << myAddress.str() << ")" << "\n";

    if (isSource) {
        numMessages = par("numMessages");
        intervalAmongNewMessages = par("intervalAmongNewMessages").doubleValue();
    }

    nodesPerRound = par("nodesPerRound");
    roundRatio = par("roundRatio");

    const char *destAddrs = par("addresses");
    cStringTokenizer tokenizer(destAddrs);
    const char *token;

    while ((token = tokenizer.nextToken()) != nullptr) {
        L3Address result;
        L3AddressResolver().tryResolve(token, result);
        if (result.isUnspecified())
            EV_ERROR << "cannot resolve destination address: " << ((token)?token:"NULL") << endl;
        else if (myself != token)
            possibleNeighbors.push_back(result);
    }

    socket.setOutputGate(gate("udpOut"));
    //socket.setBroadcast(true);
    socket.bind(localPort);

    if (isSource) {
        ctrlMsg0->setKind(NEW_GOSSIP);
        scheduleAt(simTime() +  HELLO_INTERVAL + intervalAmongNewMessages, ctrlMsg0);
    }
}

} //namespace
