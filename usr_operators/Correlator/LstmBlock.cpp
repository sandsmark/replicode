/*
 * LstmBlock.cpp
 *
 *  Created on: Oct 26, 2010
 *      Author: koutnij
 */
#include <vector>
#include <iostream>

#include "LstmBlock.h"
#include "VectorMath.h"
#include <numeric>

//#define DEBUG 1
//#define DEBUG_FWD 1


LstmBlock::LstmBlock(int nCells, int nInputs, int nOutputs, int nBlocks){
	// input gate weights
	wIi.assign(nInputs, 0.);
	wHi.assign(nBlocks*nCells, 0.);
	wCi.assign(nCells,  0.);
	wIid.assign(nInputs, 0.);
	wHid.assign(nBlocks*nCells, 0.);
	wCid.assign(nCells,  0.);
	wIim.assign(nInputs, 0.);
	wHim.assign(nBlocks*nCells, 0.);
	wCim.assign(nCells,  0.);

	// forget gate weights
	wIf.assign(nInputs, 0.);
	wHf.assign(nBlocks*nCells, 0.);
	wCf.assign(nCells,  0.);
	wIfd.assign(nInputs, 0.);
	wHfd.assign(nBlocks*nCells, 0.);
	wCfd.assign(nCells,  0.);
	wIfm.assign(nInputs, 0.);
	wHfm.assign(nBlocks*nCells, 0.);
	wCfm.assign(nCells,  0.);

	// output gate weights
	wIo.assign(nInputs, 0.);
	wHo.assign(nBlocks*nCells, 0.);
	wCo.assign(nCells,  0.);
	wIod.assign(nInputs, 0.);
	wHod.assign(nBlocks*nCells, 0.);
	wCod.assign(nCells,  0.);
	wIom.assign(nInputs, 0.);
	wHom.assign(nBlocks*nCells, 0.);
	wCom.assign(nCells,  0.);

	// cell weights
	std::vector <double> tmp (nInputs, 0.);
	wIc.assign(nCells, tmp);
	wIcd.assign(nCells, tmp);
	wIcm.assign(nCells, tmp);

	tmp.assign(nBlocks*nCells, 0.);
	wHc.assign(nCells, tmp);
	wHcd.assign(nCells, tmp);
	wHcm.assign(nCells, tmp);

	// bias
	biasI=biasF=biasO=biasId=biasFd=biasOd=biasIm=biasFm=biasOm=0.;
	biasC.assign(nCells, 0.);
	biasCd.assign(nCells, 0.);
	biasCm.assign(nCells, 0.);
}

void LstmBlock::setConstantWeights(int nCells, int nInputs, int nOutputs, int nBlocks, double w){
	// input gate weights
	wIi.assign(nInputs, w);
	wHi.assign(nBlocks*nCells, w);
	wCi.assign(nCells,  w);
    // forget gate weights
	wIf.assign(nInputs, w);
	wHf.assign(nBlocks*nCells, w);
	wCf.assign(nCells,  w);
	// output gate weights
	wIo.assign(nInputs, w);
	wHo.assign(nBlocks*nCells, w);
	wCo.assign(nCells,  w);
	// cell weights
	std::vector <double> tmp (nInputs, w); wIc.assign(nCells, tmp);
	tmp.assign(nBlocks*nCells, w); wHc.assign(nCells, tmp);
	// bias
	biasI=biasF=biasO=w;
	biasC.assign(nCells, w);

}

void LstmBlock::setConstantWeights(double w){
	for(int i=0;i<wIi.size();i++){
		wIi[i]=w;
		wIf[i]=w;
		wIo[i]=w;
	}
	for(int i=0;i<wHi.size();i++){
		wHi[i]=w;
		wHf[i]=w;
		wHo[i]=w;
	}
	for(int i=0;i<wCi.size();i++){
		wCi[i]=w;
		wCf[i]=w;
		wCo[i]=w;
	}
	for(int i=0;i<wIc.size();i++){
		for(int j=0;j<wIc[0].size();j++){
			wIc[i][j] = w;
		}
		for(int j=0;j<wHc[0].size();j++){
			wHc[i][j] = w;
		}
		biasC[i] = w;
	}

	biasI=w;
	biasF=w;
	biasO=w;

}

