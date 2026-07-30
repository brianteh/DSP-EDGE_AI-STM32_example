/**
  ******************************************************************************
  * @file    network.c
  * @author  AST Embedded Analytics Research Platform
  * @date    2026-07-29T04:48:25+0000
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
#define _STAI_NETWORK_MODEL_SIGNATURE     "0xc4c0d5e17d1abd68fc31cfc1ab483ebb"
#define _STAI_NETWORK_DATETIME            "2026-07-29T04:48:25+0000"
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
      STAI_DECLARE_ARRAY(int32_t, 3, 1, 256, 1),
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
      STAI_DECLARE_ARRAY(int32_t, 1, 19136),
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
    AI_PACK_INTQ_SCALE(0.0018245992250740528f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #1 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_5_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.022293981164693832f),
    AI_PACK_INTQ_ZP(-70)))

/* Int quant #2 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(sequential_2_1_batch_normalization_4_1_batchnorm_mul_4D_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.10017184913158417f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #3 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_6_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.017721092328429222f),
    AI_PACK_INTQ_ZP(-55)))

/* Int quant #4 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(sequential_2_1_batch_normalization_4_1_batchnorm_sub_4D_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0058682020753622055f),
    AI_PACK_INTQ_ZP(121)))

/* Int quant #5 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(pool_12_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.011716168373823166f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #6 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_13_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.03114968352019787f),
    AI_PACK_INTQ_ZP(-75)))

/* Int quant #7 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(sequential_2_1_batch_normalization_5_1_batchnorm_mul_4D_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.023883437737822533f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #8 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_14_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.027899736538529396f),
    AI_PACK_INTQ_ZP(-69)))

/* Int quant #9 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(sequential_2_1_batch_normalization_5_1_batchnorm_sub_4D_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.006505762226879597f),
    AI_PACK_INTQ_ZP(127)))

/* Int quant #10 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_17_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.03459947928786278f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #11 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(pool_19_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.02682272344827652f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #12 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(gemm_20_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.06413274258375168f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #13 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(gemm_20_weights_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 64,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0014167424524202943f, 0.0013790441444143653f, 0.0015345679130405188f, 0.0013761559966951609f, 0.0015299791703000665f, 0.001540472381748259f, 0.0014298181049525738f, 0.0014446722343564034f, 0.0023074885830283165f, 0.001512698014266789f, 0.0016284602461382747f, 0.0015083479229360819f, 0.001490374794229865f, 0.001420189393684268f, 0.002056662691757083f, 0.0015308987349271774f, 0.0015289426082745194f, 0.0015443551819771528f, 0.0015642427606508136f, 0.0016057976754382253f, 0.0014511176850646734f, 0.0018652980215847492f, 0.0015789197059348226f, 0.0015794122591614723f, 0.0015733119798824191f, 0.00143512396607548f, 0.0015633739531040192f, 0.0013933468144387007f, 0.001488451031036675f, 0.0014982467982918024f, 0.0015128481900319457f, 0.0014491126639768481f, 0.001521821366623044f, 0.001462187385186553f, 0.0015259445644915104f, 0.0013889792608097196f, 0.0015392068307846785f, 0.001391674974001944f, 0.0014527420280501246f, 0.0013771309750154614f, 0.0013878917088732123f, 0.0013869558461010456f, 0.0013831978430971503f, 0.0015327947912737727f, 0.0013910082634538412f, 0.0015630930429324508f, 0.0017190048238262534f, 0.0015516502317041159f, 0.0013472831342369318f, 0.0014445710694417357f, 0.001478745136409998f, 0.0019331409130245447f, 0.0014711682451888919f, 0.0014862887328490615f, 0.0015064586186781526f, 0.001630733022466302f, 0.0015714557375758886f, 0.0015783457783982158f, 0.001622952870093286f, 0.0018692100420594215f, 0.0015433415537700057f, 0.001504425541497767f, 0.0014947837917134166f, 0.0018316205823794007f),
    AI_PACK_INTQ_ZP(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)))

/* Int quant #14 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(gemm_21_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.16404376924037933f),
    AI_PACK_INTQ_ZP(18)))

/* Int quant #15 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(nl_22_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.00390625f),
    AI_PACK_INTQ_ZP(-128)))



/* Array#0 */
AI_ARRAY_OBJ_DECLARE(
  pool_4_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 4032, AI_STATIC)

