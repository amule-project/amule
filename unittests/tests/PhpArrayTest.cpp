// Regression tests for issue #699: amuleweb crashes when the shared-file list
// exceeds 65535 entries.
//
// array_push_back() scanned integer keys in a loop bounded by `i < 0xffff`.
// With 91k shared files the bound is hit, the function returns null, and the
// caller crashes on the immediate dereference.  The underlying PHP_ARRAY_TYPE
// uses std::map (unbounded), so the 0xffff cap is purely artificial.
//
// These tests exercise array_push_back() in isolation, compiled against
// php_syntree.cpp with PHP_STANDALONE_EN so no WebServer/EC/wx-GUI headers
// are pulled in.

#include <muleunit/test.h>

#include "php_syntree.h"
#include "php_core_lib.h"

using namespace muleunit;

// Minimal stubs: php_syntree.cpp references these but the full PHP engine
// (php_core_lib.cpp, php_amule_lib.cpp, parser/scanner) is not linked here.
CPhPLibContext *CPhPLibContext::g_curr_context = nullptr;
void CPhPLibContext::Print(const char *) {}

// php_engine_init() calls these to register built-in functions; no-ops are fine.
void php_init_core_lib() {}
void php_init_amule_lib() {}

// Symbols normally provided by the flex-generated scanner.
int phplineno = 0;
char *phptext = nullptr;

DECLARE_SIMPLE(PhpArray)

// Baseline: sequential pushes well within the old 0xffff limit must work.
TEST(PhpArray, PushBackWithinOldLimit)
{
	PHP_VALUE_NODE arr;
	arr.type = PHP_VAL_NONE;
	cast_value_array(&arr);

	for (int i = 0; i < 200; i++) {
		PHP_VAR_NODE *slot = array_push_back(&arr);
		ASSERT_TRUE_M(slot != nullptr,
			wxString::Format(wxT("array_push_back returned null at i=%d"), i));
		slot->value.type = PHP_VAL_OBJECT;
	}

	value_value_free(&arr);
}

// Boundary: entry 65536 (index 65535) was the first to fail with the old code.
// The loop condition `i < 0xffff` (i.e. i < 65535) prevented it from running
// when push_next_hint == 65535, so the function returned null.
TEST(PhpArray, PushBackAtOldBoundary)
{
	PHP_VALUE_NODE arr;
	arr.type = PHP_VAL_NONE;
	cast_value_array(&arr);

	const int N = 65536; // one past the old 0xffff limit
	for (int i = 0; i < N; i++) {
		PHP_VAR_NODE *slot = array_push_back(&arr);
		ASSERT_TRUE_M(slot != nullptr,
			wxString::Format(wxT("array_push_back returned null at i=%d"), i));
		slot->value.type = PHP_VAL_OBJECT;
	}

	value_value_free(&arr);
}

// Regression: representative of the reporter's 91k shared-file scenario.
TEST(PhpArray, PushBackBeyondOldLimit)
{
	PHP_VALUE_NODE arr;
	arr.type = PHP_VAL_NONE;
	cast_value_array(&arr);

	const int N = 92000;
	for (int i = 0; i < N; i++) {
		PHP_VAR_NODE *slot = array_push_back(&arr);
		ASSERT_TRUE_M(slot != nullptr,
			wxString::Format(wxT("array_push_back returned null at i=%d"), i));
		slot->value.type = PHP_VAL_OBJECT;
	}

	value_value_free(&arr);
}
