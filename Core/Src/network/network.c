/**
  ******************************************************************************
  * @file    network.c
  * @author  AST Embedded Analytics Research Platform
  * @date    2026-06-25T06:45:37+0000
  * @brief   AI Tool Automatic Code Generator for Embedded NN computing
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

#include "ai_lite_inspect.h"
#include "ai_platform_interface.h"
#include "layers.h"
#include "core_convert.h"
#include "network.h"
#include "network_details.h"
#include "network_data.h"
#include "stai_events.h"

#include "ai_lite_inspect.h"

#include "lite_operators.h"
/*****************************************************************************/
#define STAI_INTERNAL_API_MAJOR               (1)
#define STAI_INTERNAL_API_MINOR               (0)
#define STAI_INTERNAL_API_MICRO               (0)

#define STAI_MAGIC                            (0xB1C00100)

/*****************************************************************************/
#define _STAI_CONCAT_ARG(a, b)     a ## b
#define STAI_CONCAT(a, b)         _STAI_CONCAT_ARG(a, b)

/*!  STAI_CAST SECTION                       *********************************/
#define STAI_CAST(type, expr) \
  ((type)(expr))


/*****************************************************************************/
#define STAI_SIZE(_size) \
  ((stai_size)(_size))

/*****************************************************************************/
#define STAI_INIT_BUFFER(_flags, _size, _address) \
  { \
    .size = (_size), \
    .address = (uintptr_t)(_address), \
    .flags = (_flags), \
  }

#define STAI_INIT_TENSOR(_name, _flags, _fmt, _size_bytes, _shape, _scale, _zeropoint) \
  { \
    .size_bytes = (_size_bytes), \
    .flags = (_flags), \
    .format = (stai_format)(_fmt), \
    .shape = STAI_PACK(_shape), \
    .scale = STAI_PACK(_scale), \
    .zeropoint = STAI_PACK(_zeropoint), \
    .name = (_name) \
  }

#define STAI_INIT_ARRAY(_size, _ptr) \
  { .size = STAI_SIZE(_size), .data = STAI_PACK(_ptr) }


#define STAI_CAST_ARRAY(_type, _size, _ptr) \
  { .size = STAI_SIZE(_size), .data = (_type)STAI_PACK(_ptr) }


#define STAI_DECLARE_ARRAY(_type, _size, ...) \
  { .size = STAI_SIZE(_size), .data = (_type[_size]) { STAI_PACK(__VA_ARGS__) } }


#define STAI_EMPTY_ARRAY() \
  { .size = 0, .data = NULL }


#define STAI_INIT_VERSION(_major, _minor, _micro) \
  { .major = (_major), .minor = (_minor), .micro = (_micro), .reserved = 0x0 }

/*****************************************************************************/
/**  Getters and setters  **/

#define STAI_GET_ARRAY_SIZE(nd_array) \
  (nd_array.size)


#define STAI_GET_ARRAY_ELEM(nd_array, pos) \
  (nd_array.data[(pos)])

#define _STAI_SET_ERROR(net_ctx, cond, value, exit) { \
  if (!(net_ctx)) { return STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE; } \
  if (((uintptr_t)net_ctx) & (_STAI_CONTEXT_ALIGNMENT-1)) { return STAI_ERROR_NETWORK_INVALID_CONTEXT_ALIGNMENT; } \
  if (((value) >= STAI_ERROR_GENERIC) && (cond)) { \
    if ((net_ctx)->_return_code == STAI_SUCCESS) { \
      (net_ctx)->_return_code = (value); \
    } \
    return (exit); \
  } \
}

/*****************************************************************************/
/* TODO REMOVE THESE TWO MACROS */
#define STAI_EVENT_NODE_START_CB
#define STAI_EVENT_NODE_STOP_CB

#ifdef STAI_EVENT_NODE_START_CB
#ifndef _STAI_NETWORK_EVENT_NODE_START_CB
  #define _STAI_NETWORK_EVENT_NODE_START_CB(_node_id, _buffers_size, ...) \
  if (net_ctx->_callback) { \
    const stai_event_node_start_stop _start_event = { \
      .node_id=(_node_id), \
      .buffers={ \
        .size=(_buffers_size), \
        .data=(stai_ptr const*)(const stai_ptr[_buffers_size])STAI_PACK(__VA_ARGS__) \
      } \
    }; \
    net_ctx->_callback(net_ctx->_callback_cookie, STAI_EVENT_NODE_START, (const void*)&_start_event); \
  }
#endif
#else
  #define _STAI_NETWORK_EVENT_NODE_START_CB(_node_id, _buffers_size, ...) \
    do { /* _STAI_NETWORK_EVENT_NODE_START_CB() */ } while(0);
#endif      /* STAI_EVENT_NODE_START_CB */

#ifdef STAI_EVENT_NODE_STOP_CB
#ifndef _STAI_NETWORK_EVENT_NODE_STOP_CB
  #define _STAI_NETWORK_EVENT_NODE_STOP_CB(_node_id, _buffers_size, ...) \
  if (net_ctx->_callback) { \
    const stai_event_node_start_stop _stop_event = { \
      .node_id=(_node_id), \
      .buffers={ \
        .size=(_buffers_size), \
        .data=(stai_ptr const*)(stai_ptr[_buffers_size])STAI_PACK(__VA_ARGS__) \
      } \
    }; \
    net_ctx->_callback(net_ctx->_callback_cookie, STAI_EVENT_NODE_STOP, (const void*)&_stop_event); \
  }
#endif
#else
  #define _STAI_NETWORK_EVENT_NODE_STOP_CB(_node_id, _buffers_size, ...) \
    do { /* _STAI_NETWORK_EVENT_NODE_STOP_CB() */ } while(0);
#endif      /* STAI_EVENT_NODE_STOP_CB */


/*****************************************************************************/
#define _STAI_NETWORK_MODEL_SIGNATURE     "0xcc84bc578ab15c6bf08e697ef124bd94"
#define _STAI_NETWORK_DATETIME            "2026-06-25T06:45:37+0000"
#define _STAI_NETWORK_COMPILE_DATETIME    __DATE__ " " __TIME__

#define _STAI_CONTEXT_ALIGNMENT        STAI_NETWORK_CONTEXT_ALIGNMENT

/*****************************************************************************/
#define g_network_activations_1     (NULL)




#if defined(HAVE_NETWORK_INFO)
/*****************************************************************************/
static const stai_network_info g_network_info = {
  .model_signature = _STAI_NETWORK_MODEL_SIGNATURE,
  .c_compile_datetime = _STAI_NETWORK_COMPILE_DATETIME,
  .c_model_name = STAI_NETWORK_MODEL_NAME,
  .c_model_datetime = _STAI_NETWORK_DATETIME,
  .c_model_signature = 0x0,
  .runtime_version = STAI_INIT_VERSION(12, 0, 1),
  .tool_version = STAI_INIT_VERSION(4, 0, 1),
  .api_version = STAI_INIT_VERSION(1, 0, 0),
  .n_macc = STAI_NETWORK_MACC_NUM,
  .n_nodes = STAI_NETWORK_NODES_NUM,
  .flags = STAI_NETWORK_FLAGS,
  .n_inputs = STAI_NETWORK_IN_NUM,
  .n_outputs = STAI_NETWORK_OUT_NUM,
  .n_activations = STAI_NETWORK_ACTIVATIONS_NUM,
  .n_weights = STAI_NETWORK_WEIGHTS_NUM,
  .n_states = STAI_NETWORK_STATES_NUM,
  .inputs = (stai_tensor[STAI_NETWORK_IN_NUM]) {
    STAI_INIT_TENSOR(
      STAI_NETWORK_IN_1_NAME,
      STAI_NETWORK_IN_1_FLAGS,
      STAI_NETWORK_IN_1_FORMAT,
      STAI_NETWORK_IN_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 3, 1, 500, 1),
      STAI_DECLARE_ARRAY(float, 1, 0.003921568859368563f),
      STAI_DECLARE_ARRAY(int16_t, 1, -128)),
    },
    .outputs = (stai_tensor[STAI_NETWORK_OUT_NUM]) {
    STAI_INIT_TENSOR(
      STAI_NETWORK_OUT_1_NAME,
      STAI_NETWORK_OUT_1_FLAGS,
      STAI_NETWORK_OUT_1_FORMAT,
      STAI_NETWORK_OUT_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 2, 1, 1),
      STAI_DECLARE_ARRAY(float, 1, 0.00390625f),
      STAI_DECLARE_ARRAY(int16_t, 1, -128)),
    },
  .activations = (stai_tensor[STAI_NETWORK_ACTIVATIONS_NUM]) {
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_ACTIVATION_1_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_ACTIVATION_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 30848),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    },
  .weights = (stai_tensor[STAI_NETWORK_WEIGHTS_NUM]) {
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_1_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 44580),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    },

  .states = NULL
};
#endif

