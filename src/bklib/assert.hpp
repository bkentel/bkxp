#pragma once

#include <boost/predef.h>

//--------------------------------------------------------------------------------------------------

#if defined(BOOST_COMP_MSVC_AVAILABLE) && !(BOOST_COMP_GNUC)
#   define BK_DO_DEBUG_BREAK __debugbreak
#elif defined(BOOST_ARCH_X86_AVAILABLE)
void __inline__ BK_DO_DEBUG_BREAK() noexcept {
	__asm__ volatile("int $0x03");
}
#else
#   error no debugger break function defined for this platform
#endif

#define BK_DEBUG_BREAK (BK_DO_DEBUG_BREAK(), 0)

//--------------------------------------------------------------------------------------------------
#define BK_ASSERT(COND) (void)(((!!(COND)) || BK_DEBUG_BREAK), 0);

#define BK_PRECONDITION(COND) BK_ASSERT(COND)
#define BK_PRECONDITION_SAFE(COND) BK_ASSERT(COND)
