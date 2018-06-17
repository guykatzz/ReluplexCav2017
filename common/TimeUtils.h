/*********************                                                        */
/*! \file TimeUtils.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** (in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __Time_h__
#define __Time_h__

#include <time.h>
#include <sys/time.h>
#include "MStringf.h"

class Time
{
public:
    static time_t sample()
    {
        return time( NULL );
    }

    static struct timeval sampleMicro()
    {
        struct timeval answer;
        memset( &answer, 0, sizeof(answer) );
        struct timezone *DONT_CARE_TIME_ZONE = 0;

        gettimeofday( &answer, DONT_CARE_TIME_ZONE );

        return answer;
    }

    static String timePassed( time_t then, time_t now )
    {
        time_t difference = now - then;
        struct tm *formattedTime = gmtime( &difference );

        return Stringf( "%02u:%02u:%02u", formattedTime->tm_hour, formattedTime->tm_min, formattedTime->tm_sec );
    }

    // Return time in milliseconds
    static unsigned timePassed( struct timeval then, struct timeval now )
    {
        enum {
            MILLISECONDS_IN_SECOND = 1000,
            MICROSECONDS_IN_MILLISECOND = 1000,
            MICROSECONDS_IN_SECOND = 1000000,
        };

        unsigned long seconds = now.tv_sec - then.tv_sec;
        unsigned long useconds;
        if ( now.tv_usec > then.tv_usec )
            useconds = now.tv_usec - then.tv_usec;
        else
        {
            useconds = MICROSECONDS_IN_SECOND + ( now.tv_usec - then.tv_usec );
            --seconds;
        }

        return ( seconds * MILLISECONDS_IN_SECOND ) + ( useconds / MICROSECONDS_IN_MILLISECOND );
    }

    static String now()
    {
        time_t secodnsSinceEpoch = time( NULL );
        struct tm *formattedTime = localtime( &secodnsSinceEpoch );

        return Stringf( "%02u:%02u:%02u", formattedTime->tm_hour, formattedTime->tm_min, formattedTime->tm_sec );
    }
};

#endif // __Time_h__
