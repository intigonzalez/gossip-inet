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

enum GossipProtocolMessages {
    MSG_INITIALIZE = 57,
    MSG_NEW_GOSSIP = 58,
    MSG_EMPTY_MAILBOX = 59,
    MSG_FULL_MAILBOX = 60,
    MSG_GREET = 61,
    MSG_HELLO = 62,
    MSG_DATA = 63,
    MSG_GOSSIP = 64
};



void GossipPush::initialize(int stage)
{
    ApplicationBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {

        isSource = par("isSource").boolValue();

        EV_TRACE << "Initialized as source : " << isSource << "\n";

        ctrlMsg0 = new cMessage("controlMSG", IDLE);
    }
}

void GossipPush::handleMessageWhenUp(cMessage *msg)
{
    cPacket* pkt = nullptr;
    map<cMessage*, ITimeOut*>::iterator it;


    if (msg->isSelfMessage()) {

        switch (msg->getKind()) {
            case START:
                processStart();
                sm_proptocol->reportMessage(MSG_INITIALIZE);
                break;
            case TICK_MESSAGE:
                it = timers.find(msg);
                if (it != timers.end()) {
                    it->second->timeOut();
                    timers.erase(it);
                }
                cancelAndDelete(msg);

                break;
            default:
                break;
        }
    }
    else if (msg->getKind() == UDP_I_DATA) {
        pkt = PK(msg);

        EV_TRACE << "A network message\n";

        bool done = processReceivedGossip(pkt);
        done = done || processReceivedHello(pkt);

        // unknown package
        if (!done) {
            delete pkt;
        }
    }

    this->interpreting();

}

void GossipPush::newGossip() {
    EV_TRACE << "Message Received\n";
    if (isSource && numMessages > 0) {
        /* let's create the infection */
        GossipPush::GossipInfection infection;
        infection.idMsg = lastIdMsg++;
        infection.roundsLeft = roundRatio;
        infection.source = myAddress.str();
        infection.text = "A message is nice";
        infections.push_back(infection);
        /* reduce the number of future infections */
        numMessages--;
    }
}

bool GossipPush::processReceivedGossip(cPacket * pkt)
{

    Gossip* g = check_and_cast_nullable<Gossip*>(dynamic_cast<Gossip*>(pkt));

    if ( g == nullptr ) return false;

    sm_proptocol->getPool()->add(MSG_DATA, pkt);

    return true;
}

bool GossipPush::processReceivedHello(cPacket * pkt) {

    GossipHello* gh = check_and_cast_nullable<GossipHello*>(dynamic_cast<GossipHello*>(pkt));

    if (gh == nullptr) return false;

    sm_proptocol->getPool()->add(MSG_HELLO, pkt);

    return true;
}

