/*******************************************************************************
* Copyright 2014-2022 Intel Corporation.
*
* This software and the related documents are Intel copyrighted  materials,  and
* your use of  them is  governed by the  express license  under which  they were
* provided to you (License).  Unless the License provides otherwise, you may not
* use, modify, copy, publish, distribute,  disclose or transmit this software or
* the related documents without Intel's prior written permission.
*
* This software and the related documents  are provided as  is,  with no express
* or implied  warranties,  other  than those  that are  expressly stated  in the
* License.
*******************************************************************************/

//@HEADER
// ***************************************************
//
// HPCG: High Performance Conjugate Gradient Benchmark
//
// Contact:
// Michael A. Heroux ( maherou@sandia.gov)
// Jack Dongarra     (dongarra@eecs.utk.edu)
// Piotr Luszczek    (luszczek@eecs.utk.edu)
//
// ***************************************************
//@HEADER

/*!
 @file OptimizeProblem.cpp

 HPCG routine
 */

#include "OptimizeProblem.hpp"
/*!
  Optimizes the data structures used for CG iteration to increase the
  performance of the benchmark version of the preconditioned CG algorithm.

  @param[inout] A      The known system matrix, also contains the MG hierarchy in attributes Ac and mgData.
  @param[inout] data   The data structure with all necessary CG vectors preallocated
  @param[inout] b      The known right hand side vector
  @param[inout] x      The solution vector to be computed in future CG iteration
  @param[inout] xexact The exact solution vector

  @return returns 0 upon success and non-zero otherwise

  @see GenerateGeometry
  @see GenerateProblem
*/
#include "stdio.h"
#include "math.h"

#include "mkl.h"

#ifndef HPCG_NO_OPENMP
#include <omp.h>
#endif