void LstmBlock::setRandomWeights(double halfRange){

	for(int i=0;i<wIi.size();i++){
		wIi[i]=randomW(halfRange);
		wIf[i]=randomW(halfRange);
		wIo[i]=randomW(halfRange);
	}
	for(int i=0;i<wHi.size();i++){
		wHi[i]=randomW(halfRange);
		wHf[i]=randomW(halfRange);
		wHo[i]=randomW(halfRange);
	}
	for(int i=0;i<wCi.size();i++){
		wCi[i]=randomW(halfRange);
		wCf[i]=randomW(halfRange);
		wCo[i]=randomW(halfRange);
	}
	for(int i=0;i<wIc.size();i++){
		for(int j=0;j<wIc[0].size();j++){
			wIc[i][j] = randomW(halfRange);
		}
		for(int j=0;j<wHc[0].size();j++){
			wHc[i][j] = randomW(halfRange);
		}
		biasC[i] = randomW(halfRange);
	}

	biasI=randomW(halfRange);
	biasF=randomW(halfRange);
	biasO=randomW(halfRange);

}

// computes single forward pass step of an LSTM memory block, operates on a vector of state structs
void LstmBlock::forwardPassStep(int t, std::vector<LstmBlockState>& state,std::vector<double>& x, std::vector<double>& b){
	// input gate activation
	state[t].ai = inner_product(wIi.begin(), wIi.end(), x.begin(), 0.)
			    + inner_product(wHi.begin(), wHi.end(), b.begin(), 0.);
	if(t > 0) {
		state[t].ai += inner_product(wCi.begin(), wCi.end(), state[t-1].sc.begin(), 0.);
	}
	state[t].ai += biasI; // bias


	state[t].bi = fnF(state[t].ai);
	// forget gate activation
	state[t].af = inner_product(wIf.begin(), wIf.end(), x.begin(), 0.)
			    + inner_product(wHf.begin(), wHf.end(), b.begin(), 0.);
	if(t > 0) {
		state[t].af += inner_product(wCf.begin(), wCf.end(), state[t-1].sc.begin(), 0.);
	}
	state[t].af += biasF; // bias
	state[t].bf = fnF(state[t].af);
	// ac.assign(nCells, 0.);
	for(int i=0;i<(int)state[t].sc.size();i++){
		state[t].ac[i] = inner_product(wIc[i].begin(), wIc[i].end(), x.begin(), 0.)
		      + inner_product(wHc[i].begin(), wHc[i].end(), b.begin(), 0.); // mem. block input activation
		state[t].ac[i] += biasC[i]; // bias
		// new cell states for time t
		state[t].sc[i] = state[t].bi * fnG(state[t].ac[i]);
		if(t > 0){
			state[t].sc[i] += state[t].bf * state[t-1].sc[i];
		}
	}
	state[t].ao = inner_product(wIo.begin(), wIo.end(), x.begin(), 0.)
			    + inner_product(wHo.begin(), wHo.end(), b.begin(), 0.)
			    + inner_product(wCo.begin(), wCo.end(), state[t].sc.begin(), 0.); // output gate activation
	state[t].ao += biasO; // bias
	state[t].bo = fnF(state[t].ao);
	for(int i=0;i<(int)state[t].sc.size();i++){
		state[t].bc[i] = state[t].bo * fnH(state[t].sc[i]); // cell outputs (memory block block output)
	}

	#ifdef DEBUG_FWD
	std::cout << "Debug [LstmBlock, forward pass, sc, t = "<< t <<" ]:" << std::endl;
	for(int i=0;i<state[t].sc.size();i++){
		std::cout << state[t].sc[i] << " ";
	} std::cout << std::endl;
	std::cout << "Debug [LstmBlock, forward pass, bc, t = "<< t <<" ]:" << std::endl;
	for(int i=0;i<state[t].bc.size();i++){
		std::cout << state[t].bc[i] << " ";
	} std::cout << std::endl;
	std::cout << "Debug [LstmBlock Gates, forward pass, bi, bf, bo, t = "<< t <<" ]:" << std::endl;
	std::cout << state[t].ai << " " << state[t].af << " " << state[t].ao << " " << std::endl;
	#endif

}

