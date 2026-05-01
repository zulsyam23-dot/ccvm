/**
 * @file transforms.h
 * @brief Transformation pass definitions
 */

#ifndef CCVM_OPTIMIZER_TRANSFORMS_H
#define CCVM_OPTIMIZER_TRANSFORMS_H

#include "optimizer.h"

void ccvm_opt_inline(ccvm_optimizer_t* opt);
void ccvm_opt_dce(ccvm_optimizer_t* opt);
void ccvm_opt_const_fold(ccvm_optimizer_t* opt);

#endif
