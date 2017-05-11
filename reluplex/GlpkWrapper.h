/*********************                                                        */
/*! \file GlpkWrapper.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** (in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __GlpkWrapper_h__
#define __GlpkWrapper_h__

#include "IReluplex.h"
#include "List.h"
#include "Pair.h"
#include "Time.h"
#include "glpk.h"

class GlpkWrapper
{
public:
    enum GlpkAnswer {
        SOLVER_FAILED,
        SOLUTION_FOUND,
        NO_SOLUTION_EXISTS,
    };

    GlpkWrapper()
        : _nextGlpkInternalIndex( 1 )
        , _boundCalculationHook( NULL )
        , _iterationCountCallback( NULL )
        , _reportSoiCallback( NULL )
        , _makeReluAdjustmentsCallback( NULL )
        , _logging( false )
    {
        _lp = glp_create_prob();
        glp_set_prob_name( _lp, "reluplex" );
        glp_set_obj_dir( _lp, GLP_MIN );
    }

    ~GlpkWrapper()
    {
        glp_delete_prob( _lp );
    }

    void log( String message )
    {
        if ( !_logging )
            return;

        printf( "GlpkWrapper: %s", message.ascii() );
    }

    GlpkAnswer run( const IReluplex &reluplex )
    {
        log( "Starting\n" );

        addRows( reluplex );
        addColumns( reluplex );

        setObjectiveFunction( reluplex );
        addWeights( reluplex );

        GlpkAnswer answer = solve();

        log( "Done\n" );

        return answer;
    }

    void addRows( const IReluplex &reluplex )
    {
        Set<unsigned> basicVariables = reluplex.getBasicVariables();
        Set<unsigned> activeSlackRows = reluplex.getActiveRowSlacks();

        glp_add_rows( _lp, basicVariables.size() + activeSlackRows.size() );

        unsigned newIndex = 0;
        for ( auto basic : basicVariables )
        {
            // Need to start from 1, per GLPK rules
            ++newIndex;
            _basicToRowIndex[basic] = newIndex;

            glp_set_row_name( _lp, newIndex, Stringf( "%u", basic ).ascii() );

            double lowerBound = reluplex.getLowerBound( basic );
            double upperBound = reluplex.getUpperBound( basic );
            if ( FloatUtils::areEqual( lowerBound, upperBound ) )
                glp_set_row_bnds( _lp, newIndex, GLP_FX, lowerBound, upperBound );
            else
                glp_set_row_bnds( _lp, newIndex, GLP_DB, lowerBound, upperBound );

            _glpkEncodingToVariable[_nextGlpkInternalIndex] = basic;
            _variableToGlpkEncoding[basic] = _nextGlpkInternalIndex;

            ++_nextGlpkInternalIndex;
        }

        for ( auto slackVar : activeSlackRows )
        {
            ++newIndex;
            _basicToRowIndex[slackVar] = newIndex;

            glp_set_row_name( _lp, newIndex, Stringf( "%u", slackVar ).ascii() );

            if ( reluplex.useSlackVariablesForRelus() == IReluplex::USE_ROW_AND_COL_SLACK_VARIABLES )
            {
                // In this case, rows are fixed at zero
                glp_set_row_bnds( _lp, newIndex, GLP_FX, 0.0, 0.0 );
            }
            else
            {
                double lowerBound = reluplex.getSlackLowerBound( slackVar );
                double upperBound = reluplex.getSlackUpperBound( slackVar );
                glp_set_row_bnds( _lp, newIndex, GLP_DB, lowerBound, upperBound );
            }

            _glpkEncodingToVariable[_nextGlpkInternalIndex] = slackVar;
            _variableToGlpkEncoding[slackVar] = _nextGlpkInternalIndex;

            ++_nextGlpkInternalIndex;
        }
    }

    void addColumns( const IReluplex &reluplex )
    {
        Set<unsigned> basicVariables = reluplex.getBasicVariables();
        Set<unsigned> eliminatedVars = reluplex.getEliminatedVars();
        Set<unsigned> activeSlackCols = reluplex.getActiveColSlacks();

        unsigned numVariables = reluplex.getNumVariables();

        // The active columns variables exclude basics, eliminated vars and B vars in merged relu pairs
        glp_add_cols( _lp, numVariables - basicVariables.size() - eliminatedVars.size() - reluplex.countMerges() + activeSlackCols.size() );

        unsigned newIndex = 0;
        for ( unsigned i = 0; i < numVariables; ++i )
        {
            if ( basicVariables.exists( i ) || eliminatedVars.exists( i ) || reluplex.isDissolvedBVariable( i ) )
                continue;

            // Need to start from 1, per GLPK rules
            ++newIndex;
            _nonBasicToColumnIndex[i] = newIndex;

            glp_set_col_name( _lp, newIndex, Stringf( "%u", i ).ascii() );

            // We assume that all variables are double bounded
            double lowerBound = reluplex.getLowerBound( i );
            double upperBound = reluplex.getUpperBound( i );

            if ( FloatUtils::areEqual( lowerBound, upperBound ) )
            {
                glp_set_col_bnds( _lp, newIndex, GLP_FX, lowerBound, upperBound );
                glp_set_col_stat( _lp, newIndex, GLP_NS );
            }
            else
            {
                glp_set_col_bnds( _lp, newIndex, GLP_DB, lowerBound, upperBound );

                int bound;
                switch ( reluplex.getVarStatus( i ) )
                {
                case IReluplex::AT_UB:
                    bound = GLP_NU;
                    break;

                case IReluplex::AT_LB:
                    bound = GLP_NL;
                    break;

                case IReluplex::BETWEEN: {
                    // This is intended for relu variables: NU if active, NL if not.
                    if ( FloatUtils::isPositive( reluplex.getAssignment( i ) ) )
                        bound = GLP_NU;
                    else
                        bound = GLP_NL;
                    break;
                }

                default:
                    bound = GLP_NL;
                    break;
                }

                // Initializating the non-basic varaibles: NL for lower bound, NU for upper bound
                glp_set_col_stat( _lp, newIndex, bound );
            }

            _glpkEncodingToVariable[_nextGlpkInternalIndex] = i;
            _variableToGlpkEncoding[i] = _nextGlpkInternalIndex;

            ++_nextGlpkInternalIndex;
        }

        for ( auto slackVar : activeSlackCols )
        {
            ++newIndex;
            _nonBasicToColumnIndex[slackVar] = newIndex;

            glp_set_col_name( _lp, newIndex, Stringf( "%u", slackVar ).ascii() );

            // We assume that all variables are double bounded
            double lowerBound = reluplex.getSlackLowerBound( slackVar );
            double upperBound = reluplex.getSlackUpperBound( slackVar );

            glp_set_col_bnds( _lp, newIndex, GLP_DB, lowerBound, upperBound );

            _glpkEncodingToVariable[_nextGlpkInternalIndex] = slackVar;
            _variableToGlpkEncoding[slackVar] = _nextGlpkInternalIndex;

            ++_nextGlpkInternalIndex;
        }
    }

    unsigned glpkEncodingToVariable( unsigned glpkVarNumber ) const
    {
        return _glpkEncodingToVariable.at( glpkVarNumber );
    }

    unsigned variableToGlpkEncoding( unsigned variable ) const
    {
        return _variableToGlpkEncoding.at( variable );
    }

    void setObjectiveFunction( const IReluplex &reluplex )
    {
        Set<unsigned> activeSlackColVars = reluplex.getActiveColSlacks();

        if ( activeSlackColVars.empty() )
        {
            // The objective function is just a constant
            glp_set_obj_coef( _lp, _basicToRowIndex.begin()->second, 0.0 );
        }
        else
        {
            // Attempt to minimize the slacks, i.e. make f=b for as many pairs as possible.
            for ( const auto &slack : activeSlackColVars )
                glp_set_obj_coef( _lp, _nonBasicToColumnIndex[slack], 1.0 );
        }
    }

    void addWeights( const IReluplex &reluplex )
    {
        const Tableau *tableau = reluplex.getTableau();

        Set<unsigned> basicVariables = reluplex.getBasicVariables();
        Set<unsigned> activeRowSlacks = reluplex.getActiveRowSlacks();

        unsigned totalSize = tableau->totalSize() * 3;

        int *ia = new int[1 + totalSize];
        int *ja = new int[1 + totalSize];
        double *ar = new double[1 + totalSize];

        unsigned entryIndex = 1;
        for ( const auto &basic : basicVariables )
        {
            // Work on row i
            unsigned rowIndex = _basicToRowIndex[basic];

            const Tableau::Entry *row = tableau->getRow( basic );
            const Tableau::Entry *current;
            while ( row != NULL )
            {
                current = row;
                row = row->nextInRow();

                if ( current->getColumn() != basic )
                {
                    ia[entryIndex] = rowIndex;
                    ja[entryIndex] = _nonBasicToColumnIndex[current->getColumn()];
                    ar[entryIndex] = current->getValue();

                    ++entryIndex;
                }
            }
        }

        for ( unsigned rowSlack : activeRowSlacks )
        {
            // This is how the indices are originally assigned
            unsigned colSlack = rowSlack + 1; // Will be ignored if not using columns

            Map<unsigned, double> slackRow;
            prepareSlackRow( reluplex, rowSlack, colSlack, slackRow );

            for ( const auto &entry : slackRow )
            {
                ia[entryIndex] = _basicToRowIndex[rowSlack];
                ja[entryIndex] = _nonBasicToColumnIndex[entry.first];
                ar[entryIndex] = entry.second;
                ++entryIndex;
            }
        }

        --entryIndex;

        glp_load_matrix( _lp, entryIndex, ia, ja, ar );

        delete []ia;
        delete []ja;
        delete []ar;
    }

    void setBoundCalculationHook( BoundCalculationHook hook )
    {
        _boundCalculationHook = hook;
    }

    void setIterationCountCallback( IterationCountCallback callback )
    {
        _iterationCountCallback = callback;
    }

    void setReportSoiCallback( ReportSoiCallback callback )
    {
        _reportSoiCallback = callback;
    }

    void setMakeReluAdjustmentCallback( MakeReluAdjustmentsCallback callback )
    {
        _makeReluAdjustmentsCallback = callback;
    }

    GlpkAnswer solve()
    {
        int retValue;

        // The GLPK control parameters
        glp_smcp controlParameters;

        glp_init_smcp( &controlParameters );
        controlParameters.msg_lev = GLP_MSG_OFF;
        controlParameters.meth = GLP_PRIMAL;
        controlParameters.pricing = GLP_PT_PSE; // Steepest Edge
        controlParameters.r_test = GLP_RT_HAR; // Harris' two-pass

        // Iteration limit
        controlParameters.it_lim = 100000;

        controlParameters.presolve = 0;
        if ( _boundCalculationHook )
            controlParameters.boundCalculationHook = _boundCalculationHook;
        if ( _iterationCountCallback )
            controlParameters.iterationCountCallback = _iterationCountCallback;
        if ( _reportSoiCallback )
            controlParameters.reportSoiCallback = _reportSoiCallback;
        if ( _makeReluAdjustmentsCallback )
            controlParameters.makeReluAdjustmentsCallback = _makeReluAdjustmentsCallback;

        retValue = glp_simplex( _lp, &controlParameters );
        if ( retValue != 0 )
        {
            log( "Invocation of Glpk failed!\n" );

            switch ( retValue )
            {
            case GLP_EBADB:
                printf( "GLP_EBADB: Unable to start the search, because the initial basis "
                        "specified in the problem object is invalid—the number of basic "
                        "(auxiliary and structural) variables is not the same as the number"
                        "of rows in the problem object.\n" );
                break;

            case GLP_ESING:
                printf( "GLP_ESING: Unable to start the search, because the basis matrix "
                        "corresponding to the initial basis is singular within the working "
                        "precision.\n" );
                break;

            case GLP_ECOND:
                printf( "GLP_ECOND: Unable to start the search, because the basis matrix "
                        "corresponding to the initial basis is ill-conditioned, i.e. its "
                        "condition number is too large.\n" );
                break;

            case GLP_EBOUND:
                printf( "GLP_EBOUND: Unable to start the search, because some double-bounded "
                        "(auxiliary or structural) variables have incorrect bounds.\n" );
                break;

            case GLP_EFAIL:
                printf( "GLP_EFAIL: The search was prematurely terminated due to the solver failure.\n" );
                break;

            case GLP_EOBJLL:
                printf( "GLP_EOBJLL: The search was prematurely terminated, because the objective "
                        "function being maximized has reached its lower limit and continues "
                        "decreasing (the dual simplex only).\n" );
                break;

            case GLP_EOBJUL:
                printf( "GLP_EOBJUL: The search was prematurely terminated, because the objective "
                        "function being minimized has reached its upper limit and continues "
                        "increasing (the dual simplex only).\n" );
                break;

            case GLP_EITLIM:
                printf( "GLP_EITLIM: The search was prematurely terminated, because the simplex "
                        "iteration limit has been exceeded.\n" );
                break;

            case GLP_ETMLIM:
                printf( "GLP_ETMLIM: The search was prematurely terminated, because the time limit "
                        "has been exceeded.\n" );
                break;

            case GLP_ENOPFS:
                printf( "GLP_ENOPFS: The LP problem instance has no primal feasible solution (only "
                        "if the LP presolver is used).\n" );
                break;

            case GLP_ENODFS:
                printf( "GLP_ENODFS: The LP problem instance has no dual feasible solution (only "
                        "if the LP presolver is used).\n" );
                break;
            }

            return SOLVER_FAILED;
        }

        if ( glp_get_prim_stat( _lp ) == GLP_FEAS )
        {
            log( "A feasible solution has been found!\n" );
            return SOLUTION_FOUND;
        }

        if ( glp_get_prim_stat( _lp ) == GLP_NOFEAS )
        {
            log( "No feasible solution exists!\n" );
            return NO_SOLUTION_EXISTS;
        }

        printf( "Error! Unsupported return code from GLPK\n" );
        exit( 1 );
    }

    void extractAssignment( const IReluplex &reluplex, Map<unsigned, double> &assignment )
    {
        Set<unsigned> activeSlackRows = reluplex.getActiveRowSlacks();
        Set<unsigned> activeSlackCols = reluplex.getActiveColSlacks();

        assignment.clear();

        for ( auto nonBasic : _nonBasicToColumnIndex )
        {
            if ( !activeSlackCols.exists( nonBasic.first ) )
                assignment[nonBasic.first] = glp_get_col_prim( _lp, nonBasic.second );
        }

        for ( auto basic : _basicToRowIndex )
        {
            if ( !activeSlackRows.exists( basic.first ) )
                assignment[basic.first] = glp_get_row_prim( _lp, basic.second );
        }
    }

    void extractBasicVariables( const IReluplex &reluplex, Set<unsigned> &basics )
    {
        Set<unsigned> activeSlackRows = reluplex.getActiveRowSlacks();
        Set<unsigned> activeSlackCols = reluplex.getActiveColSlacks();

        basics.clear();

        for ( const auto &var : _nonBasicToColumnIndex )
        {
            if ( !activeSlackCols.exists( var.first ) )
                if ( glp_get_col_stat( _lp, var.second ) == GLP_BS )
                    basics.insert( var.first );
        }

        for ( const auto &var : _basicToRowIndex )
        {
            if ( !activeSlackRows.exists( var.first ) )
                if ( glp_get_row_stat( _lp, var.second ) == GLP_BS )
                    basics.insert( var.first );
        }
    }

    void extractTableau( IReluplex *reluplex,
                         Tableau *matrix,
                         Set<unsigned> *basicVariables,
                         Set<unsigned> *eliminatedVars )
    {
        if ( !reluplex->getActiveRowSlacks().empty() || !reluplex->getActiveColSlacks().empty() )
        {
            printf( "Error! Calling GlpkWrapper::extractTableau with active slack variables!\n" );
            exit( 1 );
        }

        unsigned numVars = matrix->getNumVars();
        Set<unsigned> originalBasicVariables = *basicVariables;

        basicVariables->clear();
        matrix->deleteAllEntries();

        _columnIndices = new int[numVars];
        _values = new double[numVars];

        for ( unsigned i = 0; i < numVars; ++i )
        {
            if ( !eliminatedVars->exists( i ) && !reluplex->isDissolvedBVariable( i ) )
                extractVariableRow( reluplex, i, matrix, basicVariables );
        }

        delete []_columnIndices;
        delete []_values;
    }

private:
    glp_prob *_lp;
    Map<unsigned, unsigned> _basicToRowIndex;
    Map<unsigned, unsigned> _nonBasicToColumnIndex;

    unsigned _nextGlpkInternalIndex;
    Map<unsigned, unsigned> _glpkEncodingToVariable;
    Map<unsigned, unsigned> _variableToGlpkEncoding;

    BoundCalculationHook _boundCalculationHook;
    IterationCountCallback _iterationCountCallback;
    ReportSoiCallback _reportSoiCallback;
    MakeReluAdjustmentsCallback _makeReluAdjustmentsCallback;

    int *_columnIndices;
    double *_values;

    bool _logging;

    void extractVariableRow( IReluplex *reluplex,
                             unsigned var,
                             Tableau *matrix,
                             Set<unsigned> *basicVariables )
    {
        unsigned glpkEncoding = _variableToGlpkEncoding[var];

        bool isNowBasic;
        if ( _basicToRowIndex.exists( var ) )
            isNowBasic = ( glp_get_row_stat( _lp, _basicToRowIndex[var] ) == GLP_BS );
        else
            isNowBasic = ( glp_get_col_stat( _lp, _nonBasicToColumnIndex[var] ) == GLP_BS );

        if ( !isNowBasic )
            return;

        basicVariables->insert( var );

        timeval start = Time::sampleMicro();
        int rowLength = glp_eval_tab_row( _lp, glpkEncoding, _columnIndices, _values );
        timeval end = Time::sampleMicro();
        reluplex->addTimeEvalutingGlpkRows( Time::timePassed( start, end ) );

        for ( int i = 1; i <= rowLength; ++i )
            matrix->addEntry( var, _glpkEncodingToVariable[_columnIndices[i]], _values[i] );

        matrix->addEntry( var, var, -1.0 );
    }

    void prepareSlackRow( const IReluplex &reluplex,
                          unsigned rowSlackVar,
                          unsigned colSlackVar,
                          Map<unsigned, double> &row )
    {
        Set<unsigned> basicVariables = reluplex.getBasicVariables();
        const Tableau *tableau = reluplex.getTableau();

        // Every slack variable has the equation f-b-colSlack.
        unsigned b = reluplex.slackToB( rowSlackVar );
        unsigned f = reluplex.slackToF( rowSlackVar );

        if ( !basicVariables.exists( f ) )
        {
            if ( !row.exists( f ) ) row[f] = 0.0;
            row[f] += 1.0;
        }
        else
        {
            // F is basic, so we need to add its entire row
            const Tableau::Entry *rowPointer = tableau->getRow( f );
            const Tableau::Entry *current;
            while ( rowPointer != NULL )
            {
                current = rowPointer;
                rowPointer = rowPointer->nextInRow();

                if ( current->getColumn() != f )
                {
                    unsigned column = current->getColumn();
                    double weight = current->getValue();

                    if ( !row.exists( column ) ) row[column] = 0.0;
                    row[column] += weight;
                }
            }
        }

        if ( !basicVariables.exists( b ) )
        {
            if ( !row.exists( b ) ) row[b] = 0.0;
            row[b] += -1.0;
        }
        else
        {
            // B is basic, so we need to add its entire row, negated
            const Tableau::Entry *rowPointer = tableau->getRow( b );
            const Tableau::Entry *current;
            while ( rowPointer != NULL )
            {
                current = rowPointer;
                rowPointer = rowPointer->nextInRow();

                if ( current->getColumn() != b )
                {
                    unsigned column = current->getColumn();
                    double weight = current->getValue();

                    if ( !row.exists( column ) ) row[column] = 0.0;
                    row[column] += -weight;
                }
            }
        }

        // Finally, add -slackCol, if we are using columns
        if ( reluplex.useSlackVariablesForRelus() == IReluplex::USE_ROW_AND_COL_SLACK_VARIABLES )
        {
            if ( !row.exists( colSlackVar ) ) row[colSlackVar] = 0.0;
            row[colSlackVar] += -1.0;
        }
    }
};

#endif // __GlpkWrapper_h__

//
// Local Variables:
// compile-command: "make -C . "
// tags-file-name: "./TAGS"
// c-basic-offset: 4
// End:
//
