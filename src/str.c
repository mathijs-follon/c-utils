#include "str.h"
#include "alloc.h"
#include "types.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static bool add_overflow(usize a, usize b, usize *out) {
    if (a > SIZE_MAX - b)
        return true;
    *out = a + b;
    return false;
}

static bool alloc_size8(usize capacity, usize *size) { return !add_overflow(capacity, 1, size); }

static bool alloc_size16(usize capacity, usize *size) {
    if (capacity > (SIZE_MAX / sizeof(u16)) - 1)
        return false;
    *size = (capacity + 1) * sizeof(u16);
    return true;
}

static bool utf8_decode_one(const unsigned char *s, usize len, uint32_t *codepoint,
                            usize *consumed) {
    if (!s || !len || !codepoint || !consumed)
        return false;

    unsigned char c0 = s[0];

    if (c0 <= 0x7F) {
        *codepoint = c0;
        *consumed = 1;
        return true;
    }

    if (c0 >= 0xC2 && c0 <= 0xDF) {
        if (len < 2)
            return false;
        unsigned char c1 = s[1];
        if ((c1 & 0xC0) != 0x80)
            return false;
        *codepoint = ((uint32_t)(c0 & 0x1F) << 6) | (uint32_t)(c1 & 0x3F);
        *consumed = 2;
        return true;
    }

    if (c0 >= 0xE0 && c0 <= 0xEF) {
        if (len < 3)
            return false;
        unsigned char c1 = s[1];
        unsigned char c2 = s[2];
        if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80)
            return false;
        if (c0 == 0xE0 && c1 < 0xA0)
            return false;
        if (c0 == 0xED && c1 >= 0xA0)
            return false;
        *codepoint =
            ((uint32_t)(c0 & 0x0F) << 12) | ((uint32_t)(c1 & 0x3F) << 6) | (uint32_t)(c2 & 0x3F);
        *consumed = 3;
        return true;
    }

    if (c0 >= 0xF0 && c0 <= 0xF4) {
        if (len < 4)
            return false;
        unsigned char c1 = s[1];
        unsigned char c2 = s[2];
        unsigned char c3 = s[3];
        if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80)
            return false;
        if (c0 == 0xF0 && c1 < 0x90)
            return false;
        if (c0 == 0xF4 && c1 > 0x8F)
            return false;
        *codepoint = ((uint32_t)(c0 & 0x07) << 18) | ((uint32_t)(c1 & 0x3F) << 12) |
                     ((uint32_t)(c2 & 0x3F) << 6) | (uint32_t)(c3 & 0x3F);
        *consumed = 4;
        return true;
    }

    return false;
}

static usize utf8_code_units(str8 data, usize length) {
    const unsigned char *bytes = (const unsigned char *)data;
    usize units = 0;

    for (usize i = 0; i < length;) {
        uint32_t codepoint;
        usize consumed;

        if (!utf8_decode_one(bytes + i, length - i, &codepoint, &consumed)) {
            codepoint = 0xFFFD;
            consumed = 1;
        }

        if (codepoint > 0xFFFF) {
            if (units > SIZE_MAX - 2)
                return SIZE_MAX;
            units += 2;
        } else {
            if (units == SIZE_MAX)
                return SIZE_MAX;
            units++;
        }

        i += consumed;
    }

    return units;
}

static bool buffer_contains8(str8 buffer, usize capacity, str8 data, usize length) {
    if (!buffer || !data || !length)
        return false;
    uintptr_t b = (uintptr_t)buffer;
    uintptr_t d = (uintptr_t)data;
    if (d < b)
        return false;
    return d - b < capacity + 1;
}

static bool buffer_contains16(str16 buffer, usize capacity, str16 data, usize length) {
    if (!buffer || !data || !length)
        return false;
    uintptr_t b = (uintptr_t)buffer;
    uintptr_t d = (uintptr_t)data;
    if (d < b)
        return false;
    return d - b < (capacity + 1) * sizeof(u16);
}

static bool string_ensure(String *string, usize required) {
    if (string->capacity >= required)
        return true;
    usize capacity = string->capacity ? string->capacity : 16;
    while (capacity < required) {
        if (capacity > SIZE_MAX / 2) {
            capacity = required;
            break;
        }
        capacity *= 2;
    }
    return string_reserve(string, capacity);
}

static bool ustring_ensure(UString *string, usize required) {
    if (string->capacity >= required)
        return true;
    usize capacity = string->capacity ? string->capacity : 16;
    while (capacity < required) {
        if (capacity > SIZE_MAX / 2) {
            capacity = required;
            break;
        }
        capacity *= 2;
    }
    return ustring_reserve(string, capacity);
}

usize u8_strlen(str8 str) {
    if (!str)
        return 0;
    usize len = 0;
    while (str[len] != 0)
        len++;
    return len;
}

usize u16_strlen(str16 str) {
    if (!str)
        return 0;
    usize len = 0;
    while (str[len] != 0)
        len++;
    return len;
}

String string_new(void) { return STRING_NULL; }

String string_new_cstr(str8 data) {
    if (!data)
        return STRING_NULL;
    return string_new_utf8(data, u8_strlen(data));
}

String string_new_utf8(str8 data, usize length) {
    if (!data)
        return STRING_NULL;

    usize size;
    if (!alloc_size8(length, &size))
        return STRING_NULL;

    str8 buffer = (str8)utils_malloc(size);
    if (!buffer)
        return STRING_NULL;

    if (length)
        memcpy(buffer, data, length);
    buffer[length] = '\0';

    return (String){length, length, buffer};
}

String string_new_copy(const String *string) {
    if (!string || !string->data)
        return STRING_NULL;
    return string_new_utf8(string->data, string->length);
}

String string_new_from_view(StringView view) {
    if (!view.data)
        return STRING_NULL;
    return string_new_utf8(view.data, view.length);
}

String string_new_substring(const String *string, usize offset, usize length) {
    if (!string || !string->data || offset > string->length)
        return STRING_NULL;
    if (length > string->length - offset)
        length = string->length - offset;
    return string_new_utf8(string->data + offset, length);
}

