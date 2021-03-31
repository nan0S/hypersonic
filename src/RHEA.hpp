#ifndef RHEA_HPP
#define RHEA_HPP

#include "Agent.hpp"
#include "State.hpp"

class RHEA : public Agent {
public:
    using eval_t = State::eval_t;

    static constexpr int POP_SIZE = 100;
    static constexpr int HORIZON_LENGTH = 8;
    static constexpr int OFFSPRING_SIZE = 100;
    static constexpr float MUTATION_PROBABILITY = 0.6;

    static_assert(POP_SIZE <= OFFSPRING_SIZE,
        "Offspring size has to be strictly greater "
        "than population size!");

    using Gene = Action;
    struct Chromosome {
        Gene genes[HORIZON_LENGTH];
        eval_t evaluation;
        eval_t fitness;
    };
    template<int _SIZE>
    struct Population {
        static constexpr int SIZE = _SIZE;
        Chromosome chromosomes[SIZE];
    };  

public:
    RHEA();

    Action getAction(const GameState& game,
                     const int timeLimit) override;
    
    static eval_t eval(const State& state);

private:
    void setInitialPopulation();
    Chromosome getRandomChromosome() const;
    Gene getRandomPossibleGene() const;
    Gene getRandomGene() const;
    Action getActionFromGene(const Player* me, const Gene& gene) const;
    template<int SIZE>
    void evaluatePopulation(Population<SIZE>& population) const;
    eval_t evaluateChromosome(const Chromosome& chromosome) const;
    void calculateFitness();
    void generateChildren();
    int chooseParentIndex() const;
    void crossover(const Chromosome& p1, const Chromosome& p2,
                   Chromosome& c1, Chromosome& c2) const;
    void uniformCrossover(const Chromosome& p1, const Chromosome& p2,
                          Chromosome& c1, Chromosome& c2) const;
    void shuffleCrossover(const Chromosome& p1, const Chromosome& p2,
                          Chromosome& c1, Chromosome& c2) const;
    void mutation(Chromosome& chromosome) const;
    void replacePopulation();
    template<int SIZE>
    bool sortedSanityCheck(const Population<SIZE>& population) const;
    Action getBestAction();
    void advanceHorizon();
    template<int SIZE>
    float getDiversity(const Population<SIZE>& population) const;

private:
    const GameState* game = nullptr;
    std::set<Gene> possibleGenes;
    std::vector<Gene> possibleGenesVec;

    Population<POP_SIZE> population;
    Population<OFFSPRING_SIZE> children;
    Population<POP_SIZE + OFFSPRING_SIZE> merged;
};

template<int SIZE>
void RHEA::evaluatePopulation(Population<SIZE>& population) const {
    BENCH();
    for (auto& chromosome : population.chromosomes)
        chromosome.evaluation = evaluateChromosome(chromosome);
}

template<int SIZE>
bool RHEA::sortedSanityCheck(const Population<SIZE>& population) const {
    for (const auto& chromosome : population.chromosomes)
        if (evaluateChromosome(chromosome) != chromosome.evaluation)
            return false;
    for (int i = 0; i < population.SIZE - 1; ++i)
        if (population.chromosomes[i].evaluation < population.chromosomes[i + 1].evaluation)
            return false;
    return true;
}

template<int SIZE>
float RHEA::getDiversity(const Population<SIZE>& population) const {
    // for (const auto& chromosome : population.chromosomes)
        // assert(chromosome.evaluation == evaluateChromosome(chromosome));
    std::set<eval_t> evals;
    for (const auto& chromosome : population.chromosomes)
        evals.insert(chromosome.evaluation);
    return float(evals.size()) / SIZE;
}

#endif /* RHEA_HPP */