#define _STAI_CONTEXT_ACQUIRE(_net_ctx, _net_handle) \
  _stai_network_context* _net_ctx = (_stai_network_context*)(_net_handle); \
  STAI_ASSERT(_net_ctx != NULL) \
  _STAI_SET_ERROR(_net_ctx, _net_ctx->_magic != STAI_MAGIC, \
                  STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE, _net_ctx->_return_code)


/*****************************************************************************/
static
void _stai_network_check(_stai_network_context* net_ctx)
{
  stai_size idx;

// Check activations status
  for (idx=0; idx<STAI_NETWORK_ACTIVATIONS_NUM; idx++) {
    if (net_ctx->_activations[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_NETWORK_ACTIVATIONS_NUM) ? STAI_FLAG_ACTIVATIONS : STAI_FLAG_NONE;
// Check inputs status
  for (idx=0; idx<STAI_NETWORK_IN_NUM; idx++) {
    if (net_ctx->_inputs[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_NETWORK_IN_NUM) ? STAI_FLAG_INPUTS : STAI_FLAG_NONE;

  // Check outputs status
  for (idx=0; idx<STAI_NETWORK_OUT_NUM; idx++) {
    if (net_ctx->_outputs[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_NETWORK_OUT_NUM) ? STAI_FLAG_OUTPUTS : STAI_FLAG_NONE;

// Check weights status
  for (idx=0; idx<STAI_NETWORK_WEIGHTS_NUM; idx++) {
    if (net_ctx->_weights[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_NETWORK_WEIGHTS_NUM) ? STAI_FLAG_WEIGHTS : STAI_FLAG_NONE;
STAI_PRINT("  [_stai_network_check] flags: 0x%08x\n", net_ctx->_flags)
}


/*****************************************************************************/
STAI_API_ENTRY
stai_return_code stai_network_init(
  stai_network* network)
{
  /* Memory where to store internal context is provided by applications as a raw byte buffer */
  _stai_network_context* net_ctx = (_stai_network_context*)(network);
  net_ctx->_return_code = STAI_SUCCESS;
  STAI_PRINT("[Entering Network Init] network(%p) context_size(%d)\n", net_ctx, (int32_t)sizeof(_stai_network_context))

  _STAI_SET_ERROR(net_ctx, STAI_NETWORK_CONTEXT_SIZE != sizeof(_stai_network_context),
                 STAI_ERROR_NETWORK_INVALID_CONTEXT_SIZE, net_ctx->_return_code)

  {
    const _stai_network_context _network_context = {
      ._magic = STAI_MAGIC,
      ._signature = STAI_NETWORK_MODEL_SIGNATURE,
      ._flags = STAI_NETWORK_FLAGS,
      ._return_code = STAI_SUCCESS,
      ._callback = NULL,
      ._callback_cookie = NULL,
      ._activations = {
      (stai_ptr)g_network_activations_1
      },
      ._weights = {
      (stai_ptr)g_network_weights_array
      },
      ._inputs = {
    NULL},
      ._outputs = {
    NULL},
    };

    // Deep copy of internal context to opaque buffer provided by app
    *net_ctx = _network_context;

    _stai_network_check(net_ctx);
  }

  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_deinit(
  stai_network* network)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  /*  Reset flags to initial state  */
  net_ctx->_flags = STAI_NETWORK_FLAGS;
  return net_ctx->_return_code;
}

/*****************************************************************************/



/* Int quant #0 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(pool_4_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0014048106968402863f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #1 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_5_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.021925663575530052f),
    AI_PACK_INTQ_ZP(-65)))

/* Int quant #2 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(sequential_1_1_batch_normalization_2_1_batchnorm_mul_4D_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.09018713235855103f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #3 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_6_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.017635006457567215f),
    AI_PACK_INTQ_ZP(-50)))

/* Int quant #4 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(sequential_1_1_batch_normalization_2_1_batchnorm_sub_4D_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0056320903822779655f),
    AI_PACK_INTQ_ZP(123)))

/* Int quant #5 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(pool_12_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.013224381022155285f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #6 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_13_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.03217778354883194f),
    AI_PACK_INTQ_ZP(-74)))

/* Int quant #7 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(sequential_1_1_batch_normalization_3_1_batchnorm_mul_4D_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.02801051363348961f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #8 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_14_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.028812313452363014f),
    AI_PACK_INTQ_ZP(-67)))

/* Int quant #9 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(sequential_1_1_batch_normalization_3_1_batchnorm_sub_4D_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.006843958981335163f),
    AI_PACK_INTQ_ZP(127)))

/* Int quant #10 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_17_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.028102146461606026f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #11 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(pool_19_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.02216985449194908f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #12 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(gemm_20_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.03538158908486366f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #13 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(gemm_20_weights_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 64,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0014550104970112443f, 0.001353989471681416f, 0.0015740497037768364f, 0.0014298424357548356f, 0.0014814409660175443f, 0.001478563412092626f, 0.0014510317705571651f, 0.001608731341548264f, 0.0016327861230820417f, 0.001466514659114182f, 0.0014078308595344424f, 0.0014233972178772092f, 0.0017289075767621398f, 0.0013849477982148528f, 0.0014482063706964254f, 0.0013865684159100056f, 0.001554278307594359f, 0.001541663077659905f, 0.0014700216706842184f, 0.0014070705510675907f, 0.001534191076643765f, 0.001642341841943562f, 0.0016050153644755483f, 0.0016049861442297697f, 0.0014213211834430695f, 0.001376389293000102f, 0.0014776198659092188f, 0.0016595546621829271f, 0.001475451048463583f, 0.0013860890176147223f, 0.0015543980989605188f, 0.0014133095974102616f, 0.00144456687849015f, 0.0016527643892914057f, 0.0015849706251174212f, 0.0014448690926656127f, 0.0014183768071234226f, 0.001458304701372981f, 0.0017173080705106258f, 0.0015955535927787423f, 0.0013621641555801034f, 0.0015700142830610275f, 0.0015669636195525527f, 0.0015993848210200667f, 0.0013677498791366816f, 0.001391695230267942f, 0.0014674215344712138f, 0.0014100609114393592f, 0.0014879937516525388f, 0.0014117415994405746f, 0.001611933927051723f, 0.0014928209129720926f, 0.0013861047336831689f, 0.0014382682275027037f, 0.0014936315128579736f, 0.0014359622728079557f, 0.0014158362755551934f, 0.0013914185110479593f, 0.0016393557889387012f, 0.0015332245966419578f, 0.001361423754133284f, 0.0014906154247000813f, 0.0015922284219413996f, 0.0014770904090255499f),
    AI_PACK_INTQ_ZP(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)))

/* Int quant #14 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(gemm_21_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.1490134447813034f),
    AI_PACK_INTQ_ZP(-8)))

/* Int quant #15 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(nl_22_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.00390625f),
    AI_PACK_INTQ_ZP(-128)))



/* Array#0 */
AI_ARRAY_OBJ_DECLARE(
  pool_4_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 7936, AI_STATIC)

/* Array#1 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_5_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 7936, AI_STATIC)

/* Array#2 */
AI_ARRAY_OBJ_DECLARE(
  sequential_1_1_batch_normalization_2_1_batchnorm_mul_4D_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 32, AI_STATIC)

/* Array#3 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_6_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 7936, AI_STATIC)

/* Array#4 */
AI_ARRAY_OBJ_DECLARE(
  sequential_1_1_batch_normalization_2_1_batchnorm_sub_4D_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 32, AI_STATIC)

/* Array#5 */
AI_ARRAY_OBJ_DECLARE(
  pool_12_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 7808, AI_STATIC)

/* Array#6 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_13_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 7808, AI_STATIC)

/* Array#7 */
AI_ARRAY_OBJ_DECLARE(
  sequential_1_1_batch_normalization_3_1_batchnorm_mul_4D_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 64, AI_STATIC)

/* Array#8 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_14_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 7808, AI_STATIC)

/* Array#9 */
AI_ARRAY_OBJ_DECLARE(
  sequential_1_1_batch_normalization_3_1_batchnorm_sub_4D_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 64, AI_STATIC)

/* Array#10 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_17_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 15360, AI_STATIC)

/* Array#11 */
AI_ARRAY_OBJ_DECLARE(
  pool_19_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 128, AI_STATIC)

/* Array#12 */
AI_ARRAY_OBJ_DECLARE(
  gemm_20_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 64, AI_STATIC)

/* Array#13 */
AI_ARRAY_OBJ_DECLARE(
  gemm_20_weights_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 8192, AI_STATIC)

/* Array#14 */
AI_ARRAY_OBJ_DECLARE(
  gemm_20_bias_array, AI_ARRAY_FORMAT_S32,
  NULL, NULL, 64, AI_STATIC)

/* Array#15 */
AI_ARRAY_OBJ_DECLARE(
  gemm_20_scratch0_array, AI_ARRAY_FORMAT_S16,
  NULL, NULL, 448, AI_STATIC)

/* Array#16 */
AI_ARRAY_OBJ_DECLARE(
  gemm_21_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 1, AI_STATIC)

/* Array#17 */
AI_ARRAY_OBJ_DECLARE(
  nl_22_output_array, AI_ARRAY_FORMAT_S8|AI_FMT_FLAG_IS_IO,
  NULL, NULL, 1, AI_STATIC)



/* Tensor #0 */
AI_TENSOR_OBJ_DECLARE(
  eltwise_5_output, AI_STATIC,
  15, 0x1,
  AI_SHAPE_INIT(4, 1, 32, 248, 1), AI_STRIDE_INIT(4, 1, 1, 32, 7936),
  1, &eltwise_5_output_array, &eltwise_5_output_array_intq)

/* Tensor #1 */
AI_TENSOR_OBJ_DECLARE(
  pool_4_output, AI_STATIC,
  28, 0x1,
  AI_SHAPE_INIT(4, 1, 32, 248, 1), AI_STRIDE_INIT(4, 1, 1, 32, 7936),
  1, &pool_4_output_array, &pool_4_output_array_intq)

/* Tensor #2 */
AI_TENSOR_OBJ_DECLARE(
  sequential_1_1_batch_normalization_2_1_batchnorm_mul_4D, AI_STATIC,
  29, 0x1,
  AI_SHAPE_INIT(4, 1, 32, 1, 1), AI_STRIDE_INIT(4, 1, 1, 32, 32),
  1, &sequential_1_1_batch_normalization_2_1_batchnorm_mul_4D_array, &sequential_1_1_batch_normalization_2_1_batchnorm_mul_4D_array_intq)

/* Tensor #3 */
AI_TENSOR_OBJ_DECLARE(
  eltwise_6_output, AI_STATIC,
  16, 0x1,
  AI_SHAPE_INIT(4, 1, 32, 248, 1), AI_STRIDE_INIT(4, 1, 1, 32, 7936),
  1, &eltwise_6_output_array, &eltwise_6_output_array_intq)

/* Tensor #4 */
AI_TENSOR_OBJ_DECLARE(
  sequential_1_1_batch_normalization_2_1_batchnorm_sub_4D, AI_STATIC,
  30, 0x1,
  AI_SHAPE_INIT(4, 1, 32, 1, 1), AI_STRIDE_INIT(4, 1, 1, 32, 32),
  1, &sequential_1_1_batch_normalization_2_1_batchnorm_sub_4D_array, &sequential_1_1_batch_normalization_2_1_batchnorm_sub_4D_array_intq)

/* Tensor #5 */
AI_TENSOR_OBJ_DECLARE(
  eltwise_13_output, AI_STATIC,
  13, 0x1,
  AI_SHAPE_INIT(4, 1, 64, 122, 1), AI_STRIDE_INIT(4, 1, 1, 64, 7808),
  1, &eltwise_13_output_array, &eltwise_13_output_array_intq)

/* Tensor #6 */
AI_TENSOR_OBJ_DECLARE(
  pool_12_output, AI_STATIC,
  26, 0x1,
  AI_SHAPE_INIT(4, 1, 64, 122, 1), AI_STRIDE_INIT(4, 1, 1, 64, 7808),
  1, &pool_12_output_array, &pool_12_output_array_intq)

/* Tensor #7 */
AI_TENSOR_OBJ_DECLARE(
  sequential_1_1_batch_normalization_3_1_batchnorm_mul_4D, AI_STATIC,
  31, 0x1,
  AI_SHAPE_INIT(4, 1, 64, 1, 1), AI_STRIDE_INIT(4, 1, 1, 64, 64),
  1, &sequential_1_1_batch_normalization_3_1_batchnorm_mul_4D_array, &sequential_1_1_batch_normalization_3_1_batchnorm_mul_4D_array_intq)

/* Tensor #8 */
AI_TENSOR_OBJ_DECLARE(
  eltwise_14_output, AI_STATIC,
  14, 0x1,
  AI_SHAPE_INIT(4, 1, 64, 122, 1), AI_STRIDE_INIT(4, 1, 1, 64, 7808),
  1, &eltwise_14_output_array, &eltwise_14_output_array_intq)

/* Tensor #9 */
AI_TENSOR_OBJ_DECLARE(
  sequential_1_1_batch_normalization_3_1_batchnorm_sub_4D, AI_STATIC,
  32, 0x1,
  AI_SHAPE_INIT(4, 1, 64, 1, 1), AI_STRIDE_INIT(4, 1, 1, 64, 64),
  1, &sequential_1_1_batch_normalization_3_1_batchnorm_sub_4D_array, &sequential_1_1_batch_normalization_3_1_batchnorm_sub_4D_array_intq)

/* Tensor #10 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_17_output0, AI_STATIC,
  2, 0x1,
  AI_SHAPE_INIT(4, 1, 128, 1, 120), AI_STRIDE_INIT(4, 1, 1, 128, 128),
  1, &conv2d_17_output_array, &conv2d_17_output_array_intq)

/* Tensor #11 */
AI_TENSOR_OBJ_DECLARE(
  pool_19_output, AI_STATIC,
  27, 0x1,
  AI_SHAPE_INIT(4, 1, 128, 1, 1), AI_STRIDE_INIT(4, 1, 1, 128, 128),
  1, &pool_19_output_array, &pool_19_output_array_intq)

/* Tensor #12 */
AI_TENSOR_OBJ_DECLARE(
  gemm_20_bias, AI_STATIC,
  17, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 1, 1), AI_STRIDE_INIT(4, 4, 4, 256, 256),
  1, &gemm_20_bias_array, NULL)

/* Tensor #13 */
AI_TENSOR_OBJ_DECLARE(
  gemm_20_output, AI_STATIC,
  18, 0x1,
  AI_SHAPE_INIT(4, 1, 64, 1, 1), AI_STRIDE_INIT(4, 1, 1, 64, 64),
  1, &gemm_20_output_array, &gemm_20_output_array_intq)

/* Tensor #14 */
AI_TENSOR_OBJ_DECLARE(
  gemm_20_scratch0, AI_STATIC,
  19, 0x0,
  AI_SHAPE_INIT(4, 1, 448, 1, 1), AI_STRIDE_INIT(4, 2, 2, 896, 896),
  1, &gemm_20_scratch0_array, NULL)

/* Tensor #15 */
AI_TENSOR_OBJ_DECLARE(
  gemm_20_weights, AI_STATIC,
  20, 0x1,
  AI_SHAPE_INIT(4, 128, 64, 1, 1), AI_STRIDE_INIT(4, 1, 128, 8192, 8192),
  1, &gemm_20_weights_array, &gemm_20_weights_array_intq)

/* Tensor #16 */
AI_TENSOR_OBJ_DECLARE(
  gemm_21_output, AI_STATIC,
  22, 0x1,
  AI_SHAPE_INIT(4, 1, 1, 1, 1), AI_STRIDE_INIT(4, 1, 1, 1, 1),
  1, &gemm_21_output_array, &gemm_21_output_array_intq)

/* Tensor #17 */
AI_TENSOR_OBJ_DECLARE(
  nl_22_output, AI_STATIC,
  25, 0x1,
  AI_SHAPE_INIT(4, 1, 1, 1, 1), AI_STRIDE_INIT(4, 1, 1, 1, 1),
  1, &nl_22_output_array, &nl_22_output_array_intq)


AI_TENSOR_CHAIN_OBJ_DECLARE(
  eltwise_5_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &pool_4_output, &sequential_1_1_batch_normalization_2_1_batchnorm_mul_4D),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &eltwise_5_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  eltwise_5_layer, 5,
  ELTWISE_INTEGER_TYPE, 0x0, NULL,
  eltwise_integer, forward_eltwise_integer_INT8,
  &eltwise_5_chain,
  NULL, &eltwise_5_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_INT8, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  eltwise_6_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &eltwise_5_output, &sequential_1_1_batch_normalization_2_1_batchnorm_sub_4D),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &eltwise_6_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  eltwise_6_layer, 6,
  ELTWISE_INTEGER_TYPE, 0x0, NULL,
  eltwise_integer, forward_eltwise_integer_INT8,
  &eltwise_6_chain,
  NULL, &eltwise_6_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_INT8, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  eltwise_13_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &pool_12_output, &sequential_1_1_batch_normalization_3_1_batchnorm_mul_4D),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &eltwise_13_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  eltwise_13_layer, 13,
  ELTWISE_INTEGER_TYPE, 0x0, NULL,
  eltwise_integer, forward_eltwise_integer_INT8,
  &eltwise_13_chain,
  NULL, &eltwise_13_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_INT8, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  eltwise_14_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &eltwise_13_output, &sequential_1_1_batch_normalization_3_1_batchnorm_sub_4D),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &eltwise_14_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  eltwise_14_layer, 14,
  ELTWISE_INTEGER_TYPE, 0x0, NULL,
  eltwise_integer, forward_eltwise_integer_INT8,
  &eltwise_14_chain,
  NULL, &eltwise_14_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_INT8, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  pool_19_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_17_output0),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &pool_19_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  pool_19_layer, 19,
  POOL_TYPE, 0x0, NULL,
  pool, forward_ap_integer_INT8,
  &pool_19_chain,
  NULL, &pool_19_layer, AI_STATIC, 
  .pool_size = AI_SHAPE_2D_INIT(1, 120), 
  .pool_stride = AI_SHAPE_2D_INIT(1, 120), 
  .pool_pad = AI_SHAPE_INIT(4, 0, 0, 0, 0), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  gemm_20_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &pool_19_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &gemm_20_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &gemm_20_weights, &gemm_20_bias),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &gemm_20_scratch0)
)

