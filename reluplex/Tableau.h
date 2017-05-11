/*********************                                                        */
/*! \file Tableau.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** (in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __Tableau_h__
#define __Tableau_h__

#include "Map.h"
#include "Debug.h"
#include "FloatUtils.h"
#include "Vector.h"

class Tableau
{
public:
    class Entry
    {
    public:
        Entry()
            : _nextInRow( NULL )
            , _prevInRow( NULL )
            , _nextInColumn( NULL )
            , _prevInColumn( NULL )
            , _value( 0.0 )
        {
        }

        Entry *nextInRow()
        {
            return _nextInRow;
        }

        Entry *prevInRow()
        {
            return _prevInRow;
        }

        Entry *nextInColumn()
        {
            return _nextInColumn;
        }

        Entry *prevInColumn()
        {
            return _prevInColumn;
        }

        const Entry *nextInRow() const
        {
            return _nextInRow;
        }

        const Entry *prevInRow() const
        {
            return _prevInRow;
        }

        const Entry *nextInColumn() const
        {
            return _nextInColumn;
        }

        const Entry *prevInColumn() const
        {
            return _prevInColumn;
        }

        void setNextInRow( Entry *entry )
        {
            _nextInRow = entry;
        }

        void setPrevInRow( Entry *entry )
        {
            _prevInRow = entry;
        }

        void setNextInColumn( Entry *entry )
        {
            _nextInColumn = entry;
        }

        void setPrevInColumn( Entry *entry )
        {
            _prevInColumn = entry;
        }

        void setRow( unsigned row )
        {
            _row = row;
        }

        void setColumn( unsigned column )
        {
            _column = column;
        }

        unsigned getRow() const
        {
            return _row;
        }

        unsigned getColumn() const
        {
            return _column;
        }

        void setValue( double value )
        {
            _value = value;
        }

        double getValue() const
        {
            return _value;
        }

        Entry *_nextInRow;
        Entry *_prevInRow;
        Entry *_nextInColumn;
        Entry *_prevInColumn;

        unsigned _row;
        unsigned _column;

        double _value;
    };

    Tableau( unsigned size )
        : _size( size )
        , _rows( NULL )
        , _columns( NULL )
    {
        _rows = new Tableau::Entry *[size];
        _columns = new Tableau::Entry *[size];

        for ( unsigned i = 0; i < _size; ++i )
        {
            _rows[i] = NULL;
            _columns[i] = NULL;

            _rowSize.append( 0 );
            _columnSize.append( 0 );
        }
    }

    unsigned getNumVars() const
    {
        return _size;
    }

    ~Tableau()
    {
        deleteAllEntries();
        delete []_rows;
        delete []_columns;
    }

    unsigned totalSize() const
    {
        unsigned total = 0;

        for ( unsigned i = 0; i < _size; ++i )
            total += _rowSize.get( i );
        return total;
    }

    void deleteAllEntries()
    {
        for ( unsigned i = 0; i < _size; ++i )
        {
            Entry *current, *next;
            current = _rows[i];

            while ( current )
            {
                next = current->nextInRow();
                delete current;
                current = next;
            }
        }

        for ( unsigned i = 0; i < _size; ++i )
        {
            _rows[i] = NULL;
            _columns[i] = NULL;
            _columnSize[i] = 0;
            _rowSize[i] = 0;
        }
    }

    // By row
    double getCell( unsigned row, unsigned column ) const
    {
        Entry *rowEntry = _rows[row];

        while ( rowEntry != NULL )
        {
            if ( rowEntry->getColumn() == column )
                return rowEntry->getValue();

            rowEntry = rowEntry->nextInRow();
        }

        return 0.0;
    }

    void addEntry( unsigned row, unsigned column, const double &value )
    {
        // We assume that the entry does not currently exist (we don't try to unlink it)
        if ( FloatUtils::isZero( value ) )
            return;

        Entry *entry = new Entry;
        entry->setRow( row );
        entry->setColumn( column );
        entry->setValue( value );

        entry->setNextInRow( _rows[row] );
        entry->setNextInColumn( _columns[column] );

        if ( _rows[row] != NULL )
            _rows[row]->setPrevInRow( entry );
        if ( _columns[column] != NULL )
            _columns[column]->setPrevInColumn( entry );

        _rows[row] = entry;
        _columns[column] = entry;

        ++_rowSize[row];
        ++_columnSize[column];
    }

    bool activeRow( unsigned row ) const
    {
        return _rows[row] != NULL;
    }

    bool activeColumn( unsigned column ) const
    {
        return _columns[column] != NULL;
    }

    void eraseEntry( Entry *entry )
    {
        if ( entry->nextInRow() )
            entry->nextInRow()->setPrevInRow( entry->prevInRow() );
        if ( entry->prevInRow() )
            entry->prevInRow()->setNextInRow( entry->nextInRow() );

        if ( entry->nextInColumn() )
            entry->nextInColumn()->setPrevInColumn( entry->prevInColumn() );
        if ( entry->prevInColumn() )
            entry->prevInColumn()->setNextInColumn( entry->nextInColumn() );

        if ( _rows[entry->getRow()] == entry )
            _rows[entry->getRow()] = entry->nextInRow();
        if ( _columns[entry->getColumn()] == entry )
            _columns[entry->getColumn()] = entry->nextInColumn();

        --_rowSize[entry->getRow()];
        --_columnSize[entry->getColumn()];

        delete entry;
    }

    void eraseRow( unsigned row )
    {
        Entry *entry = _rows[row];
        Entry *next;

        while ( entry != NULL )
        {
            if ( entry->nextInColumn() )
                entry->nextInColumn()->setPrevInColumn( entry->prevInColumn() );
            if ( entry->prevInColumn() )
                entry->prevInColumn()->setNextInColumn( entry->nextInColumn() );
            if ( _columns[entry->getColumn()] == entry )
                _columns[entry->getColumn()] = entry->nextInColumn();

            --_columnSize[entry->getColumn()];

            next = entry->nextInRow();
            delete entry;
            entry = next;
        }

        _rows[row] = NULL;
        _rowSize[row] = 0;
    }

    void eraseColumn( unsigned column )
    {
        Entry *entry = _columns[column];
        Entry *next;

        while ( entry != NULL )
        {
            if ( entry->nextInRow() )
                entry->nextInRow()->setPrevInRow( entry->prevInRow() );
            if ( entry->prevInRow() )
                entry->prevInRow()->setNextInRow( entry->nextInRow() );
            if ( _rows[entry->getRow()] == entry )
                _rows[entry->getRow()] = entry->nextInRow();

            --_rowSize[entry->getRow()];

            next = entry->nextInColumn();
            delete entry;
            entry = next;
        }

        _columns[column] = NULL;
        _columnSize[column] = 0;
    }

    void addScaledRow( unsigned source, double scale, unsigned target,
                       // We usually want to guarantee that a certain entry has a certain value
                       unsigned guaranteeIndex, double guaranteeValue,
                       unsigned *numCalcs = NULL )
    {
        if ( !activeRow( source ) )
            return;

        _denseMap.clear();

        Entry *targetEntry = _rows[target];
        while ( targetEntry != NULL )
        {
            _denseMap[targetEntry->getColumn()] = targetEntry;
            targetEntry = targetEntry->nextInRow();
        }

        Entry *sourceEntry = _rows[source];
        Entry *current;
        unsigned column;
        while ( sourceEntry != NULL )
        {
            current = sourceEntry;
            sourceEntry = sourceEntry->nextInRow();

            column = current->getColumn();
            Entry *entryInTarget = NULL;
            if ( _denseMap.exists( column ) )
                entryInTarget = _denseMap[column];

            double newValue = current->getValue() * scale;

            // Statistics
            if ( numCalcs )
                ++( *numCalcs );

            if ( entryInTarget )
            {
                if ( column != guaranteeIndex )
                    entryInTarget->setValue( entryInTarget->getValue() + newValue );
                else
                    entryInTarget->setValue( guaranteeValue );

                // The addition is another action
                if ( numCalcs )
                    ++( *numCalcs );

                if ( FloatUtils::isZero( entryInTarget->getValue() ) )
                {
                    _denseMap.erase( column );
                    eraseEntry( entryInTarget );
                }
            }
            else
            {
                // This is a new entry for the target row
                if ( column != guaranteeIndex )
                    addEntry( target, column, newValue );
                else
                    addEntry( target, column, guaranteeValue );
            }
        }
    }

    void addColumnEraseSource( unsigned source, unsigned target )
    {
        if ( !activeColumn( source ) )
            return;

        _denseMap.clear();

        Entry *targetEntry = _columns[target];
        while ( targetEntry != NULL )
        {
            _denseMap[targetEntry->getRow()] = targetEntry;
            targetEntry = targetEntry->nextInColumn();
        }

        Entry *sourceEntry = _columns[source];
        Entry *current;

        unsigned row;
        while ( sourceEntry != NULL )
        {
            current = sourceEntry;
            sourceEntry = sourceEntry->nextInColumn();

            row = current->getRow();
            Entry *entryInTarget = NULL;
            if ( _denseMap.exists( row ) )
            {
                // Add from source column to target column
                entryInTarget = _denseMap[row];
                entryInTarget->setValue( entryInTarget->getValue() + current->getValue() );

                if ( FloatUtils::isZero( entryInTarget->getValue() ) )
                {
                    _denseMap.erase( row );
                    eraseEntry( entryInTarget );
                }
            }
            else
            {
                // There was no entry in the target column. "Steal" the entry.
                // No need to change row pointers, just the column pointers.

                if ( current->nextInColumn() )
                    current->nextInColumn()->setPrevInColumn( current->prevInColumn() );
                if ( current->prevInColumn() )
                    current->prevInColumn()->setNextInColumn( current->nextInColumn() );
                if ( _columns[source] == current )
                    _columns[source] = current->nextInColumn();

                // Adding is done at the head of the column, so shouldn't affect this loop
                current->setColumn( target );
                current->setNextInColumn( _columns[target] );
                current->setPrevInColumn( NULL );

                if ( _columns[target] )
                    _columns[target]->setPrevInColumn( current );

                _columns[target] = current;

                ++_columnSize[target];
                --_columnSize[source];
            }
        }

        eraseColumn( source );
        _denseMap.clear();
    }

    unsigned getRowSize( unsigned row ) const
    {
        return _rowSize.get( row );
    }

    unsigned getColumnSize( unsigned column ) const
    {
        return _columnSize.get( column );
    }

    const Entry *getRow( unsigned row ) const
    {
        return _rows[row];
    }

    const Entry *getColumn( unsigned column ) const
    {
        return _columns[column];
    }

    void printRow( unsigned row )
    {
        printf( "Printing row %u\n", row );
        printf( "\t%u = ", row );

        Entry *rowEntry = _rows[row];
        Entry *current;

        unsigned column;
        double weight;

        while ( rowEntry != NULL )
        {
            current = rowEntry;
            rowEntry = rowEntry->nextInRow();

            column = current->getColumn();
            weight = current->getValue();

            if ( !FloatUtils::isNegative( weight ) )
                printf( "+" );
            printf( "%lf * %u ", weight, column );
        }
        printf( "\n" );
    }

    void backupIntoMatrix( Tableau *other ) const
    {
        // Assume other has been initialized with correct sizes
        if ( other->_size != _size )
            throw Error( Error::COPY_INCOMPATIBLE_SPARSE_MATRICES );

        other->deleteAllEntries();
        other->_rowSize = _rowSize;
        other->_columnSize = _columnSize;

        for ( unsigned i = 0; i < _size; ++i )
        {
            Entry *entry = _rows[i];
            while ( entry )
            {
                Entry *newEntry = new Entry;

                newEntry->setRow( entry->getRow() );
                newEntry->setColumn( entry->getColumn() );
                newEntry->setValue( entry->getValue() );

                newEntry->setNextInRow( other->_rows[entry->getRow()] );
                newEntry->setNextInColumn( other->_columns[entry->getColumn()] );

                if ( other->_rows[entry->getRow()] )
                    other->_rows[entry->getRow()]->setPrevInRow( newEntry );
                if ( other->_columns[entry->getColumn()] )
                    other->_columns[entry->getColumn()]->setPrevInColumn( newEntry );

                other->_rows[entry->getRow()] = newEntry;
                other->_columns[entry->getColumn()] = newEntry;

                entry = entry->nextInRow();
            }
        }
    }

    void ensureNoZeros() const
    {
        for ( unsigned i = 0; i < _size; ++i )
            ensureNoZerosInRow( i );
    }

    void ensureNoZerosInRow( unsigned row ) const
    {
        if ( _rows[row] == NULL )
            return;

        Entry *entry = _rows[row];

        while ( entry != NULL )
        {
            if ( FloatUtils::isZero( entry->getValue() ) )
            {
                printf( "Error! Found a 0 in the matrix!\n" );
                exit( 1 );
            }

            entry = entry->nextInRow();
        }
    }

    unsigned countActiveColumns() const
    {
        unsigned result = 0;

        for ( unsigned i = 0; i < _columnSize.size(); ++i )
        {
            if ( _columnSize.get( i ) > 0 )
                result += 1;
        }

        return result;
    }

private:
    unsigned _size;
    Entry **_rows;
    Entry **_columns;
    Vector<unsigned> _rowSize;
    Vector<unsigned> _columnSize;
    Map<unsigned, Entry *> _denseMap;

    void dumpDenseMap()
    {
        printf( "Dumping dense map (size = %u):\n", _denseMap.size() );
        Map<unsigned, Entry *>::iterator it = _denseMap.begin();
        while ( it != _denseMap.end() )
        {
            printf( "\t%u: %.5lf (address: 0x%p)\n", it->first, it->second->getValue(), it->second );
            ++it;
        }
    }
};

#endif // __Tableau_h__

//
// Local Variables:
// compile-command: "make -C . "
// tags-file-name: "./TAGS"
// c-basic-offset: 4
// End:
//