/* Array#1 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_5_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 4032, AI_STATIC)

/* Array#2 */
AI_ARRAY_OBJ_DECLARE(
  sequential_2_1_batch_normalization_4_1_batchnorm_mul_4D_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 32, AI_STATIC)

/* Array#3 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_6_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 4032, AI_STATIC)

/* Array#4 */
AI_ARRAY_OBJ_DECLARE(
  sequential_2_1_batch_normalization_4_1_batchnorm_sub_4D_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 32, AI_STATIC)

/* Array#5 */
AI_ARRAY_OBJ_DECLARE(
  pool_12_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 3904, AI_STATIC)

/* Array#6 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_13_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 3904, AI_STATIC)

/* Array#7 */
AI_ARRAY_OBJ_DECLARE(
  sequential_2_1_batch_normalization_5_1_batchnorm_mul_4D_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 64, AI_STATIC)

/* Array#8 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_14_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 3904, AI_STATIC)

/* Array#9 */
AI_ARRAY_OBJ_DECLARE(
  sequential_2_1_batch_normalization_5_1_batchnorm_sub_4D_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 64, AI_STATIC)

/* Array#10 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_17_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 7552, AI_STATIC)

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
  AI_SHAPE_INIT(4, 1, 32, 126, 1), AI_STRIDE_INIT(4, 1, 1, 32, 4032),
  1, &eltwise_5_output_array, &eltwise_5_output_array_intq)

/* Tensor #1 */
AI_TENSOR_OBJ_DECLARE(
  pool_4_output, AI_STATIC,
  28, 0x1,
  AI_SHAPE_INIT(4, 1, 32, 126, 1), AI_STRIDE_INIT(4, 1, 1, 32, 4032),
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
  AI_SHAPE_INIT(4, 1, 32, 126, 1), AI_STRIDE_INIT(4, 1, 1, 32, 4032),
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
  AI_SHAPE_INIT(4, 1, 64, 61, 1), AI_STRIDE_INIT(4, 1, 1, 64, 3904),
  1, &eltwise_13_output_array, &eltwise_13_output_array_intq)

/* Tensor #6 */
AI_TENSOR_OBJ_DECLARE(
  pool_12_output, AI_STATIC,
  26, 0x1,
  AI_SHAPE_INIT(4, 1, 64, 61, 1), AI_STRIDE_INIT(4, 1, 1, 64, 3904),
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
  AI_SHAPE_INIT(4, 1, 64, 61, 1), AI_STRIDE_INIT(4, 1, 1, 64, 3904),
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
  AI_SHAPE_INIT(4, 1, 128, 1, 59), AI_STRIDE_INIT(4, 1, 1, 128, 128),
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
  .pool_size = AI_SHAPE_2D_INIT(1, 59), 
  .pool_stride = AI_SHAPE_2D_INIT(1, 59), 
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


AI_STATIC_CONST ai_i8 nl_22_nl_params_data[] = { -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -127, -127, -127, -127, -127, -127, -127, -126, -126, -126, -125, -125, -124, -124, -123, -122, -121, -120, -119, -117, -115, -113, -111, -108, -105, -101, -97, -92, -86, -80, -74, -66, -58, -50, -41, -31, -21, -10, 0, 10, 21, 31, 41, 50, 58, 66, 74, 80, 86, 92, 97, 101, 105, 108, 111, 113, 115, 117, 119, 120, 121, 122, 123, 124, 124, 125, 125, 126, 126, 126, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127 };
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
  pool_4_output_array.data = AI_PTR(net_ctx->_activations[0] + 1044);
  pool_4_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 1044);
  sequential_2_1_batch_normalization_4_1_batchnorm_mul_4D_array.data = AI_PTR(net_ctx->_weights[0] + 0);
  sequential_2_1_batch_normalization_4_1_batchnorm_mul_4D_array.data_start = AI_PTR(net_ctx->_weights[0] + 0);
  eltwise_5_output_array.data = AI_PTR(net_ctx->_activations[0] + 5076);
  eltwise_5_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 5076);
  _STAI_NETWORK_EVENT_NODE_START_CB(5, 2, { pool_4_output.data->data,sequential_2_1_batch_normalization_4_1_batchnorm_mul_4D.data->data});
  forward_eltwise_integer_INT8(&eltwise_5_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(5, 1, { eltwise_5_output.data->data});
}
void forward_lite_eltwise_integer_INT8_eltwise_6(_stai_network_context* net_ctx)
{
  eltwise_5_output_array.data = AI_PTR(net_ctx->_activations[0] + 5076);
  eltwise_5_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 5076);
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
  eltwise_13_output_array.data = AI_PTR(net_ctx->_activations[0] + 3904);
  eltwise_13_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 3904);
  _STAI_NETWORK_EVENT_NODE_START_CB(13, 2, { pool_12_output.data->data,sequential_2_1_batch_normalization_5_1_batchnorm_mul_4D.data->data});
  forward_eltwise_integer_INT8(&eltwise_13_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(13, 1, { eltwise_13_output.data->data});
}
void forward_lite_eltwise_integer_INT8_eltwise_14(_stai_network_context* net_ctx)
{
  eltwise_13_output_array.data = AI_PTR(net_ctx->_activations[0] + 3904);
  eltwise_13_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 3904);
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
  conv2d_17_output_array.data = AI_PTR(net_ctx->_activations[0] + 11584);
  conv2d_17_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 11584);
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