// the wK matrix is a corresponding columns from wK matrix stored in ForwardLayer organized in a matrix by rows
// the wH matrix is a part of transposed matrix (columns as rows) for the source cells (recurrent connections)
// dk is a vector of deltas from output layer, db is vector of deltas of all cells (not squashed) in time t+1
void LstmBlock::backwardPassStep(int t, std::vector<LstmBlockState>& state, std::vector< std::vector<double> >& wK,
										std::vector<double>& dk, std::vector< std::vector<double> >& wH, std::vector<double>& db,
										std::vector< std::vector<double> >& gw, std::vector<double>& gd, std::vector<double>& x, std::vector<double>& b){
#ifdef DEBUG
	std::cout << "Debug [LstmBlock, backward pass, transposed wK]:" << std::endl;
	for(int i=0;i<wK.size();i++){
		for(int j=0;j<wK[0].size();j++){
			std::cout << wK[i][j] << " ";
		} std::cout << std::endl;
	}
	std::cout << "Debug [LstmBlock, backward pass, dk, t = "<< t <<" ]:" << std::endl;
	for(int i=0;i<dk.size();i++){
		std::cout << dk[i] << " ";
	} std::cout << std::endl;
	std::cout << "Debug [LstmBlock, backward pass, transposed wH]:" << std::endl;
	for(int i=0;i<wH.size();i++){
		for(int j=0;j<wH[0].size();j++){
			std::cout << wH[i][j] << " ";
		} std::cout << std::endl;
	}
	std::cout << "Debug [LstmBlock, backward pass, db, t = "<< t <<" ]:" << std::endl;
	for(int i=0;i<db.size();i++){
		std::cout << db[i] << " ";
	} std::cout << std::endl;

#endif
	state[t].di=0; state[t].df=0; state[t].d_o=0; // epochal bugfix :)
	for(int i=0;i<(int)state[t].ec.size();i++){
		state[t].ec[i] = inner_product(wK[i].begin(), wK[i].end(), dk.begin(), 0.) + // error propagated from outputs
						 inner_product(wH[i].begin(), wH[i].end(), db.begin(), 0.); // error propagated through recurrent connections from the cells
		// std::cout << "ec without gate errors = "<< state[t].ec[i] << std::endl;
		// std::cout << "gate deltas = " << state[t].di << " "<<state[t].df<<" "<<state[t].d_o<< std::endl;
		if(t < state.size() -1 ){
			state[t].ec[i] += inner_product(gw[i].begin(), gw[i].end(), gd.begin(), 0.); // this is missing in Alex's equations (4.12)
		}
		state[t].d_o += fnH(state[t].sc[i]) * state[t].ec[i]; // accumulate errors in states
	}
	//std::cout << std::endl;
	state[t].d_o *= fnFd(state[t].ao); // output gate delta
	for(int i=0;i<(int)state[t].es.size();i++){
		state[t].es[i] = state[t].bo * fnHd(state[t].sc[i]) * state[t].ec[i] + wCo[i] * state[t].d_o;
		if (t < state.size()-1)	{
			state[t].es[i] += state[t+1].bf * state[t+1].es[i] + wCi[i] * state[t+1].di + wCf[i] * state[t+1].df;
		}
		state[t].dc[i] = state[t].bi * fnGd(state[t].ac[i]) * state[t].es[i];
	}
	if(t > 0){
		state[t].df = fnFd(state[t].af) * inner_product(state[t-1].sc.begin(), state[t-1].sc.end(), state[t].es.begin(), 0.);
	}else{
		state[t].df = 0.;
	}
	for(int i=0;i<(int)state[t].es.size();i++){
		state[t].di += fnG(state[t].ac[i]) * state[t].es[i]; // accumulate  errors for input gate
	}
	state[t].di *= fnFd(state[t].ai);

	#ifdef DEBUG
	std::cout << "Debug [LstmBlock, backward pass, ec, t = "<< t <<" ]:" << std::endl;
	for(int i=0;i<state[t].ec.size();i++){
		std::cout << state[t].ec[i] << " ";
	} std::cout << std::endl;
	std::cout << "Debug [LstmBlock, backward pass, es, t = "<< t <<" ]:" << std::endl;
	for(int i=0;i<state[t].es.size();i++){
		std::cout << state[t].es[i] << " ";
	} std::cout << std::endl;
	std::cout << "Debug [LstmBlock, backward pass, dc, t = "<< t <<" ]:" << std::endl;
	for(int i=0;i<state[t].dc.size();i++){
		std::cout << state[t].dc[i] << " ";
	} std::cout << std::endl;
	std::cout << "Debug [LstmBlock Gates, backward pass, di, df, do, t = "<< t <<" ]:" << std::endl;
	std::cout << state[t].di << " " << state[t].df << " " << state[t].d_o << " " << std::endl;
	#endif
	//std::cout << "Debug [LstmBlock Gates, backward pass, di, df, do, t = "<< t <<" ]:" << std::endl;
	//std::cout << state[t].di << " " << state[t].df << " " << state[t].d_o << " " << std::endl;
	#ifdef DEBUG
		std::cout << "Debug [LstmBlock Gates, backward pass, weight derivatives, t = "<< t <<" ]:" << std::endl;
	#endif
	// update weight derivatives
//	std::cout << "--- weight derivatives in the beginning of the training --- " << std::endl;
//	this->printWeights();
//	std::cout << "--- weight derivatives in the beginning of the training --- " << std::endl;

	for(int i=0;i<x.size();i++){
		wIid[i] += state[t].di * x[i];
		wIfd[i] += state[t].df * x[i];
		wIod[i] += state[t].d_o * x[i];
		for(int j=0;j<state[t].dc.size();j++){
			wIcd[j][i] += state[t].dc[j] * x[i];
		}
	}

	// bias weigths
	biasId += state[t].di;
	biasFd += state[t].df;
	biasOd += state[t].d_o;

	for(int i=0;i<biasCd.size();i++){
		biasCd[i] +=state[t].dc[i];
	}

	#ifdef DEBUG
		std::cout << "Debug x -> gate derivatives (wIid, wIfd, wIod, wIcd[2D array]): " << std::endl;
		for(int i=0;i<x.size();i++){ std::cout << wIid[i] << " "; } std::cout << std::endl;
		for(int i=0;i<x.size();i++){ std::cout << wIfd[i] << " "; } std::cout << std::endl;
		for(int i=0;i<x.size();i++){ std::cout << wIod[i] << " "; } std::cout << std::endl;
		for(int i=0;i<wIcd.size();i++){
			for(int j=0;j<wIcd[0].size();j++){
				std::cout << wIcd[i][j] << " ";
			}
			std::cout << std::endl;
		}
	#endif
	for(int i=0;i<b.size();i++){
		if(t<state.size()){
			wHid[i] += state[t].di * b[i];
			wHfd[i] += state[t].df * b[i];
			wHod[i] += state[t].d_o * b[i];
			//std::cout << "XXX wHod = " << state[t].d_o << " * " << b[i] << std::endl;
			for(int j=0; j< state[t].dc.size(); j++){
				wHcd[j][i] += state[t].dc[j] * b[i];
			}
		}
	}
	#ifdef DEBUG
		std::cout << "Debug b -> gate derivatives (wHid, wHfd, wHod): " << std::endl;
		for(int i=0;i<b.size();i++){ std::cout << wHid[i] << " "; } std::cout << std::endl;
		for(int i=0;i<b.size();i++){ std::cout << wHfd[i] << " "; } std::cout << std::endl;
		for(int i=0;i<b.size();i++){ std::cout << wHod[i] << " "; } std::cout << std::endl;
		for(int i=0;i<wHcd.size();i++){
			for(int j=0;j<wHcd[0].size();j++){
				std::cout << wHcd[i][j] << " ";
			}
			std::cout << std::endl;
		}
	#endif


	for(int i=0;i<state[t].sc.size();i++){
		if(t>0){
			wCid[i] += state[t].di * state[t-1].sc[i];
			wCfd[i] += state[t].df * state[t-1].sc[i];
		}
		wCod[i] += state[t].d_o * state[t].sc[i];
	}
	#ifdef DEBUG
		std::cout << "Peephole derivatives (wCid, wCfd, wCod): " << std::endl;
		for(int i=0;i<state[t].sc.size();i++){ std::cout << wCid[i] << " "; } std::cout << std::endl;
		for(int i=0;i<state[t].sc.size();i++){ std::cout << wCfd[i] << " "; } std::cout << std::endl;
		for(int i=0;i<state[t].sc.size();i++){ std::cout << wCod[i] << " "; } std::cout << std::endl;
	#endif


}



