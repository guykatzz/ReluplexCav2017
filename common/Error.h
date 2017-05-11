/*********************                                                        */
/*! \file Error.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** (in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __Error_h__
#define __Error_h__

#include <errno.h>
#include <stdlib.h>
#include <string.h>

class Error
{
public:
	enum Code {
        APP_SPECIFIC_ERROR = 0,
        KEY_DOESNT_EXIST_IN_MAP = 1,
		SELECT_FAILED = 2,
		SOCKET_CREATION_FAILED = 3,
		BIND_FAILED = 4,
		LISTEN_FAILED = 5,
		ACCEPT_FAILED = 6,
		CONNECT_FAILED = 7,
		SEND_FAILED = 8,
		RECV_FAILED = 9,
		SOCKET_WAS_CLOSED = 10,
        VALUE_DOESNT_EXIST_IN_VECTOR = 11,
        SELECTION_DEADLOCKED = 12,
        FORK_FAILED = 13,
        PTHREAD_CREATE_FAILED = 14,
        SET_SOCK_OPT_FAILED = 15,
        OPEN_FAILED = 16,
        WRITE_FAILED = 17,
        VECTOR_OUT_OF_BOUND = 18,
        READ_FAILED = 19,
        UNKNOWN_GOAL_RESULT = 20,
        RENAME_FAILED = 21,
        UNLINK_FAILED = 22,
        COULDNT_FIND_NEW_STATE = 23,
        COULDNT_FIND_EXISTING_TRANSITION = 24,
        POPPING_FROM_EMPTY_VECTOR = 25,
        VALUE_DOESNT_EXIST_IN_CONFIG_FILE = 26,
        QUEUE_IS_EMPTY = 27,
        TIMERFD_CREATE_FAILED = 28,
        TIMERFD_SETTIME_FAILED = 29,
        ORACLE_ERROR = 30,
        GET_INVOKED_ON_NONEXISTING_FILE = 31,
        STAT_FAILED = 32,
        INET_ATON_FAILED = 33,
        NO_SUCCESSOR_IN_NFA = 34,
        NO_REACHABLE_ACCEPTING_STATES = 35,
        REFINING_FOR_AN_EMPTY_RUN = 36,
        NO_FAILED_MODULE = 37,
        RUN_UNPATCHABLE = 38,
        NO_ENABLED_EVENTS_FOR_ORACLE = 39,
		NOT_ENOUGH_MEMORY = 40,
        STACK_IS_EMPTY = 41,
        PIPE_FAILED = 42,
        DUP_FAILED = 43,
        WAITPID_FAILED = 44,
        SUCCESSOR_NOT_FOUND = 45,
        MAP_IS_EMPTY = 46,
        SOCKET_POLL_FAILED = 47,
        MISSING_DISTRIBUTER_CONNECTION = 48,
        ILLEGAL_PIVOT_OP = 49,
        NOT_RELU_VARIABLE = 50,
        NO_SUCH_VARIABLE = 51,
        INVALID_PIVOT_PATH = 52,
        CANT_DO_INITIAL_UPDATE_DUE_TO_RELU = 53,
        OUT_OF_MEMORY = 54,
        VARIABLE_NOT_BASIC = 55,
        UPPER_LOWER_INVARIANT_VIOLATED = 56,
        COPY_INCOMPATIBLE_SPARSE_MATRICES = 57,
        NONBASIC_OUT_OF_BOUNDS = 58,
        MULTIPLE_INFINITE_VARS_ON_ROW = 59,
        EXPECTED_NO_INFINITE_VARS = 60,
        CANT_FIX_BROKEN_RELU = 61,
        INACTIVE_STRATEGY_CALLED = 62,
        INVALID_SELECTION_IN_STRATEGY = 63,
        REVISITING_CACHED_STATAE = 64,
        VAR_DOESNT_APPEAR_IN_COST = 65,
        CANT_MAKE_NON_BASIC = 66,
        COST_FUNCTION_CANNOT_DECREASE = 67,
        ALL_COST_STEPS_INCREASE_VIOLATION = 68,
        LOWER_BOUND_IS_INFINITE = 69,
        UPPER_BOUND_IS_INFINITE = 70,
        CONSECUTIVE_GLPK_FAILURES = 71,
    };

	Error( Code code ) : _code( code )
	{
        memset( _userMessage, 0, sizeof(_userMessage) );
        _errno = errno;
	}

    Error( Code code, const char *userMessage ) : _code( code )
    {
        _errno = errno;
        setUserMessage( userMessage );
    }

    int getErrno() const
    {
        return _errno;
    }

	Code code() const
	{
		return _code;
	}

    void setUserMessage( const char *userMessage )
    {
        strcpy( _userMessage, userMessage );
    }

    const char *userMessage() const
    {
        return _userMessage;
    }

private:
    enum {
        MAX_USER_MESSAGE = 1024,
    };

    Code _code;
    int _errno;
    char _userMessage[MAX_USER_MESSAGE];
};

#endif // __Error_h__
