/*********************                                                        */
/*! \file MString.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** (in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __String_h__
#define __String_h__

#include "Vector.h"
#include <cstring>
#include <string>

class String
{
public:
	typedef std::string Super;

	String( Super super ) : _super( super )
	{
	}

	String()
	{
	}

    String( const char *string, unsigned length ) : _super( string, length )
    {
    }

	String( const char *string ) : _super( string )
	{
	}

	unsigned length() const
	{
		return _super.length();
	}

	const char *ascii() const
	{
		return _super.c_str();
	}

	char operator[]( int index ) const
	{
		return _super[index];
	}

	char &operator[]( int index )
	{
		return _super[index];
	}

	String &operator=( const char *string )
	{
		*this = String( string );
		return *this;
	}

	String &operator=( const String &other )
	{
		_super = other._super;
		return *this;
	}

	bool operator==( const char *string ) const
	{
		return _super.compare( string ) == 0;
	}

	String operator+( const String &other ) const
	{
		return _super + other._super;
	}

	String operator+( const char *other ) const
	{
		return _super + other;
	}

	String operator+=( const String &other )
	{
		return _super += other._super;
	}

	String operator+=( const char *other )
	{
		return _super += other;
	}

	bool operator==( const String &other ) const
	{
        return _super.compare( other._super ) == 0;
	}

    bool operator!=( const String &other ) const
    {
        return _super.compare( other._super ) != 0;
    }

    bool operator<( const String &other ) const
    {
        return _super < other._super;
    }

    Vector<String> tokenize( String delimiter ) const
    {
        Vector<String> tokens;

        char copy[length()+1];
        memcpy( copy, ascii(), sizeof(copy) );

        char *token = strtok( copy, delimiter.ascii() );

        while ( token != NULL )
        {
            tokens.append( String( token ) );
            token = strtok( NULL, delimiter.ascii() );
        }

        return tokens;
    }

    bool contains( const String &substring ) const
    {
        return find( substring ) != Super::npos;
    }

    size_t find( const String &substring ) const
    {
        return _super.find( substring._super );
    }

    String substring( unsigned fromIndex, unsigned howMany ) const
    {
        return _super.substr( fromIndex, howMany );
    }

    void replace( const String &toReplace, const String &replaceWith )
    {
        if ( find( toReplace ) != Super::npos )
        {
            _super.replace( find( toReplace ), toReplace.length(), replaceWith._super );
        }
    }

    String trim() const
    {
        unsigned firstNonSpace = 0;
        unsigned lastNonSpace = 0;

        bool firstNonSpaceFound = false;

        for ( unsigned i = 0; i < length(); ++i )
        {
            if ( ( !firstNonSpaceFound ) && ( _super[i] != ' ' ) )
            {
                firstNonSpace = i;
                firstNonSpaceFound = true;
            }

            if ( ( _super[i] != ' ' ) && ( _super[i] != '\n' ) )
            {
                lastNonSpace = i;
            }
        }

        if ( lastNonSpace == 0 )
        {
            return "";
        }

        return substring( firstNonSpace, lastNonSpace - firstNonSpace + 1 );
    }

    void replaceAll( const String &toReplace, const String &replaceWith )
    {
        while ( find( toReplace ) != Super::npos )
            _super.replace( find( toReplace ), toReplace.length(), replaceWith._super );
    }

protected:
	Super _super;
};

std::ostream &operator<<( std::ostream &stream, const String &string );

#endif // __String_h__
