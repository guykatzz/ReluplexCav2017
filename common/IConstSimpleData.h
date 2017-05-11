/*********************                                                        */
/*! \file IConstSimpleData.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** (in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __IConstSimpleData_h__
#define __IConstSimpleData_h__

class IConstSimpleData
{
public:
	virtual const void *data() const = 0;
	virtual unsigned size() const = 0;

	virtual ~IConstSimpleData() {}
};

#endif // __IConstSimpleData_h__