void LstmBlock::updateWeights(double eta, double alpha){
	double nCells = wCid.size();

	// weights from inputs
	scalar_multiple(eta, wIid.begin(), wIid.end(), wIid.begin());
	scalar_multiple(alpha, wIim.begin(), wIim.end(), wIim.begin());
	vector_add(wIim.begin(), wIim.end(), wIid.begin());
	vector_add(wIi.begin(), wIi.end(), wIim.begin());

	scalar_multiple(eta, wIfd.begin(), wIfd.end(), wIfd.begin());
	scalar_multiple(alpha, wIfm.begin(), wIfm.end(), wIfm.begin());
	vector_add(wIfm.begin(), wIfm.end(), wIfd.begin());
	vector_add(wIf.begin(), wIf.end(), wIfm.begin());

	scalar_multiple(eta, wIod.begin(), wIod.end(), wIod.begin());
	scalar_multiple(alpha, wIom.begin(), wIom.end(), wIom.begin());
	vector_add(wIom.begin(), wIom.end(), wIod.begin());
	vector_add(wIo.begin(), wIo.end(), wIom.begin());

	// peephole weights
	scalar_multiple(eta, wCid.begin(), wCid.end(), wCid.begin());
	scalar_multiple(alpha, wCim.begin(), wCim.end(), wCim.begin());
	vector_add(wCim.begin(), wCim.end(), wCid.begin());
	vector_add(wCi.begin(), wCi.end(), wCim.begin());

	scalar_multiple(eta, wCfd.begin(), wCfd.end(), wCfd.begin());
	scalar_multiple(alpha, wCfm.begin(), wCfm.end(), wCfm.begin());
	vector_add(wCfm.begin(), wCfm.end(), wCfd.begin());
	vector_add(wCf.begin(), wCf.end(), wCfm.begin());

	scalar_multiple(eta, wCod.begin(), wCod.end(), wCod.begin());
	scalar_multiple(alpha, wCom.begin(), wCom.end(), wCom.begin());
	vector_add(wCom.begin(), wCom.end(), wCod.begin());
	vector_add(wCo.begin(), wCo.end(), wCom.begin());

	// weights from other cells (recurrent connections)
	scalar_multiple(eta, wHid.begin(), wHid.end(), wHid.begin());
	scalar_multiple(alpha, wHim.begin(), wHim.end(), wHim.begin());
	vector_add(wHim.begin(), wHim.end(), wHid.begin());
	vector_add(wHi.begin(), wHi.end(), wHim.begin());

	scalar_multiple(eta, wHfd.begin(), wHfd.end(), wHfd.begin());
	scalar_multiple(alpha, wHfm.begin(), wHfm.end(), wHfm.begin());
	vector_add(wHfm.begin(), wHfm.end(), wHfd.begin());
	vector_add(wHf.begin(), wHf.end(), wHfm.begin());

	scalar_multiple(eta, wHod.begin(), wHod.end(), wHod.begin());
	scalar_multiple(alpha, wHom.begin(), wHom.end(), wHom.begin());
	vector_add(wHom.begin(), wHom.end(), wHod.begin());
	vector_add(wHo.begin(), wHo.end(), wHom.begin());

	// cells weights
	for(int i=0; i<nCells; i++){ // for
		scalar_multiple(eta, wIcd[i].begin(), wIcd[i].end(), wIcd[i].begin());
		scalar_multiple(alpha, wIcm[i].begin(), wIcm[i].end(), wIcm[i].begin());
		vector_add(wIcm[i].begin(), wIcm[i].end(), wIcd[i].begin());
		vector_add(wIc[i].begin(), wIc[i].end(), wIcm[i].begin());

		scalar_multiple(eta, wHcd[i].begin(), wHcd[i].end(), wHcd[i].begin());
		scalar_multiple(alpha, wHcm[i].begin(), wHcm[i].end(), wHcm[i].begin());
		vector_add(wHcm[i].begin(), wHcm[i].end(), wHcd[i].begin());
		vector_add(wHc[i].begin(), wHc[i].end(), wHcm[i].begin());
	}
	// bias
	biasIm = eta * biasId + alpha * biasIm;
	biasI += biasIm;
	biasFm = eta * biasFd + alpha * biasFm;
	biasF += biasFm;
	biasOm = eta * biasOd + alpha * biasOm;
	biasO += biasOm;

	scalar_multiple(eta, biasCd.begin(), biasCd.end(), biasCd.begin());
	scalar_multiple(alpha, biasCm.begin(), biasCm.end(), biasCm.begin());
	vector_add(biasCm.begin(), biasCm.end(), biasCd.begin()); // learning
	vector_add(biasC.begin(), biasC.end(), biasCm.begin()); // momentum

}