static const ai_u16 conv2d_1_t_in_0_shape_w_const_u16 = 256;
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
static const ai_float conv2d_1_t_out_0_fmt_scale_const_f32 = 0.0018245992250740528f;
static const ai_float conv2d_1_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.001356746070086956f, 0.0011558140395209193f, 0.0015147702069953084f, 0.0014867736026644707f, 0.0016588119324296713f, 0.0014257625443860888f, 0.0014858907088637352f, 0.0014309429097920656f, 0.0012030444340780377f, 0.0014360066270455718f, 0.0014274249551817775f, 0.0013138874201104045f, 0.0011804084060713649f, 0.0013691663043573499f, 0.0014128340408205986f, 0.001323028700426221f, 0.0012423485750332475f, 0.0007044161902740598f, 0.001448210678063333f, 0.0013531462755054235f, 0.0014142108848318458f, 0.0012403035070747137f, 0.001147582777775824f, 0.0014603500021621585f, 0.0011312522692605853f, 0.0012753140181303024f, 0.0013853597920387983f, 0.0012400414561852813f, 0.0014742598868906498f, 0.0009738322114571929f, 0.0013721442082896829f, 0.0015145459910854697f);
static const ai_layer_format_type conv2d_1_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_1_t_out_0_shape_w_const_u16 = 252;
static const ai_u16 conv2d_1_t_out_0_shape_h_const_u16 = 1;

static const ai_u16 pool_4_t_in_0_shape_w_const_u16 = 252;
static const ai_u16 pool_4_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 pool_4_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 pool_4_l_pool_size_1_const_u16 = 2;
static const ai_u16 pool_4_l_pool_size_0_const_u16 = 1;
static const ai_u16 pool_4_l_legacy_pool_pad_1_const_u16 = 0;
static const ai_u16 pool_4_l_legacy_pool_pad_0_const_u16 = 0;
static const ai_u16 pool_4_l_pool_stride_1_const_u16 = 2;
static const ai_u16 pool_4_l_pool_stride_0_const_u16 = 1;
static const ai_u16 pool_4_t_out_0_shape_w_const_u16 = 126;
static const ai_u16 pool_4_t_out_0_shape_h_const_u16 = 1;
static const ai_i8 pool_4_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 pool_4_t_out_0_fmt_zero_const_s8 = -128;



