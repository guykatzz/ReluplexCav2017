/*********************                                                        */
/*! \file IReluplex.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** (in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __IReluplex_h__
#define __IReluplex_h__

#include "FloatUtils.h"
#include "List.h"
#include "Map.h"
#include "Pair.h"
#include "Set.h"
#include "Tableau.h"
#include "MString.h"

class ReluPairs;
class VariableBound;

class IReluplex
{
public:
    enum VariableStatus {
        ABOVE_UB,
        AT_UB,
        BETWEEN,
        FIXED,
        AT_LB,
        BELOW_LB,
    };

    enum ReluDissolutionType {
        TYPE_SPLIT,
        TYPE_MERGE,
    };

    enum UseSlackVariables {
        DONT_USE_SLACK_VARIABLES,
        USE_ROW_SLACK_VARIABLES,
        USE_ROW_AND_COL_SLACK_VARIABLES,
    };

    virtual ~IReluplex() {}

    virtual void printStatistics() = 0;

    virtual unsigned getNumVariables() const = 0;
    virtual Set<unsigned> getEliminatedVars() const = 0;
    virtual bool isEliminatedVar( unsigned variable ) const = 0;

    virtual bool outOfBounds( unsigned variable ) const = 0;

    virtual const Tableau *getTableau() const = 0;

    virtual VariableStatus getVarStatus( unsigned variable ) const = 0;
    virtual bool canIncrease( unsigned variable ) const = 0;
    virtual bool canDecrease( unsigned variable ) const = 0;
    virtual bool tooLow( unsigned variable ) const = 0;
    virtual bool tooHigh( unsigned variable ) const = 0;

    virtual const VariableBound *getLowerBounds() const = 0;
    virtual double getLowerBound( unsigned var ) const = 0;
    virtual const VariableBound *getUpperBounds() const = 0;
    virtual double getUpperBound( unsigned var ) const = 0;
    virtual Set<unsigned> getBasicVariables() const = 0;
    virtual const double *getAssignment() const = 0;
    virtual double getAssignment( unsigned var ) const = 0;
    virtual ReluPairs *getReluPairs() = 0;
    virtual bool reluPairIsBroken( unsigned b, unsigned f ) const = 0;

    virtual bool activeReluVariable( unsigned variable ) const = 0;

    virtual const Tableau::Entry *getColumn( unsigned column ) const = 0;
    virtual const Tableau::Entry *getRow( unsigned column ) const = 0;
    virtual double getCell( unsigned row, unsigned column ) const = 0;

    virtual bool isDissolvedBVariable( unsigned variable ) const = 0;
    virtual unsigned countSplits() const = 0;
    virtual unsigned countMerges() const = 0;

#ifdef DEBUG_ON
    virtual double safeGetCell( unsigned row, unsigned column ) const = 0;
#endif

    virtual String toName( unsigned variable ) const = 0;

    virtual void setLogging( bool value ) = 0;

    virtual void computeVariableStatus() = 0;

    virtual void setLowerBounds( const List<VariableBound> &lowerBounds ) = 0;
    virtual void setUpperBounds( const List<VariableBound> &upperBounds ) = 0;
    virtual void setBasicVariables( const Set<unsigned> &basicVariables ) = 0;
    virtual void setAssignment( const List<double> &assignment ) = 0;
    virtual void setReluPairs( const ReluPairs &reluPairs ) = 0;
    virtual void updateUpperBound( unsigned variable, double bound, unsigned level ) = 0;
    virtual bool updateLowerBound( unsigned variable, double bound, unsigned level ) = 0;

    virtual void backupIntoMatrix( Tableau *matrix ) const = 0;
    virtual void restoreFromMatrix( Tableau *matrix ) = 0;

    virtual void togglePrintAssignment( bool value ) = 0;
    virtual void conflictAnalysisCausedPop() = 0;

    virtual unsigned getColumnSize( unsigned column ) const = 0;

    virtual Map<unsigned, ReluDissolutionType> getDissolvedReluPairs() const = 0;
    virtual void setDissolvedReluPairs( const Map<unsigned, ReluDissolutionType> &pairs ) = 0;
    virtual void setUseApproximation( bool value ) = 0;
    virtual void setFindAllPivotCandidates( bool value ) = 0;

    virtual void toggleAlmostBrokenReluEliminiation( bool value ) = 0;
    virtual void toggleFullTightenAllBounds( bool value ) = 0;
    virtual void toggleGlpkExtractJustBasics( bool value ) = 0;

    virtual void addTimeEvalutingGlpkRows( unsigned time ) = 0;

    virtual bool isReluVariable( unsigned variable ) const = 0;
    virtual unsigned reluVarToF( unsigned variable ) const = 0;

    virtual UseSlackVariables useSlackVariablesForRelus() const = 0;
    virtual Set<unsigned> getActiveRowSlacks() const = 0;
    virtual Set<unsigned> getActiveColSlacks() const = 0;
    virtual double getSlackLowerBound( unsigned variable ) const = 0;
    virtual double getSlackUpperBound( unsigned variable ) const = 0;
    virtual unsigned slackToB( unsigned slack ) const = 0;
    virtual unsigned slackToF( unsigned slack ) const = 0;

    // Statistics
    virtual void incNumSplits() = 0;
    virtual void incNumMerges() = 0;
    virtual void incNumPops() = 0;
    virtual void incNumStackVisitedStates() = 0;
    virtual void setCurrentStackDepth( unsigned depth ) = 0;
    virtual void setMinStackSecondPhase( unsigned depth ) = 0;

    // Debug
    virtual void checkInvariants() const = 0;
};

#endif // __IReluplex_h__

//
// Local Variables:
// compile-command: "make -C . "
// tags-file-name: "./TAGS"
// c-basic-offset: 4
// End:
//
