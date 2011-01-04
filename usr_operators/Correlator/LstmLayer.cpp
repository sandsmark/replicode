/*
 * LstmLayer.cpp
 *
 *  Created on: Oct 27, 2010
 *      Author: koutnij
 */
#include <vector>
#include <math.h>
#include <iostream>

#include "LstmLayer.h"
#include "LstmBlock.h"

LstmLayer::LstmLayer(int nCells, int nInputs, int nOutputs, int nBlocks) {
	lstmBlock.assign(nBlocks, LstmBlock(nCells,nInputs,nOutputs,nBlocks));
}

void LstmLayer::getBlockOutputs(int t, std::vector <std::vector<LstmBlockState> >& state, std::vector<double>& b){
	int nCells = state[0][0].bc.size();
	for (int i=0;i<(int)state.size();i++){
		for (int j=0;j<nCells;j++){
			if(t >= 0 && t<state[0].size()){ // the time is explicitly given from the calling method
				b[i*nCells+j] = state[i][t].bc[j];
			}
		}
	}

}

void LstmLayer::forwardPassStep(int t, std::vector <std::vector<LstmBlockState> >& state, std::vector<std::vector<double> >& x){
	int nCells = state[0][0].bc.size();
	std::vector<double> b (state.size()*nCells, 0.);
	getBlockOutputs(t - 1, state, b);

	for (int i=0;i<(int)state.size();i++){
		lstmBlock[i].forwardPassStep(t, state[i], x[t], b);
	}
}

void LstmLayer::backwardPassStep(int t, std::vector <std::vector<LstmBlockState> >& state,
								 std::vector< std::vector<double> >& wK,
								 std::vector<double>& dk, std::vector<std::vector<double> >& x){
	int nCells = state[0][0].bc.size();
	int nBlocks = state.size();
	int nOutputs = wK.size();

	std::vector<std::vector<std::vector<double> > > wKtrans(nBlocks, std::vector<std::vector<double> > (nCells, std::vector<double> (nOutputs)));
	for(int i=0;i<nBlocks;i++){
		for(int j=0;j<nCells;j++){
			for(int k=0;k<nOutputs;k++){
				wKtrans[i][j][k] = wK[k][i * nCells + j];
			}
		}
	}
	/*std::cout << "wKtrans = " << std::endl;
	for(int i=0;i<nBlocks;i++){
		for(int j=0;j<nCells;j++){
			for(int k=0;k<nOutputs;k++){
				std::cout << wKtrans[i][j][k] << " ";
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}*/


	std::vector<std::vector<std::vector<double> > > wHtrans(nBlocks, std::vector<std::vector<double> > (nCells, std::vector<double> (nBlocks * nCells)));
	for(int i=0;i<nBlocks;i++){
		for(int j=0;j<nCells;j++){
			for(int k=0;k<nCells*nBlocks;k++){
				wHtrans[i][j][k] = lstmBlock[k / nCells].wHc[k % nCells][i * nCells + j];
			}
		}
	}
	/*
	std::cout << "wHtrans = " << std::endl;
	for(int i=0;i<nBlocks;i++){
		for(int j=0;j<nCells;j++){
			for(int k=0;k<nCells*nBlocks;k++){
				std::cout << wHtrans[i][j][k] << " ";
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}*/

	std::vector<double> db(nBlocks*nCells, 0.);
	if(t < state[0].size() - 1){
		for(int i=0;i<nBlocks * nCells;i++){
			db[i]=state[i / nCells][t+1].dc[i % nCells];
		}
	}

	/*for(int i=0;i<db.size();i++){
		std::cout << db[i] << " ";
	}std::cout << std::endl;
	*/
	std::vector<double> gd(nBlocks * 3, 0.);
	std::vector<std::vector<std::vector<double> > > gw(nBlocks, std::vector<std::vector<double> > (nCells, std::vector<double> (nBlocks * 3)));
	if(t < state[0].size() - 1){
		for(int i=0;i<nBlocks;i++){
			gd[3 * i] = state[i][t+1].di;
			gd[3 * i + 1] = state[i][t+1].df;
			gd[3 * i + 2] = state[i][t+1].d_o;
		}
	}

	for(int i=0;i<nBlocks;i++){
		for(int j=0;j<nCells;j++){
			for(int k=0;k<nBlocks;k++){
				gw[i][j][3 * k] = lstmBlock[k].wHi[nCells * i + j];
				gw[i][j][3 * k+1] = lstmBlock[k].wHf[nCells * i + j];;
				gw[i][j][3 * k+2] = lstmBlock[k].wHo[nCells * i + j];;
			}
		}
	}

	// other block cells activations (needed for weight update)
	std::vector<double> b (state.size()*nCells, 0.);
	getBlockOutputs(t-1, state, b); // was t-1

	for (int i=0;i<(int)state.size();i++){
		lstmBlock[i].backwardPassStep(t, state[i], wKtrans[i], dk, wHtrans[i], db, gw[i], gd, x[t], b);  //fix
	}
}

void LstmLayer::updateWeights(double eta, double alpha){
	for(int i=0;i<lstmBlock.size();i++){
		lstmBlock[i].updateWeights(eta, alpha);
	}
}

void LstmLayer::resetDerivs(){
	for(int i=0;i<lstmBlock.size();i++){
		lstmBlock[i].resetDerivs();
	}
}

void LstmLayer::printWeights(){
	for(int i=0;i<lstmBlock.size();i++){
		std::cout << "Debug [LstmLayer, weights]"<< std::endl;
		lstmBlock[i].printWeights();
	}
}

void LstmLayer::setConstantWeights(double w){
	for(int i=0;i<lstmBlock.size();i++){
		lstmBlock[i].setConstantWeights(w);
	}
}

void LstmLayer::setRandomWeights(double halfRange){
	for(int i=0;i<lstmBlock.size();i++){
		lstmBlock[i].setRandomWeights(halfRange);
	}
}

LstmLayer::~LstmLayer() {

}