AI_LAYER_OBJ_DECLARE(
  gemm_20_layer, 20,
  DENSE_TYPE, 0x0, NULL,
  dense, forward_dense_integer_SSSA_ch,
  &gemm_20_chain,
  NULL, &gemm_20_layer, AI_STATIC, 
)


AI_STATIC_CONST ai_i8 nl_22_nl_params_data[] = { -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -127, -127, -127, -127, -127, -127, -127, -126, -126, -126, -126, -125, -125, -124, -123, -123, -122, -121, -120, -119, -117, -116, -114, -112, -109, -106, -103, -100, -96, -91, -86, -81, -75, -68, -61, -54, -46, -37, -28, -19, -10, 0, 10, 19, 28, 37, 46, 54, 61, 68, 75, 81, 86, 91, 96, 100, 103, 106, 109, 112, 114, 116, 117, 119, 120, 121, 122, 123, 123, 124, 125, 125, 126, 126, 126, 126, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127 };
AI_ARRAY_OBJ_DECLARE(
    nl_22_nl_params, AI_ARRAY_FORMAT_S8,
    nl_22_nl_params_data, nl_22_nl_params_data, 256, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  nl_22_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &gemm_21_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &nl_22_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  nl_22_layer, 22,
  NL_TYPE, 0x0, NULL,
  nl, forward_nl_integer,
  &nl_22_chain,
  NULL, &nl_22_layer, AI_STATIC, 
  .nl_params = &nl_22_nl_params, 
)
/**  Hybrid layers declarations section  *************************************/
void forward_lite_eltwise_integer_INT8_eltwise_5(_stai_network_context* net_ctx)
{
  pool_4_output_array.data = AI_PTR(net_ctx->_activations[0] + 1288);
  pool_4_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 1288);
  sequential_1_1_batch_normalization_2_1_batchnorm_mul_4D_array.data = AI_PTR(net_ctx->_weights[0] + 0);
  sequential_1_1_batch_normalization_2_1_batchnorm_mul_4D_array.data_start = AI_PTR(net_ctx->_weights[0] + 0);
  eltwise_5_output_array.data = AI_PTR(net_ctx->_activations[0] + 9224);
  eltwise_5_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 9224);
  _STAI_NETWORK_EVENT_NODE_START_CB(5, 2, { pool_4_output.data->data,sequential_1_1_batch_normalization_2_1_batchnorm_mul_4D.data->data});
  forward_eltwise_integer_INT8(&eltwise_5_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(5, 1, { eltwise_5_output.data->data});
}
void forward_lite_eltwise_integer_INT8_eltwise_6(_stai_network_context* net_ctx)
{
  eltwise_5_output_array.data = AI_PTR(net_ctx->_activations[0] + 9224);
  eltwise_5_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 9224);
  sequential_1_1_batch_normalization_2_1_batchnorm_sub_4D_array.data = AI_PTR(net_ctx->_weights[0] + 32);
  sequential_1_1_batch_normalization_2_1_batchnorm_sub_4D_array.data_start = AI_PTR(net_ctx->_weights[0] + 32);
  eltwise_6_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  eltwise_6_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_NETWORK_EVENT_NODE_START_CB(6, 2, { eltwise_5_output.data->data,sequential_1_1_batch_normalization_2_1_batchnorm_sub_4D.data->data});
  forward_eltwise_integer_INT8(&eltwise_6_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(6, 1, { eltwise_6_output.data->data});
}
void forward_lite_eltwise_integer_INT8_eltwise_13(_stai_network_context* net_ctx)
{
  pool_12_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  pool_12_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  sequential_1_1_batch_normalization_3_1_batchnorm_mul_4D_array.data = AI_PTR(net_ctx->_weights[0] + 64);
  sequential_1_1_batch_normalization_3_1_batchnorm_mul_4D_array.data_start = AI_PTR(net_ctx->_weights[0] + 64);
  eltwise_13_output_array.data = AI_PTR(net_ctx->_activations[0] + 7808);
  eltwise_13_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 7808);
  _STAI_NETWORK_EVENT_NODE_START_CB(13, 2, { pool_12_output.data->data,sequential_1_1_batch_normalization_3_1_batchnorm_mul_4D.data->data});
  forward_eltwise_integer_INT8(&eltwise_13_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(13, 1, { eltwise_13_output.data->data});
}
void forward_lite_eltwise_integer_INT8_eltwise_14(_stai_network_context* net_ctx)
{
  eltwise_13_output_array.data = AI_PTR(net_ctx->_activations[0] + 7808);
  eltwise_13_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 7808);
  sequential_1_1_batch_normalization_3_1_batchnorm_sub_4D_array.data = AI_PTR(net_ctx->_weights[0] + 128);
  sequential_1_1_batch_normalization_3_1_batchnorm_sub_4D_array.data_start = AI_PTR(net_ctx->_weights[0] + 128);
  eltwise_14_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  eltwise_14_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_NETWORK_EVENT_NODE_START_CB(14, 2, { eltwise_13_output.data->data,sequential_1_1_batch_normalization_3_1_batchnorm_sub_4D.data->data});
  forward_eltwise_integer_INT8(&eltwise_14_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(14, 1, { eltwise_14_output.data->data});
}
void forward_lite_ap_integer_INT8_pool_19(_stai_network_context* net_ctx)
{
  conv2d_17_output_array.data = AI_PTR(net_ctx->_activations[0] + 15488);
  conv2d_17_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 15488);
  pool_19_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  pool_19_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_NETWORK_EVENT_NODE_START_CB(19, 1, { conv2d_17_output0.data->data});
  forward_ap_integer_INT8(&pool_19_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(19, 1, { pool_19_output.data->data});
}
void forward_lite_dense_integer_SSSA_ch_gemm_20(_stai_network_context* net_ctx)
{
  pool_19_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  pool_19_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  gemm_20_weights_array.data = AI_PTR(net_ctx->_weights[0] + 36064);
  gemm_20_weights_array.data_start = AI_PTR(net_ctx->_weights[0] + 36064);
  gemm_20_bias_array.data = AI_PTR(net_ctx->_weights[0] + 44256);
  gemm_20_bias_array.data_start = AI_PTR(net_ctx->_weights[0] + 44256);
  gemm_20_scratch0_array.data = AI_PTR(net_ctx->_activations[0] + 128);
  gemm_20_scratch0_array.data_start = AI_PTR(net_ctx->_activations[0] + 128);
  gemm_20_output_array.data = AI_PTR(net_ctx->_activations[0] + 1024);
  gemm_20_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 1024);
  _STAI_NETWORK_EVENT_NODE_START_CB(20, 1, { pool_19_output.data->data});
  forward_dense_integer_SSSA_ch(&gemm_20_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(20, 1, { gemm_20_output.data->data});
}
void forward_lite_nl_integer_nl_22(_stai_network_context* net_ctx)
{
  gemm_21_output_array.data = AI_PTR(net_ctx->_activations[0] + 128);
  gemm_21_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 128);
  nl_22_output_array.data = AI_PTR(net_ctx->_outputs[0] + 0);
  nl_22_output_array.data_start = AI_PTR(net_ctx->_outputs[0] + 0);
  _STAI_NETWORK_EVENT_NODE_START_CB(22, 1, { gemm_21_output.data->data});
  forward_nl_integer(&nl_22_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(22, 1, { nl_22_output.data->data});
}