void LstmBlock::resetDerivs(){
	double nInputs = wIid.size();
	double nCells = wCid.size();
	double nBxnC = wHid.size();

	std::vector <double> tmpI(nInputs);
	std::vector <double> tmpC(nCells);
	std::vector <double> tmpH(nBxnC);

	//std::vector <double> tmpxx (5, 1.0);

	wIid.assign(tmpI.begin(), tmpI.end());
	wHid.assign(tmpH.begin(), tmpH.end());
	wCid.assign(tmpC.begin(), tmpC.end());

	wIfd.assign(tmpI.begin(), tmpI.end());
	wHfd.assign(tmpH.begin(), tmpH.end());
	wCfd.assign(tmpC.begin(), tmpC.end());

	wIod.assign(tmpI.begin(), tmpI.end());
	wHod.assign(tmpH.begin(), tmpH.end());
	wCod.assign(tmpC.begin(), tmpC.end());

	wIcd.assign(nCells, tmpI);
	wHcd.assign(nCells, tmpH);

	biasId=biasFd=biasOd=0.;
	for(int i=0;i<biasCd.size();i++){
		biasCd[i] = 0.;
	}
}

void LstmBlock::getSerializedWeights1(std::vector<double> & w){
	for(int i=0;i<wIi.size();i++){ w.push_back(wIi[i]); }
	for(int i=0;i<wIf.size();i++){ w.push_back(wIf[i]); }
	for(int i=0;i<wIc.size();i++){
		for(int j=0;j<wIc[i].size();j++){ w.push_back(wIc[i][j]); }
	}
	for(int i=0;i<wIo.size();i++){ w.push_back(wIo[i]); }
	for(int i=0;i<wCi.size();i++){ w.push_back(wCi[i]); }
	for(int i=0;i<wCf.size();i++){ w.push_back(wCf[i]); }
	for(int i=0;i<wCo.size();i++){ w.push_back(wCo[i]); }
}

