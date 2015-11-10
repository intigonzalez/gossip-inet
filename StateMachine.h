/*
 * StateMachine.h
 *
 *  Created on: Nov 10, 2015
 *      Author: inti
 */

#ifndef STATEMACHINE_H_
#define STATEMACHINE_H_

#include <vector>
#include <string>
#include <stdexcept>

using std::vector;
using std::string;
using std::runtime_error;

namespace inet {


typedef int MessageType;

enum BasicTypes {
    TRUE = 1,
    FALSE = 2
};

class StateMachine;
class State;
class MessagePool;

/**
 * A state Machine, isn't it obvious? :-P
 */
class StateMachine {
protected:
    vector<State> states;
    int initialState = 0;
    MessagePool* pool;
public:
    StateMachine();
    virtual ~StateMachine();

    bool addState(State& s);

    bool addTransition(MessageType id, State& from, State& to);

    State& getInitialState() { return states[initialState]; }
    void setInitialState(State& s);

    State& getState(int idx);

    virtual void reportMessage(MessageType msgId);

    MessagePool* getPool();
};

/**
 *
 */
class Transition {
protected:
    MessageType msgID;

    int to;
public:
    Transition(MessageType id, int t):msgID(id), to(t) {};
    virtual ~Transition();

    MessageType getMessageId() const { return msgID; }
    int getTo() const { return to; }
};

class StateActions {
public:
    virtual void enteringState(State& s, StateMachine* stateMachine) = 0;
};

class NoActions : public StateActions {
public:
    virtual void enteringState(State& s, StateMachine* stateMachine) {}
};

/**
 *
 */
class State {
protected:
    string name;
    vector<Transition> transitions;
    StateMachine* owner = nullptr;
    StateActions* actions = nullptr;
public:

    State(string n, StateActions* a):name(n),actions(a) {}
    State(const State& other);

    void addTransition(MessageType id, int to);

    bool existsTransition(MessageType id);

    State& next(MessageType id);

    StateActions* getActions()  { return actions; }

    bool operator==(const State& other);

    friend bool StateMachine::addState(State& s);
};

/**
 * A pool of messages, notice that is not a queue
 * FIXME: I should be sent to jail for this implementation
 */
class MessagePool {
protected:
    vector<MessageType> messages;
public:
    void add(MessageType msg) { messages.push_back(msg); }
    bool isEmpty() {  return messages.size() == 0;  }
    int count() { return messages.size(); }
    MessageType operator[] (int idx) { return messages[idx]; }
    MessageType drop(int idx);
};

} /* namespace inet */

#endif /* STATEMACHINE_H_ */