static const ai_u16 conv2d_9_t_in_0_shape_w_const_u16 = 126;
static const ai_u16 conv2d_9_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_9_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_9_t_out_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_9_t_weight_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_9_t_weight_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_9_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_9_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_9_t_in_0_fmt_zero_const_s8 = -55;
static const ai_i8 conv2d_9_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_9_t_in_0_fmt_scale_const_f32 = 0.017721092328429222f;
static const ai_float conv2d_9_t_out_0_fmt_scale_const_f32 = 0.011716168373823166f;
static const ai_float conv2d_9_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0011248474474996328f, 0.0012492187088355422f, 0.0010503430385142565f, 0.0012167595559731126f, 0.0009228425333276391f, 0.0009981875773519278f, 0.001368541968986392f, 0.0009644207311794162f, 0.0010358751751482487f, 0.0011349486885592341f, 0.0009651093278080225f, 0.0010678222170099616f, 0.0010938651394098997f, 0.001025716308504343f, 0.0014046923024579883f, 0.0010065656388178468f, 0.0014661697205156088f, 0.0011969031766057014f, 0.0013244549045339227f, 0.0010949025163426995f, 0.0009189186384901404f, 0.0012217032490298152f, 0.0011594312964007258f, 0.001098901149816811f, 0.0009696579072624445f, 0.0009366613230668008f, 0.0011402121745049953f, 0.0010248625185340643f, 0.0009399997070431709f, 0.0016742994775995612f, 0.0013350282097235322f, 0.0009446374606341124f, 0.0010095508769154549f, 0.0015977215953171253f, 0.0011106632882729173f, 0.0011767616961151361f, 0.0009311100002378225f, 0.001234250608831644f, 0.0009332482586614788f, 0.0008920772233977914f, 0.0009777996456250548f, 0.0011678559239953756f, 0.0009496182319708169f, 0.0010379765881225467f, 0.001549523905850947f, 0.0010177348740398884f, 0.0009577578748576343f, 0.0011848798021674156f, 0.001034519518725574f, 0.0014066612347960472f, 0.00132752675563097f, 0.0009831259958446026f, 0.0012348302407190204f, 0.0010152721079066396f, 0.0010590351885184646f, 0.0012038878630846739f, 0.0012460661819204688f, 0.0010993380565196276f, 0.0009940856834873557f, 0.001631367951631546f, 0.0009538137237541378f, 0.0009455889230594039f, 0.0013160596136003733f, 0.0011407320853322744f);
static const ai_layer_format_type conv2d_9_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_9_t_out_0_shape_w_const_u16 = 122;
static const ai_u16 conv2d_9_t_out_0_shape_h_const_u16 = 1;

static const ai_u16 pool_12_t_in_0_shape_w_const_u16 = 122;
static const ai_u16 pool_12_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 pool_12_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 pool_12_l_pool_size_1_const_u16 = 2;
static const ai_u16 pool_12_l_pool_size_0_const_u16 = 1;
static const ai_u16 pool_12_l_legacy_pool_pad_1_const_u16 = 0;
static const ai_u16 pool_12_l_legacy_pool_pad_0_const_u16 = 0;
static const ai_u16 pool_12_l_pool_stride_1_const_u16 = 2;
static const ai_u16 pool_12_l_pool_stride_0_const_u16 = 1;
static const ai_u16 pool_12_t_out_0_shape_w_const_u16 = 61;
static const ai_u16 pool_12_t_out_0_shape_h_const_u16 = 1;
static const ai_i8 pool_12_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 pool_12_t_out_0_fmt_zero_const_s8 = -128;



