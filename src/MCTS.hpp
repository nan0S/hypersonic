#ifndef MCTS_HPP
#define MCTS_HPP

#include "Agent.hpp"
#include "Action.hpp"
#include "GameState.hpp"
#include "Common.hpp"
#include "State.hpp"

class Node
{
public:
	bool terminal;
	int clown_children;

	Node *parent;
	State state;
	bool isRoot;
	std::vector<Node*> children;
	int visits;
	State::eval_t sum_score;

	std::vector<Action> actions;

	Node();
	Node(Node* parent);
	Node(State state, Node* parent, int visits = 1, State::eval_t sum_score = 0.f, bool terminal = false);
	~Node();

	bool fullyExpanded();

	Node* newChild(bool dbg = false);

	void setRoot();
	void updateStats(State::eval_t result);

	float UTC(const Node* node);

	Node* childBestUTC();
	int pickBestChild();
	int pickBestUTCAction();
	Action pickBestAction();
};

class MCTS : public Agent
{
public:
	Node* root;
	int k_deep;

	float death_reward = -5.0f;

	static int _allcount;

	MCTS();

	State getInitialState(const GameState& game);

	Action getAction(const GameState& game, const int timeLimit) override;
	Node* traverse(Node* node);
	State::eval_t rollout(Node* node);
	void backpropagate(Node* node, State::eval_t result);

	static State::eval_t eval(const State& state);
};

#endif /* MCTS_HPP */
