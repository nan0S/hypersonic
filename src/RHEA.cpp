#include "RHEA.hpp"

#include "Action.hpp"
#include "Common.hpp"

#include <algorithm>
#include <cstring>
#include <iomanip>

using Gene = RHEA::Gene;
using Chromosome = RHEA::Chromosome;
using eval_t = RHEA::eval_t;

RHEA::RHEA() {
    name = "RHEA";

    possibleGenes.clear();
    for (const auto& actionType : GameState::actionTypes)
        for (int i = 0; i < GameState::DIR_COUNT; ++i) {
            Gene possibleGene = {actionType, GameState::dx[i], GameState::dy[i]};
            possibleGenes.insert(possibleGene);
            possibleGenesVec.push_back(possibleGene);
        }

    setInitialPopulation();
}

void RHEA::setInitialPopulation() {
    for (auto& chromosome : population.chromosomes)
        chromosome = getRandomChromosome();
}

Chromosome RHEA::getRandomChromosome() const {
    Chromosome chromosome;
    chromosome.genes[0] = getRandomPossibleGene();
    for (int i = 1; i < HORIZON_LENGTH; ++i)
        chromosome.genes[i] = getRandomGene();
    return chromosome;
}

Gene RHEA::getRandomPossibleGene() const {
    assert(!possibleGenes.empty());
    return possibleGenesVec[Random::rand(possibleGenesVec.size())];
}

Gene RHEA::getRandomGene() const {
    int actionTypeVar = Random::rand(GameState::MAX_ACTION_TYPES);
    int dirVar = Random::rand(GameState::DIR_COUNT);
    return {actionTypeVar == 0 ? ActionType::move : ActionType::bomb,
        GameState::dx[dirVar], GameState::dy[dirVar]};
}

Action RHEA::getAction(const GameState& game,
                       const int timeLimit) {
    Timer timer(timeLimit);
    State::evalFunction = eval;

    this->game = &game;

    /* setting possible first genes */
    auto possibleActions = game.getPossibleActions();
    auto me = game.getPlayer(myid);
    assert(me);

    possibleGenes.clear();
    possibleGenesVec.clear();
    for (const auto& possibleAction : possibleActions) {
        Gene possibleGene = {possibleAction.type,
            possibleAction.x - me->x, possibleAction.y - me->y};
        possibleGenes.insert(possibleGene);
        possibleGenesVec.push_back(possibleGene);
    }

    // for (const auto& possibleAction : possibleActions)
        // std::cerr << "possibleAction: " << possibleAction << std::endl;

    for (auto& chromosome : population.chromosomes) {
        auto action = getActionFromGene(me, chromosome.genes[0]);
        if (!possibleActions.count(action))
            chromosome.genes[0] = getRandomPossibleGene();
    }

    evaluatePopulation(population);

    int generation;
    for (generation = 0; timer.isTimeLeft(); ++generation) {
        BENCH();
        /* it should be already evaluated */
        // for (const auto& chromosome : population.chromosomes)
            // assert(chromosome.evaluation == evaluateChromosome(chromosome));
        calculateFitness();
        if (!timer.isTimeLeft())
            break;
        generateChildren();
        if (!timer.isTimeLeft())
            break;
        replacePopulation();
    }

    auto bestAction = getBestAction();
    assert(possibleGenes.count({bestAction.type, bestAction.x - me->x, bestAction.y - me->y}));

    /* logging */
    if (verbose) {
        std::cerr << "Generations done: " << generation << std::endl;
        std::cerr << "Population diveristy: " << getDiversity(population) << std::endl;

        auto succInfo = game.succ(game.getExplosions(), {{myid, bestAction}});
        assert(succInfo);
        const auto& nextGame = succInfo.value().first;
        std::cerr << "Can survive: " << nextGame.canSurvive(nextGame.getExplosions()) << std::endl;

        std::cerr << "Evals: ";
        for (const auto& chromosome : population.chromosomes)
            std::cerr << chromosome.evaluation << " ";
        std::cerr << std::endl;
    }

    advanceHorizon();
    
    return bestAction;
}

eval_t RHEA::evaluateChromosome(const Chromosome& chromosome) const {
    assert(game);
    auto state = State::getInitialState(*game);
    int canSurvive = -1;
    int actionsDone = 0;

    for (const auto& gene : chromosome.genes) {
        auto me = state.game.getPlayer(myid);
        assert(me);

        auto action = getActionFromGene(me, gene);
        auto succInfo = state.succ({{myid, action}});

        if (succInfo) {
            state = succInfo.value();
            ++actionsDone;
            if (canSurvive == -1 && !state.canSurvive())
                canSurvive = actionsDone;
        }
    }

    eval_t punishment = canSurvive == -1 ?
        0 : HORIZON_LENGTH - canSurvive;

    return state.score - 10 * punishment;
}

