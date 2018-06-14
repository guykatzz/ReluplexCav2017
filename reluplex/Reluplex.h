/*********************                                                        */
/*! \file Reluplex.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** (in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __Reluplex_h__
#define __Reluplex_h__

#include "Debug.h"
#include "File.h"
#include "FloatUtils.h"
#include "GlpkWrapper.h"
#include "IReluplex.h"
#include "Map.h"
#include "Queue.h"
#include "ReluPairs.h"
#include "Set.h"
#include "Tableau.h"
#include "Stack.h"
#include "SmtCore.h"
#include "MStringf.h"
#include "TimeUtils.h"
#include "VariableBound.h"
#include <string.h>

static const double ALMOST_BROKEN_RELU_MARGIN = 0.001;
static const double GLPK_IMPRECISION_TOLERANCE = 0.001;
static const double NUMBERICAL_INSTABILITY_CONSTANT = 0.0001;
static const double OOB_EPSILON = 0.001;
static const double MAX_ALLOWED_DEGRADATION = 0.000001;

// How often should the statistics and assignment be printed
static const unsigned PRINT_STATISTICS = 500;
static const unsigned PRINT_ASSIGNMENT = 500;

// How many times GLPK is allowed to fail before tableau restoration
static const unsigned MAX_GLPK_FAILURES_BEFORE_RESOTRATION = 10;

class Reluplex;

static Reluplex *activeReluplex;

// Callbacks from GLPK
void boundCalculationHook( int n, int m, int *head, int leavingBasic, int enteringNonBasic, double *basicRow );
void iterationCountCallback( int count );
void reportSoiCallback( double soi );
int makeReluAdjustmentsCallback( int n, int m, int nonBasicEncoding, const int *head, const char *flags );

class InvariantViolationError
{
public:
    InvariantViolationError( unsigned violatingStackLevel )
        : _violatingStackLevel( violatingStackLevel )
    {
    }

    unsigned _violatingStackLevel;
};

String milliToString( unsigned long long milliseconds )
{
    unsigned seconds = milliseconds / 1000;
    unsigned minutes = seconds / 60;
    unsigned hours = minutes / 60;

    return Stringf( "%02u:%02u:%02u", hours, minutes - hours * 60, seconds - ( minutes * 60 ) );
}

class Reluplex : public IReluplex
{
public:
    enum FinalStatus {
        SAT = 0,
        UNSAT = 1,
        ERROR = 2,
        NOT_DONE = 3,
    };

    struct GlpkRowEntry
    {
        unsigned _variable;
        double _coefficient;

        GlpkRowEntry( unsigned variable, double coefficient )
            : _variable( variable )
            , _coefficient( coefficient )
        {
        }
    };

    Reluplex( unsigned numVariables, char *finalOutputFile = NULL, String reluplexName = "" )
        : _numVariables( numVariables )
        , _reluplexName( reluplexName )
        , _finalOutputFile( finalOutputFile )
        , _finalStatus( NOT_DONE )
        , _wasInitialized( false )
        , _tableau( numVariables )
        , _preprocessedTableau( numVariables )
        , _upperBounds( NULL )
        , _lowerBounds( NULL )
        , _preprocessedUpperBounds( NULL )
        , _preprocessedLowerBounds( NULL )
        , _assignment( NULL )
        , _preprocessedAssignment( NULL )
        , _smtCore( this, _numVariables )
        , _useApproximations( true )
        , _findAllPivotCandidates( false )
        , _conflictAnalysisCausedPop( 0 )
        , _logging( false )
        , _dumpStates( false )
        , _numCallsToProgress( 0 )
        , _numPivots( 0 )
        , _totalPivotTimeMilli( 0 )
        , _totalDegradationCheckingTimeMilli( 0 )
        , _totalRestorationTimeMilli( 0 )
        , _totalPivotCalculationCount( 0 )
        , _totalNumBrokenRelues( 0 )
        , _brokenRelusFixed( 0 )
        , _brokenReluFixByUpdate( 0 )
        , _brokenReluFixByPivot( 0 )
        , _brokenReluFixB( 0 )
        , _brokenReluFixF( 0 )
        , _numEliminatedVars( 0 )
        , _varsWithInfiniteBounds( 0 )
        , _numStackSplits( 0 )
        , _numStackMerges( 0 )
        , _numStackPops( 0 )
        , _numStackVisitedStates( 0 )
        , _currentStackDepth( 0 )
        , _minStackSecondPhase( 0 )
        , _maximalStackDepth( 0 )
        , _boundsTightendByTightenAllBounds( 0 )
        , _almostBrokenReluPairCount( 0 )
        , _almostBrokenReluPairFixedCount( 0 )
        , _numBoundsDerivedThroughGlpk( 0 )
        , _numBoundsDerivedThroughGlpkOnSlacks( 0 )
        , _totalTightenAllBoundsTime( 0 )
        , _eliminateAlmostBrokenRelus( false )
        , _printAssignment( false )
        , _numOutOfBoundFixes( 0 )
        , _numOutOfBoundFixesViaBland( 0 )
        , _useDegradationChecking( false )
        , _numLpSolverInvocations( 0 )
        , _numLpSolverFoundSolution( 0 )
        , _numLpSolverNoSolution( 0 )
        , _numLpSolverFailed( 0 )
        , _numLpSolverIncorrectAssignment( 0 )
        , _totalLpSolverTimeMilli( 0 )
        , _totalLpExtractionTime( 0 )
        , _totalLpPivots( 0 )
        , _maxLpSolverTimeMilli( 0 )
        , _numberOfRestorations( 0 )
        , _maxDegradation( 0.0 )
        , _totalProgressTimeMilli( 0 )
        , _timeTighteningGlpkBoundsMilli( 0 )
        , _currentGlpkWrapper( NULL )
        , _relusDissolvedByGlpkBounds( 0 )
        , _glpkSoi( 0 )
        , _storeGlpkBoundTighteningCalls( 0 )
        , _storeGlpkBoundTighteningCallsOnSlacks( 0 )
        , _storeGlpkBoundTighteningIgnored( 0 )
        , _maxBrokenReluAfterGlpk( 0 )
        , _totalBrokenReluAfterGlpk( 0 )
        , _totalBrokenNonBasicReluAfterGlpk( 0 )
        , _useSlackVariablesForRelus( USE_ROW_SLACK_VARIABLES )
        , _fixRelusInGlpkAssignmentFixes( 0 )
        , _fixRelusInGlpkAssignmentInvoked( 0 )
        , _fixRelusInGlpkAssignmentIgnore( 0 )
        , _maximalGlpkBoundTightening( false )
        , _useConflictAnalysis( true )
        , _temporarilyDontUseSlacks( false )
        , _quit( false )
        , _fullTightenAllBounds( true )
        , _glpkExtractJustBasics( true )
        , _totalTimeEvalutingGlpkRows( 0 )
        , _consecutiveGlpkFailureCount( 0 )
    {
        activeReluplex = this;

        srand( time( 0 ) );

        _upperBounds = new VariableBound[_numVariables];
        _lowerBounds = new VariableBound[_numVariables];
        _preprocessedUpperBounds = new VariableBound[_numVariables];
        _preprocessedLowerBounds = new VariableBound[_numVariables];

        _assignment = new double[_numVariables];
        _preprocessedAssignment = new double[_numVariables];

        for ( unsigned i = 0; i < _numVariables; ++i )
        {
            _assignment[i] = 0.0;
            _preprocessedAssignment[i] = 0.0;
        }

        FloatUtils::printEpsion();
        printf( "Almost-broken nuking marging: %.15lf\n", ALMOST_BROKEN_RELU_MARGIN );
    }

    ~Reluplex()
    {
        if ( _finalOutputFile )
        {
            printFinalStatistics();
        }

        if ( _upperBounds )
        {
            delete[] _upperBounds;
            _upperBounds = NULL;
        }

        if ( _lowerBounds )
        {
            delete[] _lowerBounds;
            _lowerBounds = NULL;
        }

        if ( _preprocessedLowerBounds )
        {
            delete[] _preprocessedLowerBounds;
            _preprocessedLowerBounds = NULL;
        }

        if ( _preprocessedUpperBounds )
        {
            delete[] _preprocessedUpperBounds;
            _preprocessedUpperBounds = NULL;
        }

        if ( _assignment )
        {
            delete[] _assignment;
            _assignment = NULL;
        }

        if ( _preprocessedAssignment )
        {
            delete[] _preprocessedAssignment;
            _preprocessedAssignment = NULL;
        }
    }

    void initialize()
    {
        initialUpdate();
        makeAllBoundsFinite();
        _wasInitialized = true;
    }

    FinalStatus solve()
    {
        timeval start = Time::sampleMicro();
        timeval end;

        try
        {
            if ( !_wasInitialized )
                initialize();

            countVarsWithInfiniteBounds();
            if ( !eliminateAuxVariables() )
            {
                _finalStatus = Reluplex::ERROR;
                end = Time::sampleMicro();
                _totalProgressTimeMilli += Time::timePassed( start, end );
                return _finalStatus;
            }

            storePreprocessedMatrix();

            printf( "Initialization steps over.\n" );
            printStatistics();
            dump();
            printf( "Starting the main loop\n" );

            while ( !_quit )
            {
                computeVariableStatus();

                if ( allVarsWithinBounds() && allRelusHold() )
                {
                    dump();
                    printStatistics();
                    _finalStatus = Reluplex::SAT;
                    end = Time::sampleMicro();
                    _totalProgressTimeMilli += Time::timePassed( start, end );
                    return _finalStatus;
                }

                unsigned violatingLevelInStack;
                if ( !progress( violatingLevelInStack ) )
                {
                    if ( _useConflictAnalysis )
                        _smtCore.pop( violatingLevelInStack );
                    else
                        _smtCore.pop();

                    setMinStackSecondPhase( _currentStackDepth );
                }
            }
        }
        catch ( const Error &e )
        {
            end = Time::sampleMicro();
            _totalProgressTimeMilli += Time::timePassed( start, end );

            if ( e.code() == Error::STACK_IS_EMPTY )
            {
                _finalStatus = Reluplex::UNSAT;
                return _finalStatus;
            }
            else
            {
                printf( "Found error: %u\n", e.code() );

                _finalStatus = Reluplex::ERROR;
                return _finalStatus;
            }
        }
        catch ( const InvariantViolationError &e )
        {
            end = Time::sampleMicro();
            _totalProgressTimeMilli += Time::timePassed( start, end );
            _finalStatus = Reluplex::UNSAT;
            return _finalStatus;
        }
        catch ( ... )
        {
            end = Time::sampleMicro();
            _totalProgressTimeMilli += Time::timePassed( start, end );
            _finalStatus = Reluplex::ERROR;
            return _finalStatus;
        }

        // Quit was called
        _finalStatus = Reluplex::NOT_DONE;
        end = Time::sampleMicro();
        _totalProgressTimeMilli += Time::timePassed( start, end );
        return _finalStatus;
    }

    bool progress( unsigned &violatingLevelInStack )
    {
        log( "Progress starting\n" );

        try
        {
            ++_numCallsToProgress;

            // The default
            violatingLevelInStack = _currentStackDepth;

            if ( _useDegradationChecking && ( _numCallsToProgress % 50 == 0 ) )
            {
                double currentMaxDegradation = checkDegradation();
                if ( currentMaxDegradation > MAX_ALLOWED_DEGRADATION )
                {
                    restoreTableauFromBackup();
                    return true;
                }
            }

            if ( _numCallsToProgress % PRINT_STATISTICS == 0 )
                printStatistics();

            if ( _printAssignment && _numCallsToProgress % PRINT_ASSIGNMENT == 0 )
                printAssignment();

            dump();

            List<unsigned> outOfBoundVariables;
            findOutOfBounds( outOfBoundVariables );

            // If we have out-of-bounds variables, we deal with them first
            if ( !outOfBoundVariables.empty() )
            {
                log( "Progress: have OOB vars\n" );

                GlpkWrapper::GlpkAnswer answer = fixOutOfBounds();

                if ( _consecutiveGlpkFailureCount > MAX_GLPK_FAILURES_BEFORE_RESOTRATION )
                {
                    printf( "Error: %u Consecutive GLPK failures\n", MAX_GLPK_FAILURES_BEFORE_RESOTRATION );
                    throw Error( Error::CONSECUTIVE_GLPK_FAILURES );
                }

                if ( answer == GlpkWrapper::NO_SOLUTION_EXISTS )
                    return false;

                if ( answer == GlpkWrapper::SOLVER_FAILED )
                {
                    // In this case, we restored from the original tableau; nothing left to do here.
                    return true;
                }

                if ( allRelusHold() )
                    return true;

                // Glpk solved, but we have a relu problem. See if any relu pairs can be eliminated.
                if ( learnedGlpkBounds() )
                {
                    timeval boundStart = Time::sampleMicro();

                    unsigned numDissolvedRelusBefore = countDissolvedReluPairs();
                    try
                    {
                        performGlpkBoundTightening();
                        tightenAllBounds();
                    }
                    catch ( ... )
                    {
                        timeval boundEnd = Time::sampleMicro();
                        _timeTighteningGlpkBoundsMilli += Time::timePassed( boundStart, boundEnd );
                        throw;
                    }

                    unsigned numDissolvedRelusAfter = countDissolvedReluPairs();

                    timeval boundEnd = Time::sampleMicro();
                    _timeTighteningGlpkBoundsMilli += Time::timePassed( boundStart, boundEnd );

                    if ( numDissolvedRelusAfter > numDissolvedRelusBefore )
                    {
                        _relusDissolvedByGlpkBounds += ( numDissolvedRelusAfter - numDissolvedRelusBefore );
                    }
                }

                return true;
            }

            // Reset the GLPK failure measures
            _consecutiveGlpkFailureCount = 0;
            _previousGlpkAnswer = GlpkWrapper::SOLUTION_FOUND;

            log( "No OOB variables to fix, looking at broken relus\n" );

            // If we got here, either there are no OOB variables, or they were fixed without changing the tableau
            // and we still have broken relus. Split on one of them.
            List<unsigned> brokenRelus;
            findBrokenRelues( brokenRelus );
            _totalNumBrokenRelues += brokenRelus.size();

            unsigned brokenReluVar = *brokenRelus.begin();
            unsigned f = _reluPairs.isF( brokenReluVar ) ? brokenReluVar : _reluPairs.toPartner( brokenReluVar );

            if ( _smtCore.notifyBrokenRelu( f ) )
                return true; // Splitting/Merging is a form of progress
            return fixBrokenRelu( f );
        }

        catch ( const InvariantViolationError &e )
        {
            log( "\n\n*** Upper/lower invariant violated! Failure ***\n\n" );

            if ( e._violatingStackLevel != _currentStackDepth )
            {
                violatingLevelInStack = e._violatingStackLevel;
            }

            return false;
        }
    }

    void toggleDegradationChecking( bool value )
    {
        _useDegradationChecking = value;
    }

    void toggleFullTightenAllBounds( bool value )
    {
        _fullTightenAllBounds = value;
    }

    void toggleGlpkExtractJustBasics( bool value )
    {
        _glpkExtractJustBasics = value;
    }

    void togglePrintAssignment( bool value )
    {
        _printAssignment = value;
    }

    void toggleAlmostBrokenReluEliminiation( bool value )
    {
        if ( value )
            printf( "almost-broken relu elimination has been turned on!\n" );

        _eliminateAlmostBrokenRelus = value;
    }

    bool allVarsWithinBounds( bool print = false ) const
    {
        // Only basic variables can be out-of-bounds
        for ( auto i : _basicVariables )
            if ( outOfBounds( i ) )
            {
                if ( print )
                {
                    printf( "Variable %u out of bounds: value = %.10lf, range = [%.10lf, %.10lf]\n",
                        i,
                        _assignment[i],
                        _lowerBounds[i].getBound(),
                        _upperBounds[i].getBound() );
                }

                return false;
            }


        return true;
    }

    bool allRelusHold() const
    {
        const Set<ReluPairs::ReluPair> &pairs( _reluPairs.getPairs() );
        for ( const auto &pair : pairs )
        {
            unsigned b = pair.getB();
            unsigned f = pair.getF();

            if ( ( !_dissolvedReluVariables.exists( f ) ) && reluPairIsBroken( b, f ) )
                return false;
        }

        return true;
    }

    bool reluPairIsBroken( unsigned b, unsigned f ) const
    {
        double bVal = _assignment[b];
        double fVal = _assignment[f];

        return
            ( FloatUtils::isZero( fVal ) && FloatUtils::isPositive( bVal ) ) ||
            ( FloatUtils::isPositive( fVal ) && ( FloatUtils::areDisequal( fVal, bVal ) ) );
    }

    unsigned countDissolvedReluPairs() const
    {
        return _dissolvedReluVariables.size();
    }

    unsigned countSplits() const
    {
        unsigned splits = 0;
        for ( const auto &it : _dissolvedReluVariables )
        {
            if ( it.second == TYPE_SPLIT )
                ++splits;
        }

        return splits;
    }

    unsigned countMerges() const
    {
        unsigned merges = 0;

        for ( const auto &it : _dissolvedReluVariables )
        {
            if ( it.second == TYPE_MERGE )
                ++merges;
        }

        return merges;
    }

    unsigned countReluPairsAlmostBroken()
    {
        unsigned count = 0;

        const Set<ReluPairs::ReluPair> &pairs( _reluPairs.getPairs() );
        for ( const auto &pair : pairs )
        {
            unsigned b = pair.getB();
            unsigned f = pair.getF();

            if ( reluPairAlmostBroken( b, f ) )
                ++count;
        }

        return count;
    }

    VariableStatus getVarStatus( unsigned variable ) const
    {
        return _varToStatus.at( variable );
    }

    bool reluPairAlmostBroken( unsigned b, unsigned f ) const
    {
        if ( _dissolvedReluVariables.exists( f ) )
            return false;

        if ( _upperBounds[f].finite() )
        {
            double ub = _upperBounds[f].getBound();
            if ( ( !FloatUtils::isZero( ub ) ) && FloatUtils::lte( ub, ALMOST_BROKEN_RELU_MARGIN ) )
                return true;
        }

        if ( _lowerBounds[b].finite() )
        {
            double lb = _lowerBounds[b].getBound();
            if ( FloatUtils::isNegative( lb ) && FloatUtils::gte( lb, -ALMOST_BROKEN_RELU_MARGIN ) )
                return true;
        }

        return false;
    }

    void findOutOfBounds( List<unsigned> &result ) const
    {
        // Only basic variables can be out-of-bounds
        for ( auto i : _basicVariables )
            if ( outOfBounds( i ) )
                result.append( i );
    }

    void countBrokenReluPairs( unsigned &brokenReluPairs, unsigned &brokenNonBasicReluPairs ) const
    {
        brokenReluPairs = 0;
        brokenNonBasicReluPairs = 0;

        const Set<ReluPairs::ReluPair> &pairs( _reluPairs.getPairs() );
        for ( const auto &pair : pairs )
        {
            unsigned b = pair.getB();
            unsigned f = pair.getF();

            if ( ( !_dissolvedReluVariables.exists( f ) ) && reluPairIsBroken( b, f ) )
            {
                ++brokenReluPairs;
                if ( !_basicVariables.exists( b ) && !_basicVariables.exists( f ) )
                    ++brokenNonBasicReluPairs;
            }
        }
    }

    void findBrokenRelues( List<unsigned> &result ) const
    {
        const Set<ReluPairs::ReluPair> &pairs( _reluPairs.getPairs() );
        for ( const auto &pair : pairs )
        {
            unsigned b = pair.getB();
            unsigned f = pair.getF();

            if ( ( !_dissolvedReluVariables.exists( f ) ) && reluPairIsBroken( b, f ) )
            {
                result.append( b );
                result.append( f );
            }
        }
    }

    bool partOfBrokenRelu( unsigned variable ) const
    {
        if ( !_reluPairs.isRelu( variable ) )
            return false;

        unsigned partner = _reluPairs.toPartner( variable );
        unsigned b, f;

        if ( _reluPairs.isF( variable ) )
        {
            f = variable;
            b = partner;
        }
        else
        {
            b = variable;
            f = partner;
        }

        return reluPairIsBroken( b, f );
    }

    void countVarsWithInfiniteBounds()
    {
        _varsWithInfiniteBounds = 0;
        for ( unsigned i = 0; i < _numVariables; ++i )
            if ( !_upperBounds[i].finite() || !_lowerBounds[i].finite() )
                ++_varsWithInfiniteBounds;
    }

    void computeVariableStatus()
    {
        for ( unsigned i = 0; i < _numVariables; ++i )
            computeVariableStatus( i );
    }

    void computeVariableStatus( unsigned i )
    {
        double value = _assignment[i];

        if ( _upperBounds[i].finite() && _lowerBounds[i].finite() )
        {
            // Both bounds finite
            double ub = _upperBounds[i].getBound();
            double lb = _lowerBounds[i].getBound();

            if ( FloatUtils::gt( value, ub, OOB_EPSILON ) )
                _varToStatus[i] = VariableStatus::ABOVE_UB;
            else if ( FloatUtils::areEqual( value, ub, OOB_EPSILON ) )
                _varToStatus[i] = FloatUtils::areEqual( lb, ub ) ? VariableStatus::FIXED : VariableStatus::AT_UB;
            else if ( FloatUtils::gt( value, lb, OOB_EPSILON ) )
                _varToStatus[i] = VariableStatus::BETWEEN;
            else if ( FloatUtils::areEqual( value, lb, OOB_EPSILON ) )
                _varToStatus[i] = VariableStatus::AT_LB;
            else
                _varToStatus[i] = VariableStatus::BELOW_LB;
        }

        else if ( !_upperBounds[i].finite() && _lowerBounds[i].finite() )
        {
            // Only lower bound is finite
            double lb = _lowerBounds[i].getBound();

            if ( FloatUtils::gt( value, lb, OOB_EPSILON ) )
                _varToStatus[i] = VariableStatus::BETWEEN;
            else if ( FloatUtils::areEqual( value, lb, OOB_EPSILON ) )
                _varToStatus[i] = VariableStatus::AT_LB;
            else
                _varToStatus[i] = VariableStatus::BELOW_LB;
        }

        else if ( _upperBounds[i].finite() && !_lowerBounds[i].finite() )
        {
            // Only upper bound is finite
            double ub = _upperBounds[i].getBound();

            if ( FloatUtils::gt( value, ub, OOB_EPSILON ) )
                _varToStatus[i] = VariableStatus::ABOVE_UB;
            else if ( FloatUtils::areEqual( value, ub, OOB_EPSILON ) )
                _varToStatus[i] = VariableStatus::AT_UB;
            else
                _varToStatus[i] = VariableStatus::BETWEEN;
        }

        else
        {
            // Both bounds infinite
            _varToStatus[i] = VariableStatus::BETWEEN;
        }
    }

    void printStatistics()
    {
        countVarsWithInfiniteBounds();

        printf( "\n" );
        printf( "%s Statistics update:\n", Time::now().ascii() );
        printf( "\tCalls to 'progress': %u. Total time: %llu milli. Average: %llu milli\n",
                _numCallsToProgress, _totalProgressTimeMilli,
                _numCallsToProgress > 0 ? _totalProgressTimeMilli / _numCallsToProgress : 0 );
        printf( "\tPivot operations: %u. ", _numPivots );
        printf( "Total pivot time: %llu milli.\n", _totalPivotTimeMilli );
        printf( "\tAverage pivot time: %llu milli\n", _numPivots > 0 ? _totalPivotTimeMilli / _numPivots : 0 );
        printf( "\tAverage time per calcuation in pivot: %.5f milli\n",
                _totalPivotCalculationCount > 0 ? ((double)_totalPivotTimeMilli) / _totalPivotCalculationCount : 0 );
        printf( "\tAverage number of calculations in pivot: %llu\n",
                _numPivots > 0 ? _totalPivotCalculationCount / _numPivots : 0 );
        printf( "\tAverage number of broken relues per 'progress': %llu\n",
                _numCallsToProgress > 0 ? _totalNumBrokenRelues / _numCallsToProgress : 0 );
        printf( "\tBroken Relus Fixed: %u (Fs: %u, Bs: %u, fix-by-pivot: %u, fix-by-update: %u)\n",
                _brokenRelusFixed, _brokenReluFixF, _brokenReluFixB, _brokenReluFixByPivot, _brokenReluFixByUpdate );
        printf( "\tRelu-to-OOB step ratio: %u / %u = %lf%%. Avg oob steps per relu: %.02lf.\n",
                _brokenRelusFixed, _numOutOfBoundFixes,
                _numOutOfBoundFixes > 0 ? ((double)_brokenRelusFixed) / _numOutOfBoundFixes : 0,
                _brokenRelusFixed > 0 ? ((double)_numOutOfBoundFixes) / _brokenRelusFixed : 0
                );
        printf( "\tAlmost broken relus encountered: %u. Nuked: %u\n",
                _almostBrokenReluPairCount, _almostBrokenReluPairFixedCount );

        printf( "\tTime in TightenAllBounds: %llu milli. Bounds tightened: %llu\n",
                _totalTightenAllBoundsTime, _boundsTightendByTightenAllBounds );

        printf( "\tRelu pairs dissolved: %u. Num splits: %u. Num merges: %u (remaining: %u / %u)\n",
                _dissolvedReluVariables.size(),
                countSplits(), countMerges(),
                _reluPairs.size() - _dissolvedReluVariables.size(),
                _reluPairs.size());

        printf( "\tNum LP solver invocations: %u. Found solution: %u. No Solution: %u. Failed: %u. "
                "Incorrect assignments: %u.\n",
                _numLpSolverInvocations, _numLpSolverFoundSolution, _numLpSolverNoSolution,
                _numLpSolverFailed, _numLpSolverIncorrectAssignment );
        printf( "\t\tTotal time in LP solver: %llu milli. Max: %u milli. Avg per invocation: %llu milli\n",
                _totalLpSolverTimeMilli,
                _maxLpSolverTimeMilli,
                _numLpSolverInvocations > 0 ? _totalLpSolverTimeMilli / _numLpSolverInvocations : 0 );
        printf( "\t\tNumber of pivots in LP solver: %u. Average time per LP pivot operation: %llu milli\n",
                _totalLpPivots, _totalLpPivots > 0 ? _totalLpSolverTimeMilli / _totalLpPivots : 0 );
        printf( "\t\tTotal time extracting tableaus after LP solved: %llu milli. Average: %llu milli.\n",
                _totalLpExtractionTime, _numLpSolverFoundSolution > 0 ?
                _totalLpExtractionTime / _numLpSolverFoundSolution : 0 );
        printf( "\t\tTotal time evaulating GLPK rows: %llu\n", _totalTimeEvalutingGlpkRows );
        printf( "\t\tGlpk bound reports: %llu. On slacks: %llu (= %.lf%%). "
                "Ignored due to small coefficients: %llu. Used: %.2lf%%\n",
                _storeGlpkBoundTighteningCalls,
                _storeGlpkBoundTighteningCallsOnSlacks,
                percents( _storeGlpkBoundTighteningCallsOnSlacks, _storeGlpkBoundTighteningCalls ),
                _storeGlpkBoundTighteningIgnored,
                percents( _storeGlpkBoundTighteningCalls - _storeGlpkBoundTighteningIgnored,
                          _storeGlpkBoundTighteningCalls ) );

        printf( "\t\tNumber of GLPK-derived bounds: %u. On slacks: %u (= %.2lf%%). "
                "Time: %llu milli. Relus consequently dissolved: %u\n",
                _numBoundsDerivedThroughGlpk,
                _numBoundsDerivedThroughGlpkOnSlacks,
                percents( _numBoundsDerivedThroughGlpkOnSlacks, _numBoundsDerivedThroughGlpk ),
                _timeTighteningGlpkBoundsMilli, _relusDissolvedByGlpkBounds );

        printf( "\t\tFix-relu-invariant hook invocations: %llu. Actual repairs: %llu (= %.lf%%). "
                "Ignore to prevent cycles: %llu\n",
                _fixRelusInGlpkAssignmentInvoked, _fixRelusInGlpkAssignmentFixes,
                percents( _fixRelusInGlpkAssignmentFixes, _fixRelusInGlpkAssignmentInvoked ),
                _fixRelusInGlpkAssignmentIgnore );

        printf( "\tAverage number of broken relu pairs after glpk invocation: %lf. Max: %u. "
                "Broken and non-basic pairs: %u\n",
                _numLpSolverFoundSolution > 0 ? ((double)_totalBrokenReluAfterGlpk / _numLpSolverFoundSolution) : 0,
                _maxBrokenReluAfterGlpk,
                _totalBrokenNonBasicReluAfterGlpk );
        printf( "\tVars with infinite bounds: %u / %u\n", _varsWithInfiniteBounds, _numVariables );
        printf( "\tEliminated vars: %u\n", _numEliminatedVars );
        printf( "\tStack: Current depth is: %u (maximal = %u, min second phase = %u).\n"
                "\t       So far: %u splits, %u merges, %u pops. Total visited states: %u\n",
                _currentStackDepth, _maximalStackDepth, _minStackSecondPhase,
                _numStackSplits, _numStackMerges, _numStackPops, _numStackVisitedStates );
        printf( "\t\tPops caused by conflict analysis: %u\n", _conflictAnalysisCausedPop );
        printf( "\t\tTotal time in smtCore: %llu milli\n", _smtCore.getSmtCoreTime() );
        printf( "\tCurrent degradation: %.10lf. Time spent checking: %llu milli. Max measured: %.10lf.\n",
                checkDegradation(), _totalDegradationCheckingTimeMilli, _maxDegradation );
        printf( "\tNumber of restorations: %u. Total time: %llu milli. Average: %lf\n",
                _numberOfRestorations,
                _totalRestorationTimeMilli,
                percents( _totalRestorationTimeMilli, _numberOfRestorations )
                );

        unsigned long long totalUnaccountedFor =
            _totalProgressTimeMilli -
            _totalLpSolverTimeMilli -
            _totalLpExtractionTime -
            _timeTighteningGlpkBoundsMilli -
            _smtCore.getSmtCoreTime() -
            _totalRestorationTimeMilli;

        printf( "\n\n\tSummary: Total: %llu milli\n"
                "\t\t1. GLPK: %llu milli (%.lf%%) \n"
                "\t\t2. Extraction + Postprocessing: %llu milli (%.lf%%)\n"
                "\t\t3. Tightening bounds: %llu milli (%.lf%%)\n"
                "\t\t4. Stack operations: %llu milli (%.lf%%)\n"
                "\t\t5. Tableau restoration operations: %llu milli (%.lf%%)\n"
                "\t\t6. Unaccounted for: %llu milli (%.lf%%)\n",
                _totalProgressTimeMilli,
                _totalLpSolverTimeMilli, percents( _totalLpSolverTimeMilli, _totalProgressTimeMilli ),
                _totalLpExtractionTime, percents( _totalLpExtractionTime, _totalProgressTimeMilli ),
                _timeTighteningGlpkBoundsMilli, percents( _timeTighteningGlpkBoundsMilli, _totalProgressTimeMilli ),
                _smtCore.getSmtCoreTime(), percents( _smtCore.getSmtCoreTime(), _totalProgressTimeMilli ),
                _totalRestorationTimeMilli, percents( _totalRestorationTimeMilli, _totalProgressTimeMilli ),
                totalUnaccountedFor, percents( totalUnaccountedFor, _totalProgressTimeMilli )
                );

        printf( "\n" );
    }

    void printFinalStatistics()
    {
        try
        {
            File outputFile( _finalOutputFile );
            outputFile.open( IFile::MODE_WRITE_APPEND );

            outputFile.write( _reluplexName + ", " );

            String status;
            switch ( _finalStatus )
            {
            case SAT:
                status = "SAT";
                break;

            case UNSAT:
                status = "UNSAT";
                break;

            case ERROR:
                status = "ERROR";
                break;

            case NOT_DONE:
                status = "TIMEOUT";
                break;
            }

            outputFile.write( status + ", " );
            outputFile.write( Stringf( "%llu, %s, ",
                                       _totalProgressTimeMilli, milliToString( _totalProgressTimeMilli ).ascii() ) );
            outputFile.write( Stringf( "%lu, ", _maximalStackDepth ) );
            outputFile.write( Stringf( "%lu\n", _numStackVisitedStates ) );
        }
        catch( ... )
        {
            printf( "Final staitstics printing threw an error!\n" );
        }
    }

    static double percents( double numerator, double denominator )
    {
        if ( FloatUtils::isZero( denominator ) )
            return 0;

        return ( numerator / denominator ) * 100;
    }

    void computeSlackBounds()
    {
        _slackToLowerBound.clear();
        _slackToUpperBound.clear();
        _activeSlackRowVars.clear();
        _activeSlackColVars.clear();

        const Set<ReluPairs::ReluPair> &pairs( _reluPairs.getPairs() );
        for ( const auto &pair : pairs )
        {
            unsigned b = pair.getB();
            unsigned f = pair.getF();

            if ( !_dissolvedReluVariables.exists( f ) )
            {
                unsigned slackRowVar = _fToSlackRowVar[f];
                _activeSlackRowVars.insert( slackRowVar );

                if ( _useSlackVariablesForRelus == USE_ROW_SLACK_VARIABLES )
                {
                    _slackToLowerBound[slackRowVar].setBound( 0.0 );
                    _slackToUpperBound[slackRowVar].setBound( _upperBounds[f].getBound() -
                                                              _lowerBounds[b].getBound() );

                    // The lower bounds exists since level 0.
                    // The upper bounds depend on the current bounds of the variables
                    _slackToLowerBound[slackRowVar].setLevel( 0 );
                    _slackToUpperBound[slackRowVar].setLevel
                        ( std::max( _upperBounds[f].getLevel(), _lowerBounds[b].getLevel() ) );

                }
                else
                {
                    // Use also cols
                    unsigned slackColVar = _fToSlackColVar[f];
                    _activeSlackColVars.insert( slackColVar );

                    _slackToLowerBound[slackColVar].setBound( 0.0 );
                    _slackToUpperBound[slackColVar].setBound( _upperBounds[f].getBound() -
                                                              _lowerBounds[b].getBound() );
                    _slackToUpperBound[slackRowVar].setLevel
                        ( std::max( _upperBounds[f].getLevel(), _lowerBounds[b].getLevel() ) );
                }
            }
        }
    }

    GlpkWrapper::GlpkAnswer fixOutOfBounds()
    {
        ++_numOutOfBoundFixes;
        ++_numLpSolverInvocations;

        timeval lpStart = Time::sampleMicro();
        GlpkWrapper glpkWrapper;
        _currentGlpkWrapper = &glpkWrapper;
        _glpkStoredLowerBounds.clear();
        _glpkStoredUpperBounds.clear();
        _activeSlackRowVars.clear();
        _activeSlackColVars.clear();

        if ( _useSlackVariablesForRelus != DONT_USE_SLACK_VARIABLES )
        {
            if ( _temporarilyDontUseSlacks )
            {
                log( "Temporarily disabling slacks\n" );
                _temporarilyDontUseSlacks = false;
            }
            else
                computeSlackBounds();
        }

        _reluUpdateFrequency.clear();

        glpkWrapper.setBoundCalculationHook( &boundCalculationHook );
        glpkWrapper.setIterationCountCallback( &iterationCountCallback );
        glpkWrapper.setReportSoiCallback( &reportSoiCallback );

        GlpkWrapper::GlpkAnswer answer;

        try
        {
            answer = glpkWrapper.run( *this );
        }
        catch ( InvariantViolationError &e )
        {
            _currentGlpkWrapper = NULL;
            timeval lpEnd = Time::sampleMicro();
            unsigned timePassed = Time::timePassed( lpStart, lpEnd );
            _totalLpSolverTimeMilli += timePassed;

            if ( timePassed > _maxLpSolverTimeMilli )
                _maxLpSolverTimeMilli = timePassed;

            throw e;
        }

        _currentGlpkWrapper = NULL;
        timeval lpEnd = Time::sampleMicro();
        unsigned timePassed = Time::timePassed( lpStart, lpEnd );
        _totalLpSolverTimeMilli += timePassed;

        if ( timePassed > _maxLpSolverTimeMilli )
            _maxLpSolverTimeMilli = timePassed;

        if ( answer == GlpkWrapper::SOLUTION_FOUND )
        {
            log( "LP solver solved the problem. Updating tableau and assignment\n" );
            ++_numLpSolverFoundSolution;

            timeval extractionStart = Time::sampleMicro();

            // Two options: either restore by basics (and pivot manually), or just take the entire tableau.
            if ( _glpkExtractJustBasics )
            {
                Set<unsigned> newBasics;
                glpkWrapper.extractBasicVariables( *this, newBasics );

                Set<unsigned> shouldBeBasic = Set<unsigned>::difference( newBasics, _basicVariables );
                Set<unsigned> shouldntBeBasic = Set<unsigned>::difference( _basicVariables, newBasics );
                adjustBasicVariables( shouldBeBasic, shouldntBeBasic, false );
            }
            else
            {
                glpkWrapper.extractTableau( this, &_tableau, &_basicVariables, &_eliminatedVars );
            }

            Map<unsigned, double> assignment;
            glpkWrapper.extractAssignment( *this, assignment );
            adjustGlpkAssignment( assignment );

            for ( const auto &pair : assignment )
            {
                double value = pair.second;
                _assignment[pair.first] = value;
            }

            calculateBasicVariableValues();
            computeVariableStatus();

            unsigned brokenReluPairs;
            unsigned brokenNonBasicReluPairs;
            countBrokenReluPairs( brokenReluPairs, brokenNonBasicReluPairs );

            if ( brokenReluPairs > _maxBrokenReluAfterGlpk )
                _maxBrokenReluAfterGlpk = brokenReluPairs;
            _totalBrokenReluAfterGlpk += brokenReluPairs;
            _totalBrokenNonBasicReluAfterGlpk += brokenNonBasicReluPairs;

            timeval extractionEnd = Time::sampleMicro();
            unsigned timePassed = Time::timePassed( extractionStart, extractionEnd );
            _totalLpExtractionTime += timePassed;

            DEBUG( checkInvariants() );

            if ( !allVarsWithinBounds( true ) )
            {
                // This rarely happens, but when it does - need to restore.
                // I'm guessing this is due to numerical instability when restoring the basics.
                log( "Error! Returned from GLPK but have oob variables\n" );

                ++_numLpSolverIncorrectAssignment;
                restoreTableauFromBackup( _consecutiveGlpkFailureCount < 5 );

                dump();

                if ( _previousGlpkAnswer == GlpkWrapper::SOLVER_FAILED )
                {
                    // Two failures in a row, so a restoration didn't help.
                    // Next time, don't use slacks.
                    _temporarilyDontUseSlacks = true;
                }

                _previousGlpkAnswer = GlpkWrapper::SOLVER_FAILED;
                ++_consecutiveGlpkFailureCount;
                return GlpkWrapper::SOLVER_FAILED;
            }

            _previousGlpkAnswer = GlpkWrapper::SOLUTION_FOUND;
            _consecutiveGlpkFailureCount = 0;
            return GlpkWrapper::SOLUTION_FOUND;
        }
        else if ( answer == GlpkWrapper::NO_SOLUTION_EXISTS )
        {
            log( "LP solver showed no solution exists\n" );
            ++_numLpSolverNoSolution;
            _previousGlpkAnswer = GlpkWrapper::NO_SOLUTION_EXISTS;
            _consecutiveGlpkFailureCount = 0;
            return GlpkWrapper::NO_SOLUTION_EXISTS;
        }

        log( "LP solver failed! Restoring from original matrix...\n" );
        ++_numLpSolverFailed;
        restoreTableauFromBackup( _consecutiveGlpkFailureCount < 5 );

        dump();

        if ( _previousGlpkAnswer == GlpkWrapper::SOLVER_FAILED )
        {
            // Two failures in a row, so a restoration didn't help.
            // Next time, don't use slacks.
            _temporarilyDontUseSlacks = true;
        }

        _previousGlpkAnswer = GlpkWrapper::SOLVER_FAILED;
        ++_consecutiveGlpkFailureCount;
        return GlpkWrapper::SOLVER_FAILED;
    }

    void storeGlpkBoundTightening( int n, int m, int *head,
                                   int leavingBasic,
                                   int enteringNonBasicEncoding,
                                   double *basicRow )
    {
        List<GlpkRowEntry> row;
        row.append( GlpkRowEntry( _currentGlpkWrapper->glpkEncodingToVariable( head[leavingBasic] ), -1.0 ) );

        unsigned numberOfNonBasics = n - m;
        double weightOfEntering = 0.0;
        unsigned enteringNonBasic = _currentGlpkWrapper->glpkEncodingToVariable( head[m + enteringNonBasicEncoding] );

        for ( unsigned i = 1; i <= numberOfNonBasics; ++i )
        {
            unsigned nonBasic = _currentGlpkWrapper->glpkEncodingToVariable( head[i + m] );
            double weight = basicRow[i];

            if ( nonBasic == enteringNonBasic )
                weightOfEntering = weight;

            if ( !FloatUtils::isZero( weight ) )
                row.append( GlpkRowEntry( nonBasic, weight ) );
        }

        if ( !_maximalGlpkBoundTightening )
        {
            storeGlpkBoundTighteningOnRow( row, _currentGlpkWrapper->glpkEncodingToVariable( head[leavingBasic] ) );

            DEBUG({
                    if ( FloatUtils::isZero( weightOfEntering ) )
                    {
                        printf( "Error! weightOfEntering is zero!\n" );
                        exit( 1 );
                    }
                });

            double scale = -1.0 / weightOfEntering;
            for ( auto &it : row )
            {
                if ( it._variable == enteringNonBasic )
                    it._coefficient = -1.0;
                else
                    it._coefficient *= scale;
            }

            storeGlpkBoundTighteningOnRow( row, enteringNonBasic );
        }
        else
        {
            // If maximal tightening is on, learn all possible bounds from this row
            for ( const auto &newBasic : row )
            {
                List<GlpkRowEntry> copy = row;
                if ( !FloatUtils::areEqual( newBasic._coefficient, -1.0 ) )
                {
                    // Scale the copy
                    double scale = -1.0 / newBasic._coefficient;
                    for ( auto &it : copy )
                    {
                        if ( it._variable == newBasic._variable )
                            it._coefficient = -1.0;
                        else
                            it._coefficient *= scale;
                    }
                }

                storeGlpkBoundTighteningOnRow( copy, newBasic._variable );
            }
        }
    }

    void storeGlpkBoundTighteningOnRow( List<GlpkRowEntry> &row, unsigned basic )
    {
        if ( ( _useSlackVariablesForRelus == USE_ROW_AND_COL_SLACK_VARIABLES ) &&
             _activeSlackRowVars.exists( basic ) )
        {
            // When using row and col slacks, rows are always fixed at 0, so ignore.
            return;
        }

        double max = 0.0;
        double min = 0.0;
        unsigned minBoundLevel = 0;
        unsigned maxBoundLevel = 0;

        ++_storeGlpkBoundTighteningCalls;
        if ( _activeSlackRowVars.exists( basic ) || _activeSlackColVars.exists( basic ) )
            ++_storeGlpkBoundTighteningCallsOnSlacks;

        DEBUG( Set<unsigned> seenVariables; );

        for ( const auto &entry : row )
        {
            if ( entry._variable == basic )
            {
                DEBUG({
                        if ( FloatUtils::areDisequal( entry._coefficient, -1.0 ) )
                        {
                            printf( "Error! storeGlpkBoundTighteningOnRow expected -1.0 coefficient for basic!\n" );
                            exit( 1 );
                        }
                    });

                continue;
            }

            unsigned nonBasic = entry._variable;
            double weight = entry._coefficient;

            // TODO: ignore tiny weights, for numerical stability

            DEBUG({
                    if ( !_activeSlackRowVars.exists( nonBasic ) && !_activeSlackColVars.exists( nonBasic ) &&
                         ( ( _tableau.getColumnSize( nonBasic ) == 0 ) ||
                           _eliminatedVars.exists( nonBasic ) ||
                           isDissolvedBVariable( nonBasic ) ) )
                    {
                        printf( "Error! A non active non-basic variable appeared!\n" );
                        exit( 1 );
                    }

                    if ( seenVariables.exists( nonBasic ) )
                    {
                        printf( "Error! Same variable twice!\n" );
                        exit( 1 );
                    }
                    seenVariables.insert( nonBasic );

                    if ( nonBasic == basic )
                    {
                        printf( "Error: basic == nonbasic!\n" );
                        exit( 1 );
                    }

                    if ( !_activeSlackRowVars.exists( nonBasic ) && !_activeSlackColVars.exists( nonBasic ) &&
                         ( !_lowerBounds[nonBasic].finite() || !_upperBounds[nonBasic].finite() ) )
                    {
                        printf( "Error! Encountered an infinite bound!\n" );
                        exit( 1 );
                    }
                });

            double currentLowerNB;
            double currentUpperNB;

            unsigned currentLowerNBLevel;
            unsigned currentUpperNBLevel;

            if ( !_activeSlackRowVars.exists( nonBasic ) && !_activeSlackColVars.exists( nonBasic ) )
            {
                if ( _glpkStoredLowerBounds.exists( nonBasic ) )
                {
                    currentLowerNB = _glpkStoredLowerBounds[nonBasic].getBound();
                    currentLowerNBLevel = _glpkStoredLowerBounds[nonBasic].getLevel();
                }
                else
                {
                    currentLowerNB = _lowerBounds[nonBasic].getBound();
                    currentLowerNBLevel = _lowerBounds[nonBasic].getLevel();
                }

                if ( _glpkStoredUpperBounds.exists( nonBasic ) )
                {
                    currentUpperNB = _glpkStoredUpperBounds[nonBasic].getBound();
                    currentUpperNBLevel = _glpkStoredUpperBounds[nonBasic].getLevel();
                }
                else
                {
                    currentUpperNB = _upperBounds[nonBasic].getBound();
                    currentUpperNBLevel = _upperBounds[nonBasic].getLevel();
                }
            }
            else if ( _activeSlackColVars.exists( nonBasic ) )
            {
                currentLowerNB = _slackToLowerBound[nonBasic].getBound();
                currentUpperNB = _slackToUpperBound[nonBasic].getBound();

                currentLowerNBLevel = _slackToLowerBound[nonBasic].getLevel();
                currentUpperNBLevel = _slackToUpperBound[nonBasic].getLevel();
            }
            else
            {
                // Slack row var. If we're using cols, the bounds are 0. Otherwise, read real bounds.
                // This should have no effect on the level of the learned bounds
                if ( _useSlackVariablesForRelus == USE_ROW_AND_COL_SLACK_VARIABLES )
                {
                    currentLowerNB = 0.0;
                    currentUpperNB = 0.0;

                    currentLowerNBLevel = 0.0;
                    currentUpperNBLevel = 0.0;
                }
                else
                {
                    currentLowerNB = _slackToLowerBound[nonBasic].getBound();
                    currentUpperNB = _slackToUpperBound[nonBasic].getBound();

                    currentLowerNBLevel = _slackToLowerBound[nonBasic].getLevel();
                    currentUpperNBLevel = _slackToUpperBound[nonBasic].getLevel();
                }
            }

            if ( FloatUtils::isPositive( weight ) )
            {
                max += ( currentUpperNB * weight );
                min += ( currentLowerNB * weight );

                if ( minBoundLevel < currentLowerNBLevel )
                    minBoundLevel = currentLowerNBLevel;
                if ( maxBoundLevel < currentUpperNBLevel )
                    maxBoundLevel = currentUpperNBLevel;
            }
            else if ( FloatUtils::isNegative( weight ) )
            {
                min += ( currentUpperNB * weight );
                max += ( currentLowerNB * weight );

                if ( maxBoundLevel < currentLowerNBLevel )
                    maxBoundLevel = currentLowerNBLevel;
                if ( minBoundLevel < currentUpperNBLevel )
                    minBoundLevel = currentUpperNBLevel;
            }
        }

        double currentLower;
        double currentUpper;
        double currentLowerLevel;
        double currentUpperLevel;

        if ( _activeSlackColVars.exists( basic ) )
        {
            DEBUG({
                    if ( _useSlackVariablesForRelus != USE_ROW_AND_COL_SLACK_VARIABLES )
                    {
                        printf( "Error! Learned a bound for a col slack variable!\n" );
                        exit( 1 );
                    }
                });

            currentLower = _slackToLowerBound[basic].getBound();
            currentLowerLevel = _slackToLowerBound[basic].getLevel();
            currentUpper = _slackToUpperBound[basic].getBound();
            currentUpperLevel = _slackToUpperBound[basic].getLevel();
        }
        else if ( _activeSlackRowVars.exists( basic ) )
        {
            DEBUG({
                    if ( _useSlackVariablesForRelus != USE_ROW_SLACK_VARIABLES )
                    {
                        printf( "Error! Learned a bound for a row slack variable!\n" );
                        exit( 1 );
                    }
                });

            currentLower = _slackToLowerBound[basic].getBound();
            currentLowerLevel = _slackToLowerBound[basic].getLevel();
            currentUpper = _slackToUpperBound[basic].getBound();
            currentUpperLevel = _slackToUpperBound[basic].getLevel();
        }
        else
        {
            if ( _glpkStoredLowerBounds.exists( basic ) )
            {
                currentLower = _glpkStoredLowerBounds[basic].getBound();
                currentLowerLevel = _glpkStoredLowerBounds[basic].getLevel();
            }
            else
            {
                currentLower = _lowerBounds[basic].getBound();
                currentLowerLevel = _lowerBounds[basic].getLevel();
            }

            if ( _glpkStoredUpperBounds.exists( basic ) )
            {
                currentUpper = _glpkStoredUpperBounds[basic].getBound();
                currentUpperLevel = _glpkStoredUpperBounds[basic].getLevel();
            }
            else
            {
                currentUpper = _upperBounds[basic].getBound();
                currentUpperLevel = _upperBounds[basic].getLevel();
            }
        }

        bool updateOccurred = false;
        if ( FloatUtils::lt( max, currentUpper ) )
        {
            if ( _activeSlackColVars.exists( basic ) || _activeSlackRowVars.exists( basic ) )
            {
                _slackToUpperBound[basic].setBound( max );
                _slackToUpperBound[basic].setLevel( maxBoundLevel );
            }
            else
            {
                _glpkStoredUpperBounds[basic].setBound( max );
                _glpkStoredUpperBounds[basic].setLevel( maxBoundLevel );
            }

            ++_numBoundsDerivedThroughGlpk;
            updateOccurred = true;
            currentUpper = max;
            currentUpperLevel = maxBoundLevel;
        }

        if ( FloatUtils::gt( min, currentLower ) )
        {
            if ( _activeSlackColVars.exists( basic ) || _activeSlackRowVars.exists( basic ) )
            {
                _slackToLowerBound[basic].setBound( min );
                _slackToLowerBound[basic].setLevel( minBoundLevel );
            }
            else
            {
                _glpkStoredLowerBounds[basic].setBound( min );
                _glpkStoredLowerBounds[basic].setLevel( minBoundLevel );
            }

            ++_numBoundsDerivedThroughGlpk;
            updateOccurred = true;
            currentLower = min;
            currentLowerLevel = minBoundLevel;
        }

        if ( updateOccurred && FloatUtils::gt( currentLower, currentUpper ) )
        {
            throw InvariantViolationError( std::max( currentLowerLevel, currentUpperLevel ) );
        }
    }

    bool learnedGlpkBounds() const
    {
        return _glpkStoredLowerBounds.size() + _glpkStoredUpperBounds.size() > 0;
    }

    void performGlpkBoundTightening()
    {
        log( "Starting GLPK bound tightening\n" );

        // It is wrong to assume that all bounds are improvements over existing bounds. As
        // we begin updating, things may change because of relu stuff - so check again before
        // each bound.
        for ( const auto &lowerBound : _glpkStoredLowerBounds )
        {
            if ( FloatUtils::gt( lowerBound.second.getBound(), _lowerBounds[lowerBound.first].getBound() ) )
                updateLowerBound( lowerBound.first, lowerBound.second.getBound(), lowerBound.second.getLevel() );
        }

        for ( const auto &upperBound : _glpkStoredUpperBounds )
        {
            if ( FloatUtils::lt( upperBound.second.getBound(), _upperBounds[upperBound.first].getBound() ) )
                updateUpperBound( upperBound.first, upperBound.second.getBound(), upperBound.second.getLevel() );
        }

        _glpkStoredLowerBounds.clear();
        _glpkStoredUpperBounds.clear();

        log( "Finished with GLPK bound tightening\n" );
    }

    void glpkIterationCountCallback( int count )
    {
        log( Stringf( "GLPK: number of iterations = %i\n", count ) );
        _totalLpPivots += count;
    }

    void glpkReportSoi( double soi )
    {
        log( Stringf( "GLPK report soi: %.10lf\n", soi ) );
        _glpkSoi = soi;
    }

    void setUpperBound( unsigned variable, double bound )
    {
        _upperBounds[variable].setBound( bound );
        _upperBounds[variable].setLevel( 0 );
    }

    void setLowerBound( unsigned variable, double bound )
    {
        _lowerBounds[variable].setBound( bound );
        _lowerBounds[variable].setLevel( 0 );
    }

    bool boundInvariantHolds( unsigned variable, unsigned &violatingStackLevel )
    {
        if ( !_upperBounds[variable].finite() || !_lowerBounds[variable].finite() )
            return true;

        if ( !FloatUtils::lte( _lowerBounds[variable].getBound(), _upperBounds[variable].getBound() ) )
        {
            violatingStackLevel = std::max( _lowerBounds[variable].getLevel(), _upperBounds[variable].getLevel() );
            return false;
        }

        return true;
    }

    bool activeReluVariable( unsigned variable ) const
    {
        if ( !_reluPairs.isRelu( variable ) )
            return false;

        unsigned f = _reluPairs.isF( variable ) ? variable : _reluPairs.toPartner( variable );
        return !_dissolvedReluVariables.exists( f );
    }

    void updateUpperBound( unsigned variable, double bound, unsigned level )
    {
        unsigned partner = 0, b = 0, f = 0;

        if ( _reluPairs.isRelu( variable ) )
        {
            // The variable is relu.
            partner = _reluPairs.toPartner( variable );
            f = _reluPairs.isF( variable ) ? variable : partner;
            b = _reluPairs.isB( variable ) ? variable : partner;
        }

        if ( !_reluPairs.isRelu( variable ) || _dissolvedReluVariables.exists( f ) )
        {
            // For non-relus, we can just update the bound.
            _upperBounds[variable].setBound( bound );
            _upperBounds[variable].setLevel( level );

            unsigned violatingStackLevel;
            if ( !boundInvariantHolds( variable, violatingStackLevel ) )
                throw InvariantViolationError( violatingStackLevel );

            computeVariableStatus( variable );

            // If the variable is basic, it's okay if it's out of bounds.
            // If non-basic and out of bounds, need to update.
            if ( !_basicVariables.exists( variable ) && outOfBounds( variable ) )
                update( variable, bound - _assignment[variable] );

            return;
        }

        // We are in the relu case. Should we use almost-broken elimination?
        if ( FloatUtils::isPositive( bound ) &&
             FloatUtils::lte( bound, ALMOST_BROKEN_RELU_MARGIN ) )
        {
            ++_almostBrokenReluPairCount;

            if ( _eliminateAlmostBrokenRelus )
            {
                ++_almostBrokenReluPairFixedCount;
                bound = 0.0;
            }
        }

        // If the bound is positive, update bounds on both F and B.
        if ( FloatUtils::isPositive( bound ) )
        {
            _upperBounds[variable].setBound( bound );
            _upperBounds[variable].setLevel( level );
            _upperBounds[partner].setBound( bound );
            _upperBounds[partner].setLevel( level );

            unsigned violatingStackLevel;
            if ( !boundInvariantHolds( variable, violatingStackLevel ) ||
                 !boundInvariantHolds( partner, violatingStackLevel ) )
                throw InvariantViolationError( violatingStackLevel );

            computeVariableStatus( variable );
            computeVariableStatus( partner );

            // Violations are okay for basic, but need to update if non-basic
            if ( !_basicVariables.exists( variable ) && outOfBounds( variable ) )
                update( variable, bound - _assignment[variable], true );
            if ( !_basicVariables.exists( partner ) && outOfBounds( partner ) )
                update( partner, bound - _assignment[partner], true );

            return;
        }
        else
        {
            // Non-positive bound.
            if ( FloatUtils::isNegative( bound ) && _reluPairs.isF( variable ) )
            {
                _upperBounds[variable].setBound( bound );
                _upperBounds[variable].setLevel( level );

                // F variables have lower bound of at least 0. Do this "by-the-book" for proper stack handling.
                unsigned violatingStackLevel;
                if ( !boundInvariantHolds( variable, violatingStackLevel ) )
                    throw InvariantViolationError( violatingStackLevel );
                else
                {
                    printf( "Error! Expected violation on F!\n" );
                    exit( 1 );
                }
            }

            // F will have zero as upper bound.
            markReluVariableDissolved( f, TYPE_SPLIT );

            _upperBounds[f].setBound( 0.0 );
            _upperBounds[f].setLevel( level );
            _upperBounds[b].setBound( bound );
            _upperBounds[b].setLevel( level );

            unsigned violatingStackLevel;
            if ( !boundInvariantHolds( b, violatingStackLevel ) || !boundInvariantHolds( f, violatingStackLevel ) )
                throw InvariantViolationError( violatingStackLevel );

            computeVariableStatus( b );
            computeVariableStatus( f );

            // Violations are okay for basic, but need to update if non-basic
            if ( !_basicVariables.exists( b ) && outOfBounds( b ) )
                update( b, bound - _assignment[b], true );
            if ( !_basicVariables.exists( f ) && outOfBounds( f ) )
                update( f, -_assignment[f], true );

            return;
        }
    }

    bool updateLowerBound( unsigned variable, double bound, unsigned level )
    {
        unsigned partner = 0, b = 0, f = 0;

        if ( _reluPairs.isRelu( variable ) )
        {
            // The variable is relu.
            partner = _reluPairs.toPartner( variable );
            f = _reluPairs.isF( variable ) ? variable : partner;
            b = _reluPairs.isB( variable ) ? variable : partner;
        }

        if ( !_reluPairs.isRelu( variable ) || _dissolvedReluVariables.exists( f ) )
        {
            // For non-relus, we can just update the bound.
            _lowerBounds[variable].setBound( bound );
            _lowerBounds[variable].setLevel( level );

            unsigned violatingStackLevel;
            if ( !boundInvariantHolds( variable, violatingStackLevel ) )
                throw InvariantViolationError( violatingStackLevel );

            computeVariableStatus( variable );

            // If the variable is basic, it's okay if it's out of bounds.
            // If non-basic, need to update.
            if ( !_basicVariables.exists( variable ) && outOfBounds( variable ) )
                update( variable, bound - _assignment[variable] );

            // The tableau has not changed
            return false;
        }

        // Should we use almost-broken elimination?
        if ( FloatUtils::isNegative( bound ) &&
             FloatUtils::gte( bound, -ALMOST_BROKEN_RELU_MARGIN ) )
        {
            ++_almostBrokenReluPairCount;

            if ( _eliminateAlmostBrokenRelus )
            {
                ++_almostBrokenReluPairFixedCount;
                bound = 0.0;
            }
        }

        // If the bound is non-negative, update bounds on both F and B.
        if ( !FloatUtils::isNegative( bound ) )
        {
            log( "Update lower bound: non-negative lower bound\n" );

            _lowerBounds[variable].setBound( bound );
            _lowerBounds[variable].setLevel( level );
            _lowerBounds[partner].setBound( bound );
            _lowerBounds[partner].setLevel( level );

            unsigned violatingStackLevel;
            if ( !boundInvariantHolds( variable, violatingStackLevel ) ||
                 !boundInvariantHolds( partner, violatingStackLevel ) )
                throw InvariantViolationError( violatingStackLevel );

            computeVariableStatus( variable );
            computeVariableStatus( partner );

            // Violations are okay for basic, but need to update if non-basic
            if ( !_basicVariables.exists( variable ) && outOfBounds( variable ) )
                update( variable, bound - _assignment[variable], true );
            if ( !_basicVariables.exists( partner ) && outOfBounds( partner ) )
                update( partner, bound - _assignment[partner], true );

            return unifyReluPair( f );
        }
        else
        {
            // Negative bound. This can only be called for the B, doesn't affect the F.

            _lowerBounds[variable].setBound( bound );
            _lowerBounds[variable].setLevel( level );

            unsigned violatingStackLevel;
            if ( !boundInvariantHolds( variable, violatingStackLevel ) )
                throw InvariantViolationError( violatingStackLevel );

            computeVariableStatus( variable );

            // If the variable is basic, it's okay if it's out of bounds.
            // If non-basic, need to update.
            if ( !_basicVariables.exists( variable ) && outOfBounds( variable ) )
                update( variable, bound - _assignment[variable], true );

            // The tableau has not changed
            return false;
        }
    }

    // Return true iff the tableau changes
    bool unifyReluPair( unsigned f )
    {
        unsigned b = _reluPairs.toPartner( f );
        log( Stringf( "UnifyReluPair called with f = %s, b = %s\n", toName( f ).ascii(),
                      toName( b ).ascii()) );

        // If these two have been unified before, b's column will be empty. Tableau doesn't change.
        if ( _tableau.getColumnSize( b ) == 0 )
        {
            log( Stringf( "UnifyReluPair: b's column is empty, ignroing. Previous dissolved? %s\n",
                          _dissolvedReluVariables.exists( f ) ? "YES" : "NO" ) );
            return false;
        }

        log( Stringf( "Unifying relu pair: %s, %s\n", toName( b ).ascii(), toName( f ).ascii() ) );

        // First step: make sure f and b are not basic.
        // Note: this may temporarily break the axiom that non-basic variables must be within bounds
        //       (if f or b are currently OOB). However, this will be fixed afterwards.

        if ( _basicVariables.exists( b ) )
            makeNonBasic( b, f );

        if ( _basicVariables.exists( f ) )
            makeNonBasic( f, b );

        log( "Both variables are now non-basic\n" );
        dump();

        // Next: set f to be in bounds.
        if ( tooLow( f ) )
            update( f, _lowerBounds[f].getBound() - _assignment[f], true );
        else if ( tooHigh( f ) )
            update( f, _upperBounds[f].getBound() - _assignment[f], true );

        // Get b to equal f (bounds are equal for both, so this is okay)
        update( b, _assignment[f] - _assignment[b], true );

        // b and f are now equal and non basic. Replace b with f
        _tableau.addColumnEraseSource( b, f );

        markReluVariableDissolved( f, TYPE_MERGE );

        log( "Tableau after unification:\n" );
        dump();

        return true;
    }

    void makeNonBasic( unsigned basic, unsigned forbiddenPartner )
    {
        if ( !_basicVariables.exists( basic ) )
            throw Error( Error::VARIABLE_NOT_BASIC );

        const Tableau::Entry *rowEntry = _tableau.getRow( basic );
        const Tableau::Entry *current;

        bool found = false;
        unsigned leastEvilNonBasic = 0;
        double leastEvilWeight = 0.0;

        while ( rowEntry )
        {
            current = rowEntry;
            rowEntry = rowEntry->nextInRow();

            unsigned column = current->getColumn();
            if ( ( column == basic ) || ( column == forbiddenPartner ) )
                continue;

            double weight = FloatUtils::abs( current->getValue() );
            if ( FloatUtils::gte( weight, NUMBERICAL_INSTABILITY_CONSTANT ) )
            {
                pivot( column, basic );
                return;
            }

            // The weight is smaller than the instability constant
            found = true;
            if ( FloatUtils::gt( weight, leastEvilWeight ) )
            {
                leastEvilWeight = weight;
                leastEvilNonBasic = column;
            }
        }

        if ( !found )
            throw Error( Error::CANT_MAKE_NON_BASIC );

        pivot( leastEvilNonBasic, basic );
    }

    void setReluPair( unsigned backward, unsigned forward )
    {
        _reluPairs.addPair( backward, forward );

        if ( _useSlackVariablesForRelus != DONT_USE_SLACK_VARIABLES )
        {
            unsigned nextIndex = _fToSlackRowVar.size() + _fToSlackColVar.size();
            _fToSlackRowVar[forward] = _numVariables + nextIndex;
            _slackRowVariableToF[_numVariables + nextIndex] = forward;
            _slackRowVariableToB[_numVariables + nextIndex] = backward;

            if ( _useSlackVariablesForRelus == USE_ROW_AND_COL_SLACK_VARIABLES )
            {
                ++nextIndex;
                _fToSlackColVar[forward] = _numVariables + nextIndex;
            }
        }
    }

    void initializeCell( unsigned row, unsigned column, double value )
    {
        _tableau.addEntry( row, column, value );
    }

    void markBasic( unsigned variable )
    {
        _basicVariables.insert( variable );
    }

    void setName( unsigned variable, String name )
    {
        log( Stringf( "Setting name: %s --> %u\n", name.ascii(), variable ) );
        _variableNames[variable] = name;
    }

    String toName( unsigned variable ) const
    {
        if ( _variableNames.exists( variable ) )
            return _variableNames.at( variable );
        return Stringf( "%u", variable );
    }

    void initialUpdate()
    {
        computeVariableStatus();

        for ( unsigned i = 0; i < _numVariables; ++i )
        {
            unsigned violatingStackLevel;
            if ( !boundInvariantHolds( i, violatingStackLevel ) )
            {
                printf( "Bound invariant violation on variable: %s\n", toName( i ).ascii() );
                printf( "Lower bound = %.5lf, upper bound = %.5lf\n",
                        _lowerBounds[i].getBound(), _upperBounds[i].getBound() );
                throw InvariantViolationError( violatingStackLevel );
            }

            if ( !_basicVariables.exists( i ) && outOfBounds( i ) )
            {
                if ( tooLow( i ) )
                    update( i, _lowerBounds[i].getBound() - _assignment[i] );
                else
                    update( i, _upperBounds[i].getBound() - _assignment[i] );
            }
        }

        log( "Checking invariants after initial update\n" );
        checkInvariants();
    }

    bool fixBrokenRelu( unsigned toFix )
    {
        bool isF = _reluPairs.isF( toFix );
        unsigned partner = _reluPairs.toPartner( toFix );
        unsigned f = isF ? toFix : partner;
        unsigned b = isF ? partner : toFix;

        ++_brokenRelusFixed;

        log( Stringf( "\nAttempting broken-relu fix on var: %s\n", toName( toFix ).ascii() ) );

        double fVal = _assignment[f];
        double bVal = _assignment[b];
        double fDelta;
        double bDelta;

        if ( FloatUtils::isPositive( fVal ) && !FloatUtils::isPositive( bVal ) )
        {
            fDelta = -fVal;
            bDelta = fVal - bVal;
        }
        else if ( FloatUtils::isPositive( fVal ) && FloatUtils::isPositive( bVal ) )
        {
            fDelta = bVal - fVal;
            bDelta = fVal - bVal;
        }
        else if ( FloatUtils::isZero( fVal ) && FloatUtils::isPositive( bVal ) )
        {
            fDelta = bVal;
            bDelta = -bVal;
        }
        else
            exit( 1 );                           // Unreachable

        bool increaseB = FloatUtils::isPositive( bDelta );
        bool increaseF = FloatUtils::isPositive( fDelta );

        // Always try to fix B first, and if impossible fix f.
        if ( !fixBrokenReluVariable( b, increaseB, bDelta, _brokenReluFixB ) )
            return fixBrokenReluVariable( f, increaseF, fDelta, _brokenReluFixF );

        return true;
    }

    bool fixBrokenReluVariable( unsigned var, bool increase, double &delta, unsigned &_brokenReluStat )
    {
        log( Stringf( "fixBrokenReluVariable Starting: var = %s, delta = %lf\n", toName( var ).ascii(), delta ) );

        if ( !_basicVariables.exists( var ) )
        {
            DEBUG(
                  if ( !allVarsWithinBounds() )
                  {
                      printf( "Error! Should not be broken a relu var when we have OOB vars!\n" );
                      exit( 1 );
                  }

                  if ( !canAddToNonBasic( var, delta ) )
                  {
                      // Error: this should not happen. We should not be fixing a broken relu unless
                      // both b and f are within bounds.
                      printf( "Error: var %s is not basic, but can't add delta = %lf to it!\n",
                              toName( var ).ascii(), delta );
                      throw Error( Error::CANT_FIX_BROKEN_RELU, "Unreachable code" );
                  }
                  );

            ++_brokenReluStat;
            ++_brokenReluFixByUpdate;

            log( Stringf( "Var %s isn't basic; no pivot needed, simply updating\n", toName( var ).ascii() ) );
            update( var, delta, true );
            return true;
        }
        else
        {
            ++_brokenReluStat;
            ++_brokenReluFixByPivot;

            unsigned pivotCandidate;
            if ( !findPivotCandidate( var, increase, pivotCandidate ) )
                return false;

            log( Stringf( "\nPivotAndUpdate: <%s, %5.2lf, %s>\n",
                          toName( var ).ascii(), delta, toName( pivotCandidate ).ascii() ) );

            DEBUG(
                  if ( outOfBounds( var ) )
                  {
                      printf( "Error! Performing a RELU fix when we have an OOB variable\n" );
                      exit( 1 );
                  }
                  );

            // Execute the pivot-and-update operation.
            pivot( pivotCandidate, var );
            update( var, delta, true );

            return true;
        }
    }

    void turnAlmostZeroToZero( double &x )
    {
        if ( FloatUtils::isZero( x ) )
            x = 0.0;
    }

    void update( unsigned variable, double delta, bool ignoreRelu = false )
    {
        if ( FloatUtils::isZero( delta ) )
            return;

        log( Stringf( "\t\tUpdate: %s += %.2lf\n", toName( variable ).ascii(), delta ) );

        _assignment[variable] += delta;
        turnAlmostZeroToZero( _assignment[variable] );
        computeVariableStatus( variable );

        const Tableau::Entry *columnEntry = _tableau.getColumn( variable );
        const Tableau::Entry *current;

        while ( columnEntry != NULL )
        {
            current = columnEntry;
            columnEntry = columnEntry->nextInColumn();

            unsigned row = current->getRow();
            if ( row != variable )
            {
                _assignment[row] += delta * current->getValue();
                turnAlmostZeroToZero( _assignment[row] );
                computeVariableStatus( row );
            }
        }

        // If the updated variable was Relu, we might need to fix the relu invariant
        if ( _reluPairs.isRelu( variable ) && !ignoreRelu )
        {
            unsigned partner = _reluPairs.toPartner( variable );
            bool variableIsF = _reluPairs.isF( variable );
            unsigned b = variableIsF ? partner : variable;
            unsigned f = variableIsF ? variable : partner;

            log( Stringf( "Update was on relu. Parnter = %u\n", partner ) );

            // If the partner is basic, it's okay for the pair to be broken
            if ( _basicVariables.exists( partner ) )
            {
                log( "Partner is basic. ignoring...\n" );
                return;
            }

            // The partner is NOT basic. If the connection is broken,
            // we can fix the partner, if needed.

            log( "Parnter is NOT basic. Checking if more work is needed...\n" );

            log( Stringf( "b = %u, f = %u, bVal = %lf, fVal = %lf\n", b, f, _assignment[b], _assignment[f] ) );

            if ( _dissolvedReluVariables.exists( f ) )
            {
                log( "Pair has been disolved, don't care about a violation\n" );
                return;
            }

            if ( !reluPairIsBroken( b, f ) )
            {
                log( "relu pair is NOT broken\n" );
                return;
            }

            if ( variableIsF )
            {
                // We need to fix B. This means setting the value of B to that of F.
                log( Stringf( "Cascading update: fixing non-basic relu partner b = %u\n", b ) );
                update( b, _assignment[f] - _assignment[b], true );

                DEBUG(
                      if ( _varToStatus[b] == ABOVE_UB || _varToStatus[b] == BELOW_LB )
                          throw Error( Error::NONBASIC_OUT_OF_BOUNDS,
                                       "After a cascaded b-update, b is non-basic and OOB" );
                      );
            }
            else
            {
                // We need to fix F. This means setting the value of F to 0 if B is negative,
                // and otherwise just setting it to B.
                log( Stringf( "Cascading update: fixing non-basic relu partner f = %u\n", f ) );
                if ( FloatUtils::isNegative( _assignment[b] ) )
                    update( f, -_assignment[f], true );
                else
                    update( f, _assignment[b] - _assignment[f], true );

                DEBUG(
                      if ( _varToStatus[f] == ABOVE_UB || _varToStatus[f] == BELOW_LB )
                          throw Error( Error::NONBASIC_OUT_OF_BOUNDS,
                                       "After a cascaded f-update, f is non-basic and OOB" );
                      );
            }
        }
    }

    void pivot( unsigned nonBasic, unsigned basic )
    {
        ++_numPivots;

        log( Stringf( "\t\tPivot: %s <--> %s\n", toName( basic ).ascii(), toName( nonBasic ).ascii() ) );

        // Sanity checks:
        if ( _basicVariables.exists( nonBasic ) )
            throw Error( Error::ILLEGAL_PIVOT_OP,
                         Stringf( "Non-basic variable %s is basic", toName( nonBasic ).ascii() ).ascii() );

        if ( !_basicVariables.exists( basic ) )
            throw Error( Error::ILLEGAL_PIVOT_OP, "Basic variable isn't basic" );

        _basicVariables.erase( basic );
        _basicVariables.insert( nonBasic );

        timeval start = Time::sampleMicro();
        unsigned numCalcs = 0;

        double cell = _tableau.getCell( basic, nonBasic );
        double absWeight = FloatUtils::abs( cell );

        if ( FloatUtils::lt( absWeight, NUMBERICAL_INSTABILITY_CONSTANT ) )
        {
            printf( "--- Numerical Instability Warning!! Weight = %.15lf ---\n", absWeight );
        }

        _tableau.addScaledRow( basic,
                               ( -1.0 ) / cell,
                               nonBasic,
                               // Guarantee a -1 in the (nb,nb) cell
                               nonBasic, -1.0,
                               //
                               &numCalcs
                               );
        _tableau.eraseRow( basic );

        log( Stringf( "\t\t\tPivot--clearing %u column entries--starting\n",
                      _tableau.getColumnSize( nonBasic ) ) );

        const Tableau::Entry *columnEntry = _tableau.getColumn( nonBasic );
        const Tableau::Entry *current;

        while ( columnEntry != NULL )
        {
            current = columnEntry;
            columnEntry = columnEntry->nextInColumn();

            if ( current->getRow() != nonBasic )
            {
                _tableau.addScaledRow( nonBasic,
                                       current->getValue(),
                                       current->getRow(),
                                       // Guarantee a 0 in the (*,nb) cells
                                       nonBasic, 0.0,
                                       //
                                       &numCalcs
                                       );
            }
        }

        timeval end = Time::sampleMicro();
        log( Stringf( "\t\t\tPivot--clearing column entries--done (Pivot: %u milli, %u calcs)\n",
                      Time::timePassed( start, end ), numCalcs ) );

        _totalPivotTimeMilli += Time::timePassed( start, end );
        _totalPivotCalculationCount += numCalcs;
    }

    void dump()
    {
        if ( !_dumpStates )
            return;

        log( "\nVisiting state:\n" );

        log( "\n" );
        log( "       | " );
        for ( unsigned i = 0; i < _numVariables; ++i )
            log( Stringf( "%6s", toName( i ).ascii() ) );
        log( " | Assignment               " );
        log( "\n" );
        for ( unsigned i = 0; i < 9 + ( _numVariables * 6 ) + 13 + 15; ++i )
            log( "-" );
        log( "\n" );

        for ( unsigned i = 0; i < _numVariables; ++i )
        {
            if ( _basicVariables.exists( i ) )
                log( " B " );
            else
                log( "   " );

            log( Stringf( "%4s| ", toName( i ).ascii() ) );
            for ( unsigned j = 0; j < _numVariables; ++j )
            {
                if ( !FloatUtils::isZero( _tableau.getCell( i, j ) ) )
                    log( Stringf( "%6.2lf", _tableau.getCell( i, j ) ) );
                else
                    log( "      " );
            }
            log( " | " );

            if ( _lowerBounds[i].finite() )
                log( Stringf( "%5.2lf <= ", _lowerBounds[i].getBound() ) );
            else
                log( "         " );

            log( Stringf( "%5.2lf", _assignment[i] ) );

            if ( outOfBounds( i ) || ( activeReluVariable( i ) && partOfBrokenRelu( i ) ) )
                log( " * " );
            else
                log( "   " );

            if ( _upperBounds[i].finite() )
                log( Stringf( "<= %5.2lf", _upperBounds[i].getBound() ) );
            else
                log( "         " );

            log( "\n" );
        }

        log( "\n" );
    }

    bool canAddToNonBasic( unsigned variable, double delta )
    {
        if ( FloatUtils::isZero( delta ) )
            return true;

        bool positive = FloatUtils::isPositive( delta );
        VariableStatus status = _varToStatus[variable];

        if ( status == ABOVE_UB || status == BELOW_LB )
            throw Error( Error::NONBASIC_OUT_OF_BOUNDS );

        if ( status == FIXED )
            return false;

        if ( positive )
        {
            if ( status == AT_UB && FloatUtils::gt( delta, OOB_EPSILON ) )
                return false;

            if ( !_upperBounds[variable].finite() )
                return true;

            return FloatUtils::lte( _assignment[variable] + delta, _upperBounds[variable].getBound(), OOB_EPSILON );
        }
        else
        {
            if ( status == AT_LB && FloatUtils::lt( delta, -OOB_EPSILON ) )
                return false;

            if ( !_lowerBounds[variable].finite() )
                return true;

            return FloatUtils::gte( _assignment[variable] + delta, _lowerBounds[variable].getBound(), OOB_EPSILON );
        }
    }

    bool tooLow( unsigned variable ) const
    {
        return _varToStatus.at( variable ) == VariableStatus::BELOW_LB;
    }

    bool canDecrease( unsigned variable ) const
    {
        return
            _varToStatus.at( variable ) == VariableStatus::BETWEEN ||
            _varToStatus.at( variable ) == VariableStatus::AT_UB ||
            _varToStatus.at( variable ) == VariableStatus::ABOVE_UB;
    }

    bool tooHigh( unsigned variable ) const
    {
        return _varToStatus.at( variable ) == VariableStatus::ABOVE_UB;
    }

    bool canIncrease( unsigned variable ) const
    {
        return
            _varToStatus.at( variable ) == VariableStatus::BETWEEN ||
            _varToStatus.at( variable ) == VariableStatus::AT_LB ||
            _varToStatus.at( variable ) == VariableStatus::BELOW_LB;
    }

    bool outOfBounds( unsigned variable ) const
    {
        return tooLow( variable ) || tooHigh( variable );
    }

    void log( String message )
    {
        if ( _logging )
            printf( "%s", message.ascii() );
    }

    double getAssignment( unsigned variable ) const
    {
        return _assignment[variable];
    }

    void setLogging( bool value )
    {
        _logging = value;
    }

    void setDumpStates( bool value )
    {
        _dumpStates = value;
    }

    unsigned numStatesExplored() const
    {
        return _numCallsToProgress;
    }

    bool fixedAtZero( unsigned var ) const
    {
        return
            ( _varToStatus.get( var ) == VariableStatus::FIXED ) &&
            FloatUtils::isZero( _upperBounds[var].getBound() );
    }

    bool eliminateAuxVariables()
    {
        log( "eliminateAuxVariables starting\n" );
        computeVariableStatus();

        Set<unsigned> initialAuxVariables = _basicVariables;

        for ( const auto &aux : initialAuxVariables )
        {
            if ( !eliminateIfPossible( aux ) )
            {
                log( "eliminateAuxVariables finished UNsuccessfully\n" );
                return false;
            }
        }

        log( "eliminateAuxVariables finished successfully\n" );
        return true;
    }

    bool eliminateIfPossible( unsigned var )
    {
        if ( _reluPairs.isRelu( var ) )
        {
            printf( "Attempted to eliminate a relu variable. They shouldn't be marked as aux\n" );
            exit( 1 );
        }

        bool increase = tooLow( var );
        double delta = increase?
            ( _lowerBounds[var].getBound() - _assignment[var] ) :
            ( _upperBounds[var].getBound() - _assignment[var] );

        unsigned pivotCandidate;
        if ( !findPivotCandidate( var, increase, pivotCandidate, false ) ){
            log("Can't findPivotCandidate for a variable\n");
            return true;
        }

        log( Stringf( "\nPivotAndUpdate: <%s, %5.2lf, %s>\n",
                      toName( var ).ascii(),
                      delta,
                      toName( pivotCandidate ).ascii() ) );

        pivot( pivotCandidate, var );
        update( var, delta );

        if ( !fixedAtZero( var ) )
        {
            printf( "eliminateIfPossible called for a non fixed-at-zero variable\n" );
            return true;
        }

        log( Stringf( "\nVariable %s fixed at zero. Eliminating...\n", toName( var ).ascii() ) );
        _tableau.eraseColumn( var );
        _eliminatedVars.insert( var );
        ++_numEliminatedVars;

        return true;
    }

    bool findPivotCandidate( unsigned variable, bool increase, unsigned &pivotCandidate,
                             bool ensureNumericalStability = true )
    {
        const Tableau::Entry *rowEntry = _tableau.getRow( variable );
        const Tableau::Entry *current;

        unsigned column;

        bool found = false;
        unsigned leastEvilNonBasic = 0;
        double leastEvilWeight = 0.0;

        while ( rowEntry != NULL )
        {
            current = rowEntry;
            rowEntry = rowEntry->nextInRow();

            column = current->getColumn();

            // Ignore self
            if ( column == variable )
                continue;

            const double coefficient = current->getValue();
            bool positive = FloatUtils::isPositive( coefficient );

            if ( !( ( increase && ( positive ) && canIncrease( column ) ) ||
                    ( increase && ( !positive ) && canDecrease( column ) ) ||
                    ( !increase && ( positive ) && canDecrease( column ) ) ||
                    ( !increase && ( !positive ) && canIncrease( column ) ) ) )
            {
                // The variable does not fit the direction we need.
                continue;
            }

            double weight = FloatUtils::abs( coefficient );
            if ( !ensureNumericalStability || FloatUtils::gte( weight, NUMBERICAL_INSTABILITY_CONSTANT ) )
            {
                pivotCandidate = column;
                return true;
            }

            // Have a candidate with a small pivot coefficient
            found = true;
            if ( FloatUtils::gt( weight, leastEvilWeight ) )
            {
                leastEvilWeight = weight;
                leastEvilNonBasic = column;
            }
        }

        if ( found )
        {
            log( Stringf( "findPivotCandidate: forced to pick a bad candidate! Weight = %lf\n", leastEvilWeight ) );
            pivotCandidate = leastEvilNonBasic;
            return true;
        }

        return false;
    }

    const VariableBound *getLowerBounds() const
    {
        return _lowerBounds;
    }

    const VariableBound *getUpperBounds() const
    {
        return _upperBounds;
    }

    double getLowerBound( unsigned var ) const
    {
        DEBUG(
              if ( !_lowerBounds[var].finite() )
                  throw Error( Error::LOWER_BOUND_IS_INFINITE );
              );

        return _lowerBounds[var].getBound();
    }

    double getUpperBound( unsigned var ) const
    {
        DEBUG(
              if ( !_upperBounds[var].finite() )
                  throw Error( Error::UPPER_BOUND_IS_INFINITE );
              );

        return _upperBounds[var].getBound();
    }

    Set<unsigned> getBasicVariables() const
    {
        return _basicVariables;
    }

    ReluPairs *getReluPairs()
    {
        return &_reluPairs;
    }

    void setLowerBounds( const List<VariableBound> &lowerBounds )
    {
        unsigned i = 0;
        for ( const auto &it : lowerBounds )
        {
            _lowerBounds[i] = it;
            ++i;
        }
    }

    void setUpperBounds( const List<VariableBound> &upperBounds )
    {
        unsigned i = 0;
        for ( const auto &it : upperBounds )
        {
            _upperBounds[i] = it;
            ++i;
        }
    }

    void setBasicVariables( const Set<unsigned> &basicVariables )
    {
        _basicVariables = basicVariables;
    }

    void setReluPairs( const ReluPairs &reluPairs )
    {
        _reluPairs = reluPairs;
    }

    void backupIntoMatrix( Tableau *matrix ) const
    {
        _tableau.backupIntoMatrix( matrix );
    }

    void restoreFromMatrix( Tableau *matrix )
    {
        matrix->backupIntoMatrix( &_tableau );

        DEBUG(
              log( "Printing matrix after restoration\n" );
              log( "****\n" );
              dump();
              log( "****\n\n" );
              );
    }

    const double *getAssignment() const
    {
        return _assignment;
    }

    void setAssignment( const List<double> &assignment )
    {
        unsigned i = 0;
        for ( const auto &it : assignment )
        {
            _assignment[i] = it;
            ++i;
        }
    }

    void makeAllBoundsFinite()
    {
        countVarsWithInfiniteBounds();
        log( Stringf( "makeAllBoundsFinite -- Starting (%u vars with infinite bounds)\n", _varsWithInfiniteBounds ) );
        printStatistics();

        for ( const auto &basic : _basicVariables )
            makeAllBoundsFiniteOnRow( basic );

        countVarsWithInfiniteBounds();
        log( Stringf( "makeAllBoundsFinite -- Done (%u vars with infinite bounds)\n", _varsWithInfiniteBounds ) );
        printStatistics();

        if ( _varsWithInfiniteBounds != 0 )
            throw Error( Error::EXPECTED_NO_INFINITE_VARS );
    }

    void makeAllBoundsFiniteOnRow( unsigned basic )
    {
        const Tableau::Entry *row = _tableau.getRow( basic );
        const Tableau::Entry *tighteningVar = NULL;

        while ( row != NULL )
        {
            if ( !_upperBounds[row->getColumn()].finite() || !_lowerBounds[row->getColumn()].finite() )
            {
                if ( tighteningVar != NULL )
                    throw Error( Error::MULTIPLE_INFINITE_VARS_ON_ROW );

                tighteningVar = row;
            }

            row = row->nextInRow();
        }

        // It's possible that there are no infinite vars on this row - e.g., if the user supplied
        // bounds on the outputs.
        if ( !tighteningVar )
            return;

        unsigned tighteningVarIndex = tighteningVar->getColumn();

        double scale = -1.0 / tighteningVar->getValue();

        row = _tableau.getRow( basic );
        const Tableau::Entry *current;

        double max = 0.0;
        double min = 0.0;

        while ( row != NULL )
        {
            current = row;
            row = row->nextInRow();

            if ( current->getColumn() == tighteningVarIndex )
                continue;

            double coefficient = current->getValue() * scale;
            if ( FloatUtils::isPositive( coefficient ) )
            {
                max += _upperBounds[current->getColumn()].getBound() * coefficient;
                min += _lowerBounds[current->getColumn()].getBound() * coefficient;
            }
            else
            {
                min += _upperBounds[current->getColumn()].getBound() * coefficient;
                max += _lowerBounds[current->getColumn()].getBound() * coefficient;
            }
        }

        if ( !_upperBounds[tighteningVarIndex].finite() ||
             FloatUtils::lt( max, _upperBounds[tighteningVarIndex].getBound() ) )
            updateUpperBound( tighteningVarIndex, max, 0 );

        if ( !_lowerBounds[tighteningVarIndex].finite() ||
             FloatUtils::gt( min, _lowerBounds[tighteningVarIndex].getBound() ) )
            updateLowerBound( tighteningVarIndex, min, 0 );

        computeVariableStatus( tighteningVarIndex );
        if ( !_basicVariables.exists( tighteningVarIndex ) && outOfBounds( tighteningVarIndex ) )
            update( tighteningVarIndex,
                    _lowerBounds[tighteningVarIndex].getBound() - _assignment[tighteningVarIndex] );
    }

    void setUseApproximation( bool value )
    {
        _useApproximations = value;
    }

    void setFindAllPivotCandidates( bool value )
    {
        _findAllPivotCandidates = value;
    }

    bool isDissolvedBVariable( unsigned variable ) const
    {
        if ( !_reluPairs.isRelu( variable ) )
            return false;

        if ( _reluPairs.isF( variable ) )
            return false;

        unsigned f = _reluPairs.toPartner( variable );

        if ( !_dissolvedReluVariables.exists( f ) )
            return false;

        return _dissolvedReluVariables.at( f ) == TYPE_MERGE;
    }

    bool isEliminatedVar( unsigned variable ) const
    {
        return _eliminatedVars.exists( variable );
    }

    void markReluVariableDissolved( unsigned variable, ReluDissolutionType type )
    {
        log( Stringf( "Mark var as dissolved: %u (Type: %s)\n",
                      variable,
                      type == TYPE_SPLIT ? "Split" : "Merge" ) );

        DEBUG(
              if ( _dissolvedReluVariables.exists( variable ) )
              {
                  printf( "Error -- this variable was already marked as dissolved!\n" );
                  exit( 1 );
              }
              );

        _dissolvedReluVariables[variable] = type;
    }

    void incNumSplits()
    {
        ++_numStackSplits;
    }

    void incNumMerges()
    {
        ++_numStackMerges;
    }

    void incNumPops()
    {
        ++_numStackPops;
    }

    void incNumStackVisitedStates()
    {
        ++_numStackVisitedStates;
    }

    void setCurrentStackDepth( unsigned depth )
    {
        _currentStackDepth = depth;
        if ( _currentStackDepth > _maximalStackDepth )
            _maximalStackDepth = _currentStackDepth;
    }

    void setMinStackSecondPhase( unsigned depth )
    {
        if ( ( depth < _minStackSecondPhase ) || ( _minStackSecondPhase == 0 ) )
            _minStackSecondPhase = depth;
    }

    unsigned getColumnSize( unsigned column ) const
    {
        return _tableau.getColumnSize( column );
    }

    Map<unsigned, ReluDissolutionType> getDissolvedReluPairs() const
    {
        return _dissolvedReluVariables;
    }

    void setDissolvedReluPairs( const Map<unsigned, ReluDissolutionType> &pairs )
    {
        _dissolvedReluVariables = pairs;
    }

    unsigned reluVarToF( unsigned variable ) const
    {
        if ( _reluPairs.isF( variable ) )
            return variable;

        return _reluPairs.toPartner( variable );
    }

    bool isReluVariable( unsigned variable ) const
    {
        return _reluPairs.isRelu( variable );
    }

    void printAssignment()
    {
        printf( "\nCurrent assignment:\n" );
        for ( unsigned i = 0; i < _numVariables; ++i )
        {
            if ( _eliminatedVars.exists( i ) || !outOfBounds( i ) )
                continue;

            printf( "\t%u: %.10lf <= %.10lf <= %.10lf",
                    i, _lowerBounds[i].getBound(), _assignment[i], _upperBounds[i].getBound() );
            if ( outOfBounds( i ) )
                printf( "  ***" );
            if ( _basicVariables.exists( i ) )
                printf( " B" );
            printf( "\n" );
        }
        printf( "\n" );
    }

    unsigned getNumVariables() const
    {
        return _numVariables;
    }

    const Tableau::Entry *getColumn( unsigned column ) const
    {
        return _tableau.getColumn( column );
    }

    const Tableau::Entry *getRow( unsigned row ) const
    {
        return _tableau.getRow( row );
    }

    double getCell( unsigned row, unsigned column ) const
    {
        return _tableau.getCell( row, column );
    }

    Set<unsigned> getEliminatedVars() const
    {
        return _eliminatedVars;
    }

    const Tableau *getTableau() const
    {
        return &_tableau;
    }

    void storePreprocessedMatrix()
    {
        checkInvariants();

        _tableau.backupIntoMatrix( &_preprocessedTableau );
        _preprocessedDissolvedRelus = _dissolvedReluVariables;
        _preprocessedBasicVariables = _basicVariables;
        memcpy( _preprocessedAssignment, _assignment, sizeof(double) * _numVariables );

        for ( unsigned i = 0; i < _numVariables; ++i )
        {
            // It is assumed that these bounds are all at level 0.
            _preprocessedLowerBounds[i] = _lowerBounds[i];
            _preprocessedUpperBounds[i] = _upperBounds[i];
        }
    }

    void restoreTableauFromBackup( bool keepCurrentBasicVariables = true )
    {
        timeval start = Time::sampleMicro();

        ++_numberOfRestorations;

        printf( "\n\n\t\t !!! Restore tableau from backup starting !!!\n" );
        double *backupLowerBounds = new double[_numVariables];
        double *backupUpperBounds = new double[_numVariables];
        unsigned *backupLowerBoundLevels = new unsigned[_numVariables];
        unsigned *backupUpperBoundLevels = new unsigned[_numVariables];

        Set<unsigned> backupBasicVariables = _basicVariables;

        for ( unsigned i = 0; i < _numVariables; ++i )
        {
            DEBUG({
                    if ( FloatUtils::lt( _lowerBounds[i].getBound(), _preprocessedLowerBounds[i].getBound() ) )
                    {
                        printf( "Error with a decreasing LB\n" );
                        exit( 1 );
                    }

                    if ( FloatUtils::gt( _upperBounds[i].getBound(), _preprocessedUpperBounds[i].getBound() ) )
                    {
                        printf( "Error with an increasing UBs\n" );
                        exit( 1 );
                    }

                    if ( FloatUtils::gt( _lowerBounds[i].getBound(), _upperBounds[i].getBound() ) )
                    {
                        printf( "Error! LB > UB\n" );
                        exit( 1 );
                    }
                });

            backupLowerBounds[i] = _lowerBounds[i].getBound();
            backupUpperBounds[i] = _upperBounds[i].getBound();
            backupLowerBoundLevels[i] = _lowerBounds[i].getLevel();
            backupUpperBoundLevels[i] = _upperBounds[i].getLevel();

            _lowerBounds[i] = _preprocessedLowerBounds[i];
            _upperBounds[i] = _preprocessedUpperBounds[i];
            _lowerBounds[i].setLevel( 0 );
            _upperBounds[i].setLevel( 0 );
        }

        Map<unsigned, ReluDissolutionType> backupDissolved = _dissolvedReluVariables;

        // First restore the original tableau and the state of relus
        _preprocessedTableau.backupIntoMatrix( &_tableau );
        _dissolvedReluVariables = _preprocessedDissolvedRelus;
        memcpy( _assignment, _preprocessedAssignment, sizeof(double) * _numVariables );
        _basicVariables = _preprocessedBasicVariables;
        computeVariableStatus();

        // Since we restored the original tableau, all invariants should hold.
        checkInvariants();

        // Next, go over the current bounds and assert them, one by one
        for ( unsigned i = 0; i < _numVariables; ++i )
        {
            double newLb = backupLowerBounds[i];
            double newUb = backupUpperBounds[i];

            // If the variable is not an active relu (in the original tableau), just update it.
            if ( !activeReluVariable( i ) )
            {
                if ( ( !_lowerBounds[i].finite() ) || FloatUtils::gt( newLb, _lowerBounds[i].getBound() ) )
                    updateLowerBound( i, newLb, backupLowerBoundLevels[i] );

                if ( ( !_upperBounds[i].finite() ) || FloatUtils::lt( newUb, _upperBounds[i].getBound() ) )
                    updateUpperBound( i, newUb, backupUpperBoundLevels[i] );

                continue;
            }

            // Dealing with active relu variables. Only handle F's.
            if ( !_reluPairs.isF( i ) )
            {
                continue;
            }

            unsigned f = i;
            unsigned b = _reluPairs.toPartner( i );

            double bLower = backupLowerBounds[b];
            double bUpper = backupUpperBounds[b];

            // Now, it matters whether F and B are still active in the tableau we're creating.
            if ( !backupDissolved.exists( f ) )
            {
                // Stil Active. Upper bounds should match and be equal, so can just update one of them and cascade.
                if ( ( !_upperBounds[i].finite() ) || FloatUtils::lt( newUb, _upperBounds[i].getBound() ) )
                    updateUpperBound( f, newUb, backupUpperBoundLevels[f] );

                // The lower bound for F was not updated, or the pair would be merged. Hence, just update B.
                // The update function knows how to handle this propertly.
                if ( ( !_lowerBounds[b].finite() ) || FloatUtils::gt( bLower, _lowerBounds[b].getBound() ) )
                    updateLowerBound( b, bLower, backupLowerBoundLevels[b] );
            }

            else if ( backupDissolved[f] == TYPE_SPLIT )
            {
                // Not active, due to a split. B's upper bound must be non-positive.
                // Update, and this will cause a split.
                if ( ( !_upperBounds[b].finite() ) || FloatUtils::lt( bUpper, _upperBounds[b].getBound() ) )
                    updateUpperBound( b, bUpper, backupUpperBoundLevels[b] );

                // Maybe the pair was broken at an earlier update, so fix F's levels individually.
                _upperBounds[f].setLevel( backupUpperBoundLevels[f] );

                // Also, normally update b's lower bound
                if ( ( !_lowerBounds[b].finite() ) || FloatUtils::gt( bLower, _lowerBounds[b].getBound() ) )
                    updateLowerBound( b, bLower, backupLowerBoundLevels[b] );
            }

            else
            {
                // Not active, due to a merge. B's lower bound must be non-negative.
                // Update, and this will cause the merge.
                if ( ( !_lowerBounds[b].finite() ) || FloatUtils::gt( bLower, _lowerBounds[b].getBound() ) )
                    updateLowerBound( b, bLower, backupLowerBoundLevels[b] );

                // Now that this is done, update both bounds for F - maybe they were tightend later.
                // (I.e., maybe the new lower bound for f is tighter than the one for b).
                if ( ( !_lowerBounds[f].finite() ) || FloatUtils::gt( newLb, _lowerBounds[f].getBound() ) )
                    updateLowerBound( f, newLb, backupLowerBoundLevels[f] );

                if ( ( !_upperBounds[f].finite() ) || FloatUtils::lt( newUb, _upperBounds[f].getBound() ) )
                    updateUpperBound( f, newUb, backupUpperBoundLevels[f] );
            }
        }

        DEBUG({
                if ( backupDissolved != _dissolvedReluVariables )
                {
                    printf( "Error - didnt get the same set of dissolved relus\n" );
                    exit( 1 );
                }

                checkInvariants();

                // The current bounds should be the same as before, except for merged B's.
                for ( unsigned i = 0; i < _numVariables; ++i )
                {
                    // Ignore merged B's
                    bool isB = _reluPairs.isB( i );
                    if ( isB )
                    {
                        unsigned f = _reluPairs.toPartner( i );
                        if ( _dissolvedReluVariables.exists( f ) &&
                             _dissolvedReluVariables[f] == TYPE_MERGE )
                        {
                            continue;
                        }
                    }

                    // Otherwise, bounds need to be equal.
                    if ( FloatUtils::areDisequal( _lowerBounds[i].getBound(), backupLowerBounds[i] ) )
                    {
                        // This is only allowed if we have a relu pair in which b is eliminated;
                        // its bound may be different than expected

                        if ( _reluPairs.isRelu( i ) )
                        {
                            unsigned f = _reluPairs.isF( i ) ? i : _reluPairs.toPartner( i );
                            unsigned b = _reluPairs.toPartner( f );

                            if ( !( ( i == b ) && ( _tableau.getColumnSize( b ) == 0 ) ) )
                            {
                                printf( "Error in lower bounds for var %u. %lf != %lf\n",
                                        i, _lowerBounds[i].getBound(), backupLowerBounds[i] );

                                printf( "Checking. b = %u, f = %u. Dissolved? %s.\n",
                                        b, f, _dissolvedReluVariables.exists( f ) ? "YES" : "NO" );
                                printf( "B's column size: %u. F's column size: %u\n",
                                        _tableau.getColumnSize( b ), _tableau.getColumnSize( f ) );

                                printf( "Original bounds for b = %u: lower = %.15lf, upper = %.15lf",
                                        b, backupLowerBounds[b], backupUpperBounds[b] );
                                printf( "Original bounds for f = %u: lower = %.15lf, upper = %.15lf",
                                        f, backupLowerBounds[f], backupUpperBounds[f] );

                                printf( "And, bounds after the update:\n" );
                                printf( "\tb = %u: lower = %.15lf, upper = %.15lf",
                                        b, _lowerBounds[b].getBound(), _upperBounds[b].getBound() );
                                printf( "\tf = %u: lower = %.15lf, upper = %.15lf",
                                        f, _lowerBounds[f].getBound(), _upperBounds[f].getBound() );

                                printf( "Not the case of an eliminated b variable!\n" );
                                exit( 1 );
                            }
                        }
                        else
                        {
                            printf( "Error in lower bounds for var %u. %lf != %lf\n", i, _lowerBounds[i].getBound(), backupLowerBounds[i] );
                            printf( "Not relu!\n" );
                            exit( 1 );
                        }
                    }

                    if ( FloatUtils::areDisequal( _upperBounds[i].getBound(), backupUpperBounds[i] ) )
                    {
                        if ( _reluPairs.isRelu( i ) )
                        {
                            unsigned f = _reluPairs.isF( i ) ? i : _reluPairs.toPartner( i );
                            unsigned b = _reluPairs.toPartner( f );

                            if ( !( ( i == b ) && ( _tableau.getColumnSize( b ) == 0 ) ) )
                            {
                                printf( "Error in upper bounds for var %u. %.15lf != %.15lf\n",
                                        i, _upperBounds[i].getBound(), backupUpperBounds[i] );

                                printf( "Checking. b = %u, f = %u. Dissolved? %s.\n",
                                        b, f, _dissolvedReluVariables.exists( f ) ? "YES" : "NO" );
                                printf( "B's column size: %u. F's column size: %u\n",
                                        _tableau.getColumnSize( b ), _tableau.getColumnSize( f ) );

                                printf( "Original bounds for b = %u: lower = %.15lf, upper = %.15lf\n",
                                        b, backupLowerBounds[b], backupUpperBounds[b] );
                                printf( "Original bounds for f = %u: lower = %.15lf, upper = %.15lf\n",
                                        f, backupLowerBounds[f], backupUpperBounds[f] );

                                printf( "And, bounds after the update:\n" );
                                printf( "\tb = %u: lower = %.15lf, upper = %.15lf\n",
                                        b, _lowerBounds[b].getBound(), _upperBounds[b].getBound() );
                                printf( "\tf = %u: lower = %.15lf, upper = %.15lf\n",
                                        f, _lowerBounds[f].getBound(), _upperBounds[f].getBound() );

                                printf( "Not the case of an eliminated b variable!\n" );
                                exit( 1 );
                            }
                        }
                        else
                        {
                            printf( "Error in upper bounds for var %u. %.15lf != %.15lf\n",
                                    i, _upperBounds[i].getBound(), backupUpperBounds[i] );
                            printf( "Not relu!\n" );
                            exit( 1 );
                        }
                    }

                    // Levels should also be equal
                    if ( _lowerBounds[i].getLevel() != backupLowerBoundLevels[i] )
                    {
                        printf( "Error restoring lower bound for variable %s. "
                                "Expected: %u, got: %u\n", toName( i ).ascii(),
                                backupLowerBoundLevels[i], _lowerBounds[i].getLevel() );
                        exit( 1 );
                    }

                    if ( _upperBounds[i].getLevel() != backupUpperBoundLevels[i] )
                    {
                        printf( "Error restoring upper bound for variable %s. "
                                "Expected: %u, got: %u\n", toName( i ).ascii(),
                                backupLowerBoundLevels[i], _lowerBounds[i].getLevel() );
                        exit( 1 );
                    }
                }
            });

        // The tableau now has the proper bounds and proper eliminated variables.
        // All that remains is to pivot to the correct basis.
        if ( keepCurrentBasicVariables )
        {
            printf( "\t\t\tRestoring basics\n" );
            Set<unsigned> shouldBeBasic = Set<unsigned>::difference( backupBasicVariables, _basicVariables );
            Set<unsigned> shouldntBeBasic = Set<unsigned>::difference( _basicVariables, backupBasicVariables );

            adjustBasicVariables( shouldBeBasic, shouldntBeBasic );
        }
        else
        {
            printf( "\t\t\tNot restoring basics\n" );
        }

        DEBUG( checkInvariants(); );

        delete[] backupUpperBoundLevels;
        delete[] backupLowerBoundLevels;
        delete[] backupUpperBounds;
        delete[] backupLowerBounds;

        timeval end = Time::sampleMicro();
        _totalRestorationTimeMilli += Time::timePassed( start, end );

        printf( "\n\n\t\t !!! Restore tableau from backup DONE !!!\n" );
    }

    void adjustBasicVariables( const Set<unsigned> &shouldBeBasic, Set<unsigned> shouldntBeBasic, bool adjustAssignment = true )
    {
        unsigned count = 0;
        for ( const auto &entering : shouldBeBasic )
        {
            const Tableau::Entry *columnEntry = _tableau.getColumn( entering );
            const Tableau::Entry *current;

            bool done = false;
            while ( !done && columnEntry != NULL )
            {
                current = columnEntry;
                columnEntry = columnEntry->nextInColumn();

                unsigned leaving = current->getRow();
                if ( shouldntBeBasic.exists( leaving ) )
                {
                    double weight = FloatUtils::abs( getCell( leaving, entering ) );
                    if ( FloatUtils::lt( weight, NUMBERICAL_INSTABILITY_CONSTANT ) )
                    {
                        log( Stringf( "adjustBasicVariables: skipping a bad pivot: %.10lf\n",
                                      getCell( leaving, entering ) ) );
                        continue;
                    }

                    ++count;

                    done = true;
                    shouldntBeBasic.erase( leaving );

                    pivot( entering, leaving );
                    computeVariableStatus( leaving );

                    if ( adjustAssignment )
                    {
                        // leaving is now non-basic, so the invariants needs to be enforced.
                        if ( tooLow( leaving ) )
                            update( leaving, _lowerBounds[leaving].getBound() - _assignment[leaving], true );
                        else if ( tooHigh( leaving ) )
                            update( leaving, _upperBounds[leaving].getBound() - _assignment[leaving], true );

                        // If there's a Relu problem, fix leaving
                        if ( _reluPairs.isRelu( leaving ) )
                        {
                            unsigned b = _reluPairs.isB( leaving ) ? leaving : _reluPairs.toPartner( leaving );
                            unsigned f = _reluPairs.toPartner( b );

                            if ( ( !_dissolvedReluVariables.exists( f ) ) && reluPairIsBroken( b, f ) )
                            {
                                if ( ( !_basicVariables.exists( b ) ) && ( !_basicVariables.exists( f ) ) )
                                {
                                    if ( FloatUtils::isPositive( b ) )
                                        update( f, _assignment[b] - _assignment[f], true );
                                    else
                                        update( f, -_assignment[f], true );
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    void fixAllBrokenRelus()
    {
        for ( auto &pair : _reluPairs.getPairs() )
        {
            unsigned b = pair.getB();
            unsigned f = pair.getF();

            if ( ( !_dissolvedReluVariables.exists( f ) ) && reluPairIsBroken( b, f ) )
            {
                if ( ( !_basicVariables.exists( b ) ) && ( !_basicVariables.exists( f ) ) )
                {
                    if ( FloatUtils::isPositive( b ) )
                        update( f, _assignment[b] - _assignment[f], true );
                    else
                        update( f, -_assignment[f], true );
                }
            }
        }
    }

    UseSlackVariables useSlackVariablesForRelus() const
    {
        return _useSlackVariablesForRelus;
    }

    Set<unsigned> getActiveRowSlacks() const
    {
        return _activeSlackRowVars;
    }

    Set<unsigned> getActiveColSlacks() const
    {
        return _activeSlackColVars;
    }

    double getSlackLowerBound( unsigned variable ) const
    {
        DEBUG({
                if ( !_slackToLowerBound.exists( variable ) )
                {
                    if ( ( ( _useSlackVariablesForRelus == USE_ROW_SLACK_VARIABLES ) &&
                           !_activeSlackRowVars.exists( variable ) )
                         ||
                         ( ( _useSlackVariablesForRelus == USE_ROW_AND_COL_SLACK_VARIABLES ) &&
                           !_activeSlackColVars.exists( variable ) ) )
                    {
                        printf( "Error! requested a slack lower bound on a non-slack variable (%u)!\n",
                                variable );

                        exit( 1 );
                    }
                }
            });

        return _slackToLowerBound.at( variable ).getBound();
    }

    double getSlackUpperBound( unsigned variable ) const
    {
        DEBUG({
                if ( !_slackToUpperBound.exists( variable ) )
                {
                    if ( ( ( _useSlackVariablesForRelus == USE_ROW_SLACK_VARIABLES ) &&
                           !_activeSlackRowVars.exists( variable ) )
                         ||
                         ( ( _useSlackVariablesForRelus == USE_ROW_AND_COL_SLACK_VARIABLES ) &&
                           !_activeSlackColVars.exists( variable ) ) )
                    {
                        printf( "Error! requested a slack upper bound on a non-slack variable (%u)!\n",
                                variable );
                        exit( 1 );
                    }
                }
            });

        return _slackToUpperBound.at( variable ).getBound();
    }

    unsigned slackToB( unsigned slack ) const
    {
        return _slackRowVariableToB.at( slack );
    }

    unsigned slackToF( unsigned slack ) const
    {
        return _slackRowVariableToF.at( slack );
    }

    int fixRelusInGlpkAssignment( int n, int m, int nonBasicEncoding, const int *head, const char *flags )
    {
        ++_fixRelusInGlpkAssignmentInvoked;

        unsigned nonBasic = _currentGlpkWrapper->glpkEncodingToVariable( nonBasicEncoding );
        if ( !activeReluVariable( nonBasic ) )
            return 0;

        // Check if partner is non-basic.
        unsigned partner = _reluPairs.toPartner( nonBasic );
        unsigned partnerEncoding = _currentGlpkWrapper->variableToGlpkEncoding( partner );

        bool partnerIsNonBasic = false;
        int partnerIndex = 1;
        for ( ; partnerIndex <= n-m; ++partnerIndex )
        {
            if ( head[m + partnerIndex] == (int)partnerEncoding )
            {
                partnerIsNonBasic = true;
                break;
            }
        }

        if ( !partnerIsNonBasic )
            return 0;

        char currentBound = flags[nonBasicEncoding];
        char partnerBound = flags[partnerEncoding];

        if ( currentBound != partnerBound )
        {
            // The partner needs to have its bound flipped
            if ( !_reluUpdateFrequency.exists( partner ) )
                _reluUpdateFrequency[partner] = 0;
            ++_reluUpdateFrequency[partner];
            if ( _reluUpdateFrequency[partner] > 5 )
            {
                ++_fixRelusInGlpkAssignmentIgnore;
                return 0;
            }

            ++_fixRelusInGlpkAssignmentFixes;
            return partnerIndex;
        }

        return 0;
    }

    void conflictAnalysisCausedPop()
    {
        ++_conflictAnalysisCausedPop;
    }

    void quit()
    {
        _quit = true;
    }

    void addTimeEvalutingGlpkRows( unsigned time )
    {
        _totalTimeEvalutingGlpkRows += time;
    }

private:
    unsigned _numVariables;
    String _reluplexName;
    char *_finalOutputFile;
    FinalStatus _finalStatus;
    bool _wasInitialized;
    Tableau _tableau;
    Tableau _preprocessedTableau;
    VariableBound *_upperBounds;
    VariableBound *_lowerBounds;
    VariableBound *_preprocessedUpperBounds;
    VariableBound *_preprocessedLowerBounds;
    double *_assignment;
    double *_preprocessedAssignment;
    Set<unsigned> _basicVariables;
    Set<unsigned> _preprocessedBasicVariables;
    Map<unsigned, String> _variableNames;
    ReluPairs _reluPairs;
    SmtCore _smtCore;
    bool _useApproximations;
    bool _findAllPivotCandidates;
    unsigned _conflictAnalysisCausedPop;

    GlpkWrapper::GlpkAnswer _previousGlpkAnswer;

    bool _logging;
    bool _dumpStates;

    unsigned _numCallsToProgress;
    unsigned _numPivots;
    unsigned long long _totalPivotTimeMilli;
    unsigned long long _totalDegradationCheckingTimeMilli;
    unsigned long long _totalRestorationTimeMilli;
    unsigned long long _totalPivotCalculationCount;
    unsigned long long _totalNumBrokenRelues;
    unsigned _brokenRelusFixed;
    unsigned _brokenReluFixByUpdate;
    unsigned _brokenReluFixByPivot;
    unsigned _brokenReluFixB;
    unsigned _brokenReluFixF;
    unsigned _numEliminatedVars;
    unsigned _varsWithInfiniteBounds;
    unsigned _numStackSplits;
    unsigned _numStackMerges;
    unsigned _numStackPops;
    unsigned _numStackVisitedStates;
    unsigned _currentStackDepth;
    unsigned _minStackSecondPhase;
    unsigned _maximalStackDepth;
    unsigned long long _boundsTightendByTightenAllBounds;

    unsigned _almostBrokenReluPairCount;
    unsigned _almostBrokenReluPairFixedCount;

    unsigned _numBoundsDerivedThroughGlpk;
    unsigned _numBoundsDerivedThroughGlpkOnSlacks;
    unsigned long long _totalTightenAllBoundsTime;

    bool _eliminateAlmostBrokenRelus;

    Map<unsigned, VariableStatus> _varToStatus;

    Map<unsigned, ReluDissolutionType> _dissolvedReluVariables;
    Map<unsigned, ReluDissolutionType> _preprocessedDissolvedRelus;

    bool _printAssignment;
    Set<unsigned> _eliminatedVars;

    unsigned _numOutOfBoundFixes;
    unsigned _numOutOfBoundFixesViaBland;

    bool _useDegradationChecking;
    unsigned _numLpSolverInvocations;
    unsigned _numLpSolverFoundSolution;
    unsigned _numLpSolverNoSolution;
    unsigned _numLpSolverFailed;
    unsigned _numLpSolverIncorrectAssignment;
    unsigned long long _totalLpSolverTimeMilli;
    unsigned long long _totalLpExtractionTime;
    unsigned _totalLpPivots;
    unsigned _maxLpSolverTimeMilli;

    unsigned _numberOfRestorations;
    double _maxDegradation;

    unsigned long long _totalProgressTimeMilli;
    unsigned long long _timeTighteningGlpkBoundsMilli;

    GlpkWrapper *_currentGlpkWrapper;

    unsigned _relusDissolvedByGlpkBounds;

    Map<unsigned, VariableBound> _glpkStoredUpperBounds;
    Map<unsigned, VariableBound> _glpkStoredLowerBounds;

    double _glpkSoi;

    unsigned long long _storeGlpkBoundTighteningCalls;
    unsigned long long _storeGlpkBoundTighteningCallsOnSlacks;
    unsigned long long _storeGlpkBoundTighteningIgnored;

    unsigned _maxBrokenReluAfterGlpk;
    unsigned _totalBrokenReluAfterGlpk;
    unsigned _totalBrokenNonBasicReluAfterGlpk;

    UseSlackVariables _useSlackVariablesForRelus;
    Set<unsigned> _activeSlackRowVars;
    Set<unsigned> _activeSlackColVars;

    Map<unsigned, unsigned> _fToSlackRowVar;
    Map<unsigned, unsigned> _fToSlackColVar;

    Map<unsigned, unsigned> _slackRowVariableToF;
    Map<unsigned, unsigned> _slackRowVariableToB;
    Map<unsigned, VariableBound> _slackToLowerBound;
    Map<unsigned, VariableBound> _slackToUpperBound;

    Map<unsigned, unsigned> _reluUpdateFrequency;

    unsigned long long _fixRelusInGlpkAssignmentFixes;
    unsigned long long _fixRelusInGlpkAssignmentInvoked;
    unsigned long long _fixRelusInGlpkAssignmentIgnore;

    bool _maximalGlpkBoundTightening;
    bool _useConflictAnalysis;
    bool _temporarilyDontUseSlacks;

    bool _quit;
    bool _fullTightenAllBounds;
    bool _glpkExtractJustBasics;

    unsigned long long _totalTimeEvalutingGlpkRows;
    unsigned _consecutiveGlpkFailureCount;

public:
    void checkInvariants() const
    {
#ifndef DEBUG_ON
        return;
#endif

        // Table is in tableau form
        for ( const auto &basic : _basicVariables )
        {
            if ( !_tableau.activeColumn( basic ) )
            {
                printf( "Error: basic variable's column should be active! (var: %s)\n",
                        toName( basic ).ascii() );
                exit( 1 );
            }

            const Tableau::Entry *columnEntry = _tableau.getColumn( basic );

            if ( ( _tableau.getColumnSize( basic ) != 1 ) ||
                 ( columnEntry->getRow() != basic ) ||
                 ( FloatUtils::areDisequal( columnEntry->getValue(), -1.0 ) ) )
            {
                printf( "Error: basic variable's column isn't right! (var: %s)\n",
                        toName( basic ).ascii() );
                printf( "Column size = %u\n", _tableau.getColumnSize( basic ) );

                exit( 1 );
            }

            const Tableau::Entry *rowEntry = _tableau.getRow( basic );
            const Tableau::Entry *current;

            while ( rowEntry != NULL )
            {
                current = rowEntry;
                rowEntry = rowEntry->nextInRow();

                if ( current->getColumn() == basic )
                    continue;

                if ( _basicVariables.exists( current->getColumn() ) )
                {
                    printf( "Error: a basic variable appears in another basic variable's row\n" );
                    exit( 1 );
                }
            }
        }

        // All non-basic are within bounds
        for ( unsigned i = 0; i < _numVariables; ++i )
        {
            if ( _varToStatus.get( i ) == ABOVE_UB || _varToStatus.get( i ) == BELOW_LB )
            {
                // Only basic variables can be out-of-bounds
                if ( !_basicVariables.exists( i ) )
                {
                    printf( "Error: variable is out-of-bounds but is not basic! "
                            "(var: %s, value = %.10lf, range = [%.10lf, %.10lf])\n",
                            toName( i ).ascii(),
                            _assignment[i],
                            _lowerBounds[i].getBound(),
                            _upperBounds[i].getBound() );
                    if ( _reluPairs.isRelu( i ) )
                    {
                        unsigned partner = _reluPairs.toPartner( i );

                        printf( "This is also a relu variable (%s)\n", _reluPairs.isF( i ) ? "F" : "B" );
                        printf( "Relu partner is: %s. value = %.10lf, range = [%.10lf, %.10lf])\n",
                                toName( partner ).ascii(),
                                _assignment[partner],
                                _lowerBounds[partner].getBound(),
                                _upperBounds[partner].getBound() );
                    }

                    exit( 1 );
                }
            }
        }

        // All relus that were split or merged have correct bounds and tableau shape
        for ( const auto &dissolved : _dissolvedReluVariables )
        {
            unsigned f = dissolved.first;
            unsigned b = _reluPairs.toPartner( f );

            double bUpper = _upperBounds[b].getBound();

            double fLower = _lowerBounds[f].getBound();
            double fUpper = _upperBounds[f].getBound();

            if ( dissolved.second == TYPE_SPLIT )
            {
                // F should be fixed, B's upper bound should be non-positive
                if ( !FloatUtils::isZero( fUpper ) || !FloatUtils::isZero( fLower ) )
                {
                    printf( "Error! After a split, F is not fixed at zero.\n"
                            "f = %u. Lower = %.15lf. Upper = %.15lf\n",
                            f, fLower, fUpper );
                    exit( 1 );
                }

                if ( FloatUtils::isPositive( bUpper ) )
                {
                    printf( "Error! After a split, B's upper bound is positive.\n"
                            "b = %u. Upper = %.15lf\n",
                            b, bUpper );
                    exit( 1 );
                }
            }
            else
            {
                // B should be eliminated, F should (naturally) have a non-negative lower bound
                if ( _tableau.getColumnSize( b ) != 0 )
                {
                    printf( "Error! After a merge, b's column is not of size 0! b = %u\n", b );
                    exit( 1 );
                }

                if ( FloatUtils::isNegative( fLower ) )
                {
                    printf( "Error! After a merge, F's lower bound is negative.\n"
                            "f = %u. Lower = %.15lf\n",
                            f, fLower );
                    exit( 1 );
                }
            }
        }
    }

    void printColumn( unsigned index )
    {
        printf( "\n\nDumping column for %s:\n", toName( index ).ascii() );

        const Tableau::Entry *columnEntry = _tableau.getColumn( index );
        while ( columnEntry != NULL )
        {
            printf( "\t<%u, %.5lf>\n", columnEntry->getRow(), columnEntry->getValue() );
            columnEntry = columnEntry->nextInColumn();
        }
    }

    static String statusToString( VariableStatus status )
    {
        switch ( status )
        {
        case ABOVE_UB: return "Above UB";
        case AT_UB: return "At UB";
        case BETWEEN: return "Between";
        case FIXED: return "Fixed";
        case AT_LB: return "At LB";
        case BELOW_LB: return "Below LB";
        }

        return "Unknown";
    }

    void tightenAllBounds()
    {
        log( "tightenAllBounds -- Starting\n" );

        timeval start = Time::sampleMicro();

        unsigned numLearnedBounds = 0;

        if ( !_fullTightenAllBounds )
        {
            Set<unsigned> copyOfBasics = _basicVariables;
            for ( const auto &basic : copyOfBasics )
            {
                if ( !_basicVariables.exists( basic ) )
                    continue;

                tightenBoundsOnRow( basic, numLearnedBounds );
            }
        }
        else
        {
            bool done = false;
            while ( !done )
            {
                bool needToRestart = false;
                Set<unsigned>::iterator basic = _basicVariables.begin();

                while ( ( basic != _basicVariables.end() ) && !needToRestart )
                {
                    needToRestart = tightenBoundsOnRow( *basic, numLearnedBounds );
                    if ( !needToRestart )
                    {
                        ++basic;
                    }
                }

                if ( !needToRestart )
                    done = true;
            }
        }

        timeval end = Time::sampleMicro();
        _totalTightenAllBoundsTime += Time::timePassed( start, end );

        _boundsTightendByTightenAllBounds += numLearnedBounds;

        log( Stringf( "tightenAllBounds -- Done. Number of learned bounds: %u\n", numLearnedBounds ) );
    }

    bool tightenBoundsOnRow( unsigned basic, unsigned &numLearnedBounds )
    {
        const Tableau::Entry *row = _tableau.getRow( basic );
        const Tableau::Entry *tighteningVar;

        while ( row != NULL )
        {
            tighteningVar = row;

            row = row->nextInRow();

            double scale = -1.0 / tighteningVar->getValue();

            const Tableau::Entry *otherEntry = _tableau.getRow( basic );
            const Tableau::Entry *current;
            unsigned column;

            double max = 0.0;
            double min = 0.0;
            unsigned minBoundLevel = 0;
            unsigned maxBoundLevel = 0;

            while ( otherEntry != NULL )
            {
                current = otherEntry;
                otherEntry = otherEntry->nextInRow();

                column = current->getColumn();
                if ( column == tighteningVar->getColumn() )
                    continue;

                double coefficient = current->getValue() * scale;
                if ( FloatUtils::isPositive( coefficient ) )
                {
                    // Positive coefficient
                    min += _lowerBounds[column].getBound() * coefficient;
                    max += _upperBounds[column].getBound() * coefficient;

                    if ( _lowerBounds[column].getLevel() > minBoundLevel )
                        minBoundLevel = _lowerBounds[column].getLevel();
                    if ( _upperBounds[column].getLevel() > maxBoundLevel )
                        maxBoundLevel = _upperBounds[column].getLevel();
                }
                else
                {
                    // Negative coefficient
                    min += _upperBounds[column].getBound() * coefficient;
                    max += _lowerBounds[column].getBound() * coefficient;

                    if ( _lowerBounds[column].getLevel() > maxBoundLevel )
                        maxBoundLevel = _lowerBounds[column].getLevel();
                    if ( _upperBounds[column].getLevel() > minBoundLevel )
                        minBoundLevel = _upperBounds[column].getLevel();
                }
            }

            unsigned currentVar = tighteningVar->getColumn();

            if ( FloatUtils::lt( max, _upperBounds[currentVar].getBound() ) )
            {
                // Found an UB
                ++numLearnedBounds;
                updateUpperBound( currentVar, max, maxBoundLevel );
            }

            if ( FloatUtils::gt( min, _lowerBounds[currentVar].getBound() ) )
            {
                // Found a LB
                ++numLearnedBounds;
                // Tableau changed, need to restart
                if ( updateLowerBound( currentVar, min, minBoundLevel ) )
                    return true;
            }
        }

        // Don't need to restart
        return false;
    }

    void adjustGlpkAssignment( Map<unsigned, double> &assignment )
    {
        for ( auto &pair : assignment )
        {
            unsigned var = pair.first;
            double value = pair.second;

            if ( _basicVariables.exists( pair.first ) )
                continue;

            // Adjust variables to their bounds according to our precision
            if ( FloatUtils::gt( _lowerBounds[var].getBound(), value ) )
            {
                printf( "Adjust to lower bound. Var %u: value = %lf, bound = %lf\n",
                        var, value, _lowerBounds[var].getBound() );
                pair.second = _lowerBounds[var].getBound();
            }

            if ( FloatUtils::lt( _upperBounds[var].getBound(), value ) )
            {
                printf( "Adjust to upper bound. Var %u: value = %lf, bound = %lf\n",
                        var, value, _upperBounds[var].getBound() );
                pair.second = _upperBounds[var].getBound();
            }

            if ( ( pair.second != 0.0 ) && FloatUtils::isZero( pair.second ) )
            {
                pair.second = 0.0;
            }
        }
    }

    bool checkEquationsHold( unsigned basic, Map<unsigned, double> &assignment )
    {
        double result = 0.0;

        const Tableau::Entry *rowEntry = _tableau.getRow( basic );
        const Tableau::Entry *current;

        while ( rowEntry != NULL )
        {
            current = rowEntry;
            rowEntry = rowEntry->nextInRow();

            unsigned column = current->getColumn();

            if ( column != basic )
                result += assignment[column] * current->getValue();
            else
                if ( FloatUtils::areDisequal( current->getValue(), -1.0 ) )
                {
                    printf( "Error! Basic's coefficient is not -1. It is: %lf\n", current->getValue() );
                    exit( 2 );
                }
        }

        if ( !FloatUtils::areEqual( assignment[basic], result, GLPK_IMPRECISION_TOLERANCE ) )
        {
            printf( "Error! Mismatch between glpk answer and calculation for basic var = %s. "
                    "Calculated: %.10lf. Glpk: %.10lf.\n",
                    toName( basic ).ascii(), result, assignment[basic] );
            return false;
        }

        if ( FloatUtils::isZero( result ) )
            result = 0.0;

        assignment[basic] = result;
        return true;
    }

    void calculateBasicVariableValues()
    {
        for ( unsigned basic : _basicVariables )
            calculateBasicVariableValue( basic );
    }

    void calculateBasicVariableValue( unsigned basic )
    {
        double result = 0.0;

        const Tableau::Entry *rowEntry = _tableau.getRow( basic );
        const Tableau::Entry *current;

        while ( rowEntry != NULL )
        {
            current = rowEntry;
            rowEntry = rowEntry->nextInRow();

            unsigned column = current->getColumn();

            if ( column != basic )
                result += _assignment[column] * current->getValue();
        }

        if ( FloatUtils::isZero( result ) )
            result = 0.0;

        _assignment[basic] = result;
        computeVariableStatus( basic );
    }

    double checkDegradation()
    {
        timeval start = Time::sampleMicro();

        double max = 0.0;
        for ( unsigned basic : _preprocessedBasicVariables )
        {
            double degradation = checkDegradation( basic );
            if ( FloatUtils::gt( degradation, max ) )
                max = degradation;
        }

        if ( max > _maxDegradation )
            _maxDegradation = max;

        timeval end = Time::sampleMicro();
        _totalDegradationCheckingTimeMilli += Time::timePassed( start, end );

        return max;
    }

    double checkDegradation( unsigned variable )
    {
        double result = 0.0;

        const Tableau::Entry *rowEntry = _preprocessedTableau.getRow( variable );
        const Tableau::Entry *current;

        while ( rowEntry != NULL )
        {
            current = rowEntry;
            rowEntry = rowEntry->nextInRow();

            unsigned column = current->getColumn();

            if ( column != variable )
            {
                unsigned adjustedColumn = column;

                // If column is the b variable of a merged pair, take f instead
                if ( _reluPairs.isRelu( column ) && _reluPairs.isB( column ) )
                {
                    if ( _tableau.getColumnSize( column ) == 0 )
                        adjustedColumn = _reluPairs.toPartner( column );
                }

                result += _assignment[adjustedColumn] * current->getValue();
            }
        }

        unsigned adjustedVariable = variable;
        if ( _reluPairs.isRelu( variable ) && _reluPairs.isB( variable ) )
        {
            if ( _tableau.getColumnSize( variable ) == 0 )
                adjustedVariable = _reluPairs.toPartner( variable );
        }

        return FloatUtils::abs( result - _assignment[adjustedVariable] );
    }
};

void boundCalculationHook( int n, int m, int *head, int leavingBasic, int enteringNonBasic, double *basicRow )
{
    activeReluplex->storeGlpkBoundTightening( n, m, head, leavingBasic, enteringNonBasic, basicRow );
}

void iterationCountCallback( int count )
{
    activeReluplex->glpkIterationCountCallback( count );
}

void reportSoiCallback( double soi )
{
    activeReluplex->glpkReportSoi( soi );
}

int makeReluAdjustmentsCallback( int n, int m, int nonBasicEncoding, const int *head, const char *flags )
{
    return activeReluplex->fixRelusInGlpkAssignment( n, m, nonBasicEncoding, head, flags );
}

#endif // __Reluplex_h__

//
// Local Variables:
// compile-command: "make -C . "
// tags-file-name: "./TAGS"
// c-basic-offset: 4
// End:
//