String string_new_utf16(str16 data, usize length) {
    if (!data)
        return STRING_NULL;

    usize utf8_len = 0;
    for (usize i = 0; i < length; i++) {
        uint16_t c = (uint16_t)data[i];
        if (c >= 0xD800 && c <= 0xDBFF) {
            if (i + 1 < length) {
                uint16_t next = (uint16_t)data[i + 1];
                if (next >= 0xDC00 && next <= 0xDFFF) {
                    if (utf8_len > SIZE_MAX - 4)
                        return STRING_NULL;
                    utf8_len += 4;
                    i++;
                    continue;
                }
            }
            if (utf8_len > SIZE_MAX - 3)
                return STRING_NULL;
            utf8_len += 3;
        } else if (c >= 0xDC00 && c <= 0xDFFF) {
            if (utf8_len > SIZE_MAX - 3)
                return STRING_NULL;
            utf8_len += 3;
        } else if (c <= 0x7F) {
            if (utf8_len == SIZE_MAX)
                return STRING_NULL;
            utf8_len++;
        } else if (c <= 0x7FF) {
            if (utf8_len > SIZE_MAX - 2)
                return STRING_NULL;
            utf8_len += 2;
        } else {
            if (utf8_len > SIZE_MAX - 3)
                return STRING_NULL;
            utf8_len += 3;
        }
    }

    usize size;
    if (!alloc_size8(utf8_len, &size))
        return STRING_NULL;

    str8 buffer = (str8)utils_malloc(size);
    if (!buffer)
        return STRING_NULL;

    usize out = 0;
    for (usize i = 0; i < length; i++) {
        uint32_t cp;
        uint16_t c = (uint16_t)data[i];

        if (c >= 0xD800 && c <= 0xDBFF) {
            if (i + 1 < length) {
                uint16_t next = (uint16_t)data[i + 1];
                if (next >= 0xDC00 && next <= 0xDFFF) {
                    cp = 0x10000 + (((uint32_t)c - 0xD800) << 10) + ((uint32_t)next - 0xDC00);
                    i++;
                } else {
                    cp = 0xFFFD;
                }
            } else {
                cp = 0xFFFD;
            }
        } else if (c >= 0xDC00 && c <= 0xDFFF) {
            cp = 0xFFFD;
        } else {
            cp = c;
        }

        if (cp <= 0x7F) {
            buffer[out++] = (char)(u8)cp;
        } else if (cp <= 0x7FF) {
            buffer[out++] = (char)(u8)(0xC0 | (cp >> 6));
            buffer[out++] = (char)(u8)(0x80 | (cp & 0x3F));
        } else if (cp <= 0xFFFF) {
            buffer[out++] = (char)(u8)(0xE0 | (cp >> 12));
            buffer[out++] = (char)(u8)(0x80 | ((cp >> 6) & 0x3F));
            buffer[out++] = (char)(u8)(0x80 | (cp & 0x3F));
        } else {
            buffer[out++] = (char)(u8)(0xF0 | (cp >> 18));
            buffer[out++] = (char)(u8)(0x80 | ((cp >> 12) & 0x3F));
            buffer[out++] = (char)(u8)(0x80 | ((cp >> 6) & 0x3F));
            buffer[out++] = (char)(u8)(0x80 | (cp & 0x3F));
        }
    }

    buffer[out] = '\0';
    return (String){out, out, buffer};
}

UString ustring_new(void) { return USTRING_NULL; }

UString ustring_new_cstr(str16 data) {
    if (!data)
        return USTRING_NULL;
    return ustring_new_utf16(data, u16_strlen(data));
}

UString ustring_new_utf16(str16 data, usize length) {
    if (!data)
        return USTRING_NULL;

    usize size;
    if (!alloc_size16(length, &size))
        return USTRING_NULL;

    str16 buffer = (str16)utils_malloc(size);
    if (!buffer)
        return USTRING_NULL;

    if (length)
        memcpy(buffer, data, length * sizeof(u16));
    buffer[length] = 0;
    return (UString){length, length, buffer};
}

UString ustring_new_copy(const UString *string) {
    if (!string || !string->data)
        return USTRING_NULL;
    return ustring_new_utf16(string->data, string->length);
}

UString ustring_new_substring(const UString *string, usize offset, usize length) {
    if (!string || !string->data || offset > string->length)
        return USTRING_NULL;
    if (length > string->length - offset)
        length = string->length - offset;
    return ustring_new_utf16(string->data + offset, length);
}

UString ustring_new_from_view(UStringView view) {
    if (!view.data)
        return USTRING_NULL;
    return ustring_new_utf16(view.data, view.length);
}

UString ustring_new_utf8(str8 data, usize length) {
    if (!data)
        return USTRING_NULL;

    usize utf16_len = utf8_code_units(data, length);
    if (utf16_len == SIZE_MAX)
        return USTRING_NULL;

    usize size;
    if (!alloc_size16(utf16_len, &size))
        return USTRING_NULL;

    str16 buffer = (str16)utils_malloc(size);
    if (!buffer)
        return USTRING_NULL;

    const unsigned char *bytes = (const unsigned char *)data;
    usize out = 0;

    for (usize i = 0; i < length;) {
        uint32_t cp;
        usize consumed;

        if (!utf8_decode_one(bytes + i, length - i, &cp, &consumed)) {
            cp = 0xFFFD;
            consumed = 1;
        }

        i += consumed;

        if (cp > 0xFFFF) {
            cp -= 0x10000;
            buffer[out++] = (u16)(0xD800 + (cp >> 10));
            buffer[out++] = (u16)(0xDC00 + (cp & 0x3FF));
        } else {
            buffer[out++] = (u16)cp;
        }
    }

    buffer[out] = 0;
    return (UString){out, out, buffer};
}

void string_delete(String *string) {
    if (!string)
        return;
    utils_free(string->data);
    *string = STRING_NULL;
}

void ustring_delete(UString *string) {
    if (!string)
        return;
    utils_free(string->data);
    *string = USTRING_NULL;
}

str8 string_view_to_cstr(StringView view) {
    String string;
    str8 result;
    string = string_new_from_view(view);
    if (!string_valid(&string))
        return NULL;
    result = string_cstr(&string);
    return result;
}

str16 ustring_view_to_cstr(UStringView view) {
    UString string;
    str16 result;
    string = ustring_new_from_view(view);
    if (!ustring_valid(&string))
        return NULL;
    result = ustring_cstr(&string);
    return result;
}

bool string_assign_cstr(String *string, str8 data) {
    if (!string || !data)
        return false;
    return string_assign_utf8(string, data, u8_strlen(data));
}

bool string_assign_utf8(String *string, str8 data, usize length) {
    if (!string || !data)
        return false;

    bool alias = buffer_contains8(string->data, string->capacity, data, length);

    if (alias) {
        if (length <= string->capacity) {
            if (length)
                memmove(string->data, data, length);
            string->length = length;
            string->data[length] = '\0';
            return true;
        }

        String copy = string_new_utf8(data, length);
        if (!copy.data)
            return false;
        string_delete(string);
        *string = copy;
        return true;
    }

    if (string->data && string->capacity >= length) {
        if (length)
            memcpy(string->data, data, length);
        string->data[length] = '\0';
        string->length = length;
        return true;
    }

    String copy = string_new_utf8(data, length);
    if (!copy.data)
        return false;
    string_delete(string);
    *string = copy;
    return true;
}

bool string_assign_utf16(String *string, str16 data, usize length) {
    if (!string || !data)
        return false;
    String converted = string_new_utf16(data, length);
    if (!converted.data)
        return false;

    if (string->data && string->capacity >= converted.length) {
        if (converted.length)
            memcpy(string->data, converted.data, converted.length);
        string->data[converted.length] = '\0';
        string->length = converted.length;
        string_delete(&converted);
        return true;
    }

    string_delete(string);
    *string = converted;
    return true;
}

bool string_assign_string(String *string, const String *other) {
    if (!string || !other || !other->data)
        return false;
    return string_assign_utf8(string, other->data, other->length);
}

bool string_assign_view(String *string, StringView view) {
    if (!string || !view.data)
        return false;
    return string_assign_utf8(string, view.data, view.length);
}

bool ustring_assign_cstr(UString *string, str16 data) {
    if (!string || !data)
        return false;
    return ustring_assign_utf16(string, data, u16_strlen(data));
}

