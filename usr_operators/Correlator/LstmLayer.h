/*
 * LstmLayer.h
 *
 *  Created on: Oct 27, 2010
 *      Author: koutnij
 */
#include <vector>
#include <math.h>

#include "LstmBlock.h"

#ifndef LSTMLAYER_H_
#define LSTMLAYER_H_

class LstmLayer {

public:
	LstmLayer(int nCells, int nInputs, int nOutputs, int nBlocks);
	void forwardPassStep(int t, std::vector <std::vector<LstmBlockState> >& state,  std::vector<std::vector<double> >& x);
	void backwardPassStep(int t, std::vector <std::vector<LstmBlockState> >& state, std::vector< std::vector<double> >& wK,
			std::vector<double>& dk, std::vector<std::vector<double> >& x);
	void getBlockOutputs(int t, std::vector <std::vector<LstmBlockState> >& state, std::vector<double>& b);
	void updateWeights(double eta, double alpha);
	void setConstantWeights(double w);
	void setRandomWeights(double halfRange);

	void resetDerivs();

	void printWeights();

	~LstmLayer();

	std::vector <LstmBlock> lstmBlock; // lstm blocks
private:

};

#endif /* LSTMLAYER_H_ */
