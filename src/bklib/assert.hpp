#pragma once

//--------------------------------------------------------------------------------------------------
#define BK_DEBUG_BREAK (__debugbreak(), 0)

//--------------------------------------------------------------------------------------------------
#define BK_ASSERT(COND) (void)(((!!(COND)) || BK_DEBUG_BREAK), 0);
