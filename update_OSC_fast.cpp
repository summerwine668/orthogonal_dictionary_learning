/*
Copyright © 2016 Henry Schuetze
Institute for Neuro- and Bioinformatics
University of Luebeck, Germany
Henry.Schuetze@uni-luebeck.de

Permission is hereby granted, free of charge, to any person obtaining a 
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including 
without limitation the rights to use, copy, modify, merge, publish, 
distribute, sublicense, and/or sell copies of the Software, and to permit
persons to whom the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice shall be included 
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include <mex.h>
#include <algorithm> 

using namespace std;

typedef pair<int, double> my_tuple;

bool sort_comparator(const my_tuple& left, const my_tuple& right) {
  return left.second > right.second;
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  plhs[0] = mxDuplicateArray(prhs[0]);
  
  double* U = mxGetPr(mxGetField(plhs[0], 0, "U"));
  double* x_res = mxGetPr(mxGetField(plhs[0], 0, "x"));
  int K = (int)*mxGetPr(mxGetField(plhs[0], 0, "sparsity_param"));
  
  double t = *mxGetPr(mxGetField(plhs[0], 0, "t"));
  double t_max = *mxGetPr(mxGetField(plhs[0], 0, "t_max"));
  double eps_i = *mxGetPr(mxGetField(plhs[0], 0, "eps_i"));
  double eps_f = *mxGetPr(mxGetField(plhs[0], 0, "eps_f"));
  
  // compute learning rate to apply
  double eps_t = eps_i*pow((eps_f/eps_i), (t/t_max));
  
  const int* dim_vec = mxGetDimensions(mxGetField(plhs[0], 0, "U"));
  
  int k;        // column index in the range {0, ..., num_cols-1}
  int kk;       // iterates entries of column k (w.r.t matrix U)
  int kk_begin; // initial value of kk
  int kk_end;   // final value of kk
  
  int l;        // column index in the range {k+1, ..., num_cols-1}
  int ll;       // iterates entries of column l (w.r.t matrix U)
  int ll_begin; // initial value of ll
  
  int j;        // iterates entries of x_res {0, ..., num_rows-1}
  
  double tmp, tmp2;
  
  // create coefficient vector a = U'*x_res and perform an indexed sort on absolute values
  kk = 0;
  vector<my_tuple> a(dim_vec[1]);
  for(k=0; k<dim_vec[1]; k++) {
    a[k].first = kk;
    a[k].second = 0;
    
    for(j=0; j<dim_vec[0]; j++, kk++)
      a[k].second += x_res[j] * U[kk];
    a[k].second = fabs(a[k].second);
  }
  
  sort(a.begin(), a.end(), sort_comparator);
  
  for(k=0; k < dim_vec[1]; k++) {
    kk_end = a[k].first + dim_vec[0];
    
    if(k < K) {
      // compute y (inner product of x_res and u_k)
      tmp = 0;
      for(kk=a[k].first, j=0; kk < kk_end; kk++, j++)
        tmp += U[kk]*x_res[j];
      
      // Hebbian-like update
      for(kk=a[k].first, j=0; kk < kk_end; kk++, j++)
        U[kk] += eps_t * tmp * x_res[j];
    }
    
    // compute simultaneously the norm of u_k and the inner product of x_res and u_k
    tmp = 0;
    tmp2 = 0;
    for(kk=a[k].first, j=0; kk < kk_end; kk++, j++) {
      tmp += U[kk]*U[kk];
      tmp2 += U[kk]*x_res[j];
    }
    tmp = sqrt(tmp); // stores length of u_k
    tmp2 = tmp2/tmp; // stores ratio of inner product and length of u_k (for simultaneous update)
    
    // normalize u_k to unit length and update residual
    for(kk=a[k].first, j=0; kk < kk_end; kk++, j++) {
      U[kk] /= tmp;
      x_res[j] -= tmp2*U[kk];
    }
    
    // orthogonalize remaining basis vectors to be updated
    for(l=k+1; l < dim_vec[1]; l++) {
      ll_begin = a[l].first;
      
      // compute inner produkt between u_k and u_l
      tmp = 0;
      for(kk=a[k].first, ll=ll_begin; kk < kk_end; kk++, ll++)
        tmp += U[kk]*U[ll];
      
      // subtract projection
      for(kk=a[k].first, ll=ll_begin; kk < kk_end; kk++, ll++)
        U[ll] -= tmp*U[kk];
    }
  }
}