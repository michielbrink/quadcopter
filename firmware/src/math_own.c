/*
 * =====================================================================================
 *
 *       Filename:  math_own.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  02/11/2014 09:14:35 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Reinder Cuperus (),
 *   Organization:  tkkrlab
 *
 * =====================================================================================
 */


#ifdef LOCAL_TEST_ENVIRONMENT
#include "/home/reinder/tkkrlab/drone/QuadcopterTests/QuadcopterTests/own_math.h"
#else
#include "own_math.h"
#include "stm32f30x.h"
#include "stm32f3_discovery.h"
#include "control.h"
#include "arm_math.h"
#include "helpers.h"
#include <string.h>

#endif // LOCAL_TESt_ENVIRONMENT


inline float32_t _matrix_element(const arm_matrix_instance_f32 * pSrcM, uint16_t i, uint16_t j);
/*
 * Do Sandwich multiplication X = B*A*B' + C
 */
arm_status math_mult_sandwich_add(
	const arm_matrix_instance_f32 * pSrcA,
	const arm_matrix_instance_f32 * pSrcB,
	const arm_matrix_instance_f32 * pSrcC,
	arm_matrix_instance_f32 * pSrcX ) {
    uint16_t i,j,k,l,p;
    float32_t f1, f2, f3, f4;
    arm_status status;

#ifdef ARM_MATH_MATRIX_CHECK
  /* Check for matrix mismatch condition */
  if((pSrcA->numCols != pSrcA->numRows) ||
     (pSrcA->numCols != pSrcB->numCols) ||
     (pSrcC->numRows != pSrcC->numCols) ||
     (pSrcX->numRows != pSrcX->numCols) ||
     (pSrcC->numRows != pSrcX->numRows) ||
     (pSrcB->numRows != pSrcC->numCols))
  {

    /* Set status as ARM_MATH_SIZE_MISMATCH */
    status = ARM_MATH_SIZE_MISMATCH;
  }
  else
#endif /*      #ifdef ARM_MATH_MATRIX_CHECK    */
  {
    for (i=0; i<pSrcX->numRows; i++) {
        for (j=0; j<pSrcX->numCols; j++) {
            p = i*(pSrcX->numCols) + j;
            pSrcX->pData[p] = _matrix_element(pSrcC, i, j);
            // X_{i,j} = B_{i,k} * A_{k,l} * B'_{l,j}
            // X_{i,j} = B_{i,k} * A_{k,l} * B_{j,l}
            for (k=0; k<pSrcA->numRows; k++) {
                for (l=0; l<pSrcA->numCols; l++) {
                    f1 = _matrix_element(pSrcB, i, k);
                    f2 = _matrix_element(pSrcA, k, l);
                    f3 = _matrix_element(pSrcB, j, l);
                    f4 = f1 * f2 * f3;
                    pSrcX->pData[p] += f4;
                }
            }
        }
    }
    status = ARM_MATH_SUCCESS;
  }
  return status;
}

inline float32_t _matrix_element(const arm_matrix_instance_f32 * pSrcXX, uint16_t i, uint16_t j) {
#ifdef ARM_MATH_MATRIX_CHECK
    if (i>(pSrcXX->numCols))
        { return 0.0/0.0; }
    if (j>(pSrcXX->numRows))
        { return 0.0/0.0; }
#endif
    return pSrcXX->pData[i*(pSrcXX->numCols) + j];
}
