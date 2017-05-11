/*********************                                                        */
/*! \file Stack.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** (in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __Stack_h__
#define __Stack_h__

#include <cstdio>

#include "Error.h"
#include <stack>

template<class T>
class Stack
{
    typedef std::stack<T> Super;
public:

    void push( T value )
    {
        _container.push( value );
    }

    bool empty() const
    {
        return _container.empty();
    }

    unsigned size() const
    {
        return _container.size();
    }

    void clear()
    {
        while ( !empty() )
        {
            pop();
        }
    }

    void pop()
    {
        if ( empty() )
        {
            throw Error( Error::STACK_IS_EMPTY );
        }

        _container.pop();
    }

    T &top()
    {
        if ( empty() )
        {
            throw Error( Error::STACK_IS_EMPTY );
        }

        return _container.top();
    }

protected:
    Super _container;
};

#endif // __Stack_h__
