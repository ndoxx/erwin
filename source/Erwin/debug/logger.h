#pragma once

#include "kibble/logger/logger.h"

namespace erwin
{

#if LOGGING_ENABLED == 1
#define DO_LOG true
#else
#define DO_LOG false
#endif

// These macros will be optimized out (and arguments not evaluated)
// when LOGGING_ENABLED is set to 0. Should be better performance wise than using a null stream.
#define DLOG(C, S)                                                                                                     \
    if constexpr(!DO_LOG)                                                                                              \
        ;                                                                                                              \
    else                                                                                                               \
        kb::klog::get_log(kb::H_((C)), kb::klog::MsgType::NORMAL, (S))
#define DLOGI                                                                                                          \
    if constexpr(!DO_LOG)                                                                                              \
        ;                                                                                                              \
    else                                                                                                               \
        kb::klog::get_log(0, kb::klog::MsgType::ITEM, 4)
#define DLOGR(C)                                                                                                       \
    if constexpr(!DO_LOG)                                                                                              \
        ;                                                                                                              \
    else                                                                                                               \
        kb::klog::get_log(kb::H_((C)), kb::klog::MsgType::RAW, 0)
#define DLOGN(C)                                                                                                       \
    if constexpr(!DO_LOG)                                                                                              \
        ;                                                                                                              \
    else                                                                                                               \
        kb::klog::get_log(kb::H_((C)), kb::klog::MsgType::NOTIFY, 0)
#define DLOGW(C)                                                                                                       \
    if constexpr(!DO_LOG)                                                                                              \
        ;                                                                                                              \
    else                                                                                                               \
        kb::klog::get_log(kb::H_((C)), kb::klog::MsgType::WARNING, 1, __LINE__, __FILE__)
#define DLOGE(C)                                                                                                       \
    if constexpr(!DO_LOG)                                                                                              \
        ;                                                                                                              \
    else                                                                                                               \
        kb::klog::get_log(kb::H_((C)), kb::klog::MsgType::ERROR, 2, __LINE__, __FILE__)
#define DLOGF(C)                                                                                                       \
    if constexpr(!DO_LOG)                                                                                              \
        ;                                                                                                              \
    else                                                                                                               \
        kb::klog::get_log(kb::H_((C)), kb::klog::MsgType::FATAL, 3, __LINE__, __FILE__)
#define DLOGG(C)                                                                                                       \
    if constexpr(!DO_LOG)                                                                                              \
        ;                                                                                                              \
    else                                                                                                               \
        kb::klog::get_log(kb::H_((C)), kb::klog::MsgType::GOOD, 3, __LINE__, __FILE__)
#define DLOGB(C)                                                                                                       \
    if constexpr(!DO_LOG)                                                                                              \
        ;                                                                                                              \
    else                                                                                                               \
        kb::klog::get_log(kb::H_((C)), kb::klog::MsgType::BAD, 3, __LINE__, __FILE__)
#define BANG()                                                                                                        \
    if constexpr(!DO_LOG)                                                                                              \
        ;                                                                                                              \
    else                                                                                                               \
        kb::klog::get_log(kb::H_("core"), kb::klog::MsgType::BANG, 3) << __FILE__ << ":" << __LINE__ << std::endl

#define DLOGR__(C) kb::klog::get_log(kb::H_((C)), kb::klog::MsgType::RAW, 0)

using WCC = kb::WCC;
using WCB = kb::WCB;

}