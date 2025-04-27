
#include <assert.h>
#include <bits/std_abs.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <cmath>
#include <functional>
#include <random>
#include <iostream>
#include <cstdio>

#include "app/sat/data/clause_metadata.hpp"
#include "util/logger.hpp"
#include "app/sat/data/portfolio_sequence.hpp"
#include "app/sat/data/solver_statistics.hpp"
#include "app/sat/execution/solver_setup.hpp"
#include "app/sat/sharing/store/generic_clause_store.hpp"
#include "app/sat/solvers/portfolio_solver_interface.hpp"

extern "C" {
#include "gimsatul/libgimsatul.h"
}
#include "gimsatul.hpp"
#include "util/distribution.hpp"




void gimsatul_produce_clause(void* state, int size, int glue, int ring_id) {
    ((Gimsatul*) state)->produceClause(size, glue, ring_id);
}

void gimsatul_consume_clause(void* state, int** clause, int* size, int* glue) {
    ((Gimsatul*) state)->consumeClause(clause, size, glue);
}

// int terminate_callback(void* state) {
//     return 0;
// }

/*
    bool mallob_diversification;
    std::vector<std::vector<bool>> initial_phases;
    std::vector<bool*> initial_phases_pointer;
*/

Gimsatul::Gimsatul(const SolverSetup& setup) :
    PortfolioSolverInterface(setup),
    learntClauseBuffer(setup.threads, std::vector<int>(_setup.strictMaxLitsPerClause+ClauseMetadata::numInts())),
    bufferPointer(setup.threads, nullptr),
    learntClauses(setup.threads, Mallob::Clause()),
    portfolio_size(setup.portfolio_size),
    mallob_diversification(false),
    initial_phases(setup.threads, std::vector<char>(setup.numVars, 1)),
    initial_phases_pointer(setup.threads, nullptr),
    predicted_num_vars(setup.numVars) {

    for (size_t i = 0; i < setup.threads; i++) {
        bufferPointer[i] = learntClauseBuffer[i].data();
        initial_phases_pointer[i] = initial_phases[i].data();
    }
    
    solver = gimsatul_init(setup.numVars, setup.numOriginalClauses, initial_phases_pointer.data());

    int success = gimsatul_set_option(solver, "threads", setup.threads);
    assert (success == 0);
    
    if (setup.searchOnly) {
        success += gimsatul_set_option(solver, "simplify", 0);
        assert (success == 0);
        success += gimsatul_set_option(solver, "simplify_regularly", 0);
        assert (success == 0);
        success += gimsatul_set_option(solver, "simplify_initially", 0);
        assert (success == 0);
        success += gimsatul_set_option(solver, "probe", 0);
        assert (success == 0);
    }
    

    if (success != 0)
        LOGGER(_logger, V1_WARN, "[WARN] Gimsatul Options could not be set successfully\n");
    assert (success == 0);
}

void Gimsatul::addLiteral(int lit) {
    // std::cout << ">>>>> addLiteral" << std::endl;
    gimsatul_add(solver, lit);
    numVars = std::max(numVars, std::abs(lit));
}


void Gimsatul::calc_phases(int seed) {
    // if (!_params.diversifyPhases()) return;
    if (getGlobalId() < getNumOriginalDiversifications()) return;

    int solversCount = get_threads(solver);
    int totalSolvers = solversCount * portfolio_size;
    int vars = predicted_num_vars;

    // std::cout << ">> solversCount=" << solversCount << " totalSolvers=" << totalSolvers << " vars=" << vars << " getGlobalId=" << getGlobalId() << std::endl;

    // calculate initial phases for each ring
    for (int ring_id = 0; ring_id < _setup.threads; ring_id++) {
        // calculate phase array
        SplitMix64Rng rng = SplitMix64Rng(seed + getGlobalId() + ring_id);
        for (int i = 0; i < vars; i++) {
            float numSolversRand = rng.randomInRange(0, totalSolvers);
            if (numSolversRand < 1) {
                initial_phases[ring_id][i] = -1;
                // std::cout << ">> numSolversRand < 1" << std::endl;
            }
        }
    }
}

void Gimsatul::diversify(int seed) {
    calc_phases(seed);
}

int Gimsatul::getNumOriginalDiversifications() {
    return 0;
}

void Gimsatul::setPhase(const int var, const bool phase) {    
    // ignore and implement in diversify
    /*assert(!initialVariablePhasesLocked);
	if (var >= initialVariablePhases.size())
        initialVariablePhases.resize(var+1);
    initialVariablePhases[var] = phase ? 1 : -1;*/
}

