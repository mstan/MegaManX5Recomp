#pragma once
#define PSX_EXECUTION_ENHANCED 1
#ifdef MMX5_HOST_TEST_BAD_EXECUTION
#define PSX_EXECUTION_ID "unqualified-execution"
#else
#define PSX_EXECUTION_ID MMX5_RELEASE_HOST_EXECUTION
#endif
