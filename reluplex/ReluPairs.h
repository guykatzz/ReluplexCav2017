/*********************                                                        */
/*! \file ReluPairs.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** (in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __ReluPairs_h__
#define __ReluPairs_h__

#include "Map.h"
#include "MStringf.h"

class ReluPairs
{
public:
    class ReluPair
    {
    public:
        ReluPair( unsigned backwardVar, unsigned forwardVar )
            : _b( backwardVar )
            , _f( forwardVar )
        {
        }

        bool operator<( const ReluPair &other ) const
        {
            if ( _b < other._b )
                return true;
            if ( ( _b == other._b ) && ( _f < other._f ) )
                return true;
            return false;
        }

        unsigned getB() const
        {
            return _b;
        }

        unsigned getF() const
        {
            return _f;
        }

    private:
        unsigned _b;
        unsigned _f;
    };

    void addPair( unsigned backwardVar, unsigned forwardVar )
    {
        _reluPairs.insert( ReluPair( backwardVar, forwardVar ) );
        _bToF[backwardVar] = forwardVar;
        _fToB[forwardVar] = backwardVar;
    }

    bool isB( unsigned backwardVar ) const
    {
        return _bToF.exists( backwardVar );
    }

    bool isF( unsigned forwardVar ) const
    {
        return _fToB.exists( forwardVar );
    }

    bool isRelu( unsigned var ) const
    {
        return isB( var ) || isF( var );
    }

    unsigned toPartner( unsigned var ) const
    {
        if ( isB( var ) )
            return bToF( var );
        if ( isF( var ) )
            return fToB( var );

        throw Error( Error::NOT_RELU_VARIABLE, Stringf( "Var id: %u", var ).ascii() );
    }

    unsigned fToB( unsigned forwardVar ) const
    {
        return _fToB.at( forwardVar );
    }

    unsigned bToF( unsigned backwardVar ) const
    {
        return _bToF.at( backwardVar );
    }

    ReluPair toPair( unsigned var ) const
    {
        unsigned b, f;
        if ( isF( var ) )
        {
            f = var;
            b = fToB( var );
        }
        else
        {
            b = var;
            f = bToF( var );
        }

        return ReluPair( b, f );
    }

    const Set<ReluPair> &getPairs() const
    {
        return _reluPairs;
    }

    void removePair( unsigned var )
    {
        unsigned f;
        unsigned b;

        if ( isF( var ) )
        {
            f = var;
            b = toPartner( f );
        }
        else
        {
            b = var;
            f = toPartner( b );
        }

        _bToF.erase( b );
        _fToB.erase( f );
        _reluPairs.erase( ReluPair( b, f ) );
    }

    unsigned size() const
    {
        return _reluPairs.size();
    }

private:
    Set<ReluPair> _reluPairs;
    Map<unsigned, unsigned> _bToF;
    Map<unsigned, unsigned> _fToB;
};

#endif // __ReluPairs_h__

//
// Local Variables:
// compile-command: "make -C . "
// tags-file-name: "./TAGS"
// c-basic-offset: 4
// End:
//
