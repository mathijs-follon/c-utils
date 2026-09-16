#include "str.h"
#include "test.h"
#include "vec.h"

#include <string.h>

static void test_string_basic(void) {
    String s = string_new_cstr((str8) "hello");
    TEST_CHECK(string_valid(&s));
    TEST_EQ_USIZE(string_length(&s), 5);
    TEST_CHECK(string_equal_cstr(&s, (str8) "hello"));
    TEST_CHECK(string_starts_with(&s, CSTR_TO_VIEW("he")));
    TEST_CHECK(string_ends_with(&s, CSTR_TO_VIEW("lo")));
    TEST_CHECK(string_contains(&s, CSTR_TO_VIEW("ell")));
    TEST_EQ_USIZE(string_find_u8(&s, 'l'), 2);
    TEST_EQ_USIZE(string_rfind_u8(&s, 'l'), 3);

    TEST_CHECK(string_append_cstr(&s, (str8) " world"));
    TEST_CHECK(string_equal_cstr(&s, (str8) "hello world"));

    TEST_CHECK(string_insert_cstr(&s, 5, (str8) ","));
    TEST_CHECK(string_equal_cstr(&s, (str8) "hello, world"));

    string_upper(&s);
    TEST_CHECK(string_equal_cstr(&s, (str8) "HELLO, WORLD"));
    string_lower(&s);
    TEST_CHECK(string_equal_cstr(&s, (str8) "hello, world"));

    String sub = string_new_substring(&s, 0, 5);
    TEST_CHECK(string_equal_cstr(&sub, (str8) "hello"));
    string_delete(&sub);

    string_delete(&s);
}

static void test_string_trim_and_views(void) {
    String s = string_new_cstr((str8) "  abc  ");
    string_trim(&s);
    TEST_CHECK(string_equal_cstr(&s, (str8) "abc"));

    StringView view = string_view(&s, 1, 1);
    TEST_EQ_USIZE(view.length, 1);
    TEST_EQ_INT(string_view_at(view, 0), 'b');
    TEST_CHECK(string_view_equal(view, CSTR_TO_VIEW("b")));

    string_delete(&s);
}

static void test_string_utf16_roundtrip(void) {
    String utf8 = string_new_cstr((str8) "hi");
    UString wide = ustring_from_string(&utf8);
    TEST_CHECK(ustring_valid(&wide));
    TEST_EQ_USIZE(ustring_length(&wide), 2);
    TEST_EQ_INT(ustring_at(&wide, 0), 'h');
    TEST_EQ_INT(ustring_at(&wide, 1), 'i');

    String back = string_from_ustring(&wide);
    TEST_CHECK(string_equal(&utf8, &back));

    string_delete(&utf8);
    string_delete(&back);
    ustring_delete(&wide);
}

static void test_string_assign_overlap(void) {
    String s = string_new_cstr((str8) "abcdef");
    TEST_CHECK(string_assign_utf8(&s, s.data + 2, 3));
    TEST_CHECK(string_equal_cstr(&s, (str8) "cde"));
    string_delete(&s);
}

static void test_string_split_join(void) {
    StringView src = CSTR_TO_VIEW("a,b,,c");
    Vec parts = VEC_NULL;
    TEST_CHECK(string_split(src, CSTR_TO_VIEW(","), &parts));
    TEST_EQ_USIZE(vec_length(&parts), 4);
    TEST_CHECK(string_view_equal(VEC_AT(StringView, &parts, 0), CSTR_TO_VIEW("a")));
    TEST_CHECK(string_view_equal(VEC_AT(StringView, &parts, 1), CSTR_TO_VIEW("b")));
    TEST_CHECK(string_view_equal(VEC_AT(StringView, &parts, 2), CSTR_TO_VIEW("")));
    TEST_CHECK(string_view_equal(VEC_AT(StringView, &parts, 3), CSTR_TO_VIEW("c")));

    String joined = string_new();
    TEST_CHECK(string_join_vec(&joined, CSTR_TO_VIEW("|"), &parts));
    TEST_CHECK(string_equal_cstr(&joined, (str8) "a|b||c"));

    String joined2 = string_new();
    StringView arr[] = {CSTR_TO_VIEW("x"), CSTR_TO_VIEW("y"), CSTR_TO_VIEW("z")};
    TEST_CHECK(string_join(&joined2, CSTR_TO_VIEW("-"), arr, 3));
    TEST_CHECK(string_equal_cstr(&joined2, (str8) "x-y-z"));

    TEST_CHECK(!string_split(src, CSTR_TO_VIEW(""), &parts));

    string_delete(&joined);
    string_delete(&joined2);
    vec_delete(&parts);
}

