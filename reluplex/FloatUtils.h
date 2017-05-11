/*********************                                                        */
/*! \file FloatUtils.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** (in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __FloatUtils_h__
#define __FloatUtils_h__

#include <math.h>
#include <cfloat>

const double DEFAULT_EPSILON = 0.0000000001;

// https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
// http://docs.oracle.com/cd/E19957-01/806-3568/ncg_goldberg.html

class FloatUtils
{
public:
    static void printEpsion()
    {
        printf( "Float Utils: default epsilon = %.15lf\n", DEFAULT_EPSILON );
    }

    static bool areEqual( double x, double y, double epsilon = DEFAULT_EPSILON )
    {
        // Check if the numbers are really close -- needed
        // when comparing numbers near zero.
        double diff = fabs( x - y );
        if ( diff <= epsilon )
            return true;

        x = fabs( x );
        y = fabs( y );
        double largest = ( x > y ) ? x : y;

        if ( diff <= largest * DBL_EPSILON )
            return true;

        return false;
    }

    static double abs( double x )
    {
        return fabs( x );
    }

    static bool areDisequal( double x, double y, double epsilon = DEFAULT_EPSILON )
    {
        return !areEqual( x, y, epsilon );
    }

    static bool isZero( double x, double epsilon = DEFAULT_EPSILON )
    {
        return areEqual( x, 0.0, epsilon );
    }

    static bool isPositive( double x, double epsilon = DEFAULT_EPSILON )
    {
        return ( !isZero( x, epsilon ) ) && ( x > 0.0 );
    }

    static bool isNegative( double x, double epsilon = DEFAULT_EPSILON )
    {
        return ( !isZero( x, epsilon ) ) && ( x < 0.0 );
    }

    static bool gt( double x, double y, double epsilon = DEFAULT_EPSILON )
    {
        return isPositive( x - y, epsilon );
    }

    static bool gte( double x, double y, double epsilon = DEFAULT_EPSILON )
    {
        return !isNegative( x - y, epsilon );
    }

    static bool lt( double x, double y, double epsilon = DEFAULT_EPSILON )
    {
        return gt( y, x, epsilon );
    }

    static bool lte( double x, double y, double epsilon = DEFAULT_EPSILON )
    {
        return gte( y, x, epsilon );
    }

    static double min( double x, double y, double epsilon = DEFAULT_EPSILON )
    {
        return lt( x, y, epsilon ) ? x : y;
    }

    static double max( double x, double y, double epsilon = DEFAULT_EPSILON )
    {
        return gt( x, y, epsilon ) ? x : y;
    }
};

#endif // __FloatUtils_h__

//
// Local Variables:
// compile-command: "make -C . "
// tags-file-name: "./TAGS"
// c-basic-offset: 4
// End:
//
