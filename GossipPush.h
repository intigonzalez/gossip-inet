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

#ifndef __INET_GOSSIPPUSH_H_
#define __INET_GOSSIPPUSH_H_

#include <omnetpp.h>

#include <map>
#include <vector>
#include <string>

#include "inet/common/INETDefs.h"

#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UDPSocket.h"

#include "TickAutomaton.h"
#include "StateMachine.h"
#include "StateMachineInterpreter.h"

namespace inet {

using std::map;
using std::vector;
using std::string;

const double HELLO_INTERVAL = 0.6;

/**
 * TODO - Generated class
 */
class INET_API GossipPush : public ApplicationBase, public ITimeOutProducer
{
  protected:

    int destinationPort = 10000;
    int localPort = 10000;

    bool isSource = false; // indicates whether the app is the source of messages
    int numMessages = 1; // how many messages to send
    double intervalAmongNewMessages = 5; // how much time to wait between two different new messages created in this node

    // gossip stuff
    int nodesPerRound = 1; // this node will gossip with 'nodesPerRound' in each round
    int roundRatio = 2; // the number of rounds is 'roundRatio*numberOfAddresses'

    double gossipInterval = 0.1;

    map<string, L3Address> addresses; // network members
    vector<L3Address> possibleNeighbors;

    // to assign ids to messages
    int lastIdMsg = 1;

    // list of known infections ( messages )
    class GossipInfection {
    public:
        int idMsg;
        string source;
        string text;
        int roundsLeft;
    };
    vector<GossipInfection> infections;


    // communication
    UDPSocket socket;

    // control messages
    cMessage* ctrlMsg0 = nullptr;
    cMessage* ctrlMsg1 = nullptr;
    cMessage* ctrlHello = nullptr;

    // myself as a module
    string myself;
    L3Address myAddress;

    // a state machine
    StateMachine* anotherSM;
    StateMachine* sm_tick_gossip;
    StateMachine* sm_tick_new_gossip;
    StateMachine* sm_tick_hello;
    StateMachine* sm_proptocol;
    vector<StateMachineInterpreter*> interpreters;

    map<cMessage*, ITimeOut*> timers;

  protected:

    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessageWhenUp(cMessage *msg) override;
    virtual void finish() override;

    virtual bool handleNodeStart(IDoneCallback *doneCallback) override;
    virtual bool handleNodeShutdown(IDoneCallback *doneCallback) override;
    virtual void handleNodeCrash() override;

    virtual void registerListener(ITimeOut* listener, double afterElapsedTime) override;

    virtual void processStart();

    void interpreting();

    virtual StateMachine* createProtocolStateMachine();

  public: // and by making this public, I am just signing my death sentence
    bool gossiping();
    bool sayHello();
    void newGossip();
private:
    static const int TICK_MESSAGE = 456;


};

} //namespace

#endif
