/**
  ******************************************************************************
  * @file    network.c
  * @author  AST Embedded Analytics Research Platform
  * @date    2026-08-05T08:45:57+0000
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

#include "lite_operators.h"

#include "ai_lite_inspect.h"
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
#define _STAI_NETWORK_MODEL_SIGNATURE     "0xc8ce5f5d5cdb6251fa6cb877871db1a6"
#define _STAI_NETWORK_DATETIME            "2026-08-05T08:45:57+0000"
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
      STAI_DECLARE_ARRAY(int32_t, 3, 1, 131, 1),
      STAI_DECLARE_ARRAY(float, 1, 0.0008917094091884792f),
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
      STAI_DECLARE_ARRAY(int32_t, 1, 12992),
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
    AI_PACK_INTQ_SCALE(0.00027663636137731373f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #1 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_5_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.011086185462772846f),
    AI_PACK_INTQ_ZP(-67)))

/* Int quant #2 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(sequential_2_1_batch_normalization_4_1_batchnorm_mul_4D_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.13796265423297882f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #3 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_6_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.008336790837347507f),
    AI_PACK_INTQ_ZP(-47)))

/* Int quant #4 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(sequential_2_1_batch_normalization_4_1_batchnorm_sub_4D_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0036284225061535835f),
    AI_PACK_INTQ_ZP(120)))

/* Int quant #5 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(pool_12_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.007413871120661497f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #6 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_13_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.06764340400695801f),
    AI_PACK_INTQ_ZP(-103)))

/* Int quant #7 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(sequential_2_1_batch_normalization_5_1_batchnorm_mul_4D_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.09660603851079941f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #8 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_14_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.06677153706550598f),
    AI_PACK_INTQ_ZP(-102)))

/* Int quant #9 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(sequential_2_1_batch_normalization_5_1_batchnorm_sub_4D_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.006735711358487606f),
    AI_PACK_INTQ_ZP(127)))

/* Int quant #10 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_17_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.1057518720626831f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #11 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(pool_19_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.02369517646729946f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #12 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(gemm_20_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.04072719067335129f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #13 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(gemm_20_weights_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 64,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0020338541362434626f, 0.0017587844049558043f, 0.0018013291992247105f, 0.0017254292033612728f, 0.0019882142078131437f, 0.0018674196908250451f, 0.002045466098934412f, 0.001698788022622466f, 0.001969472039490938f, 0.0017314422875642776f, 0.0018496920820325613f, 0.001636625500395894f, 0.0019454809371381998f, 0.0017063998384401202f, 0.0017866295529529452f, 0.0019014705903828144f, 0.0023337076418101788f, 0.001498028403148055f, 0.0014386418042704463f, 0.0018002399010583758f, 0.0013910826528444886f, 0.0016383862821385264f, 0.0018138089217245579f, 0.0016742757288739085f, 0.0018544516060501337f, 0.0015677844639867544f, 0.001798404729925096f, 0.0016505339881405234f, 0.001795896445401013f, 0.0014630163786932826f, 0.0018787701847031713f, 0.001371531281620264f, 0.0024119310546666384f, 0.001890472136437893f, 0.0020638962741941214f, 0.002127939136698842f, 0.0016684179427102208f, 0.001413343590684235f, 0.0017602479783818126f, 0.001808816334232688f, 0.0017940226243808866f, 0.0016085413517430425f, 0.0020536049269139767f, 0.0014435482444241643f, 0.001936362124979496f, 0.0018199977930635214f, 0.0016195593634620309f, 0.0015148570528253913f, 0.0017793586011976004f, 0.001692853169515729f, 0.0015016710385680199f, 0.0018754525808617473f, 0.0016985369147732854f, 0.001713771722279489f, 0.0014786138199269772f, 0.0017380152130499482f, 0.0013786302879452705f, 0.0014874532353132963f, 0.0018385453149676323f, 0.002262183465063572f, 0.001814930816181004f, 0.0017421087250113487f, 0.0019505289383232594f, 0.0016862411284819245f),
    AI_PACK_INTQ_ZP(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)))

/* Int quant #14 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(gemm_21_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.200651153922081f),
    AI_PACK_INTQ_ZP(-88)))

/* Int quant #15 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(nl_22_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.00390625f),
    AI_PACK_INTQ_ZP(-128)))



/* Array#0 */
AI_ARRAY_OBJ_DECLARE(
  pool_4_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 2016, AI_STATIC)