bool ustring_assign_utf16(UString *string, str16 data, usize length) {
    if (!string || !data)
        return false;

    bool alias = buffer_contains16(string->data, string->capacity, data, length);

    if (alias) {
        if (length <= string->capacity) {
            if (length)
                memmove(string->data, data, length * sizeof(u16));
            string->length = length;
            string->data[length] = 0;
            return true;
        }

        UString copy = ustring_new_utf16(data, length);
        if (!copy.data)
            return false;
        ustring_delete(string);
        *string = copy;
        return true;
    }

    if (string->data && string->capacity >= length) {
        if (length)
            memcpy(string->data, data, length * sizeof(u16));
        string->data[length] = 0;
        string->length = length;
        return true;
    }

    UString copy = ustring_new_utf16(data, length);
    if (!copy.data)
        return false;
    ustring_delete(string);
    *string = copy;
    return true;
}

bool ustring_assign_utf8(UString *string, str8 data, usize length) {
    if (!string || !data)
        return false;
    UString converted = ustring_new_utf8(data, length);
    if (!converted.data)
        return false;

    if (string->data && string->capacity >= converted.length) {
        if (converted.length)
            memcpy(string->data, converted.data, converted.length * sizeof(u16));
        string->data[converted.length] = 0;
        string->length = converted.length;
        ustring_delete(&converted);
        return true;
    }

    ustring_delete(string);
    *string = converted;
    return true;
}

bool ustring_assign_ustring(UString *string, const UString *other) {
    if (!string || !other || !other->data)
        return false;
    return ustring_assign_utf16(string, other->data, other->length);
}

bool ustring_assign_view(UString *string, UStringView view) {
    if (!string || !view.data)
        return false;
    return ustring_assign_utf16(string, view.data, view.length);
}

void string_clear(String *string) {
    if (!string)
        return;
    if (string->data)
        string->data[0] = '\0';
    string->length = 0;
}

usize string_length(const String *string) { return string ? string->length : 0; }

usize string_size(const String *string) { return string_length(string); }

usize string_capacity(const String *string) { return string ? string->capacity : 0; }

bool string_empty(const String *string) { return string_length(string) == 0; }

bool string_valid(const String *string) { return string && string->data; }

void ustring_clear(UString *string) {
    if (!string)
        return;
    if (string->data)
        string->data[0] = 0;
    string->length = 0;
}

usize ustring_length(const UString *string) { return string ? string->length : 0; }

usize ustring_size(const UString *string) { return ustring_length(string); }

usize ustring_capacity(const UString *string) { return string ? string->capacity : 0; }

bool ustring_empty(const UString *string) { return ustring_length(string) == 0; }

bool ustring_valid(const UString *string) { return string && string->data; }

u8 string_at(const String *string, usize index) {
    return string_valid(string) && index < string->length ? (u8)string->data[index] : 0;
}

u16 ustring_at(const UString *string, usize index) {
    return ustring_valid(string) && index < string->length ? string->data[index] : 0;
}

str8 string_cstr(const String *string) { return string && string->data ? string->data : (str8) ""; }

str16 ustring_cstr(const UString *string) {
    static u16 empty[] = {0};
    return string && string->data ? string->data : empty;
}

str8 string_data(String *string) { return string ? string->data : NULL; }

str8 string_data_const(const String *string) { return string ? string->data : NULL; }

str16 ustring_data(UString *string) { return string ? string->data : NULL; }

str16 ustring_data_const(const UString *string) { return string ? string->data : NULL; }

StringView string_view(const String *string, usize offset, usize length) {
    if (!string || !string->data || offset > string->length)
        return STRING_VIEW_EMPTY;
    if (length > string->length - offset)
        length = string->length - offset;
    return (StringView){length, string->data + offset};
}

StringView string_view_cstr(str8 data, usize offset, usize length) {
    if (!data)
        return STRING_VIEW_EMPTY;
    usize total = u8_strlen(data);
    if (offset > total)
        return STRING_VIEW_EMPTY;
    if (length > total - offset)
        length = total - offset;
    return (StringView){length, data + offset};
}

StringView string_view_data(str8 data, usize length) {
    if (!data && length)
        return STRING_VIEW_EMPTY;
    return (StringView){length, data};
}

UStringView ustring_view(const UString *string, usize offset, usize length) {
    if (!string || !string->data || offset > string->length)
        return USTRING_VIEW_EMPTY;
    if (length > string->length - offset)
        length = string->length - offset;
    return (UStringView){length, string->data + offset};
}

UStringView ustring_view_cstr(str16 data, usize offset, usize length) {
    if (!data)
        return USTRING_VIEW_EMPTY;
    usize total = u16_strlen(data);
    if (offset > total)
        return USTRING_VIEW_EMPTY;
    if (length > total - offset)
        length = total - offset;
    return (UStringView){length, data + offset};
}

UStringView ustring_view_data(str16 data, usize length) {
    if (!data && length)
        return USTRING_VIEW_EMPTY;
    return (UStringView){length, data};
}

usize string_view_length(StringView view) { return view.length; }

bool string_view_empty(StringView view) { return view.length == 0; }

bool string_view_valid(StringView view) { return view.data != NULL || view.length == 0; }

u8 string_view_at(StringView view, usize index) {
    return view.data && index < view.length ? (u8)view.data[index] : 0;
}

int string_view_compare(StringView a, StringView b) {
    usize min_len = a.length < b.length ? a.length : b.length;
    if (min_len && a.data && b.data) {
        int cmp = memcmp(a.data, b.data, min_len);
        if (cmp)
            return cmp;
    }
    return a.length < b.length ? -1 : a.length > b.length ? 1 : 0;
}

bool string_view_equal(StringView a, StringView b) {
    return a.length == b.length &&
           (a.length == 0 || (a.data && b.data && memcmp(a.data, b.data, a.length) == 0));
}

usize string_view_find(StringView view, StringView needle) {
    if (needle.length == 0)
        return 0;
    if (!view.data || !needle.data || needle.length > view.length)
        return STRING_NOT_FOUND;
    for (usize i = 0, end = view.length - needle.length; i <= end; i++) {
        if (memcmp(view.data + i, needle.data, needle.length) == 0)
            return i;
    }
    return STRING_NOT_FOUND;
}

usize string_view_rfind(StringView view, StringView needle) {
    if (needle.length == 0)
        return view.length;
    if (!view.data || !needle.data || needle.length > view.length)
        return STRING_NOT_FOUND;
    for (usize i = view.length - needle.length + 1; i > 0; i--) {
        if (memcmp(view.data + i - 1, needle.data, needle.length) == 0)
            return i - 1;
    }
    return STRING_NOT_FOUND;
}

bool string_view_starts_with(StringView view, StringView prefix) {
    return prefix.length <= view.length &&
           (prefix.length == 0 ||
            (view.data && prefix.data && memcmp(view.data, prefix.data, prefix.length) == 0));
}

bool string_view_ends_with(StringView view, StringView suffix) {
    return suffix.length <= view.length &&
           (suffix.length == 0 ||
            (view.data && suffix.data &&
             memcmp(view.data + view.length - suffix.length, suffix.data, suffix.length) == 0));
}

bool string_view_contains(StringView view, StringView needle) {
    return string_view_find(view, needle) != STRING_NOT_FOUND;
}

usize ustring_view_length(UStringView view) { return view.length; }

bool ustring_view_empty(UStringView view) { return view.length == 0; }

bool ustring_view_valid(UStringView view) { return view.data != NULL || view.length == 0; }

u16 ustring_view_at(UStringView view, usize index) {
    return view.data && index < view.length ? view.data[index] : 0;
}

