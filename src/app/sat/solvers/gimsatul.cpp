
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




void gimsatul_produce_clause(void* state, int size, int glue) {
    ((Gimsatul*) state)->produceClause(size, glue);
}

void gimsatul_consume_clause(void* state, int** clause, int* size, int* glue) {
    ((Gimsatul*) state)->consumeClause(clause, size, glue);
}

// int terminate_callback(void* state) {
//     return 0;
// }


Gimsatul::Gimsatul(const SolverSetup& setup)
        : PortfolioSolverInterface(setup), solver(gimsatul_init(setup.numVars, setup.numOriginalClauses)),
          learntClauseBuffer(_setup.strictMaxLitsPerClause+ClauseMetadata::numInts()) {
    int success = gimsatul_set_option(solver, "threads", setup.threads);
    std::cout << ">>>>> gimsatul_set_option: " << success << std::endl;
}

void Gimsatul::addLiteral(int lit) {
    // std::cout << ">>>>> addLiteral" << std::endl;
    gimsatul_add(solver, lit);
    numVars = std::max(numVars, std::abs(lit));
}


void Gimsatul::diversify(int seed) {}

int Gimsatul::getNumOriginalDiversifications() {
    return 0;
}

void Gimsatul::setPhase(const int var, const bool phase) {
    assert(!initialVariablePhasesLocked);
	if (var >= initialVariablePhases.size())
        initialVariablePhases.resize(var+1);
    initialVariablePhases[var] = phase ? 1 : -1;
}

// Solve the formula with a given set of assumptions
// return 10 for SAT, 20 for UNSAT, 0 for UNKNOWN
SatResult Gimsatul::solve(size_t numAssumptions, const int* assumptions) {
    // TODO handle assumptions?
    // assert(numAssumptions == 0);

    // Push the initial variable phases to gimsatul
    initialVariablePhasesLocked = true;
    gimsatul_set_initial_variable_phases (solver, initialVariablePhases.data(), initialVariablePhases.size());

    std::cout << ">>>>> solve" << std::endl;
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
    gimsatul_set_clause_export_callback(solver, this, learntClauseBuffer.data(), _setup.strictMaxLitsPerClause, &gimsatul_produce_clause);
    gimsatul_set_clause_import_callback(solver, this, &gimsatul_consume_clause);
}

void Gimsatul::produceClause(int size, int lbd) {
    interruptionInitialized = true;
    if (size > _setup.strictMaxLitsPerClause) return;
    learntClause.size = size;
    // In Kissat, long clauses of LBD 1 can be exported. => Increment LBD in this case.
    learntClause.lbd = learntClause.size == 1 ? 1 : lbd;
    if (learntClause.lbd == 1 && learntClause.size > 1) learntClause.lbd++;
    if (learntClause.lbd > _setup.strictLbdLimit) return;
    learntClause.begin = learntClauseBuffer.data();
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

void Gimsatul::writeStatistics(SolverStatistics& stats) {}

Gimsatul::~Gimsatul() {
    if (solver) {
        setSolverInterrupt();
        gimsatul_release(solver);
        solver = nullptr;
    }
}
