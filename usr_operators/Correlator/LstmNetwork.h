/*
 * LstmNetwork.h
 *
 *  Created on: Oct 27, 2010
 *      Author: koutnij
 */

#include <vector>

#include "LstmBlock.h"
#include "LstmLayer.h"
#include "ForwardLayer.h"

#ifndef LSTMNETWORK_H_
#define LSTMNETWORK_H_

class LstmNetwork {
public:
	LstmNetwork(int nCells, int nInputs, int nOutputs, int nBlocks);
	void forwardPassStep(int t, std::vector <std::vector<LstmBlockState> >& lstmLayerState,
			        std::vector<ForwardLayerState>& outputLayerState, std::vector<std::vector<double> >& x);
	void forwardPass(int t1, int t2,
			std::vector <std::vector<LstmBlockState> >& lstmLayerState,
			std::vector<ForwardLayerState>& outputLayerState, std::vector<std::vector<double> >& x);
	void backwardPassStep(int t, std::vector <std::vector<LstmBlockState> >& lstmLayerState,
	        std::vector<ForwardLayerState>& outputLayerState, std::vector<std::vector<double> >& er,
	        std::vector<std::vector <double> >& inputErrorBuffer, std::vector<std::vector<double> >& x);
	void backwardPass(int t1, int t2,
			std::vector <std::vector<LstmBlockState> >& lstmLayerState,
			std::vector<ForwardLayerState>& outputLayerState, std::vector<std::vector<double> >& er,
			std::vector<std::vector <double> >& inputErrorBuffer, std::vector<std::vector<double> >& x);
	void updateWeights(double eta, double alpha);
	void resetDerivs();
	void setConstantWeights(double w);
	void setRandomWeights(double halfRange);

	void printWeights();
	void printSerializedWeights(int lineLength);
	void printSerializedDerivs(int lineLength);
	void setSerializedWeights(std::vector<double>& w);

	~LstmNetwork();

	LstmLayer* lstmLayer;
	ForwardLayer* outputLayer;

private:

};

#endif /* LSTMNETWORK_H_ */