bool GossipPush::sayHello()
{
    // EV_TRACE << myself << " is saying hello" << endl;
    // FIXME: Figure out how to do a real broadcast

//    if (isSource) {
//
//    GossipHello* pkt = new GossipHello("Hello");
//    pkt->setId(myself.c_str());
//    socket.sendTo(pkt, IPv4Address::ALLONES_ADDRESS, destinationPort);
//
//    }

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
    auto it = std::find_if(infections.begin(), infections.end(), [] (GossipInfection f) {
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
    ctrlMsg0 = nullptr;
}

bool GossipPush::handleNodeStart(IDoneCallback *doneCallback)
{
    ctrlMsg0->setKind(START);
    scheduleAt(simTime() + 0.01, ctrlMsg0);
    return true;
}

bool GossipPush::handleNodeShutdown(IDoneCallback *doneCallback)
{
    if (ctrlMsg0)
        cancelAndDelete(ctrlMsg0);
    ctrlMsg0 = nullptr;

    return true;
}

void GossipPush::handleNodeCrash()
{
    if (ctrlMsg0)
        cancelAndDelete(ctrlMsg0);
    ctrlMsg0 = nullptr;
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
    socket.bind(localPort);
    socket.setBroadcast(true);


    EV_TRACE << "Creating State Machines\n";

    sm_proptocol = createProtocolStateMachine();
    interpreters.push_back(new StateMachineInterpreter(sm_proptocol));
    interpreters.push_back(new StateMachineInterpreter(sm_tick_hello));
    interpreters.push_back(new StateMachineInterpreter(sm_tick_gossip));
    interpreters.push_back(new StateMachineInterpreter(sm_tick_new_gossip));

    EV_TRACE << "State Machines have been created\n";

}

void GossipPush::registerListener(ITimeOut* listener, double afterElapsedTime)
{
    ITimeOutProducer::registerListener(listener, afterElapsedTime);
    //EV_TRACE << "Registering listener because we enter in the state of waiting for tick " << afterElapsedTime << "\n";
    cMessage* m = new cMessage("a tick");
    m->setKind(TICK_MESSAGE);
    timers.insert (std::pair<cMessage*, ITimeOut*>(m, listener));
    scheduleAt(simTime() + afterElapsedTime, m);


}

void GossipPush::interpreting()
{
    bool b;
    do {
        b = false;
        for (StateMachineInterpreter* i : interpreters) {
            b = b | i->move();
        }
    } while (b);
}

void GossipPush::addNewAddress(string id)
{
    if (myself != id) {
        L3Address result;
        L3AddressResolver().tryResolve(id.c_str(), result);
        auto it = addresses.find(id);
        if (it == addresses.end()) {
            EV_TRACE << "Hello from " << id << "\n";
            addresses.insert(std::pair<string, L3Address>(id, result));
        }
    }


}

void GossipPush::addNewInfection(Gossip* g)
{
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

        UDPDataIndication *ctrl = check_and_cast<UDPDataIndication *>(g->getControlInfo());

        EV_TRACE << "A new foreign message : '"  <<  t.text << "' from " << t.source << " through "<< ctrl->getSrcAddr() << "\n";
    }
}

class wActions : public StateActions {
private:
    StateMachine* sm_hello;
    StateMachine* sm_newGossip;
public:
    wActions(StateMachine* t_hello, StateMachine* t_newGossip):sm_hello(t_hello), sm_newGossip(t_newGossip) {};
    virtual void enteringState(State* s, StateMachine* stateMachine, MessageType msg, void* extraData) {
        sm_hello->reportMessage(MSG_ACTIVATE);
        sm_newGossip->reportMessage(MSG_ACTIVATE);
    }
};

class ngActions : public StateActions {
private:
    GossipPush* gp;
    StateMachine* sm_Gossip;
public:
    ngActions(GossipPush* gpp, StateMachine* t_Gossip):gp(gpp), sm_Gossip(t_Gossip) {};
    virtual void enteringState(State* s, StateMachine* stateMachine, MessageType msg, void* extraData) {
        gp->newGossip();
        sm_Gossip->reportMessage(MSG_ACTIVATE);
        stateMachine->reportMessage(MSG_TRUE);
    }
};

class hActions : public StateActions {
private:
    GossipPush* gp;
public:
    hActions(GossipPush* gpp):gp(gpp){};
    virtual void enteringState(State* s, StateMachine* stateMachine, MessageType msg, void* extraData) {
        gp->sayHello();
        stateMachine->reportMessage(MSG_TRUE);
    }
};

class gActions : public StateActions {
private:
    GossipPush* gp;
public:
    gActions(GossipPush* gpp):gp(gpp){};
    virtual void enteringState(State* s, StateMachine* stateMachine, MessageType msg, void* extraData) {
        stateMachine->reportMessage(gp->isInfected()? MSG_FULL_MAILBOX : MSG_EMPTY_MAILBOX);
    }
};

class cActions : public StateActions {
private:
    GossipPush* gp;
public:
    cActions(GossipPush* gpp):gp(gpp){};
    virtual void enteringState(State* s, StateMachine* stateMachine, MessageType msg, void* extraData) {
        gp->gossiping();
        stateMachine->reportMessage(MSG_TRUE);
    }
};

class helloActions : public StateActions {
private:
    GossipPush* gp;
public:
    helloActions(GossipPush* gpp):gp(gpp){};
    virtual void enteringState(State* s, StateMachine* stateMachine, MessageType msg, void* extraData) {
        GossipHello* gh = check_and_cast_nullable<GossipHello*>(dynamic_cast<GossipHello*>((cPacket*)extraData));

        if (gh == nullptr) {
            // panic
            EV_ERROR << "NOOOOOOOOOOOOO, processing a packet as GossipHello when it is not GossipHello\n";
            return;
        }

        gp->addNewAddress(gh->getId());

        delete gh;

        stateMachine->reportMessage(MSG_TRUE);
    }
};

class dataActions : public StateActions {
private:
    GossipPush* gp;
    StateMachine* sm_Gossip;
public:
    dataActions(GossipPush* gpp, StateMachine* t_Gossip):gp(gpp), sm_Gossip(t_Gossip) {};
    virtual void enteringState(State* s, StateMachine* stateMachine, MessageType msg, void* extraData) {
        Gossip* g = check_and_cast_nullable<Gossip*>(dynamic_cast<Gossip*>((cPacket*)extraData));

        if ( g == nullptr )  {
            // panic
            EV_ERROR << "NOOOOOOOOOOOOO, processing a packet as Gossip when it is not Gossip\n";
            return;
        }

        gp->addNewInfection(g);

        delete g;

        // activate the ticker
        sm_Gossip->reportMessage(MSG_ACTIVATE);

        stateMachine->reportMessage(MSG_TRUE);
    }
};

StateMachine* GossipPush::createProtocolStateMachine()
{

    StateMachine* sm = new StateMachine(string("protocol_") + myself);

    sm_tick_hello = buildTicker(string("ticker hello"), HELLO_INTERVAL, sm, MSG_GREET, this);
    sm_tick_gossip = buildTicker(string("ticker gossip"), gossipInterval, sm, MSG_GOSSIP, this);
    sm_tick_new_gossip = buildTicker(string("ticker new gossip"), intervalAmongNewMessages, sm, MSG_NEW_GOSSIP, this);

    auto s = new State("s", new NoActions()); // done
    auto w = new State("w", new wActions(sm_tick_hello, sm_tick_new_gossip)); // done
    auto h = new State("h", new hActions(this)); // done
    auto g = new State("g", new gActions(this)); // done
    auto ng = new State("ng", new ngActions(this, sm_tick_gossip)); // done
    auto hello = new State("hello", new helloActions(this)); // done
    auto data = new State("data", new dataActions(this, sm_tick_gossip)); // done
    auto c = new State("c", new cActions(this)); // done

    sm->addState(s);
    sm->addState(w);
    sm->addState(h);
    sm->addState(g);
    sm->addState(ng);
    sm->addState(hello);
    sm->addState(data);
    sm->addState(c);

    // from s
    sm->addTransition(MSG_INITIALIZE, s, w);

    // from w
    sm->addTransition(MSG_NEW_GOSSIP, w,ng);
    sm->addTransition(MSG_GREET, w,h);
    sm->addTransition(MSG_HELLO, w,hello);
    sm->addTransition(MSG_DATA, w,data);
    sm->addTransition(MSG_GOSSIP, w,g);

    // from ng
    sm->addTransition(MSG_TRUE, ng, w);

    // from h
    sm->addTransition(MSG_TRUE, h, w);

    // from hello
    sm->addTransition(MSG_TRUE, hello, w);

    // from data
    sm->addTransition(MSG_TRUE, data, w);

    // from g
    sm->addTransition(MSG_EMPTY_MAILBOX, g, w);
    sm->addTransition(MSG_FULL_MAILBOX, g, c);

    // from c
    sm->addTransition(MSG_TRUE, c, w);

    return sm;
}

} //namespace
