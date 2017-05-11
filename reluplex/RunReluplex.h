/*********************                                                        */
/*! \file RunReluplex.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** (in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __RunReluplex_h__
#define __RunReluplex_h__

#include "Reluplex.h"

class RunReluplex
{
public:
    RunReluplex() : _reluplex( NULL )
    {
    }

    ~RunReluplex()
    {
        if ( _reluplex )
            delete _reluplex;
    }

    // This is the simple example from the paper.
    void example1()
    {
        _reluplex = new Reluplex( 9 );

        _reluplex->setName( 0, "x1" );
        _reluplex->setName( 1, "x2b" );
        _reluplex->setName( 2, "x2f" );
        _reluplex->setName( 3, "x3b" );
        _reluplex->setName( 4, "x3f" );
        _reluplex->setName( 5, "x4" );
        _reluplex->setName( 6, "x5" );
        _reluplex->setName( 7, "x6" );
        _reluplex->setName( 8, "x7" );

        _reluplex->setLogging( true );

        _reluplex->initializeCell( 6, 2, 1.0 );
        _reluplex->initializeCell( 6, 4, 1.0 );
        _reluplex->initializeCell( 6, 5, -1.0 );
        _reluplex->initializeCell( 6, 6, -1.0 );

        _reluplex->initializeCell( 7, 0, 1.0 );
        _reluplex->initializeCell( 7, 1, -1.0 );
        _reluplex->initializeCell( 7, 7, -1.0 );

        _reluplex->initializeCell( 8, 0, 1.0 );
        _reluplex->initializeCell( 8, 3, 1.0 );
        _reluplex->initializeCell( 8, 8, -1.0 );

        _reluplex->markBasic( 6 );
        _reluplex->markBasic( 7 );
        _reluplex->markBasic( 8 );

        _reluplex->setLowerBound( 0, 0.0 );
        _reluplex->setUpperBound( 0, 1.0 );
        _reluplex->setLowerBound( 2, 0.0 );
        _reluplex->setLowerBound( 4, 0.0 );
        _reluplex->setLowerBound( 5, 0.5 );
        _reluplex->setUpperBound( 5, 1.0 );
        _reluplex->setLowerBound( 6, 0.0 );
        _reluplex->setUpperBound( 6, 0.0 );
        _reluplex->setLowerBound( 7, 0.0 );
        _reluplex->setUpperBound( 7, 0.0 );
        _reluplex->setLowerBound( 8, 0.0 );
        _reluplex->setUpperBound( 8, 0.0 );

        _reluplex->setLowerBound( 1, -9.0 );
        _reluplex->setUpperBound( 1, 9.0 );
        _reluplex->setUpperBound( 2, 9.0 );
        _reluplex->setLowerBound( 3, -9.0 );
        _reluplex->setUpperBound( 3, 9.0 );
        _reluplex->setUpperBound( 4, 9.0 );

        _reluplex->setReluPair( 1, 2 );
        _reluplex->setReluPair( 3, 4 );
    }

    // An unsolveable example
    void example2()
    {
        _reluplex = new Reluplex( 10 );

        _reluplex->setName( 0, "x1" );
        _reluplex->setName( 1, "x2b" );
        _reluplex->setName( 2, "x2f" );
        _reluplex->setName( 3, "x3b" );
        _reluplex->setName( 4, "x3f" );
        _reluplex->setName( 5, "x4" );
        _reluplex->setName( 6, "x5" );
        _reluplex->setName( 7, "x6" );
        _reluplex->setName( 8, "x7" );
        _reluplex->setName( 9, "x8" );

        _reluplex->setLogging( true );

        _reluplex->initializeCell( 6, 2, 1.0 );
        _reluplex->initializeCell( 6, 5, -1.0 );
        _reluplex->initializeCell( 6, 6, -1.0 );

        _reluplex->initializeCell( 7, 0, 1.0 );
        _reluplex->initializeCell( 7, 1, -1.0 );
        _reluplex->initializeCell( 7, 7, -1.0 );

        _reluplex->initializeCell( 8, 0, -1.0 );
        _reluplex->initializeCell( 8, 3, -1.0 );
        _reluplex->initializeCell( 8, 8, -1.0 );

        _reluplex->initializeCell( 9, 4, 1.0 );
        _reluplex->initializeCell( 9, 5, -1.0 );
        _reluplex->initializeCell( 9, 9, -1.0 );

        _reluplex->markBasic( 6 );
        _reluplex->markBasic( 7 );
        _reluplex->markBasic( 8 );
        _reluplex->markBasic( 9 );

        _reluplex->setLowerBound( 0, 0.0 );
        _reluplex->setUpperBound( 0, 1.0 );
        _reluplex->setLowerBound( 2, 0.0 );
        _reluplex->setLowerBound( 4, 0.0 );
        _reluplex->setLowerBound( 5, 0.5 );
        _reluplex->setUpperBound( 5, 1.0 );
        _reluplex->setLowerBound( 6, 0.0 );
        _reluplex->setUpperBound( 6, 0.0 );
        _reluplex->setLowerBound( 7, 0.0 );
        _reluplex->setUpperBound( 7, 0.0 );
        _reluplex->setLowerBound( 8, 0.0 );
        _reluplex->setUpperBound( 8, 0.0 );
        _reluplex->setLowerBound( 9, 0.0 );
        _reluplex->setUpperBound( 9, 0.0 );

        _reluplex->setLowerBound( 1, -9.0 );
        _reluplex->setUpperBound( 1, 9.0 );
        _reluplex->setUpperBound( 2, 9.0 );
        _reluplex->setLowerBound( 3, -9.0 );
        _reluplex->setUpperBound( 3, 9.0 );
        _reluplex->setUpperBound( 4, 9.0 );

        _reluplex->setReluPair( 1, 2 );
        _reluplex->setReluPair( 3, 4 );
    }

    void go()
    {
        // Choose between the 2 available examples
        example1();
        // example2();

        _reluplex->setDumpStates( true );

        try
        {
            Reluplex::FinalStatus result = _reluplex->solve();
            if ( result == Reluplex::SAT )
                printf( "\n*** Solved! ***\n" );
            else if ( result == Reluplex::UNSAT )
                printf( "\n*** Can't Solve ***\n" );
            else if ( result == Reluplex::ERROR )
                printf( "Reluplex error!\n" );
            else
                printf( "Reluplex not done (quit called?)\n" );
        }
        catch ( const Error &e )
        {
            if ( e.code() == Error::STACK_IS_EMPTY )
                printf( "\n*** Can't Solve (STACK EMPTY) ***\n" );
            else if ( e.code() == Error::UPPER_LOWER_INVARIANT_VIOLATED )
                printf( "\n*** Can't Solve (UPPER_LOWER_INVARIANT_VIOLATED) ***\n" );
            else
            {
                throw e;
            }
        }
    }

private:
    Reluplex *_reluplex;
};

#endif // __RunReluplex_h__

//
// Local Variables:
// compile-command: "make -C . "
// tags-file-name: "./TAGS"
// c-basic-offset: 4
// End:
//
