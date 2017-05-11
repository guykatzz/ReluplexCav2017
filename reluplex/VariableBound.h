/*********************                                                        */
/*! \file VariableBound.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** (in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __VariableBound_h__
#define __VariableBound_h__

#include <string.h>

static const double UNDEFINED = 888;

class VariableBound
{
public:
    VariableBound() : _finite( false ), _bound( UNDEFINED ), _level( 0 )
    {
    }

    VariableBound( double bound ) : _finite( true ), _bound( bound ), _level( 0 )
    {
    }

    void setBound( double bound )
    {
        _finite = true;
        _bound = bound;
    }

    bool finite() const
    {
        return _finite;
    }

    double getBound() const
    {
        return _bound;
    }

    void setLevel( unsigned level )
    {
        _level = level;
    }

    unsigned getLevel() const
    {
        return _level;
    }

private:
    bool _finite;
    double _bound;
    unsigned _level;
};

#endif // __VariableBound_h__

//
// Local Variables:
// compile-command: "make -C . "
// tags-file-name: "./TAGS"
// c-basic-offset: 4
// End:
//