/* Array#1 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_5_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 2016, AI_STATIC)

/* Array#2 */
AI_ARRAY_OBJ_DECLARE(
  sequential_2_1_batch_normalization_4_1_batchnorm_mul_4D_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 32, AI_STATIC)

/* Array#3 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_6_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 2016, AI_STATIC)

/* Array#4 */
AI_ARRAY_OBJ_DECLARE(
  sequential_2_1_batch_normalization_4_1_batchnorm_sub_4D_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 32, AI_STATIC)

/* Array#5 */
AI_ARRAY_OBJ_DECLARE(
  pool_12_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 1856, AI_STATIC)

/* Array#6 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_13_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 1856, AI_STATIC)

/* Array#7 */
AI_ARRAY_OBJ_DECLARE(
  sequential_2_1_batch_normalization_5_1_batchnorm_mul_4D_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 64, AI_STATIC)

/* Array#8 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_14_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 1856, AI_STATIC)

/* Array#9 */
AI_ARRAY_OBJ_DECLARE(
  sequential_2_1_batch_normalization_5_1_batchnorm_sub_4D_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 64, AI_STATIC)

/* Array#10 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_17_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 3456, AI_STATIC)

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
  AI_SHAPE_INIT(4, 1, 32, 63, 1), AI_STRIDE_INIT(4, 1, 1, 32, 2016),
  1, &eltwise_5_output_array, &eltwise_5_output_array_intq)

/* Tensor #1 */
AI_TENSOR_OBJ_DECLARE(
  pool_4_output, AI_STATIC,
  28, 0x1,
  AI_SHAPE_INIT(4, 1, 32, 63, 1), AI_STRIDE_INIT(4, 1, 1, 32, 2016),
  1, &pool_4_output_array, &pool_4_output_array_intq)

/* Tensor #2 */
AI_TENSOR_OBJ_DECLARE(
  sequential_2_1_batch_normalization_4_1_batchnorm_mul_4D, AI_STATIC,
  29, 0x1,
  AI_SHAPE_INIT(4, 1, 32, 1, 1), AI_STRIDE_INIT(4, 1, 1, 32, 32),
  1, &sequential_2_1_batch_normalization_4_1_batchnorm_mul_4D_array, &sequential_2_1_batch_normalization_4_1_batchnorm_mul_4D_array_intq)

/* Tensor #3 */
AI_TENSOR_OBJ_DECLARE(
  eltwise_6_output, AI_STATIC,
  16, 0x1,
  AI_SHAPE_INIT(4, 1, 32, 63, 1), AI_STRIDE_INIT(4, 1, 1, 32, 2016),
  1, &eltwise_6_output_array, &eltwise_6_output_array_intq)

/* Tensor #4 */
AI_TENSOR_OBJ_DECLARE(
  sequential_2_1_batch_normalization_4_1_batchnorm_sub_4D, AI_STATIC,
  30, 0x1,
  AI_SHAPE_INIT(4, 1, 32, 1, 1), AI_STRIDE_INIT(4, 1, 1, 32, 32),
  1, &sequential_2_1_batch_normalization_4_1_batchnorm_sub_4D_array, &sequential_2_1_batch_normalization_4_1_batchnorm_sub_4D_array_intq)

/* Tensor #5 */
AI_TENSOR_OBJ_DECLARE(
  eltwise_13_output, AI_STATIC,
  13, 0x1,
  AI_SHAPE_INIT(4, 1, 64, 29, 1), AI_STRIDE_INIT(4, 1, 1, 64, 1856),
  1, &eltwise_13_output_array, &eltwise_13_output_array_intq)

/* Tensor #6 */
AI_TENSOR_OBJ_DECLARE(
  pool_12_output, AI_STATIC,
  26, 0x1,
  AI_SHAPE_INIT(4, 1, 64, 29, 1), AI_STRIDE_INIT(4, 1, 1, 64, 1856),
  1, &pool_12_output_array, &pool_12_output_array_intq)

