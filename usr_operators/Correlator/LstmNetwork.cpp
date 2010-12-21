/*
 * LstmNetwork.cpp
 *
 *  Created on: Oct 27, 2010
 *      Author: koutnij
 */

#include <iostream>
#include "LstmNetwork.h"

//#define DEBUG 1

LstmNetwork::LstmNetwork(int nCells, int nInputs, int nOutputs, int nBlocks) {
	lstmLayer = new LstmLayer(nCells, nInputs, nOutputs, nBlocks);
	outputLayer = new ForwardLayer(nCells, nOutputs, nBlocks);
}


void LstmNetwork::forwardPassStep(int t, std::vector <std::vector<LstmBlockState> >& lstmLayerState,
	std::vector<ForwardLayerState>& outputLayerState, std::vector<std::vector<double> >& x){

	lstmLayer->forwardPassStep(t, lstmLayerState, x);
    // collect all LSTM outputs at time t, there should be a function or at least a nice iterator for that
	int nCells = lstmLayerState[0][0].bc.size();
	std::vector<double> b (lstmLayerState.size()*nCells, 0.);
	for (int i=0;i<(int)lstmLayerState.size();i++){
		for (int j=0;j<nCells;j++){
			b[i*nCells+j] = lstmLayerState[i][t].bc[j];
		}
	}
	outputLayer->forwardPassStep(t, outputLayerState, b);
}

// forwardPass operates with indices rather than with iterators
void LstmNetwork::forwardPass(int t1, int t2,
	std::vector <std::vector<LstmBlockState> >& lstmLayerState,
	std::vector<ForwardLayerState>& outputLayerState, std::vector<std::vector<double> >& x){

#ifdef DEBUG
	std::cout << "Debug [LstmNetwork::forwardPass, t1 = " << t1 << ", t2 = "<< t2 <<"]" << std::endl;
#endif

	for(int t = t1; t < t2;t++){
		forwardPassStep(t, lstmLayerState, outputLayerState, x);
	}
}

void LstmNetwork::backwardPassStep(int t, std::vector <std::vector<LstmBlockState> >& lstmLayerState,
        std::vector<ForwardLayerState>& outputLayerState, std::vector<std::vector<double> >& er,
        std::vector<std::vector <double> >& inputErrorBuffer, std::vector<std::vector<double> >& x){

	int nCells = lstmLayerState[0][0].bc.size();
	std::vector<double> b (lstmLayerState.size()*nCells, 0.);
	lstmLayer->getBlockOutputs(t, lstmLayerState, b);
	//for(int i=0;i<b.size();i++)std::cout << b[i] << ": ";
	//std::cout << lstmLayerState[0][0].bc[0] << std::endl;
	outputLayer->backwardPassStep(t, outputLayerState, b, er);
	lstmLayer->backwardPassStep(t, lstmLayerState, outputLayer->wK, outputLayerState[t].dk, x); // was outputLayerState[t+1].bk - was wrong
	// input layer error calculation
#ifdef DEBUG
		std::cout <<  "Debug [LstmNetwork::backwardPassStep, input layer Error, t = " << t << "]" << std::endl;
#endif
	for(int i=0;i<inputErrorBuffer[0].size();i++){
		inputErrorBuffer[t][i] = 0.;
		for(int j=0;j<lstmLayer->lstmBlock.size();j++){
			inputErrorBuffer[t][i] += lstmLayer->lstmBlock[j].wIi[i] * lstmLayerState[j][t].di;
			//std::cout << lstmLayer->lstmBlock[j].wIi[i] << " * " << lstmLayerState[j][t].di << std::endl;
			inputErrorBuffer[t][i] += lstmLayer->lstmBlock[j].wIf[i] * lstmLayerState[j][t].df;
			//std::cout << lstmLayer->lstmBlock[j].wIf[i] << " * " << lstmLayerState[j][t].df << std::endl;
			inputErrorBuffer[t][i] += lstmLayer->lstmBlock[j].wIo[i] * lstmLayerState[j][t].d_o;
			//std::cout << lstmLayer->lstmBlock[j].wIo[i] << " * " << lstmLayerState[j][t].d_o << std::endl;

			for(int k=0;k<lstmLayerState[j][t].dc.size();k++){
				inputErrorBuffer[t][i] += lstmLayer->lstmBlock[j].wIc[k][i] * lstmLayerState[j][t].dc[k];
				//std::cout << lstmLayer->lstmBlock[j].wIc[k][i] << " * " <<  lstmLayerState[j][t].dc[k] << std::endl;
			}
		}
		//std::cout << "INPUT RESULT " <<  inputErrorBuffer[t][i] << std::endl;
#ifdef DEBUG
		std::cout << inputErrorBuffer[t][i] << " ";
#endif
	}
#ifdef DEBUG
		std::cout << std::endl;
#endif
}

