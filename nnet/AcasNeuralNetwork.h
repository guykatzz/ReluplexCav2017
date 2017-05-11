/*********************                                                        */
/*! \file AcasNeuralNetwork.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** (in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __AcasNeuralNetwork_h__
#define __AcasNeuralNetwork_h__

#include "String.h"
#include "nnet.h"
#include <cassert>
#include <iomanip>
#include <iostream>
#include <sstream>

class AcasNeuralNetwork
{
public:
    AcasNeuralNetwork( const String &path ) : _network( NULL )
    {
        _network = load_network( path.ascii() );
    }

    ~AcasNeuralNetwork()
    {
        if ( _network )
        {
            destroy_network( _network );
            _network = NULL;
        }
    }

    double getWeight( int sourceLayer, int sourceNeuron, int targetNeuron )
    {
        return _network->matrix[sourceLayer][0][targetNeuron][sourceNeuron];
    }

    static String doubleToString( double x )
    {
        enum {
            MAX_PRECISION = 6,
        };

        std::ostringstream strout;
        strout << std::fixed << std::setprecision(MAX_PRECISION) << x;
        std::string str = strout.str();
        size_t end = str.find_last_not_of( '0' ) + 1;
        str.erase( end );

        if ( str[str.size() - 1] == '.' )
            str = str.substr(0, str.size() - 1);

        return str;
    }

    String getWeightAsString( int sourceLayer, int sourceNeuron, int targetNeuron )
    {
        double weight = getWeight( sourceLayer, sourceNeuron, targetNeuron );
        return doubleToString( weight );
    }

    String getBiasAsString( int layer, int neuron )
    {
        double bias = getBias( layer, neuron );
        return doubleToString( bias );
    }

    double getBias( int layer, int neuron )
    {
        // The bias for layer i is in index i-1 in the array.
        assert( layer > 0 );
        return _network->matrix[layer - 1][1][neuron][0];
    }

    int getNumLayers() const
    {
        return _network->numLayers;
    }

    unsigned getLayerSize( unsigned layer )
    {
        return (unsigned)_network->layerSizes[layer];
    }

    void evaluate( const Vector<double> &inputs, Vector<double> &outputs, unsigned outputSize ) const
    {
        double input[inputs.size()];
        double output[outputSize];

        memset( input, 0, num_inputs( _network ) );
        memset( output, 0, num_outputs( _network ) );

        for ( unsigned i = 0; i < inputs.size();  ++i )
            input[i] = inputs.get( i );

        bool normalizeInput = false;
        bool normalizeOutput = false;

        if ( evaluate_network( _network, input, output, normalizeInput, normalizeOutput ) != 1 )
        {
            std::cout << "Error! Network evaluation failed" << std::endl;
            exit( 1 );
        }

        for ( unsigned i = 0; i < outputSize; ++i )
        {
            outputs.append( output[i] );
        }
    }

    NNet *_network;
};

#endif // __AcasNeuralNetwork_h__

//
// Local Variables:
// c-basic-offset: 4
// End:
//
