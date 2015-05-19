#pragma once

//--------------------------------------------------------------------------------------------------
#define BK_DEBUG_BREAK (__debugbreak(), 0)

//--------------------------------------------------------------------------------------------------
#define BK_ASSERT(COND) (void)(((!!(COND)) || BK_DEBUG_BREAK), 0);

#define BK_PRECONDITION(COND) BK_ASSERT(COND)
#define BK_PRECONDITION_SAFE(COND) BK_ASSERT(COND)
