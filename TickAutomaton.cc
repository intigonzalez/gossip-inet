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
    virtual void enteringState(State& s, StateMachine* stateMachine) {
        target->reportMessage(msgId);
        stateMachine->reportMessage(TRUE);
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


    virtual void enteringState(State& s, StateMachine* stateMachine) {
        sm = stateMachine;
        top->registerListener(this, d);
    }

    virtual void timeOut() {
        sm->reportMessage(TIME_OUT);
    }
};


StateMachine* buildTicker(string name, double d, StateMachine* target, MessageType msgId, ITimeOutProducer* top)
{
    StateMachine* sm = new StateMachine();

    // adding initial state
    StateActions* a = new NoActions();
    State s0(string("initial"), a);
    sm->addState(s0);

    // adding middle state
    a = new ActivateTick(d, top);
    State s1(string("middle"), a);
    sm->addState(s1);

    // adding last state
    a = new NotifyTick(target, msgId);
    State s2(string("last"), a);
    sm->addState(s2);

    // adding transitions
    sm->addTransition(TRUE, s2, s1);
    sm->addTransition(FALSE, s1, s2);
    sm->addTransition(TIME_OUT, s1, s2);
    sm->addTransition(ACTIVATE, s0, s1);

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