static const ai_u16 conv2d_17_t_in_0_shape_w_const_u16 = 61;
static const ai_u16 conv2d_17_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_17_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_17_t_out_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_17_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_17_t_weight_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_17_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_17_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_17_t_in_0_fmt_zero_const_s8 = -69;
static const ai_i8 conv2d_17_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_17_t_in_0_fmt_scale_const_f32 = 0.027899736538529396f;
static const ai_float conv2d_17_t_out_0_fmt_scale_const_f32 = 0.03459947928786278f;
static const ai_float conv2d_17_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0009763924754224718f, 0.000926405715290457f, 0.0009503955370746553f, 0.0008882400579750538f, 0.0008580427966080606f, 0.0010538626229390502f, 0.0009316276991739869f, 0.0008634555852040648f, 0.0008444972918368876f, 0.0008209352963604033f, 0.0009768394520506263f, 0.0009388062753714621f, 0.0008949538460001349f, 0.0009484481415711343f, 0.0008963736472651362f, 0.0008465933497063816f, 0.0009645649115554988f, 0.0008237567963078618f, 0.001080090063624084f, 0.0008025126880966127f, 0.0008882685215212405f, 0.0009707921999506652f, 0.0009457481210120022f, 0.0009231697185896337f, 0.0008777961484156549f, 0.0009397140820510685f, 0.0008581607835367322f, 0.0009102835319936275f, 0.0008455648785457015f, 0.0008591326186433434f, 0.0009489887743256986f, 0.0011027602013200521f, 0.0008730691624805331f, 0.0008770652348175645f, 0.0009575250442139804f, 0.0008353735320270061f, 0.0009485087357461452f, 0.0008545874734409153f, 0.0010121185332536697f, 0.0008674705168232322f, 0.000842126552015543f, 0.0008837177883833647f, 0.0009085542405955493f, 0.0009382284479215741f, 0.0009745336137712002f, 0.0009284107945859432f, 0.0010294134262949228f, 0.0008313967846333981f, 0.0008045283611863852f, 0.0009288386208936572f, 0.0010288445046171546f, 0.0008541019633412361f, 0.0011456584325060248f, 0.0008773329318501055f, 0.0009070243686437607f, 0.0008443642873317003f, 0.000929109170101583f, 0.0009196580504067242f, 0.0009357506642118096f, 0.0008842995157465339f, 0.0009038354619406164f, 0.001132839359343052f, 0.0008349640993401408f, 0.0009142550406977534f, 0.0008449680753983557f, 0.0009471907396800816f, 0.0009143872885033488f, 0.0009568122331984341f, 0.0008977027609944344f, 0.0008743377402424812f, 0.0009008593624457717f, 0.00093655294040218f, 0.0010137554490938783f, 0.000854699348565191f, 0.0009827273897826672f, 0.0008348450646735728f, 0.0010750119108706713f, 0.0009075411362573504f, 0.0009576320881024003f, 0.0009550477843731642f, 0.0008679960737936199f, 0.0008242756593972445f, 0.0009984245989471674f, 0.0008926114533096552f, 0.000831684737931937f, 0.0008665023487992585f, 0.0008738914621062577f, 0.0009562958730384707f, 0.0008447470027022064f, 0.0008679039892740548f, 0.0008903950802050531f, 0.0008822202798910439f, 0.0008651858661323786f, 0.0008944959263317287f, 0.0009706247947178781f, 0.0008853385224938393f, 0.0008128357003442943f, 0.0008502563578076661f, 0.0008289426332339644f, 0.0009687779820524156f, 0.0008682322804816067f, 0.0008966564782895148f, 0.0008791161235421896f, 0.0009225123794749379f, 0.0009335165959782898f, 0.0008596621919423342f, 0.0009393567452207208f, 0.0008408645517192781f, 0.0009310913737863302f, 0.0008753138827160001f, 0.00098802603315562f, 0.0010286474134773016f, 0.0008070297772064805f, 0.0008972671930678189f, 0.0008286320953629911f, 0.0009343805140815675f, 0.0008935632067732513f, 0.000944261031690985f, 0.0008722587954252958f, 0.000884201901499182f, 0.0008503514109179378f, 0.001025694073177874f, 0.0009076991118490696f, 0.0008365685353055596f, 0.0009298975928686559f, 0.0009101210744120181f, 0.0008269971003755927f, 0.001020776224322617f);
static const ai_layer_format_type conv2d_17_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_17_t_out_0_shape_w_const_u16 = 59;
static const ai_u16 conv2d_17_t_out_0_shape_h_const_u16 = 1;



