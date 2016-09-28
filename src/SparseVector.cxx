/**
 * @file SparseVector.cxx
 * @brief Implmentation of a sparse vector (similar to an std::map, but with less functionality, and using less memory)
 * @author E. Charles
 *
 * $Header$
 */


#include "Likelihood/SparseVector.h"

namespace Likelihood {

  static SparseVector<int> sparse_int;
  static SparseVector<float> sparse_float;
  static SparseVector<double> sparse_double;
 
} // namespace Likelihood
