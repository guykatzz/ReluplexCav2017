/*********************                                                        */
/*! \file IHeapData.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** (in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __IHeapData_h__
#define __IHeapData_h__

class IHeapData
{
public:
    virtual void *data() = 0;
    virtual const void *data() const = 0;
    virtual unsigned size() const = 0;

    virtual ~IHeapData() {}
};

#endif // __IHeapData_h__
