/*
 * CorrelatorCore.h
 *
 *  Created on: Nov 15, 2010
 *      Author: koutnij
 */

#ifndef CORRELATORCORE_H_
#define CORRELATORCORE_H_


#include <vector>
#include <iostream>
#include <numeric>
#include <string>

#include "CorrelatorCore.h"
#include "LstmLayer.h"
#include "ForwardLayer.h"
#include "LstmNetwork.h"
#include "VectorMath.h"

class CorrelatorCore {
public:
	std::vector< std::vector<double> > inputSequenceBuffer;
	std::vector< std::vector<LstmBlockState> > lstmStateBuffer;
	std::vector<ForwardLayerState> outputLayerStateBuffer;
	std::vector<std::vector<double> > trainingSequenceBuffer;
	std::vector<std::vector<double> > inputLayerErrorBuffer;
	std::vector<std::vector<double> > outputErrorBuffer;

	std::vector< std::vector<double> > inputSequenceBufferSnapshot;
	std::vector< std::vector<LstmBlockState> > lstmStateBufferSnapshot;
	std::vector<ForwardLayerState> outputLayerStateBufferSnapshot;
	std::vector<std::vector<double> > trainingSequenceBufferSnapshot;
	std::vector<std::vector<double> > inputLayerErrorBufferSnapshot;
	std::vector<std::vector<double> > outputErrorBufferSnapshot;

	int nCells, nBlocks, nInputs, nOutputs;
	int buffersLength;

	LstmNetwork* lstmNetwork;

	CorrelatorCore();

	//int nCells=1,nBlocks=2,nInputs=3,nOutputs=3,nX=10,dimX=3;
	void initializeOneStepPrediction(int nC, int nB, std::vector<std::vector <double> >& dataSequence);
	void appendBuffers(std::vector<std::vector <double> >& dataSequence);
	void forwardPass();
	void backwardPass();
	double trainingEpoch(double learningRate, double momentum);
	void snapshot(int t1, int t2);
	void getJacobian(int t1, int t2, std::vector<std::vector <double> >& jacobian);
	void readMatrix(std::vector<std::vector<double> >& mat, int lineLength);
	~CorrelatorCore();
};

#endif /* CORRELATORCORE_H_ */