/* Tensor #7 */
AI_TENSOR_OBJ_DECLARE(
  sequential_2_1_batch_normalization_5_1_batchnorm_mul_4D, AI_STATIC,
  31, 0x1,
  AI_SHAPE_INIT(4, 1, 64, 1, 1), AI_STRIDE_INIT(4, 1, 1, 64, 64),
  1, &sequential_2_1_batch_normalization_5_1_batchnorm_mul_4D_array, &sequential_2_1_batch_normalization_5_1_batchnorm_mul_4D_array_intq)

/* Tensor #8 */
AI_TENSOR_OBJ_DECLARE(
  eltwise_14_output, AI_STATIC,
  14, 0x1,
  AI_SHAPE_INIT(4, 1, 64, 29, 1), AI_STRIDE_INIT(4, 1, 1, 64, 1856),
  1, &eltwise_14_output_array, &eltwise_14_output_array_intq)

/* Tensor #9 */
AI_TENSOR_OBJ_DECLARE(
  sequential_2_1_batch_normalization_5_1_batchnorm_sub_4D, AI_STATIC,
  32, 0x1,
  AI_SHAPE_INIT(4, 1, 64, 1, 1), AI_STRIDE_INIT(4, 1, 1, 64, 64),
  1, &sequential_2_1_batch_normalization_5_1_batchnorm_sub_4D_array, &sequential_2_1_batch_normalization_5_1_batchnorm_sub_4D_array_intq)

