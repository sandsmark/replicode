/*
 * ForwardLayer.h
 *
 *  Created on: Oct 27, 2010
 *      Author: koutnij
 */

#include <vector>
#include <math.h>

#ifndef FORWARDLAYER_H_
#define FORWARDLAYER_H_

struct ForwardLayerState{
	// current cell states
	std::vector <double> ak; // neuron excitations
	std::vector <double> bk; // neuron activations
	std::vector <double> ek; // errors
	std::vector <double> dk; // deltas
};

class ForwardLayer {
public:
	ForwardLayer(int nCells, int nOutputs, int nBlocks);
	void forwardPassStep(int t, std::vector<ForwardLayerState>& state, std::vector<double>& b);
	// backwardPassStep(time, state, training vector)
	void backwardPassStep(int t, std::vector<ForwardLayerState>& state, std::vector<double>& b, std::vector<std::vector<double> >& er);
	void setwK(std::vector <std::vector<double> >& newwK);
	void setConstantWeights(double w);
	void setRandomWeights(double halfRange);
	void updateWeights(double eta, double alpha);
	void resetDerivs();

	void printWeights();
	void getSerializedWeights(std::vector<double> & w);
	void getSerializedDerivs(std::vector<double> & w);
	void updateOutputError(std::vector<ForwardLayerState>& state, std::vector<std::vector<double> >& tr,std::vector<std::vector<double> >& er);
	~ForwardLayer();

	std::vector <std::vector<double> > wK;
	std::vector <std::vector<double> > wKd; // weight derivatives, updated through the backward pass
	std::vector <std::vector<double> > wKm;	// weights after the previous training episode - for momentum

	std::vector <double> biasK; // bias vector, biases are treated separately
	std::vector <double> biasKd;
	std::vector <double> biasKm;
private:

};

inline double fnO(double x){
	return 1/(1+exp(-x));
	//return x;
}

inline double fnOd(double x){
	double ex=exp(x);
	return ex/((1+ex)*(1+ex));
	//return 1.;
}

#endif /* FORWARDLAYER_H_ */
