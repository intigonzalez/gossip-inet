/*
 * TickAutomaton.cc
 *
 *  Created on: Nov 10, 2015
 *      Author: inti
 */

#include "TickAutomaton.h"

namespace inet {

class NotifyTick: public StateActions {
protected:
    StateMachine* target;
    MessageType msgId;
public:
    NotifyTick(StateMachine* t, MessageType mi):target(t), msgId(mi) {}
    virtual void enteringState(State* s, StateMachine* stateMachine, MessageType msg, void* extraData) {
        target->reportMessage(msgId);
        stateMachine->reportMessage(MSG_TRUE);
    }
};

class ActivateTick: public StateActions, public ITimeOut {
protected:
    StateMachine* sm;
    ITimeOutProducer* top;
    double d;
public:

    ActivateTick(double d, ITimeOutProducer* top) {
        this->top= top;
        this->d = d;
    }


    virtual void enteringState(State* s, StateMachine* stateMachine, MessageType msg, void* extraData) {
        sm = stateMachine;
        top->registerListener(this, d);
    }

    virtual void timeOut() {
        sm->reportMessage(MSG_TIME_OUT);
    }
};


StateMachine* buildTicker(string name, double d, StateMachine* target, MessageType msgId, ITimeOutProducer* top)
{
    StateMachine* sm = new StateMachine(name);

    // adding initial state
    StateActions* a = new NoActions();
    State* s0 = new State(string("initial"), a);
    sm->addState(s0);

    // adding middle state
    a = new ActivateTick(d, top);
    State* s1 = new State(string("middle"), a);
    sm->addState(s1);

    // adding last state
    a = new NotifyTick(target, msgId);
    State* s2 = new State(string("last"), a);
    sm->addState(s2);

    // adding transitions
    sm->addTransition(MSG_TRUE, s2, s1);
    sm->addTransition(MSG_FALSE, s1, s2);
    sm->addTransition(MSG_TIME_OUT, s1, s2);
    sm->addTransition(MSG_ACTIVATE, s0, s1);
    //sm->addTransition(MSG_ACTIVATE, s1, s1);

    return sm;
}

StateMachine* buildDummyAutomaton(MessageType msgId)
{
    StateMachine* sm = new StateMachine("dummy");
    StateActions* a = new LogActions(std::string("I received a message from a remote automaton"));
    State* s0 = new State(string("initial"), a);
    sm->addState(s0);
    sm->addTransition(msgId, s0, s0);
    return sm;
}

void ITimeOutProducer::registerListener(ITimeOut* listener, double afterElapsedTime)
{
    ITimeOutProducer::Record r = ITimeOutProducer::Record {
            afterElapsedTime, listener
    };
    listeners.push_back(r);
}

}