int ustring_view_compare(UStringView a, UStringView b) {
    usize min_len = a.length < b.length ? a.length : b.length;
    if (min_len && a.data && b.data) {
        int cmp = memcmp(a.data, b.data, min_len * sizeof(u16));
        if (cmp)
            return cmp;
    }
    return a.length < b.length ? -1 : a.length > b.length ? 1 : 0;
}

bool ustring_view_equal(UStringView a, UStringView b) {
    return a.length == b.length &&
           (a.length == 0 ||
            (a.data && b.data && memcmp(a.data, b.data, a.length * sizeof(u16)) == 0));
}

usize ustring_view_find(UStringView view, UStringView needle) {
    if (needle.length == 0)
        return 0;
    if (!view.data || !needle.data || needle.length > view.length)
        return STRING_NOT_FOUND;
    for (usize i = 0, end = view.length - needle.length; i <= end; i++) {
        if (memcmp(view.data + i, needle.data, needle.length * sizeof(u16)) == 0)
            return i;
    }
    return STRING_NOT_FOUND;
}

usize ustring_view_rfind(UStringView view, UStringView needle) {
    if (needle.length == 0)
        return view.length;
    if (!view.data || !needle.data || needle.length > view.length)
        return STRING_NOT_FOUND;
    for (usize i = view.length - needle.length + 1; i > 0; i--) {
        if (memcmp(view.data + i - 1, needle.data, needle.length * sizeof(u16)) == 0)
            return i - 1;
    }
    return STRING_NOT_FOUND;
}

bool ustring_view_starts_with(UStringView view, UStringView prefix) {
    return prefix.length <= view.length &&
           (prefix.length == 0 ||
            (view.data && prefix.data &&
             memcmp(view.data, prefix.data, prefix.length * sizeof(u16)) == 0));
}

bool ustring_view_ends_with(UStringView view, UStringView suffix) {
    return suffix.length <= view.length &&
           (suffix.length == 0 || (view.data && suffix.data &&
                                   memcmp(view.data + view.length - suffix.length, suffix.data,
                                          suffix.length * sizeof(u16)) == 0));
}

bool ustring_view_contains(UStringView view, UStringView needle) {
    return ustring_view_find(view, needle) != STRING_NOT_FOUND;
}

int string_compare(const String *a, const String *b) {
    if (!a && !b)
        return 0;
    if (!a)
        return -1;
    if (!b)
        return 1;
    StringView va = a->data ? (StringView){a->length, a->data} : STRING_VIEW_EMPTY;
    StringView vb = b->data ? (StringView){b->length, b->data} : STRING_VIEW_EMPTY;
    return string_view_compare(va, vb);
}

int string_compare_cstr(const String *string, str8 cstr) {
    if (!string && !cstr)
        return 0;
    if (!string)
        return -1;
    if (!cstr)
        return 1;
    return string_view_compare(string->data ? (StringView){string->length, string->data}
                                            : STRING_VIEW_EMPTY,
                               (StringView){u8_strlen(cstr), cstr});
}

int string_compare_view(const String *string, StringView view) {
    if (!string)
        return -1;
    return string_view_compare(
        string->data ? (StringView){string->length, string->data} : STRING_VIEW_EMPTY, view);
}

bool string_equal(const String *a, const String *b) { return string_compare(a, b) == 0; }

bool string_equal_cstr(const String *string, str8 cstr) {
    return string_compare_cstr(string, cstr) == 0;
}

bool string_equal_view(const String *string, StringView view) {
    return string_compare_view(string, view) == 0;
}

int ustring_compare(const UString *a, const UString *b) {
    if (!a && !b)
        return 0;
    if (!a)
        return -1;
    if (!b)
        return 1;
    UStringView va = a->data ? (UStringView){a->length, a->data} : USTRING_VIEW_EMPTY;
    UStringView vb = b->data ? (UStringView){b->length, b->data} : USTRING_VIEW_EMPTY;
    return ustring_view_compare(va, vb);
}

int ustring_compare_cstr(const UString *string, str16 cstr) {
    if (!string && !cstr)
        return 0;
    if (!string)
        return -1;
    if (!cstr)
        return 1;
    return ustring_view_compare(string->data ? (UStringView){string->length, string->data}
                                             : USTRING_VIEW_EMPTY,
                                (UStringView){u16_strlen(cstr), cstr});
}

int ustring_compare_view(const UString *string, UStringView view) {
    if (!string)
        return -1;
    return ustring_view_compare(
        string->data ? (UStringView){string->length, string->data} : USTRING_VIEW_EMPTY, view);
}

bool ustring_equal(const UString *a, const UString *b) { return ustring_compare(a, b) == 0; }

bool ustring_equal_cstr(const UString *string, str16 cstr) {
    return ustring_compare_cstr(string, cstr) == 0;
}

bool ustring_equal_view(const UString *string, UStringView view) {
    return ustring_compare_view(string, view) == 0;
}

usize string_find(const String *string, StringView needle) {
    if (!string)
        return STRING_NOT_FOUND;
    return string_view_find(
        string->data ? (StringView){string->length, string->data} : STRING_VIEW_EMPTY, needle);
}

usize string_rfind(const String *string, StringView needle) {
    if (!string)
        return STRING_NOT_FOUND;
    return string_view_rfind(
        string->data ? (StringView){string->length, string->data} : STRING_VIEW_EMPTY, needle);
}

usize string_find_u8(const String *string, u8 character) {
    if (!string || !string->data)
        return STRING_NOT_FOUND;
    for (usize i = 0; i < string->length; i++)
        if (string->data[i] == character)
            return i;
    return STRING_NOT_FOUND;
}

usize string_rfind_u8(const String *string, u8 character) {
    if (!string || !string->data)
        return STRING_NOT_FOUND;
    for (usize i = string->length; i > 0; i--)
        if (string->data[i - 1] == character)
            return i - 1;
    return STRING_NOT_FOUND;
}

usize ustring_find(const UString *string, UStringView needle) {
    if (!string)
        return STRING_NOT_FOUND;
    return ustring_view_find(
        string->data ? (UStringView){string->length, string->data} : USTRING_VIEW_EMPTY, needle);
}

usize ustring_rfind(const UString *string, UStringView needle) {
    if (!string)
        return STRING_NOT_FOUND;
    return ustring_view_rfind(
        string->data ? (UStringView){string->length, string->data} : USTRING_VIEW_EMPTY, needle);
}

usize ustring_find_u16(const UString *string, u16 character) {
    if (!string || !string->data)
        return STRING_NOT_FOUND;
    for (usize i = 0; i < string->length; i++)
        if (string->data[i] == character)
            return i;
    return STRING_NOT_FOUND;
}

usize ustring_rfind_u16(const UString *string, u16 character) {
    if (!string || !string->data)
        return STRING_NOT_FOUND;
    for (usize i = string->length; i > 0; i--)
        if (string->data[i - 1] == character)
            return i - 1;
    return STRING_NOT_FOUND;
}

bool string_starts_with(const String *string, StringView prefix) {
    if (!string)
        return false;
    return string_view_starts_with(
        string->data ? (StringView){string->length, string->data} : STRING_VIEW_EMPTY, prefix);
}

bool string_ends_with(const String *string, StringView suffix) {
    if (!string)
        return false;
    return string_view_ends_with(
        string->data ? (StringView){string->length, string->data} : STRING_VIEW_EMPTY, suffix);
}

