#ifndef OWNMATH
#define OWNMATH

#ifndef LOCAL_TEST_ENVIRONMENT
#include "helpers.h"
#include "arm_math.h"
#endif

/*
 * Do Sandwich multiplication X = B*A*B' + C
 */
arm_status math_mult_sandwich_add(
	const arm_matrix_instance_f32 * pSrcA,
	const arm_matrix_instance_f32 * pSrcB,
	const arm_matrix_instance_f32 * pSrcC,
	arm_matrix_instance_f32 * pSrcX
);


#endif