Action RHEA::getActionFromGene(const Player* me, const Gene& gene) const {
    return {gene.type, me->x + gene.dx, me->y + gene.dy};
}

void RHEA::calculateFitness() {
    BENCH();
    /* TODO: improve that loops */
    auto minValue = std::min_element(
        population.chromosomes,
        population.chromosomes + population.SIZE,
        [](const Chromosome& c1, const Chromosome& c2){
            return c1.evaluation < c2.evaluation;
        })->evaluation;

    eval_t sum = 0;
    for (const auto& chromosome : population.chromosomes)
        sum += (chromosome.evaluation - minValue);

    /* TODO: maybe do not divide / maybe use sigmoid */
    if (sum == 0) {
        eval_t uniform = eval_t(1) / population.SIZE;
        for (auto& chromosome : population.chromosomes)
            chromosome.fitness = uniform;
    }
    else {
        for (auto& chromosome : population.chromosomes)
            chromosome.fitness = (chromosome.evaluation - minValue) / sum;
    }

    eval_t fitnessSum = 0;
    for (const auto& chromosome : population.chromosomes)
        fitnessSum += chromosome.fitness;
    if (!almostEqual<eval_t>(fitnessSum, 1)) {
        std::cerr << std::setprecision(10) << std::fixed;
        debug(fitnessSum);
    }
    assert(almostEqual<eval_t>(fitnessSum, 1));
}

void RHEA::generateChildren() {
    BENCH();
    /* recombination */
    for (int i = 0; i < children.SIZE; i += 2) {
        int p1 = chooseParentIndex();
        int p2 = chooseParentIndex();
        crossover(population.chromosomes[p1], population.chromosomes[p2],
            children.chromosomes[i], children.chromosomes[i + 1]);
    }
    if (children.SIZE % 2 == 1) {
        int index = Random::rand(population.SIZE);
        children.chromosomes[children.SIZE - 1] = population.chromosomes[index];
    }

    /* mutation */
    for ( auto& chromosome : children.chromosomes)
        mutation(chromosome);
}

int RHEA::chooseParentIndex() const {
    eval_t fitnessSum = 0;
    for (const auto& chromosome : population.chromosomes)
        fitnessSum += chromosome.fitness;
    assert(almostEqual<eval_t>(fitnessSum, 1));

    /* TODO: improve with binary search and prefix sum table */
    eval_t value = Random::rand<eval_t>();
    eval_t pref = 0;
    for (int i = 0; i < population.SIZE; ++i) {
        const auto& chromosome = population.chromosomes[i];
        pref += chromosome.fitness;
        if (value <= pref)
            return i;
    }

    debug(pref, value);
    assert(false);
}

void RHEA::crossover(const Chromosome& p1, const Chromosome& p2, 
                     Chromosome& c1, Chromosome& c2) const {
    // uniformCrossover(p1, p2, c1, c2);
    shuffleCrossover(p1, p2, c1, c2);
}

void RHEA::uniformCrossover(const Chromosome& p1, const Chromosome& p2, 
                            Chromosome& c1, Chromosome& c2) const {
    int splitIndex = Random::rand(HORIZON_LENGTH + 1);
    assert(0 <= splitIndex && splitIndex <= HORIZON_LENGTH);
    for (int i = 0; i < splitIndex; ++i) {
        c1.genes[i] = p1.genes[i];
        c2.genes[i] = p2.genes[i];
    }
    for (int i = splitIndex; i < HORIZON_LENGTH; ++i) {
        c1.genes[i] = p2.genes[i];
        c2.genes[i] = p1.genes[i];
    }
}

void RHEA::shuffleCrossover(const Chromosome& p1, const Chromosome& p2, 
                            Chromosome& c1, Chromosome& c2) const {
    for (int i = 0; i < HORIZON_LENGTH; ++i) {
        if (Random::rand<float>() <= 0.5f) {
            c1.genes[i] = p1.genes[i];
            c2.genes[i] = p2.genes[i];
        }
        else {
            c1.genes[i] = p2.genes[i];
            c2.genes[i] = p1.genes[i];
        }
    }
}

void RHEA::mutation(Chromosome& chromosome) const {
    if (Random::rand<float>() < MUTATION_PROBABILITY)
        chromosome.genes[0] = getRandomPossibleGene();
    for (int i = 1; i < HORIZON_LENGTH; ++i)
        if (Random::rand<float>() < MUTATION_PROBABILITY)
            chromosome.genes[i] = getRandomGene();
}