bool string_contains(const String *string, StringView needle) {
    if (!string)
        return false;
    return string_view_contains(
        string->data ? (StringView){string->length, string->data} : STRING_VIEW_EMPTY, needle);
}

bool ustring_starts_with(const UString *string, UStringView prefix) {
    if (!string)
        return false;
    return ustring_view_starts_with(
        string->data ? (UStringView){string->length, string->data} : USTRING_VIEW_EMPTY, prefix);
}

bool ustring_ends_with(const UString *string, UStringView suffix) {
    if (!string)
        return false;
    return ustring_view_ends_with(
        string->data ? (UStringView){string->length, string->data} : USTRING_VIEW_EMPTY, suffix);
}

bool ustring_contains(const UString *string, UStringView needle) {
    if (!string)
        return false;
    return ustring_view_contains(
        string->data ? (UStringView){string->length, string->data} : USTRING_VIEW_EMPTY, needle);
}

bool string_append_utf8(String *string, str8 data, usize length) {
    if (!string || (!data && length))
        return false;
    if (!length)
        return true;

    usize new_len;
    if (add_overflow(string->length, length, &new_len))
        return false;

    bool alias = buffer_contains8((str8)string->data, string->capacity, data, length);
    usize offset = alias ? (usize)((uintptr_t)data - (uintptr_t)string->data) : 0;

    if (!string_ensure(string, new_len))
        return false;

    str8 source = alias ? (str8)string->data + offset : data;
    memmove(string->data + string->length, source, length);
    string->length = new_len;
    string->data[new_len] = '\0';
    return true;
}

bool string_append_cstr(String *string, str8 data) {
    if (!data)
        return false;
    return string_append_utf8(string, data, u8_strlen(data));
}

bool string_append_utf16(String *string, str16 data, usize length) {
    if (!string || !data)
        return false;
    String converted = string_new_utf16(data, length);
    if (!converted.data)
        return false;
    bool result = string_append_utf8(string, converted.data, converted.length);
    string_delete(&converted);
    return result;
}

bool string_append_string(String *string, const String *other) {
    if (!other || !other->data)
        return false;
    return string_append_utf8(string, other->data, other->length);
}

bool string_append_view(String *string, StringView view) {
    return string_append_utf8(string, view.data, view.length);
}

bool ustring_append_utf16(UString *string, str16 data, usize length) {
    if (!string || (!data && length))
        return false;
    if (!length)
        return true;

    usize new_len;
    if (add_overflow(string->length, length, &new_len))
        return false;

    bool alias = buffer_contains16(string->data, string->capacity, data, length);
    usize offset = alias ? (usize)(((uintptr_t)data - (uintptr_t)string->data) / sizeof(u16)) : 0;

    if (!ustring_ensure(string, new_len))
        return false;

    str16 source = alias ? string->data + offset : data;
    memmove(string->data + string->length, source, length * sizeof(u16));
    string->length = new_len;
    string->data[new_len] = 0;
    return true;
}

bool ustring_append_cstr(UString *string, str16 data) {
    if (!data)
        return false;
    return ustring_append_utf16(string, data, u16_strlen(data));
}

bool ustring_append_utf8(UString *string, str8 data, usize length) {
    if (!string || !data)
        return false;
    UString converted = ustring_new_utf8(data, length);
    if (!converted.data)
        return false;
    bool result = ustring_append_utf16(string, converted.data, converted.length);
    ustring_delete(&converted);
    return result;
}

bool ustring_append_string(UString *string, const UString *other) {
    if (!other || !other->data)
        return false;
    return ustring_append_utf16(string, other->data, other->length);
}

bool ustring_append_view(UString *string, UStringView view) {
    return ustring_append_utf16(string, view.data, view.length);
}

bool string_insert_utf8(String *string, usize offset, str8 data, usize length) {
    if (!string || offset > string->length || (!data && length))
        return false;
    if (!length)
        return true;

    usize new_len;
    if (add_overflow(string->length, length, &new_len))
        return false;

    bool alias = buffer_contains8(string->data, string->capacity, data, length);
    str8 temp = NULL;

    if (alias) {
        temp = (str8)utils_malloc(length);
        if (!temp)
            return false;
        memcpy(temp, data, length);
    }

    str8 insert_data = alias ? temp : data;
    bool result = false;

    if (string_ensure(string, new_len)) {
        memmove(string->data + offset + length, string->data + offset, string->length - offset);
        memcpy(string->data + offset, insert_data, length);
        string->length = new_len;
        string->data[new_len] = '\0';
        result = true;
    }

    utils_free(temp);
    return result;
}

bool string_insert_cstr(String *string, usize offset, str8 data) {
    if (!data)
        return false;
    return string_insert_utf8(string, offset, data, u8_strlen(data));
}

bool string_insert_utf16(String *string, usize offset, str16 data, usize length) {
    if (!string || !data)
        return false;
    String converted = string_new_utf16(data, length);
    if (!converted.data)
        return false;
    bool result = string_insert_utf8(string, offset, converted.data, converted.length);
    string_delete(&converted);
    return result;
}

bool string_insert_string(String *string, usize offset, const String *other) {
    if (!other || !other->data)
        return false;
    return string_insert_utf8(string, offset, other->data, other->length);
}

bool string_insert_view(String *string, usize offset, StringView view) {
    return string_insert_utf8(string, offset, view.data, view.length);
}

bool ustring_insert_utf16(UString *string, usize offset, str16 data, usize length) {
    if (!string || offset > string->length || (!data && length))
        return false;
    if (!length)
        return true;

    usize new_len;
    if (add_overflow(string->length, length, &new_len))
        return false;

    bool alias = buffer_contains16(string->data, string->capacity, data, length);
    str16 temp = NULL;

    if (alias) {
        if (length > SIZE_MAX / sizeof(u16))
            return false;
        temp = (str16)utils_malloc(length * sizeof(u16));
        if (!temp)
            return false;
        memcpy(temp, data, length * sizeof(u16));
    }

    str16 insert_data = alias ? temp : data;
    bool result = false;

    if (ustring_ensure(string, new_len)) {
        memmove(string->data + offset + length, string->data + offset,
                (string->length - offset) * sizeof(u16));
        memcpy(string->data + offset, insert_data, length * sizeof(u16));
        string->length = new_len;
        string->data[new_len] = 0;
        result = true;
    }

    utils_free(temp);
    return result;
}

bool ustring_insert_cstr(UString *string, usize offset, str16 data) {
    if (!data)
        return false;
    return ustring_insert_utf16(string, offset, data, u16_strlen(data));
}

bool ustring_insert_utf8(UString *string, usize offset, str8 data, usize length) {
    if (!string || !data)
        return false;
    UString converted = ustring_new_utf8(data, length);
    if (!converted.data)
        return false;
    bool result = ustring_insert_utf16(string, offset, converted.data, converted.length);
    ustring_delete(&converted);
    return result;
}

bool ustring_insert_string(UString *string, usize offset, const UString *other) {
    if (!other || !other->data)
        return false;
    return ustring_insert_utf16(string, offset, other->data, other->length);
}

bool ustring_insert_view(UString *string, usize offset, UStringView view) {
    return ustring_insert_utf16(string, offset, view.data, view.length);
}

