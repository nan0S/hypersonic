#include "MCTS.hpp"

Node::Node()
{
	SIG();
	terminal = true;
	parent = nullptr;
	visits = 1;
	sum_score = 0;
	this->isRoot = false;
	this->clown_children = 0;
}

Node::Node(Node* parent)
{
	SIG();
	this->terminal = true;
	this->parent = parent;
	this->visits = 1;
	this->sum_score = 0.0;
	this->state.score = -5.0;
	this->isRoot = false;
}

Node::Node(State state, Node* parent, int visits, State::eval_t sum_score, bool terminal)
{
	SIG();
	this->state = state;
	this->parent = parent;
	this->visits = visits;
	this->sum_score = sum_score;

	this->actions = *(new std::vector<Action>());
	std::set<Action> as = state.game.getPossibleActions();
	this->actions.clear();
	for(auto x = as.begin(); x!= as.end(); x++)
    {
    	this->actions.push_back(*x);
    }

	for(const Action& a : this->actions)
	{
		auto p = state.game.getPlayer(state.game.myid);
		assert(abs(p->x - a.x) + abs(p->y - a.y) < 2);
	}
	this->isRoot = false;
	this->terminal = terminal;
	this->clown_children = 0;
}

void Node::setRoot()
{
	this->isRoot = true;
	std::set<Action> as = state.game.getPossibleActions();
	this->actions.clear();
	for(auto x = as.begin(); x!= as.end(); x++)
    {
    	this->actions.push_back(*x);
    }
}

Node::~Node()
{
	for(size_t i=0; i<this->children.size(); i++)
	{
		delete this->children[i];
	}
}

bool Node::fullyExpanded()
{
	int c_size = this->children.size();
	return c_size >= int(this->actions.size());
}

Node* Node::newChild(bool)
{
	Action newAction = this->actions[children.size()];

	auto succState = this->state.succ({{state.game.myid, newAction}});

	if(!succState)
	{
		children.push_back(new Node(this));
		return children[children.size() - 1];
	}

	State newState = succState.value();

	children.push_back(new Node(newState, this));
	return children[children.size()-1];
}

void Node::updateStats(State::eval_t result)
{
	this->sum_score += result/10.0f;
	this->visits += 1;
}

float Node::UTC(const Node* child)
{
	float C = 1.0f;
	float D = 1000.0f;

	float Xdash = child->sum_score/child->visits;
	float first_sqrt = sqrt(log(this->visits)/child->visits);
	float second_sqrt = sqrt((pow(child->sum_score, 2) - child->visits * Xdash + D)/child->visits);

	return Xdash + C*first_sqrt + second_sqrt;
}

Node* Node::childBestUTC()
{
	float bestVal = -1000000.f;
	int bestIt = -1;

	assert(this->children.size() > 0);
	for(size_t i = 0; i<this->children.size(); i++)
	{
		Node* child = this->children[i];
		float val = UTC(child);
		if(val > bestVal)
		{
			bestVal = val;
			bestIt = i;
		}
	}

	// bestIt = rand() % int(children.size());

	assert(bestIt >= 0);
	return this->children[bestIt];
}

int Node::pickBestChild()
{
	float bestVal = -10000000.0;
	int bestIt = -1;
	for(size_t i=0; i<children.size(); i++)
	{
		if(children[i]->sum_score/children[i]->visits > bestVal && children[i]->visits>1)
		{
			bestVal = children[i]->sum_score/children[i]->visits;
			bestIt = i;
		}
	}

	return bestIt;
}

int Node::pickBestUTCAction()
{
	float bestVal = -10000000.0;
	int bestIt = -1;
	for(size_t i=0; i<children.size(); i++)
	{
		if(UTC(children[i])> bestVal)
		{
			bestVal = UTC(children[i]);
			bestIt = i;
		}
	}
	return bestIt;
}