static const ai_i8 gemm_21_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 gemm_21_t_out_0_fmt_zero_const_s8 = 18;
static const ai_u16 gemm_21_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 gemm_21_t_out_0_shape_ch_const_u16 = 1;
static const ai_u32 gemm_21_t_out_0_shape_h_w_prod_const_u32 = 1;
static const ai_float gemm_21_t_in_0_fmt_scale_const_f32 = 0.06413274258375168f;
static const ai_float gemm_21_t_out_0_fmt_scale_const_f32 = 0.16404376924037933f;
static const ai_float gemm_21_t_weight_0_fmt_scale_const_f32 = 0.0031745778396725655f;

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
    ai_i8* conv2d_1_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1044);
    ai_i16* conv2d_1_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 256);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(1, 1, {(stai_ptr) conv2d_1_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_sssa8_ch(conv2d_1_t_in_0_ptr_const_s8, conv2d_1_t_in_0_shape_w_const_u16, conv2d_1_t_in_0_shape_h_const_u16, conv2d_1_t_in_0_shape_ch_const_u16, conv2d_1_t_weight_0_ptr_const_s8, conv2d_1_t_out_0_shape_ch_const_u16, conv2d_1_t_weight_0_shape_w_const_u16, conv2d_1_t_weight_0_shape_h_const_u16, conv2d_1_l_stride_1_const_u16, conv2d_1_l_stride_0_const_u16, conv2d_1_l_pad_W_0_const_s32, conv2d_1_l_pad_H_0_const_s32, conv2d_1_t_weight_1_ptr_const_s32, conv2d_1_t_in_0_fmt_zero_const_s8, conv2d_1_t_out_0_fmt_zero_const_s8, conv2d_1_t_in_0_fmt_scale_const_f32, conv2d_1_t_out_0_fmt_scale_const_f32, conv2d_1_t_weight_0_fmt_scale_const_f32, conv2d_1_l_out_ch_format_const_layer_format_type, conv2d_1_t_out_0_ptr_s8, conv2d_1_t_out_0_shape_w_const_u16, conv2d_1_t_out_0_shape_h_const_u16, 1, 788, conv2d_1_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(1, 1, {(stai_ptr) conv2d_1_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_1 */
  /* LITE_KERNEL_SECTION BEGIN pool_4 */
  {
      const ai_i8* pool_4_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1044);
    ai_i8* pool_4_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1044);
  
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
    ai_i8* conv2d_9_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 10688);
    ai_i16* conv2d_9_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 4032);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(9, 1, {(stai_ptr) conv2d_9_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_sssa8_ch(conv2d_9_t_in_0_ptr_const_s8, conv2d_9_t_in_0_shape_w_const_u16, conv2d_9_t_in_0_shape_h_const_u16, conv2d_9_t_in_0_shape_ch_const_u16, conv2d_9_t_weight_0_ptr_const_s8, conv2d_9_t_out_0_shape_ch_const_u16, conv2d_9_t_weight_0_shape_w_const_u16, conv2d_9_t_weight_0_shape_h_const_u16, conv2d_9_l_stride_1_const_u16, conv2d_9_l_stride_0_const_u16, conv2d_9_t_weight_1_ptr_const_s32, conv2d_9_t_in_0_fmt_zero_const_s8, conv2d_9_t_out_0_fmt_zero_const_s8, conv2d_9_t_in_0_fmt_scale_const_f32, conv2d_9_t_out_0_fmt_scale_const_f32, conv2d_9_t_weight_0_fmt_scale_const_f32, conv2d_9_l_out_ch_format_const_layer_format_type, conv2d_9_t_out_0_ptr_s8, conv2d_9_t_out_0_shape_w_const_u16, conv2d_9_t_out_0_shape_h_const_u16, 1, 1, 6656, conv2d_9_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(9, 1, {(stai_ptr) conv2d_9_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_9 */
  /* LITE_KERNEL_SECTION BEGIN pool_12 */
  {
      const ai_i8* pool_12_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 10688);
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
    ai_i8* conv2d_17_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 11584);
    ai_i16* conv2d_17_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 3904);
  
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