bool string_remove(String *string, usize offset, usize length) {
    if (!string || !string->data || offset > string->length)
        return false;
    if (length > string->length - offset)
        length = string->length - offset;
    if (!length)
        return true;

    memmove(string->data + offset, string->data + offset + length,
            string->length - offset - length + 1);
    string->length -= length;
    return true;
}

bool ustring_remove(UString *string, usize offset, usize length) {
    if (!string || !string->data || offset > string->length)
        return false;
    if (length > string->length - offset)
        length = string->length - offset;
    if (!length)
        return true;

    memmove(string->data + offset, string->data + offset + length,
            (string->length - offset - length + 1) * sizeof(u16));
    string->length -= length;
    return true;
}

bool string_replace_utf8(String *string, usize offset, usize length, str8 data, usize data_length) {
    if (!string || !string->data || offset > string->length || (!data && data_length))
        return false;
    if (length > string->length - offset)
        length = string->length - offset;

    bool alias = data_length && buffer_contains8(string->data, string->capacity, data, data_length);
    str8 temp = NULL;

    if (alias) {
        temp = (str8)utils_malloc(data_length);
        if (!temp)
            return false;
        memcpy(temp, data, data_length);
    }

    str8 replacement = alias ? temp : data;
    usize base = string->length - length;
    usize new_len;

    if (add_overflow(base, data_length, &new_len)) {
        utils_free(temp);
        return false;
    }

    bool result = false;

    if (string_ensure(string, new_len)) {
        memmove(string->data + offset + data_length, string->data + offset + length,
                string->length - offset - length);
        if (data_length)
            memcpy(string->data + offset, replacement, data_length);
        string->length = new_len;
        string->data[new_len] = '\0';
        result = true;
    }

    utils_free(temp);
    return result;
}

bool string_replace_cstr(String *string, usize offset, usize length, str8 data) {
    if (!data)
        return false;
    return string_replace_utf8(string, offset, length, data, u8_strlen(data));
}

bool string_replace_utf16(String *string, usize offset, usize length, str16 data,
                          usize data_length) {
    if (!string || !data)
        return false;
    String converted = string_new_utf16(data, data_length);
    if (!converted.data)
        return false;
    bool result = string_replace_utf8(string, offset, length, converted.data, converted.length);
    string_delete(&converted);
    return result;
}

bool string_replace_string(String *string, usize offset, usize length, const String *replacement) {
    if (!replacement || !replacement->data)
        return false;
    return string_replace_utf8(string, offset, length, replacement->data, replacement->length);
}

bool string_replace_view(String *string, usize offset, usize length, StringView replacement) {
    return string_replace_utf8(string, offset, length, replacement.data, replacement.length);
}

bool ustring_replace_utf16(UString *string, usize offset, usize length, str16 data,
                           usize data_length) {
    if (!string || !string->data || offset > string->length || (!data && data_length))
        return false;
    if (length > string->length - offset)
        length = string->length - offset;

    bool alias =
        data_length && buffer_contains16(string->data, string->capacity, data, data_length);
    str16 temp = NULL;

    if (alias) {
        if (data_length > SIZE_MAX / sizeof(u16))
            return false;
        temp = (str16)utils_malloc(data_length * sizeof(u16));
        if (!temp)
            return false;
        memcpy(temp, data, data_length * sizeof(u16));
    }

    str16 replacement = alias ? temp : data;
    usize base = string->length - length;
    usize new_len;

    if (add_overflow(base, data_length, &new_len)) {
        utils_free(temp);
        return false;
    }

    bool result = false;

    if (ustring_ensure(string, new_len)) {
        memmove(string->data + offset + data_length, string->data + offset + length,
                (string->length - offset - length) * sizeof(u16));
        if (data_length)
            memcpy(string->data + offset, replacement, data_length * sizeof(u16));
        string->length = new_len;
        string->data[new_len] = 0;
        result = true;
    }

    utils_free(temp);
    return result;
}

bool ustring_replace_cstr(UString *string, usize offset, usize length, str16 data) {
    if (!data)
        return false;
    return ustring_replace_utf16(string, offset, length, data, u16_strlen(data));
}

bool ustring_replace_utf8(UString *string, usize offset, usize length, str8 data,
                          usize data_length) {
    if (!string || !data)
        return false;
    UString converted = ustring_new_utf8(data, data_length);
    if (!converted.data)
        return false;
    bool result = ustring_replace_utf16(string, offset, length, converted.data, converted.length);
    ustring_delete(&converted);
    return result;
}

bool ustring_replace_string(UString *string, usize offset, usize length,
                            const UString *replacement) {
    if (!replacement || !replacement->data)
        return false;
    return ustring_replace_utf16(string, offset, length, replacement->data, replacement->length);
}

bool ustring_replace_view(UString *string, usize offset, usize length, UStringView replacement) {
    return ustring_replace_utf16(string, offset, length, replacement.data, replacement.length);
}

void string_lower(String *string) {
    if (!string || !string->data)
        return;
    for (usize i = 0; i < string->length; i++)
        string->data[i] = (char)(u8)tolower((unsigned char)string->data[i]);
}

void string_upper(String *string) {
    if (!string || !string->data)
        return;
    for (usize i = 0; i < string->length; i++)
        string->data[i] = (char)(u8)toupper((unsigned char)string->data[i]);
}

static Codepoint latin1_to_lower(Codepoint cp) {
    if (cp >= 'A' && cp <= 'Z')
        return cp + ('a' - 'A');
    if ((cp >= 0xC0 && cp <= 0xD6) || (cp >= 0xD8 && cp <= 0xDE))
        return cp + 0x20;
    return cp;
}

static Codepoint latin1_to_upper(Codepoint cp) {
    if (cp >= 'a' && cp <= 'z')
        return cp - ('a' - 'A');
    if ((cp >= 0xE0 && cp <= 0xF6) || (cp >= 0xF8 && cp <= 0xFE))
        return cp - 0x20;
    return cp;
}

static void string_map_case_utf8(String *string, Codepoint (*map)(Codepoint)) {
    if (!string || !string->data || !string->length)
        return;

    const u8 *bytes = (const u8 *)string->data;
    usize i = 0;
    while (i < string->length) {
        usize start = i;
        Codepoint cp;
        if (!utf8_decode(bytes, string->length, &i, &cp)) {
            i = start + 1;
            continue;
        }
        Codepoint mapped = map(cp);
        if (mapped == cp)
            continue;
        u8 encoded[4];
        usize n = utf8_encode(mapped, encoded);
        usize old_n = i - start;
        if (n == 0 || n != old_n)
            continue;
        memcpy(string->data + start, encoded, n);
    }
}

void string_lower_utf8(String *string) { string_map_case_utf8(string, latin1_to_lower); }

void string_upper_utf8(String *string) { string_map_case_utf8(string, latin1_to_upper); }

void string_trim_left(String *string) {
    if (!string || !string->data)
        return;
    usize start = 0;
    while (start < string->length && isspace((unsigned char)string->data[start]))
        start++;
    if (start)
        string_remove(string, 0, start);
}

void string_trim_right(String *string) {
    if (!string || !string->data)
        return;
    usize end = string->length;
    while (end && isspace((unsigned char)string->data[end - 1]))
        end--;
    if (end != string->length) {
        string->length = end;
        string->data[end] = '\0';
    }
}

void string_trim(String *string) {
    string_trim_right(string);
    string_trim_left(string);
}