/*****************************************************************************/


static const ai_u16 conv2d_1_t_in_0_shape_w_const_u16 = 500;
static const ai_u16 conv2d_1_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_1_t_in_0_shape_ch_const_u16 = 1;
static const ai_u16 conv2d_1_t_out_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_1_t_weight_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_1_t_weight_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_1_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_1_l_stride_0_const_u16 = 1;
static const ai_i32 conv2d_1_l_pad_W_0_const_s32 = 0;
static const ai_i32 conv2d_1_l_pad_H_0_const_s32 = 0;
static const ai_i8 conv2d_1_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_1_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_1_t_in_0_fmt_scale_const_f32 = 0.003921568859368563f;
static const ai_float conv2d_1_t_out_0_fmt_scale_const_f32 = 0.0014048106968402863f;
static const ai_float conv2d_1_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0011568026384338737f, 0.0014715592842549086f, 0.0010826109210029244f, 0.001140361069701612f, 0.001131748198531568f, 0.0013720296556130052f, 0.0015431619249284267f, 0.0008482378907501698f, 0.0008713211864233017f, 0.0008336667087860405f, 0.001417863997630775f, 0.0014975275844335556f, 0.0010351253440603614f, 0.0013209027238190174f, 0.001390355871990323f, 0.0013371767709031701f, 0.0014162756269797683f, 0.0012625670060515404f, 0.0013974477769806981f, 0.0013717940310016274f, 0.0015239565400406718f, 0.0014657097635790706f, 0.001352403312921524f, 0.0015354568604379892f, 0.0012920423178002238f, 0.0009993392741307616f, 0.0014771304558962584f, 0.0014495807699859142f, 0.0014109271578490734f, 0.0015433040680363774f, 0.0012976722791790962f, 0.0003895174595527351f);
static const ai_layer_format_type conv2d_1_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_1_t_out_0_shape_w_const_u16 = 496;
static const ai_u16 conv2d_1_t_out_0_shape_h_const_u16 = 1;