Action Node::pickBestAction()
{
	int it = this->pickBestChild();
	assert(it>=0);
	return this->actions[it];
}

int MCTS::_allcount = 0;

MCTS::MCTS() {
	name = "MCTS";
}

State MCTS::getInitialState(const GameState& game) {
    SIG();

    State initialState;

    initialState.game = game;
    initialState.explosions = game.getExplosions();
    assert(game.getPlayer(game.myid));
    initialState.myRange = game.getPlayer(game.myid)->range;
    initialState.myBombs = game.countAllBombs(game.myid);
    initialState.boxAdd = 0;
    initialState.boxSum = 0;
    initialState.firstLayer = true;
    initialState.score = initialState.eval();
    initialState.hash = initialState.getHash();

    return initialState;
}

Action MCTS::getAction(const GameState& game, const int timeLimit)
{
	State::evalFunction = eval;

	this->k_deep = 8;
	Node node = Node(this->getInitialState(game), nullptr);
	node.setRoot();

	auto t1 = std::chrono::high_resolution_clock::now();
	auto t2 = std::chrono::high_resolution_clock::now();

	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( t2 - t1 ).count();

	int _count = 0;
	while(duration < timeLimit)
	{
		assert(!node.parent);
		assert(node.isRoot);
		
        Node* leaf = this->traverse(&node);
		State::eval_t result;
		if(!leaf->terminal)
			result = this->rollout(leaf);
		else
			result = death_reward;

		this->backpropagate(leaf, result);
		t2 = std::chrono::high_resolution_clock::now();
		duration = std::chrono::duration_cast<std::chrono::milliseconds>( t2 - t1 ).count();
		_count++;
	}

	return node.pickBestAction();
}

Node* MCTS::traverse(Node* node)
{
	Node* curNode = node;
	while(curNode->fullyExpanded() && !curNode->terminal)
	{
		curNode = curNode->childBestUTC();
	}
	if(curNode->terminal)
		return curNode;
	else
		return curNode->newChild(false);
}

State::eval_t MCTS::rollout(Node* node)
{
	State curState = node->state;

	int k = 0;

	while(k < this->k_deep)
	{
        Action a = curState.getRandomAction();
        int c = 0;
        while(!curState.game.validActions({{curState.game.myid, a}}))
        {
        	a = curState.getRandomAction();
        	c++;
        	if(c >= 8)
        		return death_reward;
        }

		auto succState = curState.succ({{node->state.game.myid,a}});
		if(!succState)
			return death_reward;
		curState = succState.value();
		k++;
	}
	return curState.score;
}


State::eval_t MCTS::eval(const State& s) {
    State::eval_t points = 0.0;

    const State::eval_t boxReward = 10.0;
    points += boxReward * s.boxAdd;
    points += s.boxSum;

    auto me = s.game.getPlayer(s.game.myid);
    assert(me);

    points += 0.8 * std::min(5, s.myRange) + 0.4 * s.myRange;
    points += 6.5 * std::min(3, s.myBombs) + 3.1 * std::min(4, s.myBombs) + 1.5 * s.myBombs;
    points -= 0.2 * (s.myBombs - me->bombs);

    for (int y = 0; y < s.game.H; ++y)
        for (int x = 0; x < s.game.W; ++x)
            if (s.game.board.isBox(x, y) && s.explosions.isOwner(x, y, s.game.myid))
                points += boxReward - s.explosions.e[y][x].time;
            
    points -= 0.04 * std::abs(me->x - s.game.W * 0.5);
    points -= 0.04 * std::abs(me->y - s.game.H * 0.5);

    return points;
}

void MCTS::backpropagate(Node* node, State::eval_t result)
{
	assert(node);
	int k = 0;
	while(node)
	{
		if(node->isRoot)
		{
			node->visits += 1;
			return;
		}
		node->updateStats(result);
		k++;
		if(node->parent)
			node = node->parent;
	}
}