// t1 < t2
void LstmNetwork::backwardPass(int t1, int t2,
		std::vector <std::vector<LstmBlockState> >& lstmLayerState,
		std::vector<ForwardLayerState>& outputLayerState, std::vector<std::vector<double> >& er,
		std::vector<std::vector <double> >& inputErrorBuffer, std::vector<std::vector<double> >& x){

#ifdef DEBUG
	std::cout << "Debug [LstmNetwork::backwardPass, t1 = " << t1 << ", t2 = "<< t2 <<"]" << std::endl;
#endif

	for(int t = t2 - 1 ;t>=t1;t--){
		backwardPassStep(t,lstmLayerState,outputLayerState, er, inputErrorBuffer, x);
	}
}

void LstmNetwork::updateWeights(double eta, double alpha){
	outputLayer->updateWeights(eta, alpha);
	lstmLayer->updateWeights(eta, alpha);
}

void LstmNetwork::resetDerivs(){
	outputLayer->resetDerivs();
	lstmLayer->resetDerivs();
}

void LstmNetwork::printWeights(){
	outputLayer->printWeights();
	lstmLayer->printWeights();
}

void LstmNetwork::printSerializedWeights(int lineLength){
	std::vector<double> w;
	for(int i=0;i<lstmLayer->lstmBlock.size();i++){
		lstmLayer->lstmBlock[i].getSerializedWeights1(w);
	}
	outputLayer->getSerializedWeights(w);
	for(int i=0;i<lstmLayer->lstmBlock.size();i++){
		lstmLayer->lstmBlock[i].getSerializedWeights2(w);
	}
	for(int i=0;i<w.size();i++){
		std::cout << w[i] << " ";
		if(i % lineLength == lineLength - 1) std::cout << std::endl;
	}
	std::cout << std::endl;
}

void LstmNetwork::printSerializedDerivs(int lineLength){
	std::vector<double> w;
	for(int i=0;i<lstmLayer->lstmBlock.size();i++){
		lstmLayer->lstmBlock[i].getSerializedDerivs1(w);
	}
	outputLayer->getSerializedDerivs(w);
	for(int i=0;i<lstmLayer->lstmBlock.size();i++){
		lstmLayer->lstmBlock[i].getSerializedDerivs2(w);
	}
	for(int i=0;i<w.size();i++){
		std::cout << w[i] << " ";
		if(i % lineLength == lineLength - 1) std::cout << std::endl;
	}
	std::cout << std::endl;
}

void LstmNetwork::setSerializedWeights(std::vector<double>& w){
	for(int i=0;i<lstmLayer->lstmBlock[0].wIi.size();i++){
		lstmLayer->lstmBlock[0].wIi[i] = w[i];
		lstmLayer->lstmBlock[0].wIf[i] = w[i+3];
		lstmLayer->lstmBlock[0].wIc[0][i] = w[i+6];
		lstmLayer->lstmBlock[0].wIo[i] = w[i+9];

	}
	lstmLayer->lstmBlock[0].wCi[0] = w[12];
	lstmLayer->lstmBlock[0].wCf[0] = w[13];
	lstmLayer->lstmBlock[0].wCo[0] = w[14];

	lstmLayer->lstmBlock[0].wHi[0] = w[18];
	lstmLayer->lstmBlock[0].wHf[0] = w[19];
	lstmLayer->lstmBlock[0].wHo[0] = w[21];

	lstmLayer->lstmBlock[0].wHc[0][0] = w[20];

	for(int i=0;i<outputLayer->wK.size();i++){
		outputLayer->wK[i][0] = w[i + 15];
	}

}

void LstmNetwork::setConstantWeights(double w){
	outputLayer->setConstantWeights(w);
	lstmLayer->setConstantWeights(w);
}

void LstmNetwork::setRandomWeights(double halfRange){
	outputLayer->setRandomWeights(halfRange);
	lstmLayer->setRandomWeights(halfRange);
}

LstmNetwork::~LstmNetwork() {
	delete lstmLayer;
	delete outputLayer;
}