void RHEA::replacePopulation() {
    BENCH();
    assert(population.SIZE <= children.SIZE);

    #if 0
    /* Mi, Lambda replacement with full elitism */
    evaluatePopulation(children);
    std::partial_sort(
        children.chromosomes,
        children.chromosomes + population.SIZE,
        children.chromosomes + children.SIZE,
        [](const Chromosome& c1, const Chromosome& c2){
            return c1.evaluation > c2.evaluation;
        });

    for (int i = 0; i < population.SIZE - 1; ++i)
        assert(children.chromosomes[i].evaluation >= children.chromosomes[i + 1].evaluation);
    for (int i = population.SIZE; i < children.SIZE; ++i)
        assert(children.chromosomes[population.SIZE - 1].evaluation >= children.chromosomes[i].evaluation);

    std::memcpy(population.chromosomes,
        children.chromosomes,
        population.SIZE * sizeof(Chromosome));

    assert(sortedSanityCheck(population));
    #else
    /* Mi + Lambda replacement with full elitism */
    evaluatePopulation(children);

    std::memcpy(merged.chromosomes,
        population.chromosomes,
        population.SIZE * sizeof(Chromosome));
    std::memcpy(merged.chromosomes + population.SIZE,
        children.chromosomes,
        children.SIZE * sizeof(Chromosome));

    for (const auto& chromosome : merged.chromosomes)
        assert(chromosome.evaluation == evaluateChromosome(chromosome));

    std::partial_sort(merged.chromosomes,
        merged.chromosomes + population.SIZE,
        merged.chromosomes + merged.SIZE,
        [](const Chromosome& c1, const Chromosome& c2){
            return c1.evaluation > c2.evaluation; 
        });

    for (int i = 0; i < population.SIZE - 1; ++i)
        assert(merged.chromosomes[i].evaluation >= merged.chromosomes[i + 1].evaluation);
    for (int i = population.SIZE; i < population.SIZE + children.SIZE; ++i)
        assert(merged.chromosomes[population.SIZE - 1].evaluation >= merged.chromosomes[i].evaluation);

    std::memcpy(population.chromosomes,
        merged.chromosomes,
        population.SIZE * sizeof(Chromosome));

    assert(sortedSanityCheck(population));
    #endif
}

Action RHEA::getBestAction() {
    assert(sortedSanityCheck(population));

    Action bestAction;
    bool bestActionFound = false;
    auto state = State::getInitialState(*game);

    for (const auto& gene : population.chromosomes[0].genes) {
        auto me = state.game.getPlayer(myid);
        assert(me);
        auto action = getActionFromGene(me, gene);
        auto succInfo = state.succ({{myid, action}});

        if (succInfo) {
            bestAction = action;
            bestActionFound = true;
            break;
        }
    }

    if (!bestActionFound) {
        // if (verbose)
            std::cerr << "Probably dying!" << std::endl;
        auto me = state.game.getPlayer(myid);
        assert(me);
        bestAction = {ActionType::move, me->x, me->y};
    }

    return bestAction;
}

void RHEA::advanceHorizon() {
    std::memmove(population.chromosomes,
        population.chromosomes + 1,
        (population.SIZE - 1) * sizeof(Chromosome));

    for (auto& chromosome : population.chromosomes)
        chromosome.genes[HORIZON_LENGTH - 1] = getRandomGene();
}

eval_t RHEA::eval(const State& s) {
    eval_t points = 0.0;

    const eval_t boxReward = 9;
    points += boxReward * s.boxAdd;
    points += s.boxSum * 2;

    auto me = s.game.getPlayer(s.game.myid);
    assert(me);
    
    points += 0.65 * std::min(5, s.myRange) + 0.5 * s.myRange;
    points += 3.2 * std::min(s.myBombs, 3) + 1.9 * s.myBombs;
    points -= 0.2 * (s.myBombs - me->bombs);

    for (int y = 0; y < s.game.H; ++y)
        for (int x = 0; x < s.game.W; ++x)
            if (s.game.board.isBox(x, y) && s.explosions.isOwner(x, y, s.game.myid))
                points += 1 * (boxReward - s.explosions.e[y][x].time);
            
    points -= 0.04 * std::abs(me->y - s.game.H * 0.5);
    points -= 0.04 * std::abs(me->x - s.game.W * 0.5);

    // if (!s.canSurvive())
        // points -= 20;

    return points;
}