void OptimizeProblem(SparseMatrix * A, double & t7)
{
    t7 = 0.0;
#ifndef HPCG_LOCAL_LONG_LONG
    double t1 = 0.0;
  // This function can be used to completely transform any part of the data structures.
  // Right now it does nothing, so compiling with a check for unused variables results in complaints

//    SparseMatrix *Ac = &A;
    SparseMatrix *Ac = A;
    while (Ac != NULL) {
    local_int_t i, j, k, l, p;
    struct optData *optData = (struct optData *)mkl_malloc(sizeof(struct optData),128);
    const local_int_t nrow = Ac->localNumberOfRows;
    const local_int_t ncol = Ac->localNumberOfColumns;
    local_int_t nnz = 0, nrow_b = 0, nnz_b = 0;
    local_int_t nthr = A->nproc;
    if( Ac->mtxIndG ) { MKL_free(Ac->mtxIndG);       Ac->mtxIndG       = NULL; }

    local_int_t *ia   = (local_int_t*)mkl_malloc(sizeof(local_int_t)*(nrow+1), 512);
    local_int_t *ia_b = (local_int_t*)mkl_malloc(sizeof(local_int_t)*(nrow+1), 512);
    local_int_t *bmap = (local_int_t*)mkl_malloc(sizeof(local_int_t)*nrow, 512);
    double *diag = (double *)mkl_malloc(sizeof(double)*nrow, 512);

    if ( ia == NULL || ia_b == NULL || bmap == NULL || diag == NULL || optData == NULL ) return;

    init_optData(*optData);

    //calculate mkl csr arrays from hpcg matrix representation
    ia_b[0] = ia[0] = 0;
#ifndef HPCG_NO_OPENMP    
    #pragma omp parallel num_threads(nthr) default(shared) private(i,j) reduction(+:nnz)
#endif    
    {
#ifndef HPCG_NO_OPENMP    
        int ithr = omp_get_thread_num();
#else
	int ithr = 0;
#endif

        for (i = (ithr*nrow)/nthr; i < (ithr+1)*nrow/nthr; i++ )
        {
            const double * const cur_vals = Ac->matrixValues[i];
            const local_int_t *  const cur_inds = Ac->mtxIndL[i];
            ia_b[i+1] = ia[i+1] = 0;
            for (j = 0; j < Ac->nonzerosInRow[i]; j++)
            {
                if ( cur_inds[j] < nrow ) ia[i+1] ++;
                else                      ia_b[i+1] ++;
                if ( cur_inds[j] == i )
                {
                    diag[i] = cur_vals[j];
                }
            }
            nnz += ia[i+1];//Ac->nonzerosInRow[i];
        }
    }

    for ( i = 0; i < nrow; i ++ ) ia[i+1] += ia[i];
    nrow_b = 0;
    for ( i = 0; i < nrow; i ++ )
    {
        if( ia_b[i+1] > 0 )
        {
            nnz_b += ia_b[i+1];
            nrow_b ++;
        }
    }

    local_int_t *ja = (local_int_t *)mkl_malloc(sizeof(local_int_t)*nnz, 512);
    double *a = (double *)mkl_malloc(sizeof(double)*nnz, 512);

    if ( ja == NULL || a == NULL ) return;
#ifndef HPCG_NO_OPENMP
    #pragma omp parallel num_threads(nthr) default(shared) private(i,j,k,l,p)
#endif
    {
#ifndef HPCG_NO_OPENMP    
        int ithr = omp_get_thread_num();
#else
	int ithr = 0;
#endif

        for (i = (ithr*nrow)/nthr; i < (ithr+1)*nrow/nthr; i++ )
        {
            const double * const cur_vals = Ac->matrixValues[i];
            const local_int_t *  const cur_inds = Ac->mtxIndL[i];
            k = ia[i];
            for (j = 0; j<Ac->nonzerosInRow[i]; j++)
            {
                if ( cur_inds[j] < nrow )
                {
                    a [k] = cur_vals[j];
                    ja[k] = cur_inds[j];
                    k ++;
                }
            }
        }
    }

    local_int_t *ja_b = (local_int_t *)mkl_malloc(sizeof(local_int_t)*nnz_b, 512);
    double *a_b = (double *)mkl_malloc(sizeof(double)*nnz_b, 512);

    if ( (ja_b == NULL || a_b == NULL) && nnz_b > 0 ) return;

    p = k = 0;
    for (i = 0; i < nrow; i++ )
    {
        const double * const cur_vals = Ac->matrixValues[i];
        const local_int_t *  const cur_inds = Ac->mtxIndL[i];

        if ( ia_b[i+1] > 0 )
        {
            for (j = 0; j<Ac->nonzerosInRow[i]; j++)
            {
                if ( cur_inds[j] >= nrow )
                {
                    a_b [p] = cur_vals[j];
                    ja_b[p] = cur_inds[j];
                    p ++;
                }
            }
            bmap[k] = i;
            ia_b[k+1] = ia_b[i+1];
            k ++;
        }
    }
    for ( i = 0; i < nrow_b; i ++ ) ia_b[i+1] += ia_b[i];

    if(Ac->mtxL) { MKL_free(Ac->mtxL); Ac->mtxL          = NULL;}
    if(Ac->mtxA) { MKL_free(Ac->mtxA); Ac->mtxA          = NULL;}
    if(Ac->nonzerosInRow) { MKL_free(Ac->nonzerosInRow); Ac->nonzerosInRow = NULL; }
    if(Ac->matrixValues) { MKL_free(Ac->matrixValues);  Ac->matrixValues  = NULL; }
    if(Ac->mtxIndL) { MKL_free(Ac->mtxIndL);       Ac->mtxIndL       = NULL; }

    t1 = mytimer();

    sparse_status_t status = SPARSE_STATUS_SUCCESS;
    struct matrix_descr descr;
    sparse_matrix_t csrA = NULL, csrB = NULL;
    descr.type = SPARSE_MATRIX_TYPE_SYMMETRIC;
    descr.mode = SPARSE_FILL_MODE_FULL;
    descr.diag = SPARSE_DIAG_NON_UNIT;

    status = mkl_sparse_d_create_csr ( &csrA, SPARSE_INDEX_BASE_ZERO, nrow, nrow, ia, ia+1, ja, a );

    status = mkl_sparse_d_create_csr ( &csrB, SPARSE_INDEX_BASE_ZERO, nrow_b, ncol, ia_b, ia_b+1, ja_b, a_b );

    status = mkl_sparse_set_symgs_hint ( csrA, SPARSE_OPERATION_NON_TRANSPOSE, descr, 20);
    status = mkl_sparse_set_memory_hint( csrA, SPARSE_MEMORY_NONE);


    descr.type = SPARSE_MATRIX_TYPE_GENERAL;
    descr.mode = SPARSE_FILL_MODE_FULL;
    descr.diag = SPARSE_DIAG_NON_UNIT;
    status = mkl_sparse_set_mv_hint ( csrB, SPARSE_OPERATION_NON_TRANSPOSE, descr, 199);

    status = mkl_sparse_optimize( csrA );

    mkl_free(ia); mkl_free(ja); mkl_free(a);

    status = mkl_sparse_optimize(csrB);

    t7 += (mytimer() - t1);

    mkl_free(ia_b); mkl_free(ja_b); mkl_free(a_b);

    double *dtmp = (double *)mkl_malloc(sizeof(double)*4*nrow, 512);

    if ( dtmp == NULL ) return;

    optData->csrA  = csrA;
    optData->csrB  = csrB;
    optData->diag  = diag;
    optData->dtmp  = dtmp;
    optData->dtmp2 = dtmp + nrow;
    optData->dtmp3 = dtmp + 2*nrow;
    optData->dtmp4 = dtmp + 3*nrow;
    optData->bmap  = bmap;
    optData->nrow_b = nrow_b;



    Ac->optimizationData = optData;
    Ac = Ac->Ac;
    }//while Ac!=NULL
#else
    return;
#endif
}

// Helper function (see OptimizeProblem.hpp for details)
double OptimizeProblemMemoryUse(const SparseMatrix & A) {

  return 0.0;

}
