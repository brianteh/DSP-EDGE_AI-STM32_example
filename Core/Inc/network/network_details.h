/**
  ******************************************************************************
  * @file    network.h
  * @date    2026-06-25T06:45:37+0000
  * @brief   ST.AI Tool Automatic Code Generator for Embedded NN computing
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  ******************************************************************************
  */
#ifndef STAI_NETWORK_DETAILS_H
#define STAI_NETWORK_DETAILS_H

#include "stai.h"
#include "layers.h"

const stai_network_details g_network_details = {
  .tensors = (const stai_tensor[14]) {
   { .size_bytes = 500, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {3, (const int32_t[3]){1, 500, 1}}, .scale = {1, (const float[1]){0.003921568859368563}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "serving_default_input_layer_10_output" },
   { .size_bytes = 15872, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 496, 32}}, .scale = {1, (const float[1]){0.0014048106968402863}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_1_output" },
   { .size_bytes = 7936, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 248, 32}}, .scale = {1, (const float[1]){0.0014048106968402863}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "pool_4_output" },
   { .size_bytes = 7936, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 248, 32}}, .scale = {1, (const float[1]){0.021925663575530052}}, .zeropoint = {1, (const int16_t[1]){-65}}, .name = "eltwise_5_output" },
   { .size_bytes = 7936, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 248, 32}}, .scale = {1, (const float[1]){0.017635006457567215}}, .zeropoint = {1, (const int16_t[1]){-50}}, .name = "eltwise_6_output" },
   { .size_bytes = 15616, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 244, 64}}, .scale = {1, (const float[1]){0.013224381022155285}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_9_output" },
   { .size_bytes = 7808, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 122, 64}}, .scale = {1, (const float[1]){0.013224381022155285}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "pool_12_output" },
   { .size_bytes = 7808, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 122, 64}}, .scale = {1, (const float[1]){0.03217778354883194}}, .zeropoint = {1, (const int16_t[1]){-74}}, .name = "eltwise_13_output" },
   { .size_bytes = 7808, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 122, 64}}, .scale = {1, (const float[1]){0.028812313452363014}}, .zeropoint = {1, (const int16_t[1]){-67}}, .name = "eltwise_14_output" },
   { .size_bytes = 15360, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 1, 120, 128}}, .scale = {1, (const float[1]){0.028102146461606026}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_17_output" },
   { .size_bytes = 128, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {3, (const int32_t[3]){1, 1, 128}}, .scale = {1, (const float[1]){0.02216985449194908}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "pool_19_output" },
   { .size_bytes = 64, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {2, (const int32_t[2]){1, 64}}, .scale = {1, (const float[1]){0.03538158908486366}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "gemm_20_output" },
   { .size_bytes = 1, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {2, (const int32_t[2]){1, 1}}, .scale = {1, (const float[1]){0.1490134447813034}}, .zeropoint = {1, (const int16_t[1]){-8}}, .name = "gemm_21_output" },
   { .size_bytes = 1, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {2, (const int32_t[2]){1, 1}}, .scale = {1, (const float[1]){0.00390625}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "nl_22_output" }
  },
  .nodes = (const stai_node_details[13]){
    {.id = 1, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){0}}, .output_tensors = {1, (const int32_t[1]){1}} }, /* conv2d_1 */
    {.id = 4, .type = AI_LAYER_POOL_TYPE, .input_tensors = {1, (const int32_t[1]){1}}, .output_tensors = {1, (const int32_t[1]){2}} }, /* pool_4 */
    {.id = 5, .type = AI_LAYER_ELTWISE_INTEGER_TYPE, .input_tensors = {1, (const int32_t[1]){2}}, .output_tensors = {1, (const int32_t[1]){3}} }, /* eltwise_5 */
    {.id = 6, .type = AI_LAYER_ELTWISE_INTEGER_TYPE, .input_tensors = {1, (const int32_t[1]){3}}, .output_tensors = {1, (const int32_t[1]){4}} }, /* eltwise_6 */
    {.id = 9, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){4}}, .output_tensors = {1, (const int32_t[1]){5}} }, /* conv2d_9 */
    {.id = 12, .type = AI_LAYER_POOL_TYPE, .input_tensors = {1, (const int32_t[1]){5}}, .output_tensors = {1, (const int32_t[1]){6}} }, /* pool_12 */
    {.id = 13, .type = AI_LAYER_ELTWISE_INTEGER_TYPE, .input_tensors = {1, (const int32_t[1]){6}}, .output_tensors = {1, (const int32_t[1]){7}} }, /* eltwise_13 */
    {.id = 14, .type = AI_LAYER_ELTWISE_INTEGER_TYPE, .input_tensors = {1, (const int32_t[1]){7}}, .output_tensors = {1, (const int32_t[1]){8}} }, /* eltwise_14 */
    {.id = 17, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){8}}, .output_tensors = {1, (const int32_t[1]){9}} }, /* conv2d_17 */
    {.id = 19, .type = AI_LAYER_POOL_TYPE, .input_tensors = {1, (const int32_t[1]){9}}, .output_tensors = {1, (const int32_t[1]){10}} }, /* pool_19 */
    {.id = 20, .type = AI_LAYER_DENSE_TYPE, .input_tensors = {1, (const int32_t[1]){10}}, .output_tensors = {1, (const int32_t[1]){11}} }, /* gemm_20 */
    {.id = 21, .type = AI_LAYER_DENSE_TYPE, .input_tensors = {1, (const int32_t[1]){11}}, .output_tensors = {1, (const int32_t[1]){12}} }, /* gemm_21 */
    {.id = 22, .type = AI_LAYER_NL_TYPE, .input_tensors = {1, (const int32_t[1]){12}}, .output_tensors = {1, (const int32_t[1]){13}} } /* nl_22 */
  },
  .n_nodes = 13
};
#endif