static const ai_u16 pool_4_t_in_0_shape_w_const_u16 = 496;
static const ai_u16 pool_4_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 pool_4_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 pool_4_l_pool_size_1_const_u16 = 2;
static const ai_u16 pool_4_l_pool_size_0_const_u16 = 1;
static const ai_u16 pool_4_l_legacy_pool_pad_1_const_u16 = 0;
static const ai_u16 pool_4_l_legacy_pool_pad_0_const_u16 = 0;
static const ai_u16 pool_4_l_pool_stride_1_const_u16 = 2;
static const ai_u16 pool_4_l_pool_stride_0_const_u16 = 1;
static const ai_u16 pool_4_t_out_0_shape_w_const_u16 = 248;
static const ai_u16 pool_4_t_out_0_shape_h_const_u16 = 1;
static const ai_i8 pool_4_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 pool_4_t_out_0_fmt_zero_const_s8 = -128;



static const ai_u16 conv2d_9_t_in_0_shape_w_const_u16 = 248;
static const ai_u16 conv2d_9_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_9_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_9_t_out_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_9_t_weight_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_9_t_weight_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_9_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_9_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_9_t_in_0_fmt_zero_const_s8 = -50;
static const ai_i8 conv2d_9_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_9_t_in_0_fmt_scale_const_f32 = 0.017635006457567215f;
static const ai_float conv2d_9_t_out_0_fmt_scale_const_f32 = 0.013224381022155285f;
static const ai_float conv2d_9_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.001389324781484902f, 0.0012454285752028227f, 0.0019722175784409046f, 0.0010584192350506783f, 0.0013317989651113749f, 0.0016040774062275887f, 0.0011707309167832136f, 0.0009628189145587385f, 0.001388463657349348f, 0.0017462719697505236f, 0.0009877795819193125f, 0.0009515173151157796f, 0.001455332268960774f, 0.0011738172033801675f, 0.0014379429630935192f, 0.0012868938501924276f, 0.001506621832959354f, 0.0017100502736866474f, 0.000959322729613632f, 0.0015413581859320402f, 0.00109871756285429f, 0.000928883848246187f, 0.0018622780917212367f, 0.001621708506718278f, 0.0009992389241233468f, 0.0013210908509790897f, 0.0009711563470773399f, 0.0015871045179665089f, 0.002006299328058958f, 0.0013600458623841405f, 0.00213726912625134f, 0.0009101873729377985f, 0.0017169428756460547f, 0.0011287451488897204f, 0.0018947409698739648f, 0.0009598873439244926f, 0.0013964134268462658f, 0.0015857797116041183f, 0.001015506568364799f, 0.0016483730869367719f, 0.0015067750355228782f, 0.0010528250131756067f, 0.0019264882430434227f, 0.0009399460977874696f, 0.001316850888542831f, 0.0014909447636455297f, 0.0013180241221562028f, 0.0010968793649226427f, 0.0009463186142966151f, 0.0013830515090376139f, 0.0011409490834921598f, 0.0010557514615356922f, 0.0017542432760819793f, 0.001024506171233952f, 0.0012743020197376609f, 0.0016486982349306345f, 0.0015997971640899777f, 0.0017477278597652912f, 0.0016554923495277762f, 0.0010630300967022777f, 0.0013550739968195558f, 0.0018545727944001555f, 0.001215414609760046f, 0.0010536391055211425f);
static const ai_layer_format_type conv2d_9_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_9_t_out_0_shape_w_const_u16 = 244;
static const ai_u16 conv2d_9_t_out_0_shape_h_const_u16 = 1;

