#ifndef STR_H
#define STR_H

#include "types.h"
#include "vec.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STRING_NULL ((String){0, 0, NULL})
typedef struct {
    usize length;
    usize capacity;
    str8 data;
} String;

#define USTRING_NULL ((UString){0, 0, NULL})
typedef struct {
    usize length;
    usize capacity;
    str16 data;
} UString;

#define STRING_VIEW_EMPTY ((StringView){0, NULL})
typedef struct {
    usize length;
    str8 data;
} StringView;

#define USTRING_VIEW_EMPTY ((UStringView){0, NULL})
typedef struct {
    usize length;
    str16 data;
} UStringView;

/* Unicode scalar value (U+0000..U+10FFFF, excluding surrogates). */
typedef u32 Codepoint;

#define STRING_NOT_FOUND ((usize) - 1)
#define LENGTH(arr) (sizeof(arr) / sizeof((arr)[0]) - 1)
#define CSTR_TO_VIEW(str) ((StringView){LENGTH(str), (str8)(str)})
#define CSTR_TO_UVIEW(str) ((UStringView){LENGTH(str), (str16)(str)})

#define VIEW_TO_STRING(view) string_new_from_view((view))
#define UVIEW_TO_USTRING(view) ustring_new_from_view((view))

#define VIEW_TO_CSTR(view) string_view_to_cstr(view)
#define UVIEW_TO_CSTR(view) ustring_view_to_cstr(view)

String string_new(void);
String string_new_cstr(str8 data);
String string_new_utf8(str8 data, usize length);
String string_new_utf16(str16 data, usize length);
String string_new_copy(const String *string);
String string_new_substring(const String *string, usize offset, usize length);
String string_new_from_view(StringView view);

UString ustring_new(void);
UString ustring_new_cstr(str16 data);
UString ustring_new_utf16(str16 data, usize length);
UString ustring_new_utf8(str8 data, usize length);
UString ustring_new_copy(const UString *string);
UString ustring_new_substring(const UString *string, usize offset, usize length);
UString ustring_new_from_view(UStringView view);

void string_delete(String *string);
void ustring_delete(UString *string);

str8 string_view_to_cstr(StringView view);
str16 ustring_view_to_cstr(UStringView view);

bool string_assign_cstr(String *string, str8 data);
bool string_assign_utf8(String *string, str8 data, usize length);
bool string_assign_utf16(String *string, str16 data, usize length);
bool string_assign_string(String *string, const String *other);
bool string_assign_view(String *string, StringView view);

bool ustring_assign_cstr(UString *string, str16 data);
bool ustring_assign_utf16(UString *string, str16 data, usize length);
bool ustring_assign_utf8(UString *string, str8 data, usize length);
bool ustring_assign_ustring(UString *string, const UString *other);
bool ustring_assign_view(UString *string, UStringView view);

void string_clear(String *string);
usize string_length(const String *string);
usize string_size(const String *string);
usize string_capacity(const String *string);
bool string_empty(const String *string);
bool string_valid(const String *string);

void ustring_clear(UString *string);
usize ustring_length(const UString *string);
usize ustring_size(const UString *string);
usize ustring_capacity(const UString *string);
bool ustring_empty(const UString *string);
bool ustring_valid(const UString *string);

u8 string_at(const String *string, usize index);
u16 ustring_at(const UString *string, usize index);

str8 string_cstr(const String *string);
str16 ustring_cstr(const UString *string);

str8 string_data(String *string);
str8 string_data_const(const String *string);
str16 ustring_data(UString *string);
str16 ustring_data_const(const UString *string);

StringView string_view(const String *string, usize offset, usize length);
StringView string_view_cstr(str8 data, usize offset, usize length);
StringView string_view_data(str8 data, usize length);

UStringView ustring_view(const UString *string, usize offset, usize length);
UStringView ustring_view_cstr(str16 data, usize offset, usize length);
UStringView ustring_view_data(str16 data, usize length);