void ustring_lower(UString *string) {
    if (!string || !string->data)
        return;
    for (usize i = 0; i < string->length; i++) {
        u16 c = string->data[i];
        if (c >= 'A' && c <= 'Z')
            string->data[i] = c + ('a' - 'A');
    }
}

void ustring_upper(UString *string) {
    if (!string || !string->data)
        return;
    for (usize i = 0; i < string->length; i++) {
        u16 c = string->data[i];
        if (c >= 'a' && c <= 'z')
            string->data[i] = c - ('a' - 'A');
    }
}

void ustring_trim_left(UString *string) {
    if (!string || !string->data)
        return;
    usize start = 0;
    while (start < string->length && string->data[start] <= 0x7F &&
           isspace((unsigned char)string->data[start]))
        start++;
    if (start)
        ustring_remove(string, 0, start);
}

void ustring_trim_right(UString *string) {
    if (!string || !string->data)
        return;
    usize end = string->length;
    while (end && string->data[end - 1] <= 0x7F && isspace((unsigned char)string->data[end - 1]))
        end--;
    if (end != string->length) {
        string->length = end;
        string->data[end] = 0;
    }
}

void ustring_trim(UString *string) {
    ustring_trim_right(string);
    ustring_trim_left(string);
}

bool string_is_ascii(const String *string) {
    if (!string || !string->data)
        return true;
    for (usize i = 0; i < string->length; i++)
        if ((unsigned char)string->data[i] > 0x7F)
            return false;
    return true;
}

bool string_is_utf8(const String *string) {
    if (!string || !string->data)
        return true;
    for (usize i = 0; i < string->length;) {
        uint32_t cp;
        usize consumed;
        if (!utf8_decode_one((const unsigned char *)string->data + i, string->length - i, &cp,
                             &consumed))
            return false;
        i += consumed;
    }
    return true;
}

bool ustring_is_valid_utf16(const UString *string) {
    if (!string || !string->data)
        return true;
    for (usize i = 0; i < string->length; i++) {
        uint16_t c = (uint16_t)string->data[i];
        if (c >= 0xD800 && c <= 0xDBFF) {
            if (i + 1 >= string->length)
                return false;
            uint16_t next = (uint16_t)string->data[++i];
            if (next < 0xDC00 || next > 0xDFFF)
                return false;
        } else if (c >= 0xDC00 && c <= 0xDFFF) {
            return false;
        }
    }
    return true;
}

bool string_reserve(String *string, usize capacity) {
    if (!string || capacity < string->length)
        return false;

    usize size;
    if (!alloc_size8(capacity, &size))
        return false;
    if (string->capacity >= capacity && string->data)
        return true;

    str8 data = (str8)utils_realloc(string->data, size);
    if (!data)
        return false;

    string->data = data;
    string->capacity = capacity;
    string->data[string->length] = '\0';
    return true;
}

bool string_shrink_to_fit(String *string) {
    if (!string || string->length > string->capacity)
        return false;
    if (string->length == 0) {
        utils_free(string->data);
        string->data = NULL;
        string->capacity = 0;
        return true;
    }

    usize size;
    if (!alloc_size8(string->length, &size))
        return false;

    str8 data = (str8)utils_realloc(string->data, size);
    if (!data)
        return false;

    string->data = data;
    string->capacity = string->length;
    string->data[string->length] = '\0';
    return true;
}

bool ustring_reserve(UString *string, usize capacity) {
    if (!string || capacity < string->length)
        return false;

    usize size;
    if (!alloc_size16(capacity, &size))
        return false;
    if (string->capacity >= capacity && string->data)
        return true;

    str16 data = (str16)utils_realloc(string->data, size);
    if (!data)
        return false;

    string->data = data;
    string->capacity = capacity;
    string->data[string->length] = 0;
    return true;
}

bool ustring_shrink_to_fit(UString *string) {
    if (!string || string->length > string->capacity)
        return false;
    if (string->length == 0) {
        utils_free(string->data);
        string->data = NULL;
        string->capacity = 0;
        return true;
    }

    usize size;
    if (!alloc_size16(string->length, &size))
        return false;

    str16 data = (str16)utils_realloc(string->data, size);
    if (!data)
        return false;

    string->data = data;
    string->capacity = string->length;
    string->data[string->length] = 0;
    return true;
}

void string_swap(String *a, String *b) {
    if (!a || !b || a == b)
        return;
    String temp = *a;
    *a = *b;
    *b = temp;
}

void ustring_swap(UString *a, UString *b) {
    if (!a || !b || a == b)
        return;
    UString temp = *a;
    *a = *b;
    *b = temp;
}

UString ustring_from_string(const String *string) {
    if (!string || !string->data)
        return USTRING_NULL;
    return ustring_new_utf8(string->data, string->length);
}

String string_from_ustring(const UString *string) {
    if (!string || !string->data)
        return STRING_NULL;
    return string_new_utf16(string->data, string->length);
}

bool string_assign_ustring(String *string, const UString *other) {
    if (!string || !other || !other->data)
        return false;
    return string_assign_utf16(string, other->data, other->length);
}

bool ustring_assign_string(UString *string, const String *other) {
    if (!string || !other || !other->data)
        return false;
    return ustring_assign_utf8(string, other->data, other->length);
}

usize utf8_encode(Codepoint cp, u8 out[4]) {
    if (!out)
        return 0;
    if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
        return 0;

    if (cp <= 0x7F) {
        out[0] = (u8)cp;
        return 1;
    }
    if (cp <= 0x7FF) {
        out[0] = (u8)(0xC0 | (cp >> 6));
        out[1] = (u8)(0x80 | (cp & 0x3F));
        return 2;
    }
    if (cp <= 0xFFFF) {
        out[0] = (u8)(0xE0 | (cp >> 12));
        out[1] = (u8)(0x80 | ((cp >> 6) & 0x3F));
        out[2] = (u8)(0x80 | (cp & 0x3F));
        return 3;
    }
    out[0] = (u8)(0xF0 | (cp >> 18));
    out[1] = (u8)(0x80 | ((cp >> 12) & 0x3F));
    out[2] = (u8)(0x80 | ((cp >> 6) & 0x3F));
    out[3] = (u8)(0x80 | (cp & 0x3F));
    return 4;
}

bool utf8_decode(const u8 *data, usize len, usize *byte_index, Codepoint *out) {
    if (!data || !byte_index || !out || *byte_index >= len)
        return false;

    uint32_t cp;
    usize consumed;
    if (!utf8_decode_one(data + *byte_index, len - *byte_index, &cp, &consumed))
        return false;

    *out = (Codepoint)cp;
    *byte_index += consumed;
    return true;
}

usize utf8_codepoint_count(StringView view) {
    if (!view.data && view.length)
        return 0;
    usize count = 0;
    usize i = 0;
    while (i < view.length) {
        Codepoint cp;
        if (!utf8_decode((const u8 *)view.data, view.length, &i, &cp)) {
            i++;
            continue;
        }
        count++;
    }
    return count;
}

bool utf8_next(StringView view, usize *byte_index, Codepoint *out) {
    if (!byte_index || !out || !view.data)
        return false;
    return utf8_decode((const u8 *)view.data, view.length, byte_index, out);
}

