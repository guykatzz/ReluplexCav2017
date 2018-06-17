/*********************                                                        */
/*! \file SmtCore.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** (in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __SmtCore_h__
#define __SmtCore_h__

#include "IReluplex.h"
#include "Stack.h"
#include "MStringf.h"
#include "Tableau.h"
#include "TimeUtils.h"
#include "VariableBound.h"

// The number of times a ReLU pair can be corrected before a split occurs.
static const unsigned NUM_RELU_OPERATIONS_BEFORE_SPLIT = 5;

class SmtCore
{
public:
    class SplitInformation
    {
    public:
        enum Type {
            SPLITTING_RELU = 0,
            MERGING_RELU = 1,
        };

        SplitInformation( unsigned numVariables ) : _tableau( numVariables )
        {
        }

        Type _type;

        unsigned _variable;

        bool _firstAttempt;

        List<VariableBound> _lowerBounds;
        List<VariableBound> _upperBounds;
        List<double> _assignment;
        Map<unsigned, IReluplex::ReluDissolutionType> _dissolvedReluPairs;
        Set<unsigned> _basicVariables;
        Tableau _tableau;
    };

    SmtCore( IReluplex *reluplex, unsigned numVariables )
        : _reluplex( reluplex )
        , _numVariables( numVariables )
        , _totalSmtCoreTimeMilli( 0 )
        , _logging( false )
    {
    }

    ~SmtCore()
    {
        while ( !_stack.empty() )
        {
            delete _stack.top();
            _stack.pop();
        }
    }

    unsigned long long getSmtCoreTime() const
    {
        return _totalSmtCoreTimeMilli;
    }

    void storeCurrentState( SplitInformation *splitInformation, unsigned variable )
    {
        splitInformation->_variable = variable;

        // Store current bounds
        const VariableBound *lowerBounds = _reluplex->getLowerBounds();
        const VariableBound *upperBounds = _reluplex->getUpperBounds();
        for ( unsigned i = 0; i < _numVariables; ++i )
        {
            splitInformation->_lowerBounds.append( lowerBounds[i] );
            splitInformation->_upperBounds.append( upperBounds[i] );
        }

        // Store current dissolved pairs
        splitInformation->_dissolvedReluPairs = _reluplex->getDissolvedReluPairs();

        // Store basic variables, assignment and reluplex
        splitInformation->_basicVariables = _reluplex->getBasicVariables();

        const double *assignment = _reluplex->getAssignment();
        for ( unsigned i = 0; i < _numVariables; ++i )
            splitInformation->_assignment.append( assignment[i] );
        _reluplex->backupIntoMatrix( &(splitInformation->_tableau) );
    }

    void restorePreviousState( SplitInformation *previousState )
    {
        // Undo bounds as a result of the pop.
        _reluplex->setLowerBounds( previousState->_lowerBounds );
        _reluplex->setUpperBounds( previousState->_upperBounds );

        // Restore dissolved relu pairs
        _reluplex->setDissolvedReluPairs( previousState->_dissolvedReluPairs );

        // Undo the reluplex, assignment and basic variables
        _reluplex->setBasicVariables( previousState->_basicVariables );
        _reluplex->setAssignment( previousState->_assignment );
        _reluplex->restoreFromMatrix( &previousState->_tableau );

        _reluplex->computeVariableStatus();
    }

    bool beginWithSplit( unsigned f )
    {
        // Return true for split, false for merge.
        // Decide according to current assignment (this is the F variable).

        double assignment = _reluplex->getAssignment()[f];

        if ( FloatUtils::isPositive( assignment ) )
        {
            log( "Starting with merge\n" );
            // If F is currently positive, we want a merge
            return false;
        }

        log( "Starting with split\n" );
        // F is zero, so we want a split.
        return true;
    }

    void dissolveReluOnVar( unsigned variable )
    {
        log( Stringf( "Resolving relu on var: %s. (current depth = %u)\n",
                      _reluplex->toName( variable ).ascii(), _stack.size() ) );

        log( Stringf( "Column size of %s when dissolving: %u\n",
                      _reluplex->toName( variable ).ascii(),
                      _reluplex->getColumnSize( variable ) ) );

        // Store the current state in splitInformation
        SplitInformation *splitInformation = new SplitInformation( _numVariables );

        storeCurrentState( splitInformation, variable );

        // Set the type for the first attempt
        splitInformation->_firstAttempt = true;

        if ( beginWithSplit( variable ) )
        {
            // Do a split
            splitInformation->_type = SplitInformation::SPLITTING_RELU;
            _reluplex->incNumSplits();
            _stack.push( splitInformation );

            // Adjust upper bounds
            _reluplex->updateUpperBound( variable, 0.0, _stack.size() );
        }
        else
        {
            // Do a merge
            splitInformation->_type = SmtCore::SplitInformation::MERGING_RELU;
            _reluplex->incNumMerges();

            _stack.push( splitInformation );

            // Adjust lower bounds
            _reluplex->updateLowerBound( variable, 0.0, _stack.size() );
        }

        _reluplex->incNumStackVisitedStates();
        _reluplex->setCurrentStackDepth( _stack.size() );
    }

    void pop( unsigned violatingStackLevel )
    {
        if ( violatingStackLevel == 0 )
        {
            // If level 0 is violating, nothing to do.
            throw Error( Error::STACK_IS_EMPTY, "Stack is empty" );
        }

        pop();
        while ( _stack.size() > violatingStackLevel )
        {
            _reluplex->conflictAnalysisCausedPop();
            pop();
        }
    }

    void pop()
    {
        timeval start = Time::sampleMicro();

        while ( true )
        {
            if ( _stack.empty() )
            {
                timeval end = Time::sampleMicro();
                _totalSmtCoreTimeMilli += Time::timePassed( start, end );
                throw Error( Error::STACK_IS_EMPTY, "Stack is empty" );
            }

            SmtCore::SplitInformation *oldState = _stack.top();
            _stack.pop();

            log( Stringf( "popping (variable = %s)\n", _reluplex->toName( oldState->_variable ).ascii() ) );

            restorePreviousState( oldState );

            if ( oldState->_firstAttempt )
            {
                oldState->_firstAttempt = false;

                if ( oldState->_type == SmtCore::SplitInformation::SPLITTING_RELU )
                {
                    // Earlier round was a split. Now comes the merge.

                    log( "Popped a split, now doing a merge\n" );
                    log( Stringf( "Column size of %s when doing the merge: %u\n",
                                  _reluplex->toName( oldState->_variable ).ascii(),
                                  _reluplex->getColumnSize( oldState->_variable ) ) );

                    oldState->_type = SmtCore::SplitInformation::MERGING_RELU;
                    _stack.push( oldState );

                    // Adjust lower bounds
                    _reluplex->updateLowerBound( oldState->_variable, 0.0, _stack.size() );
                    _reluplex->incNumMerges();
                    _reluplex->computeVariableStatus();
                }
                else
                {
                    // Earlier round was a merge. Now comes the split.
                    log( "Popped a merge, now doing a split\n" );

                    oldState->_type = SmtCore::SplitInformation::SPLITTING_RELU;
                    _stack.push( oldState );

                    // Adjust upper bounds
                    _reluplex->updateUpperBound( oldState->_variable, 0.0, _stack.size() );
                    _reluplex->incNumSplits();
                    _reluplex->computeVariableStatus();
                }

                _reluplex->incNumStackVisitedStates();
                _reluplex->setMinStackSecondPhase( _stack.size() );

                timeval end = Time::sampleMicro();
                _totalSmtCoreTimeMilli += Time::timePassed( start, end );

                return;
            }

            DEBUG(
                  _currentlyInStack.erase( oldState->_variable );
                  );

            log( Stringf( "\t\tAfter popping a MERGE, depth = %u\n", _stack.size() ) );

            delete oldState;
            oldState = NULL;

            _reluplex->incNumPops();
            _reluplex->setCurrentStackDepth( _stack.size() );
        }
    }

    bool notifyBrokenRelu( unsigned f )
    {
        timeval start = Time::sampleMicro();

        if ( !_fToViolations.exists( f ) )
            _fToViolations[f] = 0;

        ++_fToViolations[f];

        if ( _fToViolations[f] >= NUM_RELU_OPERATIONS_BEFORE_SPLIT )
        {
            DEBUG(
                  if ( _currentlyInStack.exists( f ) )
                  {
                      printf( "Error!! Splitting on the same var again (var = %u)\n", f );
                      exit( 1 );
                  }
                  _currentlyInStack.insert( f );
                  );

            dissolveReluOnVar( f );
            _fToViolations.clear();

            timeval end = Time::sampleMicro();
            _totalSmtCoreTimeMilli += Time::timePassed( start, end );

            return true;
        }

        timeval end = Time::sampleMicro();
        _totalSmtCoreTimeMilli += Time::timePassed( start, end );

        return false;
    }

private:
    Stack<SplitInformation *> _stack;
    IReluplex *_reluplex;
    unsigned _numVariables;
    Map<unsigned, unsigned> _fToViolations;
    unsigned long long _totalSmtCoreTimeMilli;
    bool _logging;

    DEBUG(
          Set<unsigned> _currentlyInStack;
          );

    void log( String message )
    {
        if ( !_logging )
            return;

        printf( "SMTCORE: %s", message.ascii() );
    }
};

#endif // __SmtCore_h__

//
// Local Variables:
// compile-command: "make -C . "
// tags-file-name: "./TAGS"
// c-basic-offset: 4
// End:
//