usize string_view_length(StringView view);
bool string_view_empty(StringView view);
bool string_view_valid(StringView view);
u8 string_view_at(StringView view, usize index);
int string_view_compare(StringView a, StringView b);
bool string_view_equal(StringView a, StringView b);
usize string_view_find(StringView view, StringView needle);
usize string_view_rfind(StringView view, StringView needle);
bool string_view_starts_with(StringView view, StringView prefix);
bool string_view_ends_with(StringView view, StringView suffix);
bool string_view_contains(StringView view, StringView needle);

usize ustring_view_length(UStringView view);
bool ustring_view_empty(UStringView view);
bool ustring_view_valid(UStringView view);
u16 ustring_view_at(UStringView view, usize index);
int ustring_view_compare(UStringView a, UStringView b);
bool ustring_view_equal(UStringView a, UStringView b);
usize ustring_view_find(UStringView view, UStringView needle);
usize ustring_view_rfind(UStringView view, UStringView needle);
bool ustring_view_starts_with(UStringView view, UStringView prefix);
bool ustring_view_ends_with(UStringView view, UStringView suffix);
bool ustring_view_contains(UStringView view, UStringView needle);

int string_compare(const String *a, const String *b);
int string_compare_cstr(const String *string, str8 cstr);
int string_compare_view(const String *string, StringView view);
bool string_equal(const String *a, const String *b);
bool string_equal_cstr(const String *string, str8 cstr);
bool string_equal_view(const String *string, StringView view);

int ustring_compare(const UString *a, const UString *b);
int ustring_compare_cstr(const UString *string, str16 cstr);
int ustring_compare_view(const UString *string, UStringView view);
bool ustring_equal(const UString *a, const UString *b);
bool ustring_equal_cstr(const UString *string, str16 cstr);
bool ustring_equal_view(const UString *string, UStringView view);

usize string_find(const String *string, StringView needle);
usize string_rfind(const String *string, StringView needle);
usize string_find_u8(const String *string, u8 character);
usize string_rfind_u8(const String *string, u8 character);
usize ustring_find(const UString *string, UStringView needle);
usize ustring_rfind(const UString *string, UStringView needle);
usize ustring_find_u16(const UString *string, u16 character);
usize ustring_rfind_u16(const UString *string, u16 character);
bool string_starts_with(const String *string, StringView prefix);
bool string_ends_with(const String *string, StringView suffix);
bool string_contains(const String *string, StringView needle);
bool ustring_starts_with(const UString *string, UStringView prefix);
bool ustring_ends_with(const UString *string, UStringView suffix);
bool ustring_contains(const UString *string, UStringView needle);

bool string_append_cstr(String *string, str8 data);
bool string_append_utf8(String *string, str8 data, usize length);
bool string_append_utf16(String *string, str16 data, usize length);
bool string_append_string(String *string, const String *other);
bool string_append_view(String *string, StringView view);

bool ustring_append_cstr(UString *string, str16 data);
bool ustring_append_utf16(UString *string, str16 data, usize length);
bool ustring_append_utf8(UString *string, str8 data, usize length);
bool ustring_append_string(UString *string, const UString *other);
bool ustring_append_view(UString *string, UStringView view);

bool string_insert_cstr(String *string, usize offset, str8 data);
bool string_insert_utf8(String *string, usize offset, str8 data, usize length);
bool string_insert_utf16(String *string, usize offset, str16 data, usize length);
bool string_insert_string(String *string, usize offset, const String *other);
bool string_insert_view(String *string, usize offset, StringView view);

bool ustring_insert_cstr(UString *string, usize offset, str16 data);
bool ustring_insert_utf16(UString *string, usize offset, str16 data, usize length);
bool ustring_insert_utf8(UString *string, usize offset, str8 data, usize length);
bool ustring_insert_string(UString *string, usize offset, const UString *other);
bool ustring_insert_view(UString *string, usize offset, UStringView view);