static const ai_u16 pool_12_t_in_0_shape_w_const_u16 = 244;
static const ai_u16 pool_12_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 pool_12_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 pool_12_l_pool_size_1_const_u16 = 2;
static const ai_u16 pool_12_l_pool_size_0_const_u16 = 1;
static const ai_u16 pool_12_l_legacy_pool_pad_1_const_u16 = 0;
static const ai_u16 pool_12_l_legacy_pool_pad_0_const_u16 = 0;
static const ai_u16 pool_12_l_pool_stride_1_const_u16 = 2;
static const ai_u16 pool_12_l_pool_stride_0_const_u16 = 1;
static const ai_u16 pool_12_t_out_0_shape_w_const_u16 = 122;
static const ai_u16 pool_12_t_out_0_shape_h_const_u16 = 1;
static const ai_i8 pool_12_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 pool_12_t_out_0_fmt_zero_const_s8 = -128;



static const ai_u16 conv2d_17_t_in_0_shape_w_const_u16 = 122;
static const ai_u16 conv2d_17_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_17_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_17_t_out_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_17_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_17_t_weight_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_17_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_17_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_17_t_in_0_fmt_zero_const_s8 = -67;
static const ai_i8 conv2d_17_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_17_t_in_0_fmt_scale_const_f32 = 0.028812313452363014f;
static const ai_float conv2d_17_t_out_0_fmt_scale_const_f32 = 0.028102146461606026f;
static const ai_float conv2d_17_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0008924197754822671f, 0.0008559733396396041f, 0.0008481998229399323f, 0.0008823257521726191f, 0.0008641626336611807f, 0.0010344069451093674f, 0.0009595336159691215f, 0.0008820114308036864f, 0.000900686951354146f, 0.0009167311363853514f, 0.0008701869519427419f, 0.0009672135347500443f, 0.000895805424079299f, 0.001006670412607491f, 0.0008376614423468709f, 0.0010509069543331861f, 0.0008903170237317681f, 0.0008789114071987569f, 0.0008674940327182412f, 0.0008385144756175578f, 0.0009709215955808759f, 0.0008611958473920822f, 0.0009899957804009318f, 0.0009032886591739953f, 0.0010285945609211922f, 0.0009225244866684079f, 0.0009310216992162168f, 0.0008035055361688137f, 0.0010137555655092f, 0.0009548167581669986f, 0.0008767598192207515f, 0.00085774454055354f, 0.0009330482571385801f, 0.000941049656830728f, 0.0010004821233451366f, 0.0008330190903507173f, 0.0009630331187509f, 0.0009146883385255933f, 0.0008604262839071453f, 0.0008355822064913809f, 0.0010492898290976882f, 0.0008702656487002969f, 0.0009455234394408762f, 0.0008224465418606997f, 0.0009584868093952537f, 0.0008896227809600532f, 0.0009452185477130115f, 0.0009950384264811873f, 0.0010029623517766595f, 0.0008661391912028193f, 0.0008873327751643956f, 0.000958419288508594f, 0.0009502093307673931f, 0.0010495486203581095f, 0.0009751336765475571f, 0.0008152006193995476f, 0.000995979062281549f, 0.0008456049836240709f, 0.0008769097621552646f, 0.0008833375759422779f, 0.0008686692453920841f, 0.0008424724801443517f, 0.0009161013877019286f, 0.0008534053340554237f, 0.0008921021362766623f, 0.0008828216232359409f, 0.000832687015645206f, 0.0008306404342874885f, 0.0008390851435251534f, 0.0009132391423918307f, 0.0009178046020679176f, 0.0008692233241163194f, 0.0009081162279471755f, 0.0008713903953321278f, 0.0009407027391716838f, 0.0010116100311279297f, 0.0009250136790797114f, 0.0008631695527583361f, 0.0010198649251833558f, 0.0008550554630346596f, 0.0010377343278378248f, 0.0008444400154985487f, 0.0011001665843650699f, 0.0008807057747617364f, 0.0008964742883108556f, 0.0009091724641621113f, 0.0009640809730626643f, 0.0008833399042487144f, 0.0009730129386298358f, 0.0009274579933844507f, 0.0010376336285844445f, 0.000878418970387429f, 0.0008672987460158765f, 0.0008812840096652508f, 0.0009049431537277997f, 0.0008029010496102273f, 0.000919486628845334f, 0.0009597696480341256f, 0.0010176614159718156f, 0.0008942889980971813f, 0.0009905382758006454f, 0.0009730008896440268f, 0.0009541307808831334f, 0.0008997818222269416f, 0.0008818487403914332f, 0.0008027621661312878f, 0.0010145364794880152f, 0.0009882854064926505f, 0.00100108259357512f, 0.0009191833669319749f, 0.0008456447394564748f, 0.0008762610959820449f, 0.0008751931600272655f, 0.0009399837581440806f, 0.0008215215057134628f, 0.0010092706652358174f, 0.0010069863637909293f, 0.0009122927440330386f, 0.0008908496820367873f, 0.0010060109198093414f, 0.0008987805340439081f, 0.0008356587495654821f, 0.0008041891851462424f, 0.0010117802303284407f, 0.0008809770806692541f, 0.0008731458801776171f, 0.0008710592519491911f, 0.0008785809041000903f);
static const ai_layer_format_type conv2d_17_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_17_t_out_0_shape_w_const_u16 = 120;
static const ai_u16 conv2d_17_t_out_0_shape_h_const_u16 = 1;



static const ai_i8 gemm_21_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 gemm_21_t_out_0_fmt_zero_const_s8 = -8;
static const ai_u16 gemm_21_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 gemm_21_t_out_0_shape_ch_const_u16 = 1;
static const ai_u32 gemm_21_t_out_0_shape_h_w_prod_const_u32 = 1;
static const ai_float gemm_21_t_in_0_fmt_scale_const_f32 = 0.03538158908486366f;
static const ai_float gemm_21_t_out_0_fmt_scale_const_f32 = 0.1490134447813034f;
static const ai_float gemm_21_t_weight_0_fmt_scale_const_f32 = 0.002825228264555335f;

