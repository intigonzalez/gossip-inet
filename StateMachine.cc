/*
 * StateMachine.cc
 *
 *  Created on: Nov 10, 2015
 *      Author: inti
 */

#include "StateMachine.h"

#include <algorithm>
#include <iterator>     // std::distance

#include <iostream>

using namespace std;

namespace inet {

StateMachine::StateMachine() {
    pool = new MessagePool();
}

StateMachine::~StateMachine() {
}

bool StateMachine::addState(State* s)
{
    if (s->owner) return false;
    s->owner = this;
    this->states.push_back(s);
    return true;
}

bool StateMachine::addTransition(MessageType id, State* from, State* to)
{
    vector<State*>::iterator it0 = std::find(states.begin(), states.end(), from);
    vector<State*>::iterator it1 = std::find(states.begin(), states.end(), to);

    if (it0 == states.end()) return false;
    if (it1 == states.end()) return false;

    int idx1 = std::distance(states.begin(), it1);

    from->addTransition(id, idx1);
    return true;
}

void StateMachine::setInitialState(State* s)
{
    vector<State*>::iterator it0 = std::find(states.begin(), states.end(), s);
    if (it0 != states.end()) {
        initialState = std::distance(states.begin(), it0);
    }
}

State* StateMachine::getState(int idx)
{
    return states[idx];
}

MessagePool* StateMachine::getPool()
{
    return pool;
}

void StateMachine::reportMessage(MessageType msgId)
{
    pool->add(msgId);
}


State::State(const State& other)
{
    this->name = other.name;
    this->owner = other.owner;
    this->actions = other.actions;
    for (Transition* t : other.transitions)
        transitions.push_back(t);
}

void State::addTransition(MessageType id, int to)
{
    transitions.push_back( new Transition(id, to) );
}

bool State::existsTransition(MessageType id)
{
    return std::any_of(transitions.begin(), transitions.end(), [&] (Transition* t) {
        return t->getMessageId() == id;
    });
}

State* State::next(MessageType id)
{
    vector<Transition*>::iterator it = std::find_if(transitions.begin(), transitions.end(), [&] (Transition* t) {
        return t->getMessageId() == id;
    });

    if (it != transitions.end()) {
        int to = (*it)->getTo();

        return this->owner->getState(to);
    }
    throw runtime_error("There is not a transition with such an ID");
}

bool State::operator==(const State& other)
{
    return this->name == other.name && this->owner == other.owner;
}

MessageType MessagePool::drop(int idx)
{
    vector<MessageType>::iterator it = messages.begin();
    std::advance(it, idx);
    MessageType m = *it;
    messages.erase(it);
    return m;
}

Transition::~Transition()
{

}

} /* namespace inet */