/* Tensor #10 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_17_output0, AI_STATIC,
  2, 0x1,
  AI_SHAPE_INIT(4, 1, 128, 1, 27), AI_STRIDE_INIT(4, 1, 1, 128, 128),
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
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &pool_4_output, &sequential_2_1_batch_normalization_4_1_batchnorm_mul_4D),
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
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &eltwise_5_output, &sequential_2_1_batch_normalization_4_1_batchnorm_sub_4D),
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
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &pool_12_output, &sequential_2_1_batch_normalization_5_1_batchnorm_mul_4D),
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
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &eltwise_13_output, &sequential_2_1_batch_normalization_5_1_batchnorm_sub_4D),
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
  .pool_size = AI_SHAPE_2D_INIT(1, 27), 
  .pool_stride = AI_SHAPE_2D_INIT(1, 27), 
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


AI_STATIC_CONST ai_i8 nl_22_nl_params_data[] = { -128, -128, -128, -128, -128, -128, -128, -128, -128, -127, -127, -127, -127, -127, -127, -126, -126, -125, -125, -124, -123, -122, -121, -120, -118, -116, -113, -110, -107, -103, -98, -92, -85, -78, -69, -59, -49, -37, -25, -13, 0, 13, 25, 37, 49, 59, 69, 78, 85, 92, 98, 103, 107, 110, 113, 116, 118, 120, 121, 122, 123, 124, 125, 125, 126, 126, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127 };
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
  pool_4_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  pool_4_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  sequential_2_1_batch_normalization_4_1_batchnorm_mul_4D_array.data = AI_PTR(net_ctx->_weights[0] + 0);
  sequential_2_1_batch_normalization_4_1_batchnorm_mul_4D_array.data_start = AI_PTR(net_ctx->_weights[0] + 0);
  eltwise_5_output_array.data = AI_PTR(net_ctx->_activations[0] + 2016);
  eltwise_5_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 2016);
  _STAI_NETWORK_EVENT_NODE_START_CB(5, 2, { pool_4_output.data->data,sequential_2_1_batch_normalization_4_1_batchnorm_mul_4D.data->data});
  forward_eltwise_integer_INT8(&eltwise_5_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(5, 1, { eltwise_5_output.data->data});
}
void forward_lite_eltwise_integer_INT8_eltwise_6(_stai_network_context* net_ctx)
{
  eltwise_5_output_array.data = AI_PTR(net_ctx->_activations[0] + 2016);
  eltwise_5_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 2016);
  sequential_2_1_batch_normalization_4_1_batchnorm_sub_4D_array.data = AI_PTR(net_ctx->_weights[0] + 32);
  sequential_2_1_batch_normalization_4_1_batchnorm_sub_4D_array.data_start = AI_PTR(net_ctx->_weights[0] + 32);
  eltwise_6_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  eltwise_6_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_NETWORK_EVENT_NODE_START_CB(6, 2, { eltwise_5_output.data->data,sequential_2_1_batch_normalization_4_1_batchnorm_sub_4D.data->data});
  forward_eltwise_integer_INT8(&eltwise_6_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(6, 1, { eltwise_6_output.data->data});
}
void forward_lite_eltwise_integer_INT8_eltwise_13(_stai_network_context* net_ctx)
{
  pool_12_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  pool_12_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  sequential_2_1_batch_normalization_5_1_batchnorm_mul_4D_array.data = AI_PTR(net_ctx->_weights[0] + 64);
  sequential_2_1_batch_normalization_5_1_batchnorm_mul_4D_array.data_start = AI_PTR(net_ctx->_weights[0] + 64);
  eltwise_13_output_array.data = AI_PTR(net_ctx->_activations[0] + 1856);
  eltwise_13_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 1856);
  _STAI_NETWORK_EVENT_NODE_START_CB(13, 2, { pool_12_output.data->data,sequential_2_1_batch_normalization_5_1_batchnorm_mul_4D.data->data});
  forward_eltwise_integer_INT8(&eltwise_13_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(13, 1, { eltwise_13_output.data->data});
}
void forward_lite_eltwise_integer_INT8_eltwise_14(_stai_network_context* net_ctx)
{
  eltwise_13_output_array.data = AI_PTR(net_ctx->_activations[0] + 1856);
  eltwise_13_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 1856);
  sequential_2_1_batch_normalization_5_1_batchnorm_sub_4D_array.data = AI_PTR(net_ctx->_weights[0] + 128);
  sequential_2_1_batch_normalization_5_1_batchnorm_sub_4D_array.data_start = AI_PTR(net_ctx->_weights[0] + 128);
  eltwise_14_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  eltwise_14_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_NETWORK_EVENT_NODE_START_CB(14, 2, { eltwise_13_output.data->data,sequential_2_1_batch_normalization_5_1_batchnorm_sub_4D.data->data});
  forward_eltwise_integer_INT8(&eltwise_14_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(14, 1, { eltwise_14_output.data->data});
}
void forward_lite_ap_integer_INT8_pool_19(_stai_network_context* net_ctx)
{
  conv2d_17_output_array.data = AI_PTR(net_ctx->_activations[0] + 9536);
  conv2d_17_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 9536);
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


static const ai_u16 conv2d_1_t_in_0_shape_w_const_u16 = 131;
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
static const ai_float conv2d_1_t_in_0_fmt_scale_const_f32 = 0.0008917094091884792f;
static const ai_float conv2d_1_t_out_0_fmt_scale_const_f32 = 0.00027663636137731373f;
static const ai_float conv2d_1_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0014880847884342074f, 0.0016906640958040953f, 0.001217953278683126f, 0.0009867119370028377f, 0.0017134418012574315f, 0.0012546214275062084f, 0.001159018836915493f, 0.0012512969551607966f, 0.001696854131296277f, 0.0012338407104834914f, 0.0017019567312672734f, 0.0010016487212851644f, 0.001303596654906869f, 0.0014277705922722816f, 0.0020689822267740965f, 0.001450562383979559f, 0.0013346902560442686f, 0.0014764828374609351f, 0.0018019311828538775f, 0.0016896111192181706f, 0.0010262040887027979f, 0.0018617024179548025f, 0.0012039837893098593f, 0.00038130339817143977f, 0.0011160660069435835f, 0.0011276955483481288f, 0.0016045441152527928f, 0.0012799982214346528f, 0.0011229036608710885f, 0.0013729194179177284f, 0.0012681473745033145f, 0.0008988177869468927f);
static const ai_layer_format_type conv2d_1_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_1_t_out_0_shape_w_const_u16 = 127;
static const ai_u16 conv2d_1_t_out_0_shape_h_const_u16 = 1;

static const ai_u16 pool_4_t_in_0_shape_w_const_u16 = 127;
static const ai_u16 pool_4_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 pool_4_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 pool_4_l_pool_size_1_const_u16 = 2;
static const ai_u16 pool_4_l_pool_size_0_const_u16 = 1;
static const ai_u16 pool_4_l_legacy_pool_pad_1_const_u16 = 0;
static const ai_u16 pool_4_l_legacy_pool_pad_0_const_u16 = 0;
static const ai_u16 pool_4_l_pool_stride_1_const_u16 = 2;
static const ai_u16 pool_4_l_pool_stride_0_const_u16 = 1;
static const ai_u16 pool_4_t_out_0_shape_w_const_u16 = 63;
static const ai_u16 pool_4_t_out_0_shape_h_const_u16 = 1;
static const ai_i8 pool_4_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 pool_4_t_out_0_fmt_zero_const_s8 = -128;



static const ai_u16 conv2d_9_t_in_0_shape_w_const_u16 = 63;
static const ai_u16 conv2d_9_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_9_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_9_t_out_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_9_t_weight_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_9_t_weight_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_9_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_9_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_9_t_in_0_fmt_zero_const_s8 = -47;
static const ai_i8 conv2d_9_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_9_t_in_0_fmt_scale_const_f32 = 0.008336790837347507f;
static const ai_float conv2d_9_t_out_0_fmt_scale_const_f32 = 0.007413871120661497f;
static const ai_float conv2d_9_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.001569531043060124f, 0.001509647467173636f, 0.0017703102203086019f, 0.0014755531447008252f, 0.0013428194215521216f, 0.0017001208616420627f, 0.0015317447250708938f, 0.0016788161592558026f, 0.0012808343162760139f, 0.0012391249183565378f, 0.0013005195651203394f, 0.0016118764178827405f, 0.001376956352032721f, 0.0017418446950614452f, 0.0015783915296196938f, 0.0016978583298623562f, 0.0015484821051359177f, 0.0015617582248523831f, 0.0014899929519742727f, 0.001387743977829814f, 0.0015555121935904026f, 0.001396120060235262f, 0.0014303341740742326f, 0.001288736704736948f, 0.0014138486003503203f, 0.0015903087332844734f, 0.0012947650393471122f, 0.0016547514824196696f, 0.0013651024783030152f, 0.001453666016459465f, 0.0015852574724704027f, 0.0015709748258814216f, 0.0014355482999235392f, 0.0015832290519028902f, 0.0017160556744784117f, 0.0016622819239273667f, 0.0016060970956459641f, 0.0016932208091020584f, 0.0016132616437971592f, 0.0017594960518181324f, 0.0013445320073515177f, 0.0020343312062323093f, 0.0017501722322776914f, 0.001631727907806635f, 0.0017993263900279999f, 0.0013500810600817204f, 0.0018392257625237107f, 0.001546299085021019f, 0.0017481559189036489f, 0.001570042222738266f, 0.0020871630404144526f, 0.001634728629142046f, 0.0016026058001443744f, 0.0017329647671431303f, 0.0013423688942566514f, 0.0014166971668601036f, 0.0018798470264300704f, 0.001468914677388966f, 0.0014983935980126262f, 0.0014472284819930792f, 0.0014926373260095716f, 0.001451742136850953f, 0.0014132341602817178f, 0.0015741667011752725f);
static const ai_layer_format_type conv2d_9_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_9_t_out_0_shape_w_const_u16 = 59;
static const ai_u16 conv2d_9_t_out_0_shape_h_const_u16 = 1;

static const ai_u16 pool_12_t_in_0_shape_w_const_u16 = 59;
static const ai_u16 pool_12_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 pool_12_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 pool_12_l_pool_size_1_const_u16 = 2;
static const ai_u16 pool_12_l_pool_size_0_const_u16 = 1;
static const ai_u16 pool_12_l_legacy_pool_pad_1_const_u16 = 0;
static const ai_u16 pool_12_l_legacy_pool_pad_0_const_u16 = 0;
static const ai_u16 pool_12_l_pool_stride_1_const_u16 = 2;
static const ai_u16 pool_12_l_pool_stride_0_const_u16 = 1;
static const ai_u16 pool_12_t_out_0_shape_w_const_u16 = 29;
static const ai_u16 pool_12_t_out_0_shape_h_const_u16 = 1;
static const ai_i8 pool_12_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 pool_12_t_out_0_fmt_zero_const_s8 = -128;



static const ai_u16 conv2d_17_t_in_0_shape_w_const_u16 = 29;
static const ai_u16 conv2d_17_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_17_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_17_t_out_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_17_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_17_t_weight_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_17_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_17_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_17_t_in_0_fmt_zero_const_s8 = -102;
static const ai_i8 conv2d_17_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_17_t_in_0_fmt_scale_const_f32 = 0.06677153706550598f;
static const ai_float conv2d_17_t_out_0_fmt_scale_const_f32 = 0.1057518720626831f;
static const ai_float conv2d_17_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0014242499601095915f, 0.0013847621157765388f, 0.0014511714689433575f, 0.0013097102055326104f, 0.0012142508057877421f, 0.0012481515295803547f, 0.0011622868478298187f, 0.0016392632387578487f, 0.0018860026029869914f, 0.0016092141158878803f, 0.00135239923838526f, 0.0015881340950727463f, 0.0020575604867190123f, 0.0014090434415265918f, 0.002051946707069874f, 0.001522165723145008f, 0.0015168162062764168f, 0.0017149228369817138f, 0.0014900509268045425f, 0.001097384956665337f, 0.0015009008347988129f, 0.0013631745241582394f, 0.0014606475597247481f, 0.0014447779394686222f, 0.0015244322130456567f, 0.0011164868483319879f, 0.0010389890521764755f, 0.0011186368064954877f, 0.001518014119938016f, 0.0011426671408116817f, 0.001664438983425498f, 0.0013595826458185911f, 0.0009516123682260513f, 0.001682206173427403f, 0.001591284992173314f, 0.0015960977179929614f, 0.0012023590970784426f, 0.0016217557713389397f, 0.0014647359494119883f, 0.001043466036207974f, 0.0012338386150076985f, 0.0015250119613483548f, 0.0009603472426533699f, 0.001804908737540245f, 0.001181750907562673f, 0.0015470359940081835f, 0.0013766083866357803f, 0.00142473797313869f, 0.0013310678768903017f, 0.0015042060986161232f, 0.0016007485100999475f, 0.0012345018330961466f, 0.0016940360656008124f, 0.0018045830074697733f, 0.0016719235572963953f, 0.0010603332193568349f, 0.0013068008702248335f, 0.0010111761512234807f, 0.0014002142706885934f, 0.0016217257361859083f, 0.0011764747323468328f, 0.0011765469098463655f, 0.0014654239639639854f, 0.0011588463094085455f, 0.0014080196851864457f, 0.0011617750860750675f, 0.0012217476032674313f, 0.0012565490324050188f, 0.0012797282543033361f, 0.0009430749341845512f, 0.0012978583108633757f, 0.001583593082614243f, 0.0017601412255316973f, 0.0014210278168320656f, 0.0014140661805868149f, 0.0011506982846185565f, 0.001195050310343504f, 0.0014484593411907554f, 0.0013719922862946987f, 0.001996650593355298f, 0.0013629927998408675f, 0.001617884961888194f, 0.0014825260732322931f, 0.0014240002492442727f, 0.0012034578248858452f, 0.0012876654509454966f, 0.0011723034549504519f, 0.001595099689438939f, 0.0014937111409381032f, 0.0014916537329554558f, 0.0011504468275234103f, 0.0015721116214990616f, 0.0013840309111401439f, 0.0012824049917981029f, 0.0017442767275497317f, 0.001760713872499764f, 0.001177747268229723f, 0.0011542665306478739f, 0.0010595611529424787f, 0.0015325496206060052f, 0.001681809895671904f, 0.0014262739568948746f, 0.0014757185708731413f, 0.0015715216286480427f, 0.0009396484820172191f, 0.001304943929426372f, 0.0014475438510999084f, 0.0010035207960754633f, 0.0013989232247695327f, 0.0013380965683609247f, 0.001424679416231811f, 0.0014475835487246513f, 0.0018658553017303348f, 0.0017280030297115445f, 0.0012261681258678436f, 0.0015514102997258306f, 0.0014402298256754875f, 0.0018700773362070322f, 0.0017341328784823418f, 0.001845481456257403f, 0.001284800237044692f, 0.0011661199387162924f, 0.0014904644340276718f, 0.0018255988834425807f, 0.0008410721202380955f, 0.0013048667460680008f, 0.0012458886485546827f, 0.001155910431407392f);
static const ai_layer_format_type conv2d_17_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_17_t_out_0_shape_w_const_u16 = 27;
static const ai_u16 conv2d_17_t_out_0_shape_h_const_u16 = 1;



static const ai_i8 gemm_21_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 gemm_21_t_out_0_fmt_zero_const_s8 = -88;
static const ai_u16 gemm_21_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 gemm_21_t_out_0_shape_ch_const_u16 = 1;
static const ai_u32 gemm_21_t_out_0_shape_h_w_prod_const_u32 = 1;
static const ai_float gemm_21_t_in_0_fmt_scale_const_f32 = 0.04072719067335129f;
static const ai_float gemm_21_t_out_0_fmt_scale_const_f32 = 0.200651153922081f;
static const ai_float gemm_21_t_weight_0_fmt_scale_const_f32 = 0.0029235987458378077f;

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
    ai_i8* conv2d_1_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_1_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 4064);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(1, 1, {(stai_ptr) conv2d_1_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_sssa8_ch(conv2d_1_t_in_0_ptr_const_s8, conv2d_1_t_in_0_shape_w_const_u16, conv2d_1_t_in_0_shape_h_const_u16, conv2d_1_t_in_0_shape_ch_const_u16, conv2d_1_t_weight_0_ptr_const_s8, conv2d_1_t_out_0_shape_ch_const_u16, conv2d_1_t_weight_0_shape_w_const_u16, conv2d_1_t_weight_0_shape_h_const_u16, conv2d_1_l_stride_1_const_u16, conv2d_1_l_stride_0_const_u16, conv2d_1_l_pad_W_0_const_s32, conv2d_1_l_pad_H_0_const_s32, conv2d_1_t_weight_1_ptr_const_s32, conv2d_1_t_in_0_fmt_zero_const_s8, conv2d_1_t_out_0_fmt_zero_const_s8, conv2d_1_t_in_0_fmt_scale_const_f32, conv2d_1_t_out_0_fmt_scale_const_f32, conv2d_1_t_weight_0_fmt_scale_const_f32, conv2d_1_l_out_ch_format_const_layer_format_type, conv2d_1_t_out_0_ptr_s8, conv2d_1_t_out_0_shape_w_const_u16, conv2d_1_t_out_0_shape_h_const_u16, 1, 788, conv2d_1_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(1, 1, {(stai_ptr) conv2d_1_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_1 */
  /* LITE_KERNEL_SECTION BEGIN pool_4 */
  {
      const ai_i8* pool_4_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i8* pool_4_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
  
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
    ai_i8* conv2d_9_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 8672);
    ai_i16* conv2d_9_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 2016);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(9, 1, {(stai_ptr) conv2d_9_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_sssa8_ch(conv2d_9_t_in_0_ptr_const_s8, conv2d_9_t_in_0_shape_w_const_u16, conv2d_9_t_in_0_shape_h_const_u16, conv2d_9_t_in_0_shape_ch_const_u16, conv2d_9_t_weight_0_ptr_const_s8, conv2d_9_t_out_0_shape_ch_const_u16, conv2d_9_t_weight_0_shape_w_const_u16, conv2d_9_t_weight_0_shape_h_const_u16, conv2d_9_l_stride_1_const_u16, conv2d_9_l_stride_0_const_u16, conv2d_9_t_weight_1_ptr_const_s32, conv2d_9_t_in_0_fmt_zero_const_s8, conv2d_9_t_out_0_fmt_zero_const_s8, conv2d_9_t_in_0_fmt_scale_const_f32, conv2d_9_t_out_0_fmt_scale_const_f32, conv2d_9_t_weight_0_fmt_scale_const_f32, conv2d_9_l_out_ch_format_const_layer_format_type, conv2d_9_t_out_0_ptr_s8, conv2d_9_t_out_0_shape_w_const_u16, conv2d_9_t_out_0_shape_h_const_u16, 1, 1, 6656, conv2d_9_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(9, 1, {(stai_ptr) conv2d_9_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_9 */
  /* LITE_KERNEL_SECTION BEGIN pool_12 */
  {
      const ai_i8* pool_12_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 8672);
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
    ai_i8* conv2d_17_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 9536);
    ai_i16* conv2d_17_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 1856);
  
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
  net_ctx->_inputs[0] = activations[0] + 4852;

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