// Solve the formula with a given set of assumptions
// return 10 for SAT, 20 for UNSAT, 0 for UNKNOWN
SatResult Gimsatul::solve(size_t numAssumptions, const int* assumptions) {
    // TODO handle assumptions?
    // assert(numAssumptions == 0);

    // Push the initial variable phases to gimsatul
    initialVariablePhasesLocked = true;
    gimsatul_set_initial_variable_phases (solver, initialVariablePhases.data(), initialVariablePhases.size());

    // std::cout << ">>>>> solve" << std::endl;
    // start solving
    int res = gimsatul_solve(solver);
    switch (res) {
        case 10:
            return SAT;
        case 20:
            return UNSAT;
        default:
            return UNKNOWN;
    }
}

void Gimsatul::setSolverInterrupt() {}

void Gimsatul::unsetSolverInterrupt() {}

void Gimsatul::setSolverSuspend() {}

void Gimsatul::unsetSolverSuspend() {}

bool Gimsatul::shouldTerminate() {
    return false;
}

void Gimsatul::cleanUp() {}

std::vector<int> Gimsatul::getSolution() {
    std::vector<int> result = {0};

    for (int i = 1; i <= getVariablesCount(); i++) {
        int val = gimsatul_value(solver, i);
        assert(val == i || val == -i || val == 0 ||
               LOG_RETURN_FALSE("[ERROR] value of variable %i/%i returned %i\n",
                                i, getVariablesCount(), val));
        result.push_back(val == 0 ? -i : val);
    }

    return result;
}

std::set<int> Gimsatul::getFailedAssumptions() {
    // return empty set
    std::set<int> x;
    return x;
}

void Gimsatul::setLearnedClauseCallback(const LearnedClauseCallback& callback) {
    this->callback = callback;
    gimsatul_set_clause_export_callback(solver, this, bufferPointer.data(), _setup.strictMaxLitsPerClause, &gimsatul_produce_clause);
    gimsatul_set_clause_import_callback(solver, this, &gimsatul_consume_clause);
}

void Gimsatul::produceClause(int size, int lbd, int ring_id) {
    interruptionInitialized = true;
    if (size > _setup.strictMaxLitsPerClause) return;
    auto &learntClause = learntClauses[ring_id];
    learntClause.size = size;
    // In Kissat, long clauses of LBD 1 can be exported. => Increment LBD in this case.
    learntClause.lbd = learntClause.size == 1 ? 1 : lbd;
    if (learntClause.lbd == 1 && learntClause.size > 1) learntClause.lbd++;
    if (learntClause.lbd > _setup.strictLbdLimit) return;
    assert (ring_id >= 0);
    assert (bufferPointer[ring_id] != nullptr);
    learntClause.begin = bufferPointer[ring_id];
    callback(learntClause, _setup.localId);
}

void Gimsatul::consumeClause(int** clause, int* size, int* lbd) {
    Mallob::Clause c;
    bool success = fetchLearnedClause(c, GenericClauseStore::ANY);
    if (success) {
        assert(c.begin != nullptr);
        assert(c.size >= 1);
        *size = c.size - ClauseMetadata::numInts();
        producedClause.resize(*size);
        memcpy(producedClause.data(), c.begin+ClauseMetadata::numInts(), *size*sizeof(int));
        *clause = producedClause.data();
        *lbd = c.lbd;
    } else {
        *clause = 0;
        *size = 0;
    }
}

int Gimsatul::getVariablesCount() {
    return (int) this->numVars;
}

int Gimsatul::getSplittingVariable() {
    return 0;
}

void Gimsatul::writeStatistics(SolverStatistics& stats) {
    if (!solver) return;
    gimsatul_statistics gstats = gimsatul_get_statistics(solver);
    stats.conflicts = gstats.conflicts;
    stats.decisions = gstats.decisions;
    stats.propagations = gstats.propagations;
    stats.restarts = gstats.restarts;
    stats.imported = gstats.imported;
    stats.discarded = gstats.discarded;
    LOGGER(_logger, V4_VVER, "disc_reasons r_ee:%ld,r_ed:%ld,r_pb:%ld,r_ss:%ld,r_sw:%ld,r_tr:%ld,r_fx:%ld,r_ia:%ld,r_tl:%ld\n",
        gstats.r_ee, gstats.r_ed, gstats.r_pb, gstats.r_ss, gstats.r_sw, gstats.r_tr, gstats.r_fx, gstats.r_ia, gstats.r_tl);
}

Gimsatul::~Gimsatul() {
    if (solver) {
        setSolverInterrupt();
        gimsatul_release(solver);
        solver = nullptr;
    }
}