void LstmBlock::getSerializedWeights2(std::vector<double> & w){
	for(int i=0;i<wHi.size();i++){ w.push_back(wHi[i]); }
	for(int i=0;i<wHf.size();i++){ w.push_back(wHf[i]); }
	for(int i=0;i<wIc.size();i++){
		for(int j=0;j<wHc[i].size();j++){ w.push_back(wHc[i][j]); }
	}
	for(int i=0;i<wHo.size();i++){ w.push_back(wHo[i]); }
}

void LstmBlock::getSerializedDerivs1(std::vector<double> & w){
	for(int i=0;i<wIid.size();i++){ w.push_back(wIid[i]); }
	for(int i=0;i<wIfd.size();i++){ w.push_back(wIfd[i]); }
	for(int i=0;i<wIcd.size();i++){
		for(int j=0;j<wIcd[i].size();j++){ w.push_back(wIcd[i][j]); }
	}
	for(int i=0;i<wIod.size();i++){ w.push_back(wIod[i]); }
	for(int i=0;i<wCid.size();i++){ w.push_back(wCid[i]); }
	for(int i=0;i<wCfd.size();i++){ w.push_back(wCfd[i]); }
	for(int i=0;i<wCod.size();i++){ w.push_back(wCod[i]); }
}

void LstmBlock::getSerializedDerivs2(std::vector<double> & w){
	for(int i=0;i<wHid.size();i++){ w.push_back(wHid[i]); }
	for(int i=0;i<wHfd.size();i++){ w.push_back(wHfd[i]); }
	for(int i=0;i<wIcd.size();i++){
		for(int j=0;j<wHcd[i].size();j++){ w.push_back(wHcd[i][j]); }
	}
	for(int i=0;i<wHod.size();i++){ w.push_back(wHod[i]); }
}