bool utf8_prev(StringView view, usize *byte_index, Codepoint *out) {
    if (!byte_index || !out || !view.data || *byte_index == 0 || *byte_index > view.length)
        return false;

    usize i = *byte_index;
    do {
        i--;
    } while (i > 0 && (((u8)view.data[i]) & 0xC0) == 0x80);

    usize idx = i;
    Codepoint cp;
    if (!utf8_decode((const u8 *)view.data, view.length, &idx, &cp))
        return false;
    if (idx != *byte_index)
        return false;

    *byte_index = i;
    *out = cp;
    return true;
}

bool string_split(StringView s, StringView sep, Vec *out_views) {
    if (!out_views || sep.length == 0)
        return false;
    if (!s.data && s.length)
        return false;
    if (out_views->stride && out_views->stride != sizeof(StringView))
        return false;

    if (!out_views->stride) {
        *out_views = vec_new(sizeof(StringView));
        if (!vec_valid(out_views))
            return false;
    } else {
        vec_clear(out_views);
    }

    if (!s.data || s.length == 0) {
        StringView empty = {0, s.data};
        return vec_push(out_views, &empty);
    }

    usize start = 0;
    while (start <= s.length) {
        StringView rest = {s.length - start, s.data + start};
        usize found = string_view_find(rest, sep);
        if (found == STRING_NOT_FOUND) {
            StringView part = {s.length - start, s.data + start};
            return vec_push(out_views, &part);
        }
        StringView part = {found, s.data + start};
        if (!vec_push(out_views, &part))
            return false;
        start += found + sep.length;
        if (start == s.length) {
            StringView empty = {0, s.data + start};
            return vec_push(out_views, &empty);
        }
    }
    return true;
}

bool string_join(String *out, StringView sep, const StringView *parts, usize count) {
    if (!out)
        return false;
    if (count && !parts)
        return false;

    string_clear(out);
    if (count == 0)
        return true;

    for (usize i = 0; i < count; i++) {
        if (i > 0 && !string_append_view(out, sep))
            return false;
        if (!string_append_view(out, parts[i]))
            return false;
    }
    return true;
}

bool string_join_vec(String *out, StringView sep, const Vec *parts) {
    if (!out || !parts || !vec_valid(parts) || parts->stride != sizeof(StringView))
        return false;
    return string_join(out, sep, (const StringView *)vec_data_const(parts), parts->length);
}

String string_format(const char *fmt, ...) {
    if (!fmt)
        return STRING_NULL;

    va_list ap;
    va_start(ap, fmt);
    int needed = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (needed < 0)
        return STRING_NULL;

    usize length = (usize)needed;
    usize size;
    if (!alloc_size8(length, &size))
        return STRING_NULL;

    str8 buffer = (str8)utils_malloc(size);
    if (!buffer)
        return STRING_NULL;

    va_start(ap, fmt);
    int written = vsnprintf(buffer, size, fmt, ap);
    va_end(ap);
    if (written < 0 || (usize)written != length) {
        utils_free(buffer);
        return STRING_NULL;
    }

    return (String){length, length, buffer};
}

bool string_append_format(String *string, const char *fmt, ...) {
    if (!string || !fmt)
        return false;

    va_list ap;
    va_start(ap, fmt);
    int needed = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (needed < 0)
        return false;

    usize add_len = (usize)needed;
    usize new_len;
    if (add_overflow(string->length, add_len, &new_len))
        return false;
    if (!string_ensure(string, new_len))
        return false;

    va_start(ap, fmt);
    int written = vsnprintf(string->data + string->length, add_len + 1, fmt, ap);
    va_end(ap);
    if (written < 0 || (usize)written != add_len)
        return false;

    string->length = new_len;
    string->data[string->length] = '\0';
    return true;
}

static bool string_replace_needle(String *string, StringView needle, StringView replacement,
                                  bool all) {
    if (!string || !string->data || needle.length == 0 || !needle.data)
        return false;
    if (replacement.length && !replacement.data)
        return false;

    if (!all) {
        usize pos = string_find(string, needle);
        if (pos == STRING_NOT_FOUND)
            return false;
        return string_replace_view(string, pos, needle.length, replacement);
    }

    String result = string_new();
    usize pos = 0;
    bool any = false;

    while (pos <= string->length) {
        StringView rest = {string->length - pos, string->data + pos};
        usize found = string_view_find(rest, needle);
        if (found == STRING_NOT_FOUND) {
            if (!string_append_utf8(&result, string->data + pos, string->length - pos)) {
                string_delete(&result);
                return false;
            }
            break;
        }
        any = true;
        if (!string_append_utf8(&result, string->data + pos, found)) {
            string_delete(&result);
            return false;
        }
        if (!string_append_view(&result, replacement)) {
            string_delete(&result);
            return false;
        }
        pos += found + needle.length;
        if (pos == string->length) {
            break;
        }
    }

    if (!any) {
        string_delete(&result);
        return false;
    }

    string_swap(string, &result);
    string_delete(&result);
    return true;
}

bool string_replace_all_view(String *string, StringView needle, StringView replacement) {
    return string_replace_needle(string, needle, replacement, true);
}

bool string_replace_first_view(String *string, StringView needle, StringView replacement) {
    return string_replace_needle(string, needle, replacement, false);
}

bool string_replace_all_cstr(String *string, str8 needle, str8 replacement) {
    if (!needle || !replacement)
        return false;
    return string_replace_all_view(string, string_view_data(needle, u8_strlen(needle)),
                                   string_view_data(replacement, u8_strlen(replacement)));
}

bool string_replace_first_cstr(String *string, str8 needle, str8 replacement) {
    if (!needle || !replacement)
        return false;
    return string_replace_first_view(string, string_view_data(needle, u8_strlen(needle)),
                                     string_view_data(replacement, u8_strlen(replacement)));
}

static bool ustring_replace_needle(UString *string, UStringView needle, UStringView replacement,
                                   bool all) {
    if (!string || !string->data || needle.length == 0 || !needle.data)
        return false;
    if (replacement.length && !replacement.data)
        return false;

    if (!all) {
        usize pos = ustring_find(string, needle);
        if (pos == STRING_NOT_FOUND)
            return false;
        return ustring_replace_view(string, pos, needle.length, replacement);
    }

    UString result = ustring_new();
    usize pos = 0;
    bool any = false;

    while (pos <= string->length) {
        UStringView rest = {string->length - pos, string->data + pos};
        usize found = ustring_view_find(rest, needle);
        if (found == STRING_NOT_FOUND) {
            if (!ustring_append_utf16(&result, string->data + pos, string->length - pos)) {
                ustring_delete(&result);
                return false;
            }
            break;
        }
        any = true;
        if (!ustring_append_utf16(&result, string->data + pos, found)) {
            ustring_delete(&result);
            return false;
        }
        if (!ustring_append_view(&result, replacement)) {
            ustring_delete(&result);
            return false;
        }
        pos += found + needle.length;
        if (pos == string->length)
            break;
    }

    if (!any) {
        ustring_delete(&result);
        return false;
    }

    ustring_swap(string, &result);
    ustring_delete(&result);
    return true;
}

bool ustring_replace_all_view(UString *string, UStringView needle, UStringView replacement) {
    return ustring_replace_needle(string, needle, replacement, true);
}

bool ustring_replace_first_view(UString *string, UStringView needle, UStringView replacement) {
    return ustring_replace_needle(string, needle, replacement, false);
}
