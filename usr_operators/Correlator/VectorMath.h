/*
 * VectorMath.h
 *
 *  Created on: Oct 28, 2010
 *      Author: koutnij
 */

#ifndef VECTORMATH_H_
#define VECTORMATH_H_

#include <cstdlib>

// outer product
template<class InputIterator1, class InputIterator2, class OutputIterator>
static OutputIterator
    outer_product(InputIterator1 first1, InputIterator1 last1,
              InputIterator2 first2, InputIterator2 last2,
              OutputIterator result) {

		for(; first1 != last1; ++first1)
			for(InputIterator2 it2 = first2; it2 != last2; ++it2)
				*result++ = (*first1) * (*it2);
		return result;
}

// vector difference
template <class InputIterator1, class InputIterator2, class OutputIterator>
   static OutputIterator vector_difference ( InputIterator1 first1, InputIterator1 last1,
											 InputIterator2 first2, OutputIterator result){
   while (first1!=last1){
	   *result++ = (*first1++)-(*first2++);
   }
   return result;
}

// vector sum
template <class InputIterator1, class InputIterator2, class OutputIterator>
   static OutputIterator vector_sum ( InputIterator1 first1, InputIterator1 last1,
											 InputIterator2 first2, OutputIterator result){
   while (first1!=last1){
	   *result++ = (*first1++)+(*first2++);
   }
   return result;
}

// vector add
template <class InputIterator1, class InputIterator2>
   static InputIterator2 vector_add ( InputIterator1 first1, InputIterator1 last1,
											 InputIterator2 first2){
   while (first1!=last1){
	   *first1++ += (*first2++);
   }
   return first1;
}

// vector subtract
template <class InputIterator1, class InputIterator2>
   static InputIterator2 vector_sub ( InputIterator1 first1, InputIterator1 last1,
											 InputIterator2 first2){
   while (first1!=last1){
	   *first1++ -= (*first2++);
   }
   return first1;
}

// scalar multiple
template <class T, class InputIterator, class OutputIterator>
    static OutputIterator scalar_multiple (T k, InputIterator first1, InputIterator last1,
		  OutputIterator first2){
		while (first1!=last1){
			*first2++ = k * (*first1++);
		}
    return first2;
}

// total 1D
template <class InputIterator, class T>
    static T total (InputIterator first1, InputIterator last1, T t){
		//T t = 0;
		while (first1!=last1){
			t += (*first1++);
		}
    return t;
}

// mean square error
template <class InputIterator, class T>
    static T mse (InputIterator first1, InputIterator last1, T t){
		//T t = 0;
		while (first1!=last1){
			t += (*first1)*(*first1++);
		}
    return t/2.;
}

static double randomW(double halfRange){
	return halfRange * (2 *  (rand()/(double)RAND_MAX) -1 );
}

static void rescale_matrix(std::vector<std::vector <double> >& mat){
	double min=mat[0][0];
	double max=mat[0][0];
	double d;

	for(int i=0;i<mat.size();i++){
		for(int j=0;j<mat[0].size();j++){
			if(mat[i][j] < min) min = mat[i][j];
			if(mat[i][j] > max) max = mat[i][j];
		}
	}
	if(min != max){
		d = max - min;
		for(int i=0;i<mat.size();i++){
			for(int j=0;j<mat[0].size();j++){
				mat[i][j] = (mat[i][j] - min) / d;
			}
		}
	}

}

static void abs_matrix(std::vector<std::vector <double> >& mat){
	for(int i=0;i<mat.size();i++){
		for(int j=0;j<mat[0].size();j++){
			mat[i][j] = fabs(mat[i][j]);

		}

	}
}

#endif /* VECTORMATH_H_ */