void LstmBlock::printWeights(){
	std::cout << "Debug [LstmBlock, wIi["<<wIi.size() <<"], wHi["<<wHi.size() <<"], wCi["<<wCi.size() <<"] ]"<< std::endl;
	for(int i=0;i<wIi.size();i++){ std::cout << wIi[i] << " "; }
	for(int i=0;i<wHi.size();i++){ std::cout << wHi[i] << " "; }
	for(int i=0;i<wCi.size();i++){ std::cout << wCi[i] << " "; } std::cout << std::endl;
	std::cout << "Debug [LstmBlock, wIid["<<wIid.size() <<"], wHid["<<wHi.size() <<"], wCid["<<wCi.size() <<"] ]"<< std::endl;
	for(int i=0;i<wIid.size();i++){ std::cout << wIid[i] << " "; }
	for(int i=0;i<wHid.size();i++){ std::cout << wHid[i] << " "; }
	for(int i=0;i<wCid.size();i++){ std::cout << wCid[i] << " "; } std::cout << std::endl;
	std::cout << "Debug [LstmBlock, wIim["<<wIim.size() <<"], wHim["<<wHi.size() <<"], wCim["<<wCi.size() <<"] ]"<< std::endl;
	for(int i=0;i<wIim.size();i++){ std::cout << wIim[i] << " "; }
	for(int i=0;i<wHim.size();i++){ std::cout << wHim[i] << " "; }
	for(int i=0;i<wCim.size();i++){ std::cout << wCim[i] << " "; } std::cout << std::endl;

	std::cout << "Debug [LstmBlock, wIf["<<wIi.size() <<"], wHf["<<wHi.size() <<"], wCf["<<wCi.size() <<"] ]"<< std::endl;
	for(int i=0;i<wIf.size();i++){ std::cout << wIf[i] << " "; }
	for(int i=0;i<wHf.size();i++){ std::cout << wHf[i] << " "; }
	for(int i=0;i<wCf.size();i++){ std::cout << wCf[i] << " "; } std::cout << std::endl;
	std::cout << "Debug [LstmBlock, wIfd["<<wIfd.size() <<"], wHfd["<<wHf.size() <<"], wCfd["<<wCf.size() <<"] ]"<< std::endl;
	for(int i=0;i<wIfd.size();i++){ std::cout << wIfd[i] << " "; }
	for(int i=0;i<wHfd.size();i++){ std::cout << wHfd[i] << " "; }
	for(int i=0;i<wCfd.size();i++){ std::cout << wCfd[i] << " "; } std::cout << std::endl;
	std::cout << "Debug [LstmBlock, wIfm["<<wIfm.size() <<"], wHim["<<wHf.size() <<"], wCfm["<<wCi.size() <<"] ]"<< std::endl;
	for(int i=0;i<wIfm.size();i++){ std::cout << wIfm[i] << " "; }
	for(int i=0;i<wHfm.size();i++){ std::cout << wHfm[i] << " "; }
	for(int i=0;i<wCfm.size();i++){ std::cout << wCfm[i] << " "; } std::cout << std::endl;

	std::cout << "Debug [LstmBlock, wIo["<<wIo.size() <<"], wHo["<<wHo.size() <<"], wCo["<<wCo.size() <<"] ]"<< std::endl;
	for(int i=0;i<wIo.size();i++){ std::cout << wIo[i] << " "; }
	for(int i=0;i<wHo.size();i++){ std::cout << wHo[i] << " "; }
	for(int i=0;i<wCo.size();i++){ std::cout << wCo[i] << " "; } std::cout << std::endl;
	std::cout << "Debug [LstmBlock, wIod["<<wIod.size() <<"], wHod["<<wHo.size() <<"], wCod["<<wCo.size() <<"] ]"<< std::endl;
	for(int i=0;i<wIod.size();i++){ std::cout << wIod[i] << " "; }
	for(int i=0;i<wHod.size();i++){ std::cout << wHod[i] << " "; }
	for(int i=0;i<wCod.size();i++){ std::cout << wCod[i] << " "; } std::cout << std::endl;
	std::cout << "Debug [LstmBlock, wIom["<<wIom.size() <<"], wHom["<<wHo.size() <<"], wCom["<<wCo.size() <<"] ]"<< std::endl;
	for(int i=0;i<wIom.size();i++){ std::cout << wIom[i] << " "; }
	for(int i=0;i<wHom.size();i++){ std::cout << wHom[i] << " "; }
	for(int i=0;i<wCom.size();i++){ std::cout << wCom[i] << " "; } std::cout << std::endl;

	std::cout << "Debug [LstmBlock, wIc["<<wIc[0].size() <<"], wHc["<<wHc[0].size() <<"] ]"<< std::endl;
	for(int i=0;i<wIc.size();i++){
		for(int j=0;j<wIc[i].size();j++){ std::cout << wIc[i][j] << " "; }
		for(int j=0;j<wHc[i].size();j++){ std::cout << wHc[i][j] << " "; } std::cout << std::endl;
	}

	std::cout << "Debug [LstmBlock, wIcd["<<wIcd[0].size() <<"], wHcd["<<wHcd[0].size() <<"] ]"<< std::endl;
	for(int i=0;i<wIcd.size();i++){
		for(int j=0;j<wIcd[i].size();j++){ std::cout << wIcd[i][j] << " "; }
		for(int j=0;j<wHcd[i].size();j++){ std::cout << wHcd[i][j] << " "; } std::cout << std::endl;
	}

	std::cout << "Debug [LstmBlock, wIcm["<<wIc[0].size() <<"], wHcm["<<wHcm[0].size() <<"] ]"<< std::endl;
	for(int i=0;i<wIcm.size();i++){
		for(int j=0;j<wIcm[i].size();j++){ std::cout << wIcm[i][j] << " "; }
		for(int j=0;j<wHcm[i].size();j++){ std::cout << wHcm[i][j] << " "; } std::cout << std::endl;
	}


	std::cout << std::endl;
}

LstmBlock::~LstmBlock() {

}