static void test_string_format(void) {
    String s = string_format("hello %s %d", "world", 42);
    TEST_CHECK(string_valid(&s));
    TEST_CHECK(string_equal_cstr(&s, (str8) "hello world 42"));

    TEST_CHECK(string_append_format(&s, "/%x", 255));
    TEST_CHECK(string_equal_cstr(&s, (str8) "hello world 42/ff"));

    string_delete(&s);
}

static void test_string_replace_needle(void) {
    String s = string_new_cstr((str8) "foo bar foo");
    TEST_CHECK(string_replace_first_cstr(&s, (str8) "foo", (str8) "baz"));
    TEST_CHECK(string_equal_cstr(&s, (str8) "baz bar foo"));

    TEST_CHECK(string_replace_all_view(&s, CSTR_TO_VIEW(" "), CSTR_TO_VIEW("_")));
    TEST_CHECK(string_equal_cstr(&s, (str8) "baz_bar_foo"));

    TEST_CHECK(string_replace_all_cstr(&s, (str8) "ba", (str8) "X"));
    TEST_CHECK(string_equal_cstr(&s, (str8) "Xz_Xr_foo"));

    TEST_CHECK(!string_replace_first_cstr(&s, (str8) "missing", (str8) "y"));
    TEST_CHECK(!string_replace_all_cstr(&s, (str8) "", (str8) "y"));

    string_delete(&s);
}

static void test_utf8_walk(void) {
    /* "Aé🍺" = A, U+00E9, U+1F37A */
    const char *raw = "A\xC3\xA9\xF0\x9F\x8D\xBA";
    StringView view = string_view_data((str8)raw, strlen(raw));

    TEST_EQ_USIZE(utf8_codepoint_count(view), 3);

    u8 buf[4];
    TEST_EQ_USIZE(utf8_encode(0xE9, buf), 2);
    TEST_EQ_INT(buf[0], 0xC3);
    TEST_EQ_INT(buf[1], 0xA9);
    TEST_EQ_USIZE(utf8_encode(0x1F37A, buf), 4);
    TEST_EQ_USIZE(utf8_encode(0xD800, buf), 0);

    usize i = 0;
    Codepoint cp;
    TEST_CHECK(utf8_next(view, &i, &cp));
    TEST_EQ_USIZE(cp, 'A');
    TEST_CHECK(utf8_next(view, &i, &cp));
    TEST_EQ_USIZE(cp, 0xE9);
    TEST_CHECK(utf8_next(view, &i, &cp));
    TEST_EQ_USIZE(cp, 0x1F37A);
    TEST_CHECK(!utf8_next(view, &i, &cp));

    TEST_CHECK(utf8_prev(view, &i, &cp));
    TEST_EQ_USIZE(cp, 0x1F37A);
    TEST_CHECK(utf8_prev(view, &i, &cp));
    TEST_EQ_USIZE(cp, 0xE9);
    TEST_CHECK(utf8_prev(view, &i, &cp));
    TEST_EQ_USIZE(cp, 'A');
    TEST_CHECK(!utf8_prev(view, &i, &cp));
}

static void test_string_case_utf8(void) {
    String s = string_new_cstr((str8) "AbC");
    string_lower_utf8(&s);
    TEST_CHECK(string_equal_cstr(&s, (str8) "abc"));
    string_upper_utf8(&s);
    TEST_CHECK(string_equal_cstr(&s, (str8) "ABC"));
    string_delete(&s);

    /* À (C0) / à (E0), Ö (D6) / ö (F6); leave × (D7) alone */
    String latin = string_new_utf8((str8) "\xC3\x80\xC3\x97\xC3\x96", 6);
    string_lower_utf8(&latin);
    TEST_CHECK(string_equal_cstr(&latin, (str8) "\xC3\xA0\xC3\x97\xC3\xB6"));
    string_upper_utf8(&latin);
    TEST_CHECK(string_equal_cstr(&latin, (str8) "\xC3\x80\xC3\x97\xC3\x96"));

    /* Multi-byte emoji must stay intact under ASCII-range case mapping */
    TEST_CHECK(string_append_cstr(&latin, (str8) "\xF0\x9F\x8D\xBA"));
    string_lower_utf8(&latin);
    TEST_CHECK(string_ends_with(&latin, string_view_data((str8) "\xF0\x9F\x8D\xBA", 4)));
    string_delete(&latin);
}

int main(void) {
    test_string_basic();
    test_string_trim_and_views();
    test_string_utf16_roundtrip();
    test_string_assign_overlap();
    test_string_split_join();
    test_string_format();
    test_string_replace_needle();
    test_utf8_walk();
    test_string_case_utf8();
    return test_report("str");
}