STAI_API_ENTRY
stai_return_code stai_network_run(
  stai_network* network,
  const stai_run_mode mode)
{
   STAI_UNUSED(mode)
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_ACTIVATIONS) != STAI_FLAG_ACTIVATIONS,
        STAI_ERROR_NETWORK_INVALID_ACTIVATIONS_PTR, net_ctx->_return_code)

  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_INPUTS) != STAI_FLAG_INPUTS,
                  STAI_ERROR_NETWORK_INVALID_IN_PTR, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_OUTPUTS) != STAI_FLAG_OUTPUTS,
                  STAI_ERROR_NETWORK_INVALID_OUT_PTR, net_ctx->_return_code)

  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_WEIGHTS) != STAI_FLAG_WEIGHTS,
                  STAI_ERROR_NETWORK_INVALID_WEIGHTS_PTR, net_ctx->_return_code)


  /* LITE_KERNEL_SECTION BEGIN conv2d_1 */
  {
      const ai_i8* conv2d_1_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_inputs[0] + 0);
    const ai_i8* conv2d_1_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 192);
    const ai_i32* conv2d_1_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 352);
    ai_i8* conv2d_1_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1288);
    ai_i16* conv2d_1_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 500);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(1, 1, {(stai_ptr) conv2d_1_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_sssa8_ch(conv2d_1_t_in_0_ptr_const_s8, conv2d_1_t_in_0_shape_w_const_u16, conv2d_1_t_in_0_shape_h_const_u16, conv2d_1_t_in_0_shape_ch_const_u16, conv2d_1_t_weight_0_ptr_const_s8, conv2d_1_t_out_0_shape_ch_const_u16, conv2d_1_t_weight_0_shape_w_const_u16, conv2d_1_t_weight_0_shape_h_const_u16, conv2d_1_l_stride_1_const_u16, conv2d_1_l_stride_0_const_u16, conv2d_1_l_pad_W_0_const_s32, conv2d_1_l_pad_H_0_const_s32, conv2d_1_t_weight_1_ptr_const_s32, conv2d_1_t_in_0_fmt_zero_const_s8, conv2d_1_t_out_0_fmt_zero_const_s8, conv2d_1_t_in_0_fmt_scale_const_f32, conv2d_1_t_out_0_fmt_scale_const_f32, conv2d_1_t_weight_0_fmt_scale_const_f32, conv2d_1_l_out_ch_format_const_layer_format_type, conv2d_1_t_out_0_ptr_s8, conv2d_1_t_out_0_shape_w_const_u16, conv2d_1_t_out_0_shape_h_const_u16, 1, 788, conv2d_1_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(1, 1, {(stai_ptr) conv2d_1_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_1 */
  /* LITE_KERNEL_SECTION BEGIN pool_4 */
  {
      const ai_i8* pool_4_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1288);
    ai_i8* pool_4_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1288);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(4, 1, {(stai_ptr) pool_4_t_in_0_ptr_const_s8});
    
  forward_lite_maxpool_is8os8_scalepos(pool_4_t_in_0_ptr_const_s8, pool_4_t_out_0_ptr_s8, pool_4_t_in_0_shape_w_const_u16, pool_4_t_in_0_shape_h_const_u16, pool_4_t_in_0_shape_ch_const_u16, pool_4_l_pool_size_1_const_u16, pool_4_l_pool_size_0_const_u16, pool_4_l_legacy_pool_pad_1_const_u16, pool_4_l_legacy_pool_pad_0_const_u16, pool_4_l_pool_stride_1_const_u16, pool_4_l_pool_stride_0_const_u16, pool_4_t_out_0_shape_w_const_u16, pool_4_t_out_0_shape_h_const_u16, 1.0f, pool_4_t_in_0_fmt_zero_const_s8, pool_4_t_out_0_fmt_zero_const_s8);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(4, 1, {(stai_ptr) pool_4_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END pool_4 */
  /* LITE_KERNEL_SECTION BEGIN eltwise_5 */
  {
    
  forward_lite_eltwise_integer_INT8_eltwise_5(net_ctx);
  }
  /* LITE_KERNEL_SECTION END eltwise_5 */
  /* LITE_KERNEL_SECTION BEGIN eltwise_6 */
  {
    
  forward_lite_eltwise_integer_INT8_eltwise_6(net_ctx);
  }
  /* LITE_KERNEL_SECTION END eltwise_6 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_9 */
  {
      const ai_i8* conv2d_9_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    const ai_i8* conv2d_9_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 480);
    const ai_i32* conv2d_9_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 10720);
    ai_i8* conv2d_9_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 14592);
    ai_i16* conv2d_9_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 7936);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(9, 1, {(stai_ptr) conv2d_9_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_sssa8_ch(conv2d_9_t_in_0_ptr_const_s8, conv2d_9_t_in_0_shape_w_const_u16, conv2d_9_t_in_0_shape_h_const_u16, conv2d_9_t_in_0_shape_ch_const_u16, conv2d_9_t_weight_0_ptr_const_s8, conv2d_9_t_out_0_shape_ch_const_u16, conv2d_9_t_weight_0_shape_w_const_u16, conv2d_9_t_weight_0_shape_h_const_u16, conv2d_9_l_stride_1_const_u16, conv2d_9_l_stride_0_const_u16, conv2d_9_t_weight_1_ptr_const_s32, conv2d_9_t_in_0_fmt_zero_const_s8, conv2d_9_t_out_0_fmt_zero_const_s8, conv2d_9_t_in_0_fmt_scale_const_f32, conv2d_9_t_out_0_fmt_scale_const_f32, conv2d_9_t_weight_0_fmt_scale_const_f32, conv2d_9_l_out_ch_format_const_layer_format_type, conv2d_9_t_out_0_ptr_s8, conv2d_9_t_out_0_shape_w_const_u16, conv2d_9_t_out_0_shape_h_const_u16, 1, 1, 6656, conv2d_9_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(9, 1, {(stai_ptr) conv2d_9_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_9 */
  /* LITE_KERNEL_SECTION BEGIN pool_12 */
  {
      const ai_i8* pool_12_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 14592);
    ai_i8* pool_12_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(12, 1, {(stai_ptr) pool_12_t_in_0_ptr_const_s8});
    
  forward_lite_maxpool_is8os8_scalepos(pool_12_t_in_0_ptr_const_s8, pool_12_t_out_0_ptr_s8, pool_12_t_in_0_shape_w_const_u16, pool_12_t_in_0_shape_h_const_u16, pool_12_t_in_0_shape_ch_const_u16, pool_12_l_pool_size_1_const_u16, pool_12_l_pool_size_0_const_u16, pool_12_l_legacy_pool_pad_1_const_u16, pool_12_l_legacy_pool_pad_0_const_u16, pool_12_l_pool_stride_1_const_u16, pool_12_l_pool_stride_0_const_u16, pool_12_t_out_0_shape_w_const_u16, pool_12_t_out_0_shape_h_const_u16, 1.0f, pool_12_t_in_0_fmt_zero_const_s8, pool_12_t_out_0_fmt_zero_const_s8);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(12, 1, {(stai_ptr) pool_12_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END pool_12 */
  /* LITE_KERNEL_SECTION BEGIN eltwise_13 */
  {
    
  forward_lite_eltwise_integer_INT8_eltwise_13(net_ctx);
  }
  /* LITE_KERNEL_SECTION END eltwise_13 */
  /* LITE_KERNEL_SECTION BEGIN eltwise_14 */
  {
    
  forward_lite_eltwise_integer_INT8_eltwise_14(net_ctx);
  }
  /* LITE_KERNEL_SECTION END eltwise_14 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_17 */
  {
      const ai_i8* conv2d_17_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    const ai_i8* conv2d_17_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 10976);
    const ai_i32* conv2d_17_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 35552);
    ai_i8* conv2d_17_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 15488);
    ai_i16* conv2d_17_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 7808);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(17, 1, {(stai_ptr) conv2d_17_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_sssa8_ch(conv2d_17_t_in_0_ptr_const_s8, conv2d_17_t_in_0_shape_w_const_u16, conv2d_17_t_in_0_shape_h_const_u16, conv2d_17_t_in_0_shape_ch_const_u16, conv2d_17_t_weight_0_ptr_const_s8, conv2d_17_t_out_0_shape_ch_const_u16, conv2d_17_t_weight_0_shape_w_const_u16, conv2d_17_t_weight_0_shape_h_const_u16, conv2d_17_l_stride_1_const_u16, conv2d_17_l_stride_0_const_u16, conv2d_17_t_weight_1_ptr_const_s32, conv2d_17_t_in_0_fmt_zero_const_s8, conv2d_17_t_out_0_fmt_zero_const_s8, conv2d_17_t_in_0_fmt_scale_const_f32, conv2d_17_t_out_0_fmt_scale_const_f32, conv2d_17_t_weight_0_fmt_scale_const_f32, conv2d_17_l_out_ch_format_const_layer_format_type, conv2d_17_t_out_0_ptr_s8, conv2d_17_t_out_0_shape_w_const_u16, conv2d_17_t_out_0_shape_h_const_u16, 1, 1, 7680, conv2d_17_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(17, 1, {(stai_ptr) conv2d_17_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_17 */
  /* LITE_KERNEL_SECTION BEGIN pool_19 */
  {
    
  forward_lite_ap_integer_INT8_pool_19(net_ctx);
  }
  /* LITE_KERNEL_SECTION END pool_19 */
  /* LITE_KERNEL_SECTION BEGIN gemm_20 */
  {
    
  forward_lite_dense_integer_SSSA_ch_gemm_20(net_ctx);
  }
  /* LITE_KERNEL_SECTION END gemm_20 */
  /* LITE_KERNEL_SECTION BEGIN gemm_21 */
  {
      ai_i8* gemm_21_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 128);
    const ai_i8* gemm_21_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1024);
    const ai_i8* gemm_21_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 44512);
    const ai_i32* gemm_21_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 44576);
    ai_i16* gemm_21_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(21, 1, {(stai_ptr) gemm_21_t_in_0_ptr_const_s8});
    
  forward_lite_dense_is8os8ws8(gemm_21_t_out_0_ptr_s8, gemm_21_t_in_0_ptr_const_s8, gemm_21_t_weight_0_ptr_const_s8, gemm_21_t_weight_1_ptr_const_s32, gemm_21_t_in_0_fmt_zero_const_s8, gemm_21_t_out_0_fmt_zero_const_s8, gemm_21_t_in_0_shape_ch_const_u16, gemm_21_t_out_0_shape_ch_const_u16, gemm_21_t_out_0_shape_h_w_prod_const_u32, gemm_21_t_in_0_fmt_scale_const_f32, gemm_21_t_out_0_fmt_scale_const_f32, gemm_21_t_weight_0_fmt_scale_const_f32, gemm_21_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(21, 1, {(stai_ptr) gemm_21_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END gemm_21 */
  /* LITE_KERNEL_SECTION BEGIN nl_22 */
  {
    
  forward_lite_nl_integer_nl_22(net_ctx);
  }
  /* LITE_KERNEL_SECTION END nl_22 */
  return net_ctx->_return_code;
}

/*****************************************************************************/
/*  Getters APIs Section  */
STAI_API_ENTRY
stai_size stai_network_get_context_size()
{
  return (stai_size)STAI_NETWORK_CONTEXT_SIZE;
}

#if defined(HAVE_NETWORK_INFO)
STAI_API_ENTRY
stai_return_code stai_network_get_info(
  stai_network* network,
  stai_network_info* info)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, info==NULL, STAI_ERROR_NETWORK_INVALID_INFO, net_ctx->_return_code)

  // Copy of network info struct
  *info = g_network_info;

  return STAI_SUCCESS;
}
#endif


STAI_API_ENTRY
stai_return_code stai_network_get_activations(
  stai_network* network, stai_ptr* activations, stai_size* n_activations)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  _STAI_SET_ERROR(net_ctx, !n_activations, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_activations = STAI_NETWORK_ACTIVATIONS_NUM;
for (stai_size idx=0; activations && (idx<STAI_NETWORK_ACTIVATIONS_NUM); idx++) {
    // get address of the activations buffers
    activations[idx] = net_ctx->_activations[idx];
  }return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_weights(
  stai_network* network, stai_ptr* weights, stai_size* n_weights)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_weights, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_weights = STAI_NETWORK_WEIGHTS_NUM;
for (stai_size idx=0; weights && (idx<STAI_NETWORK_WEIGHTS_NUM); idx++) {
    // get address of the weights buffers
    weights[idx] = net_ctx->_weights[idx];
  }return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_inputs(
  stai_network* network, stai_ptr* inputs, stai_size* n_inputs)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_inputs, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_inputs = STAI_NETWORK_IN_NUM;
  for (stai_size idx=0; inputs && (idx<STAI_NETWORK_IN_NUM); idx++) {
    inputs[idx] = net_ctx->_inputs[idx];
  }
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_outputs(
  stai_network* network, stai_ptr* outputs, stai_size* n_outputs)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_outputs, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_outputs = STAI_NETWORK_OUT_NUM;
  for (stai_size idx=0; outputs && (idx<STAI_NETWORK_OUT_NUM); idx++) {
    outputs[idx] = net_ctx->_outputs[idx];
  }
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_error(
  stai_network* network)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  /* return 1st generated error or STAI_SUCCESS if no errors so far */
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_states(
  stai_network* network, stai_ptr* states, stai_size* n_states)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_states, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  /* get the number of internals states (supporting multi-heap also for internal states) */
  *n_states = STAI_NETWORK_STATES_NUM;

  STAI_UNUSED(states)
return net_ctx->_return_code;
}


/*****************************************************************************/
/*  Setters APIs Section  */

STAI_API_ENTRY
stai_return_code stai_network_set_activations(
  stai_network* network,
  const stai_ptr* activations,
  const stai_size n_activations)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
const uintptr_t _activations_alignment[] = STAI_NETWORK_ACTIVATIONS_ALIGNMENTS;
  STAI_PRINT("  [stai_network_set_activations] network(%p) activations[%d]: %p\n\n", net_ctx, n_activations, activations)
  _STAI_SET_ERROR(net_ctx, !activations,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_activations!=STAI_NETWORK_ACTIVATIONS_NUM,
                  STAI_ERROR_NETWORK_INVALID_ACTIVATIONS_NUM, net_ctx->_return_code)

  for (stai_size idx=0; activations && idx<STAI_NETWORK_ACTIVATIONS_NUM; idx++) {
    STAI_PRINT("  activation[%d]: %p\n", idx, activations[idx])
    _STAI_SET_ERROR(net_ctx, activations[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_ACTIVATIONS_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)activations[idx]) & (_activations_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_activations[idx] = activations[idx];
  }
  net_ctx->_inputs[0] = activations[0] + 0;

  net_ctx->_outputs[0] = activations[0] + 0;
_stai_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_weights(
  stai_network* network,
  const stai_ptr* weights,
  const stai_size n_weights)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
const uintptr_t _weights_alignment[] = STAI_NETWORK_WEIGHTS_ALIGNMENTS;
  _STAI_SET_ERROR(net_ctx, !weights,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_weights!=STAI_NETWORK_WEIGHTS_NUM,
                  STAI_ERROR_NETWORK_INVALID_WEIGHTS_NUM, net_ctx->_return_code)
  for (stai_size idx=0; weights && idx<STAI_NETWORK_WEIGHTS_NUM; idx++) {
    STAI_PRINT("  weight[%d]: %p\n", idx, weights[idx])
    _STAI_SET_ERROR(net_ctx, weights[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_WEIGHTS_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)weights[idx]) & (_weights_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_weights[idx] = weights[idx];
  }_stai_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_inputs(
  stai_network* network,
  const stai_ptr* inputs,
  const stai_size n_inputs)
{
  const uintptr_t _inputs_alignment[] = STAI_NETWORK_IN_ALIGNMENTS;
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !inputs,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_inputs!=STAI_NETWORK_IN_NUM,
                  STAI_ERROR_NETWORK_INVALID_IN_NUM, net_ctx->_return_code)

  for (stai_size idx=0; inputs && idx<STAI_NETWORK_IN_NUM; idx++) {
    STAI_PRINT("  input[%d]: %p\n", idx, inputs[idx])
    _STAI_SET_ERROR(net_ctx, inputs[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_IN_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)inputs[idx]) & (_inputs_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_inputs[idx] = inputs[idx];
  }

  _stai_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_outputs(
  stai_network* network,
  const stai_ptr* outputs,
  const stai_size n_outputs)
{
  const uintptr_t _outputs_alignment[] = STAI_NETWORK_OUT_ALIGNMENTS;
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !outputs,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_outputs!=STAI_NETWORK_OUT_NUM,
                  STAI_ERROR_NETWORK_INVALID_OUT_NUM, net_ctx->_return_code)

  for (stai_size idx=0; outputs && idx<n_outputs; idx++) {
    STAI_PRINT("  output[%d]: %p\n", idx, outputs[idx])
    _STAI_SET_ERROR(net_ctx, outputs[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_OUT_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)outputs[idx]) & (_outputs_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_outputs[idx] = outputs[idx];
  }

  _stai_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_states(
  stai_network* network,
  const stai_ptr* states,
  const stai_size n_states)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  STAI_UNUSED(states)
  STAI_UNUSED(n_states)
_stai_network_check(net_ctx);
  return net_ctx->_return_code;
}

STAI_API_ENTRY
stai_return_code stai_network_set_callback(
  stai_network* network, const stai_event_cb cb, void* cb_cookie)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  STAI_PRINT("  set_callback %p cb %p cookie %p\n", net_ctx, cb, cb_cookie)
  // _STAI_SET_ERROR(net_ctx, cb==NULL, STAI_ERROR_NETWORK_INVALID_CALLBACK, net_ctx->_return_code)
  net_ctx->_callback = cb;
  net_ctx->_callback_cookie = cb_cookie;
  return net_ctx->_return_code;
}

#undef _STAI_SET_ERROR
#undef _STAI_CONTEXT_ALIGNMENT
#undef _STAI_CONTEXT_ACQUIRE
#undef _STAI_NETWORK_EVENT_NODE_START_CB
#undef _STAI_NETWORK_EVENT_NODE_STOP_CB
#undef _STAI_NETWORK_MODEL_SIGNATURE
#undef _STAI_NETWORK_DATETIME
#undef _STAI_NETWORK_COMPILE_DATETIME