bool string_remove(String *string, usize offset, usize length);
bool string_replace_cstr(String *string, usize offset, usize length, str8 data);
bool string_replace_utf8(String *string, usize offset, usize length, str8 data, usize data_length);
bool string_replace_utf16(String *string, usize offset, usize length, str16 data,
                          usize data_length);
bool string_replace_string(String *string, usize offset, usize length, const String *replacement);
bool string_replace_view(String *string, usize offset, usize length, StringView replacement);

bool ustring_remove(UString *string, usize offset, usize length);
bool ustring_replace_cstr(UString *string, usize offset, usize length, str16 data);
bool ustring_replace_utf16(UString *string, usize offset, usize length, str16 data,
                           usize data_length);
bool ustring_replace_utf8(UString *string, usize offset, usize length, str8 data,
                          usize data_length);
bool ustring_replace_string(UString *string, usize offset, usize length,
                            const UString *replacement);
bool ustring_replace_view(UString *string, usize offset, usize length, UStringView replacement);

/* Replace by needle (non-overlapping, left-to-right). Empty needle is a no-op failure. */
bool string_replace_all_view(String *string, StringView needle, StringView replacement);
bool string_replace_first_view(String *string, StringView needle, StringView replacement);
bool string_replace_all_cstr(String *string, str8 needle, str8 replacement);
bool string_replace_first_cstr(String *string, str8 needle, str8 replacement);

bool ustring_replace_all_view(UString *string, UStringView needle, UStringView replacement);
bool ustring_replace_first_view(UString *string, UStringView needle, UStringView replacement);

/*
 * Split into views that point into s.data. Caller must keep s alive for as long as
 * the views are used. out_views is a Vec of StringView (stride sizeof(StringView));
 * on success it is cleared and filled. Empty sep returns false.
 */
bool string_split(StringView s, StringView sep, Vec *out_views);
bool string_join(String *out, StringView sep, const StringView *parts, usize count);
bool string_join_vec(String *out, StringView sep, const Vec *parts);

String string_format(const char *fmt, ...);
bool string_append_format(String *string, const char *fmt, ...);

void string_lower(String *string); /* ASCII only via tolower */
void string_upper(String *string); /* ASCII only via toupper */
/*
 * Simple Unicode case: ASCII + Latin-1 supplement letter pairs (U+00C0–U+00FF,
 * excluding U+00D7 multiply and U+00F7 divide). Not full Unicode SpecialCasing.
 * Invalid UTF-8 bytes are left unchanged.
 */
void string_lower_utf8(String *string);
void string_upper_utf8(String *string);
void string_trim(String *string);
void string_trim_left(String *string);
void string_trim_right(String *string);
void ustring_lower(UString *string);
void ustring_upper(UString *string);
void ustring_trim(UString *string);
void ustring_trim_left(UString *string);
void ustring_trim_right(UString *string);

/* UTF-8 encode/decode. utf8_encode returns bytes written, or 0 if cp is invalid. */
usize utf8_encode(Codepoint cp, u8 out[4]);
/* Decode one codepoint at *byte_index and advance *byte_index on success. */
bool utf8_decode(const u8 *data, usize len, usize *byte_index, Codepoint *out);
usize utf8_codepoint_count(StringView view);
bool utf8_next(StringView view, usize *byte_index, Codepoint *out);
/* Move *byte_index to the previous codepoint start and decode it. */
bool utf8_prev(StringView view, usize *byte_index, Codepoint *out);

bool string_is_ascii(const String *string);
bool string_is_utf8(const String *string);
bool ustring_is_valid_utf16(const UString *string);

bool string_reserve(String *string, usize capacity);
bool string_shrink_to_fit(String *string);
bool ustring_reserve(UString *string, usize capacity);
bool ustring_shrink_to_fit(UString *string);

void string_swap(String *a, String *b);
void ustring_swap(UString *a, UString *b);

UString ustring_from_string(const String *string);
String string_from_ustring(const UString *string);
bool string_assign_ustring(String *string, const UString *other);
bool ustring_assign_string(UString *string, const String *other);

#ifdef __cplusplus
}
#endif

#endif
