/*
 * coreJSON v3.2.0
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file core_json_utest.c
 * @brief Unit tests for the coreJSON library.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "unity.h"
#include "catch_assert.h"

/* Include paths for public enums, structures, and macros. */
#include "core_json.h"
#include "core_json_annex.h"


/* Sample test from the docs. */
#define JSON_QUERY_SEPARATOR                "."

#define FIRST_QUERY_KEY                     "bar"
#define FIRST_QUERY_KEY_LENGTH              ( sizeof( FIRST_QUERY_KEY ) - 1 )

#define SECOND_QUERY_KEY                    "foo"
#define SECOND_QUERY_KEY_LENGTH             ( sizeof( SECOND_QUERY_KEY ) - 1 )

#define COMPLETE_QUERY_KEY \
    FIRST_QUERY_KEY        \
    JSON_QUERY_SEPARATOR   \
    SECOND_QUERY_KEY
#define COMPLETE_QUERY_KEY_LENGTH           ( sizeof( COMPLETE_QUERY_KEY ) - 1 )

#define COMPLETE_QUERY_KEY_ANSWER           "xyz"
#define COMPLETE_QUERY_KEY_ANSWER_TYPE      JSONString
#define COMPLETE_QUERY_KEY_ANSWER_LENGTH    ( sizeof( COMPLETE_QUERY_KEY_ANSWER ) - 1 )

#define FIRST_QUERY_KEY_ANSWER     \
    "{\"" SECOND_QUERY_KEY "\":\"" \
    COMPLETE_QUERY_KEY_ANSWER "\"}"
#define FIRST_QUERY_KEY_ANSWER_TYPE         JSONObject
#define FIRST_QUERY_KEY_ANSWER_LENGTH       ( sizeof( FIRST_QUERY_KEY_ANSWER ) - 1 )

#define ARRAY_ELEMENT_0                     "123"
#define ARRAY_ELEMENT_1                     "456"
#define ARRAY_ELEMENT_2_SUB_0               "abc"
#define ARRAY_ELEMENT_2_SUB_1               "[88,99]"
#define ARRAY_ELEMENT_2_SUB_1_SUB_0         "88"
#define ARRAY_ELEMENT_2_SUB_1_SUB_1         "99"
#define ARRAY_ELEMENT_3                     "true"
#define ARRAY_ELEMENT_4                     "false"
#define ARRAY_ELEMENT_5                     "null"
#define JSON_NESTED_OBJECT                                      \
    "{\"" FIRST_QUERY_KEY "\":\"" ARRAY_ELEMENT_2_SUB_0 "\",\"" \
    SECOND_QUERY_KEY "\":" ARRAY_ELEMENT_2_SUB_1 "}"
#define JSON_NESTED_OBJECT_LENGTH           ( sizeof( JSON_NESTED_OBJECT ) - 1 )
#define ARRAY_ELEMENT_2                     JSON_NESTED_OBJECT
#define JSON_DOC_LEGAL_ARRAY                                        \
    "[" ARRAY_ELEMENT_0 "," ARRAY_ELEMENT_1 "," ARRAY_ELEMENT_2 "," \
    ARRAY_ELEMENT_3 "," ARRAY_ELEMENT_4 "," ARRAY_ELEMENT_5 "]"
#define JSON_DOC_LEGAL_ARRAY_LENGTH         ( sizeof( JSON_DOC_LEGAL_ARRAY ) - 1 )

#define ARRAY_ELEMENT_0_TYPE                JSONNumber
#define ARRAY_ELEMENT_1_TYPE                JSONNumber
#define ARRAY_ELEMENT_2_TYPE                JSONObject
#define ARRAY_ELEMENT_2_SUB_0_TYPE          JSONString
#define ARRAY_ELEMENT_2_SUB_1_TYPE          JSONArray
#define ARRAY_ELEMENT_2_SUB_1_SUB_0_TYPE    JSONNumber
#define ARRAY_ELEMENT_2_SUB_1_SUB_1_TYPE    JSONNumber
#define ARRAY_ELEMENT_3_TYPE                JSONTrue
#define ARRAY_ELEMENT_4_TYPE                JSONFalse
#define ARRAY_ELEMENT_5_TYPE                JSONNull

/* This JSON document covers all cases where scalars are exponents, literals, numbers, and decimals. */
#define JSON_DOC_VARIED_SCALARS                                                      \
    "{\"literal\":true, \"more_literals\": {\"literal2\":false, \"literal3\":null}," \
    "\"exp1\": 5E+3, \"more_exponents\": [5e+2,\t4e-2,\r93E-5, 128E-6],\n "          \
    "\"number\": -123412, "                                                          \
    "\"decimal\":109238.42091289, "                                                  \
    "\"foo\":\"abc\",\"" FIRST_QUERY_KEY "\":" FIRST_QUERY_KEY_ANSWER "}"
#define JSON_DOC_VARIED_SCALARS_LENGTH                     ( sizeof( JSON_DOC_VARIED_SCALARS ) - 1 )

#define MULTIPLE_VALID_ESCAPES                             "\\\\ \\\" \\/ \\b \\f \\n \\r \\t \\\x12"
#define MULTIPLE_VALID_ESCAPES_LENGTH                      ( sizeof( MULTIPLE_VALID_ESCAPES ) - 1 )

#define JSON_DOC_QUERY_KEY_NOT_FOUND                       "{\"hello\": \"world\"}"
#define JSON_DOC_QUERY_KEY_NOT_FOUND_LENGTH                ( sizeof( JSON_DOC_QUERY_KEY_NOT_FOUND ) - 1 )

#define JSON_DOC_MULTIPLE_VALID_ESCAPES   \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" :\t\"" MULTIPLE_VALID_ESCAPES "\"}}"
#define JSON_DOC_MULTIPLE_VALID_ESCAPES_LENGTH             ( sizeof( JSON_DOC_MULTIPLE_VALID_ESCAPES ) - 1 )

/* A single byte in UTF-8 is just an ASCII character, so it's not included here. */
#define LEGAL_UTF8_BYTE_SEQUENCES                          "\xc2\xa9 \xe2\x98\x95 \xf0\x9f\x98\x80"
#define LEGAL_UTF8_BYTE_SEQUENCES_LENGTH                   ( sizeof( LEGAL_UTF8_BYTE_SEQUENCES ) - 1 )

#define JSON_DOC_LEGAL_UTF8_BYTE_SEQUENCES \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY  \
    "\":{\"" SECOND_QUERY_KEY "\" : \"" LEGAL_UTF8_BYTE_SEQUENCES "\"}}"
#define JSON_DOC_LEGAL_UTF8_BYTE_SEQUENCES_LENGTH          ( sizeof( JSON_DOC_LEGAL_UTF8_BYTE_SEQUENCES ) - 1 )

/* Unicode escape sequences in the Basic Multilingual Plane. */
#define UNICODE_ESCAPE_SEQUENCES_BMP                       "\\uCB00\\uEFFF"
#define UNICODE_ESCAPE_SEQUENCES_BMP_LENGTH                ( sizeof( UNICODE_ESCAPE_SEQUENCES_BMP ) - 1 )

#define JSON_DOC_UNICODE_ESCAPE_SEQUENCES_BMP \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY     \
    "\":{\"" SECOND_QUERY_KEY "\" : \"" UNICODE_ESCAPE_SEQUENCES_BMP "\"}}"
#define JSON_DOC_UNICODE_ESCAPE_SEQUENCES_BMP_LENGTH       ( sizeof( JSON_DOC_UNICODE_ESCAPE_SEQUENCES_BMP ) - 1 )

/* Unicode escape sequences using surrogates for Astral Code Points (outside BMP). */
#define LEGAL_UNICODE_ESCAPE_SURROGATES                    "\\uD83D\\ude07"
#define LEGAL_UNICODE_ESCAPE_SURROGATES_LENGTH             ( sizeof( LEGAL_UNICODE_ESCAPE_SURROGATES ) - 1 )

#define JSON_DOC_LEGAL_UNICODE_ESCAPE_SURROGATES \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY        \
    "\":{\"" SECOND_QUERY_KEY "\" : \"" LEGAL_UNICODE_ESCAPE_SURROGATES "\"}}"
#define JSON_DOC_LEGAL_UNICODE_ESCAPE_SURROGATES_LENGTH    ( sizeof( JSON_DOC_LEGAL_UNICODE_ESCAPE_SURROGATES ) - 1 )

#define JSON_DOC_LEGAL_TRAILING_SPACE     \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : \"" COMPLETE_QUERY_KEY_ANSWER "\"}}  "
#define JSON_DOC_LEGAL_TRAILING_SPACE_LENGTH               ( sizeof( JSON_DOC_LEGAL_TRAILING_SPACE ) - 1 )

/* A single scalar is still considered a valid JSON document. */
#define SINGLE_SCALAR                                      "\"l33t\""
#define SINGLE_SCALAR_LENGTH                               ( sizeof( SINGLE_SCALAR ) - 1 )

/* Illegal scalar entry in the array. */
#define ILLEGAL_SCALAR_IN_ARRAY                            "{\"hello\": [42, world]\""
#define ILLEGAL_SCALAR_IN_ARRAY_LENGTH                     ( sizeof( ILLEGAL_SCALAR_IN_ARRAY ) - 1 )

#define ILLEGAL_SCALAR_IN_ARRAY2                           "[42, world]"
#define ILLEGAL_SCALAR_IN_ARRAY2_LENGTH                    ( sizeof( ILLEGAL_SCALAR_IN_ARRAY2 ) - 1 )

#define TRAILING_COMMA_AFTER_VALUE        \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : \"" COMPLETE_QUERY_KEY_ANSWER "\",}}"
#define TRAILING_COMMA_AFTER_VALUE_LENGTH                  ( sizeof( TRAILING_COMMA_AFTER_VALUE ) - 1 )

#define MISSING_COMMA_AFTER_VALUE                          "{\"foo\":{}\"bar\":\"abc\"}"
#define MISSING_COMMA_AFTER_VALUE_LENGTH                   ( sizeof( MISSING_COMMA_AFTER_VALUE ) - 1 )

#define MISSING_VALUE_AFTER_KEY                            "{\"foo\":{\"bar\":}}"
#define MISSING_VALUE_AFTER_KEY_LENGTH                     ( sizeof( MISSING_VALUE_AFTER_KEY ) - 1 )

#define MISMATCHED_BRACKETS                                "{\"foo\":{\"bar\":\"xyz\"]}"
#define MISMATCHED_BRACKETS_LENGTH                         ( sizeof( MISMATCHED_BRACKETS ) - 1 )

#define MISMATCHED_BRACKETS2                               "{\"foo\":[\"bar\",\"xyz\"}}"
#define MISMATCHED_BRACKETS2_LENGTH                        ( sizeof( MISMATCHED_BRACKETS2 ) - 1 )

#define MISMATCHED_BRACKETS3                               "{\"foo\":[\"bar\",\"xyz\"]]"
#define MISMATCHED_BRACKETS3_LENGTH                        ( sizeof( MISMATCHED_BRACKETS3 ) - 1 )

#define MISMATCHED_BRACKETS4                               "[\"foo\",\"bar\",\"xyz\"}"
#define MISMATCHED_BRACKETS4_LENGTH                        ( sizeof( MISMATCHED_BRACKETS4 ) - 1 )

#define INCORRECT_OBJECT_SEPARATOR                         "{\"foo\": \"bar\"; \"bar\": \"foo\"}"
#define INCORRECT_OBJECT_SEPARATOR_LENGTH                  ( sizeof( INCORRECT_OBJECT_SEPARATOR ) - 1 )

#define MISSING_ENCLOSING_ARRAY_MARKER    \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : []]}}"
#define MISSING_ENCLOSING_ARRAY_MARKER_LENGTH              ( sizeof( MISSING_ENCLOSING_ARRAY_MARKER ) - 1 )

#define MISSING_ENCLOSING_OBJECT_MARKER   \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : \"" COMPLETE_QUERY_KEY_ANSWER "\"}"
#define MISSING_ENCLOSING_OBJECT_MARKER_LENGTH             ( sizeof( MISSING_ENCLOSING_OBJECT_MARKER ) - 1 )

#define CUT_AFTER_OBJECT_OPEN_BRACE                        "{\"foo\":\"abc\",\"bar\":{"
#define CUT_AFTER_OBJECT_OPEN_BRACE_LENGTH                 ( sizeof( CUT_AFTER_OBJECT_OPEN_BRACE ) - 1 )

#define LEADING_ZEROS_IN_NUMBER           \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : 07}}"
#define LEADING_ZEROS_IN_NUMBER_LENGTH                     ( sizeof( LEADING_ZEROS_IN_NUMBER ) - 1 )

#define TRAILING_COMMA_IN_ARRAY                            "[{\"hello\": [\"foo\",]}]"
#define TRAILING_COMMA_IN_ARRAY_LENGTH                     ( sizeof( TRAILING_COMMA_IN_ARRAY ) - 1 )

#define CUT_AFTER_COMMA_SEPARATOR                          "{\"hello\": [5,"
#define CUT_AFTER_COMMA_SEPARATOR_LENGTH                   ( sizeof( CUT_AFTER_COMMA_SEPARATOR ) - 1 )

#define CLOSING_SQUARE_BRACKET                             "]"
#define CLOSING_SQUARE_BRACKET_LENGTH                      ( sizeof( CLOSING_SQUARE_BRACKET ) - 1 )

#define CLOSING_CURLY_BRACKET                              "}"
#define CLOSING_CURLY_BRACKET_LENGTH                       ( sizeof( CLOSING_CURLY_BRACKET ) - 1 )

#define OPENING_CURLY_BRACKET                              "{"
#define OPENING_CURLY_BRACKET_LENGTH                       ( sizeof( OPENING_CURLY_BRACKET ) - 1 )

#define QUERY_KEY_TRAILING_SEPARATOR                       FIRST_QUERY_KEY JSON_QUERY_SEPARATOR
#define QUERY_KEY_TRAILING_SEPARATOR_LENGTH                ( sizeof( QUERY_KEY_TRAILING_SEPARATOR ) - 1 )

#define QUERY_KEY_EMPTY                                    JSON_QUERY_SEPARATOR SECOND_QUERY_KEY
#define QUERY_KEY_EMPTY_LENGTH                             ( sizeof( QUERY_KEY_EMPTY ) - 1 )

/* Separator between a key and a value must be a colon (:). */
#define WRONG_KEY_VALUE_SEPARATOR         \
    "{\"foo\";\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\":\"" COMPLETE_QUERY_KEY_ANSWER "\"}}  "
#define WRONG_KEY_VALUE_SEPARATOR_LENGTH    ( sizeof( WRONG_KEY_VALUE_SEPARATOR ) - 1 )

/* Key must be a string. */
#define ILLEGAL_KEY_NOT_STRING        \
    "{foo:\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : \"" COMPLETE_QUERY_KEY_ANSWER "\"}}"
#define ILLEGAL_KEY_NOT_STRING_LENGTH    ( sizeof( ILLEGAL_KEY_NOT_STRING ) - 1 )

/* A non-number after the exponent marker is illegal. */
#define LETTER_AS_EXPONENT                \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : 5Ea}}"
#define LETTER_AS_EXPONENT_LENGTH    ( sizeof( LETTER_AS_EXPONENT ) - 1 )

/* The octet values C0, C1, and F5 to FF are illegal, since C0 and C1
 * would introduce a non-shortest sequence, and F5 or above would
 * introduce a value greater than the last code point, 0x10FFFF. */
#define ILLEGAL_UTF8_NEXT_BYTE            \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : \"\xc2\x00\"}}"
#define ILLEGAL_UTF8_NEXT_BYTE_LENGTH    ( sizeof( ILLEGAL_UTF8_NEXT_BYTE ) - 1 )

/* The first byte in a UTF-8 sequence must be greater than C1. */
#define ILLEGAL_UTF8_START_C1             \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : \"\xC1\"}}"
#define ILLEGAL_UTF8_START_C1_LENGTH    ( sizeof( ILLEGAL_UTF8_START_C1 ) - 1 )

/* The first byte in a UTF-8 sequence must be less than F5. */
#define ILLEGAL_UTF8_START_F5             \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : \"\xF5\"}}"
#define ILLEGAL_UTF8_START_F5_LENGTH    ( sizeof( ILLEGAL_UTF8_START_F5 ) - 1 )

/* Additional bytes must match 10xxxxxx, so this case is illegal UTF8. */
#define ILLEGAL_UTF8_NEXT_BYTES           \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : \"\xc2\xC0\"}}"
#define ILLEGAL_UTF8_NEXT_BYTES_LENGTH               ( sizeof( ILLEGAL_UTF8_NEXT_BYTES ) - 1 )

#define ILLEGAL_UTF8_SURROGATE_RANGE_MIN  \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : \"\xED\xA0\x80\"}}"
#define ILLEGAL_UTF8_SURROGATE_RANGE_MIN_LENGTH      ( sizeof( ILLEGAL_UTF8_SURROGATE_RANGE_MIN ) - 1 )

#define ILLEGAL_UTF8_SURROGATE_RANGE_MAX  \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : \"\xED\xBF\xBF\"}}"
#define ILLEGAL_UTF8_SURROGATE_RANGE_MAX_LENGTH      ( sizeof( ILLEGAL_UTF8_SURROGATE_RANGE_MAX ) - 1 )

#define ILLEGAL_UTF8_GT_MIN_CP_THREE_BYTES \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY  \
    "\":{\"" SECOND_QUERY_KEY "\" : \"\xC2\x80\x80\"}}"
#define ILLEGAL_UTF8_GT_MIN_CP_THREE_BYTES_LENGTH    ( sizeof( ILLEGAL_UTF8_GT_MIN_CP_THREE_BYTES ) - 1 )

#define ILLEGAL_UTF8_GT_MIN_CP_FOUR_BYTES \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : \"\xF4\x9F\xBF\xBF\"}}"
#define ILLEGAL_UTF8_GT_MIN_CP_FOUR_BYTES_LENGTH     ( sizeof( ILLEGAL_UTF8_GT_MIN_CP_FOUR_BYTES ) - 1 )

#define ILLEGAL_UTF8_LT_MAX_CP_FOUR_BYTES \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : \"\xF0\x80\x80\x80\"}}"
#define ILLEGAL_UTF8_LT_MAX_CP_FOUR_BYTES_LENGTH     ( sizeof( ILLEGAL_UTF8_LT_MAX_CP_FOUR_BYTES ) - 1 )

/* The following escapes are considered ILLEGAL. */

/* Hex characters must be used for the unicode escape sequence to be valid. */
#define ILLEGAL_UNICODE_LITERAL_HEX       \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : \"\\u\xD8\x3D\\u\xde\x07\"}}"
#define ILLEGAL_UNICODE_LITERAL_HEX_LENGTH                  ( sizeof( ILLEGAL_UNICODE_LITERAL_HEX ) - 1 )

#define UNICODE_PREMATURE_LOW_SURROGATE   \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : \"\\ude07\\uD83D\"}}"
#define UNICODE_PREMATURE_LOW_SURROGATE_LENGTH              ( sizeof( UNICODE_PREMATURE_LOW_SURROGATE ) - 1 )

#define UNICODE_INVALID_LOWERCASE_HEX     \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : \"\\uge07\\uD83D\"}}"
#define UNICODE_INVALID_LOWERCASE_HEX_LENGTH                ( sizeof( UNICODE_INVALID_LOWERCASE_HEX ) - 1 )

#define UNICODE_INVALID_UPPERCASE_HEX     \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : \"\\ude07\\uG83D\"}}"
#define UNICODE_INVALID_UPPERCASE_HEX_LENGTH                ( sizeof( UNICODE_INVALID_UPPERCASE_HEX ) - 1 )

#define UNICODE_NON_LETTER_OR_DIGIT_HEX   \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : \"\\u\0e07\\uG83D\"}}"
#define UNICODE_NON_LETTER_OR_DIGIT_HEX_LENGTH              ( sizeof( UNICODE_NON_LETTER_OR_DIGIT_HEX ) - 1 )

#define UNICODE_VALID_HIGH_NO_LOW_SURROGATE \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY   \
    "\":{\"" SECOND_QUERY_KEY "\" : \"\\uD83D. Hello there!\"}}"
#define UNICODE_VALID_HIGH_NO_LOW_SURROGATE_LENGTH          ( sizeof( UNICODE_VALID_HIGH_NO_LOW_SURROGATE ) - 1 )

#define UNICODE_WRONG_ESCAPE_AFTER_HIGH_SURROGATE \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY         \
    "\":{\"" SECOND_QUERY_KEY "\" : \"\\uD83D\\Ude07\"}}"
#define UNICODE_WRONG_ESCAPE_AFTER_HIGH_SURROGATE_LENGTH    ( sizeof( UNICODE_WRONG_ESCAPE_AFTER_HIGH_SURROGATE ) - 1 )

#define UNICODE_VALID_HIGH_INVALID_LOW_SURROGATE \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY        \
    "\":{\"" SECOND_QUERY_KEY "\" : \"\\uD83D\\uEFFF\"}}"
#define UNICODE_VALID_HIGH_INVALID_LOW_SURROGATE_LENGTH     ( sizeof( UNICODE_VALID_HIGH_INVALID_LOW_SURROGATE ) - 1 )

#define UNICODE_BOTH_SURROGATES_HIGH      \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : \"\\uD83D\\uD83D\"}}"
#define UNICODE_BOTH_SURROGATES_HIGH_LENGTH                 ( sizeof( UNICODE_BOTH_SURROGATES_HIGH ) - 1 )

/* For security, \u0000 is disallowed. */
#define UNICODE_ESCAPE_SEQUENCE_ZERO_CP   \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : \"\\u0000\"}}"
#define UNICODE_ESCAPE_SEQUENCE_ZERO_CP_LENGTH    ( sizeof( UNICODE_ESCAPE_SEQUENCE_ZERO_CP ) - 1 )

/* /NUL escape is disallowed. */
#define NUL_ESCAPE                        \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : \"\\\x0\"}}"
#define NUL_ESCAPE_LENGTH           ( sizeof( NUL_ESCAPE ) - 1 )

#define ESCAPE_CHAR_ALONE                 \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : \"\\\"}}"
#define ESCAPE_CHAR_ALONE_LENGTH    ( sizeof( ESCAPE_CHAR_ALONE ) - 1 )

/* Valid control characters are those in the range of (NUL,SPACE).
 * Therefore, both cases below are invalid. */
#define SPACE_CONTROL_CHAR                \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : \"\\ \"}}"
#define SPACE_CONTROL_CHAR_LENGTH    ( sizeof( SPACE_CONTROL_CHAR ) - 1 )

/* \x80 implies a single one in the MSB, leading to a negative value. */
#define LT_ZERO_CONTROL_CHAR              \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : \"\\\x80 \"}}"
#define LT_ZERO_CONTROL_CHAR_LENGTH    ( sizeof( LT_ZERO_CONTROL_CHAR ) - 1 )

/* An unescaped control character is considered ILLEGAL. */
#define UNESCAPED_CONTROL_CHAR            \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : \"\x15\"}}"
#define UNESCAPED_CONTROL_CHAR_LENGTH    ( sizeof( UNESCAPED_CONTROL_CHAR ) - 1 )

/* Each skip function has a check that the iterator i has not exceeded the
 * length of the buffer. The cases below test that those checks work as intended. */

/* Triggers the case in which i >= max for search. */
#define PADDED_OPENING_CURLY_BRACKET           "  {  "
#define PADDED_OPENING_CURLY_BRACKET_LENGTH    ( sizeof( PADDED_OPENING_CURLY_BRACKET ) - 1 )

/* Triggers the case in which i >= max for skipUTF8MultiByte.
 * UTF-8 is illegal if the number of bytes in the sequence is
 * less than what was expected from the first byte. */
#define CUT_AFTER_UTF8_FIRST_BYTE         \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : \"\xC2"
#define CUT_AFTER_UTF8_FIRST_BYTE_LENGTH    ( sizeof( CUT_AFTER_UTF8_FIRST_BYTE ) - 1 )

/* Triggers the case in which end >= max for skipHexEscape. */
#define UNICODE_STRING_END_AFTER_HIGH_SURROGATE \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY       \
    "\":{\"" SECOND_QUERY_KEY "\" : \"\\uD83D\"}}"
#define UNICODE_STRING_END_AFTER_HIGH_SURROGATE_LENGTH    ( sizeof( UNICODE_STRING_END_AFTER_HIGH_SURROGATE ) - 1 )

/* Triggers the case in which i >= max for skipDigits. */
#define CUT_AFTER_NUMBER                  \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : 1"
#define CUT_AFTER_NUMBER_LENGTH    ( sizeof( CUT_AFTER_NUMBER ) - 1 )

/* Triggers the case in which i >= max for skipDecimals. */
#define CUT_AFTER_DECIMAL_POINT           \
    "{\"foo\":\"abc\",\"" FIRST_QUERY_KEY \
    "\":{\"" SECOND_QUERY_KEY "\" : 1."
#define CUT_AFTER_DECIMAL_POINT_LENGTH           ( sizeof( CUT_AFTER_DECIMAL_POINT ) - 1 )

/* Triggers the case in which ( i + 1U ) >= max for skipEscape. */
#define ESCAPE_CHAR_ALONE_NOT_ENCLOSED           "\"\\"
#define ESCAPE_CHAR_ALONE_NOT_ENCLOSED_LENGTH    ( sizeof( ESCAPE_CHAR_ALONE_NOT_ENCLOSED ) - 1 )

/* Triggers the case in which i >= max for skipExponent. */
#define CUT_AFTER_EXPONENT_MARKER                "4e"
#define CUT_AFTER_EXPONENT_MARKER_LENGTH         ( sizeof( CUT_AFTER_EXPONENT_MARKER ) - 1 )

/* Triggers the case in which i >= max for skipString. */
#define WHITE_SPACE                              "    "
#define WHITE_SPACE_LENGTH                       ( sizeof( WHITE_SPACE ) - 1 )

/* Triggers the case in which i >= max for skipArrayScalars. */
#define CUT_AFTER_ARRAY_START_MARKER             "{\"hello\": ["
#define CUT_AFTER_ARRAY_START_MARKER_LENGTH      ( sizeof( CUT_AFTER_ARRAY_START_MARKER ) - 1 )

/* Triggers the cases in which i >= max for skipObjectScalars and nextKeyValuePair. */
#define CUT_AFTER_OBJECT_START_MARKER            "{\"hello\": {"
#define CUT_AFTER_OBJECT_START_MARKER_LENGTH     ( sizeof( CUT_AFTER_OBJECT_START_MARKER ) - 1 )
#define CUT_AFTER_KEY                            "{\"hello\""
#define CUT_AFTER_KEY_LENGTH                     ( sizeof( CUT_AFTER_KEY ) - 1 )

/* This prefix is used to generate multiple levels of nested objects. */
#define NESTED_OBJECT_PREFIX                     "{\"k\":"
#define NESTED_OBJECT_PREFIX_LENGTH              ( sizeof( NESTED_OBJECT_PREFIX ) - 1 )

/* The value of the nested object with the largest depth. */
#define NESTED_OBJECT_VALUE                      "\"v\""
#define NESTED_OBJECT_VALUE_LENGTH               ( sizeof( NESTED_OBJECT_VALUE ) - 1 )

#ifndef JSON_MAX_DEPTH
    #define JSON_MAX_DEPTH                       32
#endif


/* ============================   UNITY FIXTURES ============================ */

/* Called before each test method. */
void setUp()
{
}

/* Called after each test method. */
void tearDown()
{
}

/* Called at the beginning of the whole suite. */
void suiteSetUp()
{
}

/* Called at the end of the whole suite. */
int suiteTearDown( int numFailures )
{
    return numFailures;
}

/* ========================================================================== */

/**
 * @brief Create a nested JSON array that exceeds JSON_MAX_DEPTH.
 */
char * allocateMaxDepthArray( void )
{
    size_t i, len = ( JSON_MAX_DEPTH + 1 ) * 2;
    char * nestedArray;

    nestedArray = malloc( sizeof( char ) * ( len + 1 ) );

    for( i = 0; i < len / 2; i++ )
    {
        nestedArray[ i ] = '[';
        nestedArray[ len - 1 - i ] = ']';
    }

    nestedArray[ len ] = '\0';

    return nestedArray;
}

/**
 * @brief Create a nested JSON object that exceeds JSON_MAX_DEPTH.
 */
char * allocateMaxDepthObject( void )
{
    size_t i = 0, len = NESTED_OBJECT_VALUE_LENGTH +
                        ( JSON_MAX_DEPTH + 1 ) * ( NESTED_OBJECT_PREFIX_LENGTH +
                                                   CLOSING_CURLY_BRACKET_LENGTH );
    char * nestedObject, * nestedObjectCur;

    nestedObject = malloc( sizeof( char ) * ( len + 1 ) );
    nestedObjectCur = nestedObject;

    while( i < ( JSON_MAX_DEPTH + 1 ) * NESTED_OBJECT_PREFIX_LENGTH )
    {
        memcpy( nestedObjectCur, NESTED_OBJECT_PREFIX, NESTED_OBJECT_PREFIX_LENGTH );
        i += NESTED_OBJECT_PREFIX_LENGTH;
        nestedObjectCur += NESTED_OBJECT_PREFIX_LENGTH;
    }

    memcpy( nestedObjectCur, NESTED_OBJECT_VALUE, NESTED_OBJECT_VALUE_LENGTH );
    i += NESTED_OBJECT_VALUE_LENGTH;
    nestedObjectCur += NESTED_OBJECT_VALUE_LENGTH;

    /* This loop writes the correct number of closing brackets so long as there
     * are JSON_MAX_DEPTH + 1 entries for each bracket accounted for in len. */
    while( i < len )
    {
        *( nestedObjectCur++ ) = '}';
        i++;
    }

    nestedObject[ len ] = '\0';

    return nestedObject;
}

/**
 * @brief Test that JSON_Validate is able to classify any null or bad parameters.
 */
void test_JSON_Validate_Invalid_Params( void )
{
    JSONStatus_t jsonStatus;

    jsonStatus = JSON_Validate( NULL, 0 );
    TEST_ASSERT_EQUAL( JSONNullParameter, jsonStatus );

    jsonStatus = JSON_Validate( JSON_DOC_LEGAL_TRAILING_SPACE,
                                0 );
    TEST_ASSERT_EQUAL( JSONBadParameter, jsonStatus );
}

/**
 * @brief Test that JSON_Validate is able to classify valid JSON correctly.
 */
void test_JSON_Validate_Legal_Documents( void )
{
    JSONStatus_t jsonStatus;

    jsonStatus = JSON_Validate( JSON_DOC_VARIED_SCALARS, JSON_DOC_VARIED_SCALARS_LENGTH );
    TEST_ASSERT_EQUAL( JSONSuccess, jsonStatus );

    jsonStatus = JSON_Validate( JSON_DOC_LEGAL_TRAILING_SPACE,
                                JSON_DOC_LEGAL_TRAILING_SPACE_LENGTH );
    TEST_ASSERT_EQUAL( JSONSuccess, jsonStatus );

    jsonStatus = JSON_Validate( JSON_DOC_MULTIPLE_VALID_ESCAPES,
                                JSON_DOC_MULTIPLE_VALID_ESCAPES_LENGTH );
    TEST_ASSERT_EQUAL( JSONSuccess, jsonStatus );

    jsonStatus = JSON_Validate( JSON_DOC_LEGAL_UTF8_BYTE_SEQUENCES,
                                JSON_DOC_LEGAL_UTF8_BYTE_SEQUENCES_LENGTH );
    TEST_ASSERT_EQUAL( JSONSuccess, jsonStatus );

    jsonStatus = JSON_Validate( JSON_DOC_LEGAL_UNICODE_ESCAPE_SURROGATES,
                                JSON_DOC_LEGAL_UNICODE_ESCAPE_SURROGATES_LENGTH );
    TEST_ASSERT_EQUAL( JSONSuccess, jsonStatus );

    jsonStatus = JSON_Validate( JSON_DOC_UNICODE_ESCAPE_SEQUENCES_BMP,
                                JSON_DOC_UNICODE_ESCAPE_SEQUENCES_BMP_LENGTH );
    TEST_ASSERT_EQUAL( JSONSuccess, jsonStatus );

    jsonStatus = JSON_Validate( JSON_DOC_LEGAL_ARRAY,
                                JSON_DOC_LEGAL_ARRAY_LENGTH );
    TEST_ASSERT_EQUAL( JSONSuccess, jsonStatus );
}

/**
 * @brief Test that JSON_Validate is able to classify an illegal JSON document correctly.
 */
void test_JSON_Validate_Illegal_Documents( void )
{
    JSONStatus_t jsonStatus;

    jsonStatus = JSON_Validate( INCORRECT_OBJECT_SEPARATOR,
                                INCORRECT_OBJECT_SEPARATOR_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( ILLEGAL_KEY_NOT_STRING,
                                ILLEGAL_KEY_NOT_STRING_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( WRONG_KEY_VALUE_SEPARATOR,
                                WRONG_KEY_VALUE_SEPARATOR_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( TRAILING_COMMA_IN_ARRAY,
                                TRAILING_COMMA_IN_ARRAY_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( CUT_AFTER_COMMA_SEPARATOR,
                                CUT_AFTER_COMMA_SEPARATOR_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( CUT_AFTER_KEY,
                                CUT_AFTER_KEY_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( TRAILING_COMMA_AFTER_VALUE,
                                TRAILING_COMMA_AFTER_VALUE_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( MISSING_COMMA_AFTER_VALUE,
                                MISSING_COMMA_AFTER_VALUE_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( MISSING_VALUE_AFTER_KEY,
                                MISSING_VALUE_AFTER_KEY_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( MISMATCHED_BRACKETS,
                                MISMATCHED_BRACKETS_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( MISMATCHED_BRACKETS2,
                                MISMATCHED_BRACKETS2_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( MISMATCHED_BRACKETS3,
                                MISMATCHED_BRACKETS3_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( MISMATCHED_BRACKETS4,
                                MISMATCHED_BRACKETS4_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( NUL_ESCAPE, NUL_ESCAPE_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( SPACE_CONTROL_CHAR, SPACE_CONTROL_CHAR_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( LT_ZERO_CONTROL_CHAR, LT_ZERO_CONTROL_CHAR_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( CLOSING_SQUARE_BRACKET,
                                CLOSING_SQUARE_BRACKET_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( CLOSING_CURLY_BRACKET,
                                CLOSING_CURLY_BRACKET_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( CUT_AFTER_EXPONENT_MARKER,
                                CUT_AFTER_EXPONENT_MARKER_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( MISSING_ENCLOSING_ARRAY_MARKER,
                                MISSING_ENCLOSING_ARRAY_MARKER_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( LETTER_AS_EXPONENT,
                                LETTER_AS_EXPONENT_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( CUT_AFTER_DECIMAL_POINT,
                                CUT_AFTER_DECIMAL_POINT_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( LEADING_ZEROS_IN_NUMBER,
                                LEADING_ZEROS_IN_NUMBER_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( ILLEGAL_SCALAR_IN_ARRAY,
                                ILLEGAL_SCALAR_IN_ARRAY_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( ESCAPE_CHAR_ALONE, ESCAPE_CHAR_ALONE_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( ESCAPE_CHAR_ALONE_NOT_ENCLOSED,
                                ESCAPE_CHAR_ALONE_NOT_ENCLOSED_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( UNESCAPED_CONTROL_CHAR,
                                UNESCAPED_CONTROL_CHAR_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( ILLEGAL_UTF8_NEXT_BYTE,
                                ILLEGAL_UTF8_NEXT_BYTE_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( ILLEGAL_UTF8_START_C1,
                                ILLEGAL_UTF8_START_C1_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( ILLEGAL_UTF8_START_F5,
                                ILLEGAL_UTF8_START_F5_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( CUT_AFTER_UTF8_FIRST_BYTE,
                                CUT_AFTER_UTF8_FIRST_BYTE_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( ILLEGAL_UTF8_NEXT_BYTES,
                                ILLEGAL_UTF8_NEXT_BYTES_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( ILLEGAL_UTF8_GT_MIN_CP_FOUR_BYTES,
                                ILLEGAL_UTF8_GT_MIN_CP_FOUR_BYTES_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( ILLEGAL_UTF8_GT_MIN_CP_THREE_BYTES,
                                ILLEGAL_UTF8_GT_MIN_CP_THREE_BYTES_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( ILLEGAL_UTF8_LT_MAX_CP_FOUR_BYTES,
                                ILLEGAL_UTF8_LT_MAX_CP_FOUR_BYTES_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( ILLEGAL_UTF8_SURROGATE_RANGE_MIN,
                                ILLEGAL_UTF8_SURROGATE_RANGE_MIN_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( ILLEGAL_UTF8_SURROGATE_RANGE_MAX,
                                ILLEGAL_UTF8_SURROGATE_RANGE_MAX_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( ILLEGAL_UTF8_SURROGATE_RANGE_MAX,
                                ILLEGAL_UTF8_SURROGATE_RANGE_MAX_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( ILLEGAL_UNICODE_LITERAL_HEX,
                                ILLEGAL_UNICODE_LITERAL_HEX_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( UNICODE_VALID_HIGH_NO_LOW_SURROGATE,
                                UNICODE_VALID_HIGH_NO_LOW_SURROGATE_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( UNICODE_WRONG_ESCAPE_AFTER_HIGH_SURROGATE,
                                UNICODE_WRONG_ESCAPE_AFTER_HIGH_SURROGATE_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( UNICODE_STRING_END_AFTER_HIGH_SURROGATE,
                                UNICODE_STRING_END_AFTER_HIGH_SURROGATE_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( UNICODE_PREMATURE_LOW_SURROGATE,
                                UNICODE_PREMATURE_LOW_SURROGATE_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( UNICODE_INVALID_LOWERCASE_HEX,
                                UNICODE_INVALID_LOWERCASE_HEX_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( UNICODE_INVALID_UPPERCASE_HEX,
                                UNICODE_INVALID_UPPERCASE_HEX_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( UNICODE_NON_LETTER_OR_DIGIT_HEX,
                                UNICODE_NON_LETTER_OR_DIGIT_HEX_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( UNICODE_BOTH_SURROGATES_HIGH,
                                UNICODE_BOTH_SURROGATES_HIGH_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( UNICODE_ESCAPE_SEQUENCE_ZERO_CP,
                                UNICODE_ESCAPE_SEQUENCE_ZERO_CP_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );

    jsonStatus = JSON_Validate( UNICODE_VALID_HIGH_INVALID_LOW_SURROGATE,
                                UNICODE_VALID_HIGH_INVALID_LOW_SURROGATE_LENGTH );
    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );
}

/**
 * @brief Test that JSON_Validate is able to classify a partial JSON document correctly.
 */
void test_JSON_Validate_Partial_Documents( void )
{
    JSONStatus_t jsonStatus;


    jsonStatus = JSON_Validate( OPENING_CURLY_BRACKET,
                                OPENING_CURLY_BRACKET_LENGTH );
    TEST_ASSERT_EQUAL( JSONPartial, jsonStatus );

    jsonStatus = JSON_Validate( WHITE_SPACE, WHITE_SPACE_LENGTH );
    TEST_ASSERT_EQUAL( JSONPartial, jsonStatus );

    jsonStatus = JSON_Validate( CUT_AFTER_OBJECT_OPEN_BRACE,
                                CUT_AFTER_OBJECT_OPEN_BRACE_LENGTH );
    TEST_ASSERT_EQUAL( JSONPartial, jsonStatus );

    jsonStatus = JSON_Validate( CUT_AFTER_NUMBER,
                                CUT_AFTER_NUMBER_LENGTH );
    TEST_ASSERT_EQUAL( JSONPartial, jsonStatus );

    jsonStatus = JSON_Validate( CUT_AFTER_ARRAY_START_MARKER,
                                CUT_AFTER_ARRAY_START_MARKER_LENGTH );
    TEST_ASSERT_EQUAL( JSONPartial, jsonStatus );

    jsonStatus = JSON_Validate( CUT_AFTER_OBJECT_START_MARKER,
                                CUT_AFTER_OBJECT_START_MARKER_LENGTH );
    TEST_ASSERT_EQUAL( JSONPartial, jsonStatus );
}

/**
 * @brief Test that JSON_Search can find the right value given a query key.
 */
void test_JSON_Search_Legal_Documents( void )
{
    JSONStatus_t jsonStatus;
    char * outValue;
    size_t outValueLength;
    JSONTypes_t outType;

    jsonStatus = JSON_SearchT( JSON_DOC_LEGAL_TRAILING_SPACE,
                               JSON_DOC_LEGAL_TRAILING_SPACE_LENGTH,
                               COMPLETE_QUERY_KEY,
                               COMPLETE_QUERY_KEY_LENGTH,
                               &outValue,
                               &outValueLength,
                               &outType );
    TEST_ASSERT_EQUAL( JSONSuccess, jsonStatus );
    TEST_ASSERT_EQUAL( COMPLETE_QUERY_KEY_ANSWER_TYPE, outType );
    TEST_ASSERT_EQUAL( outValueLength, COMPLETE_QUERY_KEY_ANSWER_LENGTH );
    TEST_ASSERT_EQUAL_STRING_LEN( COMPLETE_QUERY_KEY_ANSWER,
                                  outValue,
                                  outValueLength );

    jsonStatus = JSON_Search( JSON_DOC_LEGAL_TRAILING_SPACE,
                              JSON_DOC_LEGAL_TRAILING_SPACE_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONSuccess, jsonStatus );
    TEST_ASSERT_EQUAL( outValueLength, COMPLETE_QUERY_KEY_ANSWER_LENGTH );
    TEST_ASSERT_EQUAL_STRING_LEN( COMPLETE_QUERY_KEY_ANSWER,
                                  outValue,
                                  outValueLength );

    jsonStatus = JSON_Search( JSON_DOC_VARIED_SCALARS,
                              JSON_DOC_VARIED_SCALARS_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONSuccess, jsonStatus );
    TEST_ASSERT_EQUAL( COMPLETE_QUERY_KEY_ANSWER_LENGTH, outValueLength );
    TEST_ASSERT_EQUAL_STRING_LEN( COMPLETE_QUERY_KEY_ANSWER,
                                  outValue,
                                  COMPLETE_QUERY_KEY_ANSWER_LENGTH );

    jsonStatus = JSON_SearchT( JSON_DOC_VARIED_SCALARS,
                               JSON_DOC_VARIED_SCALARS_LENGTH,
                               FIRST_QUERY_KEY,
                               FIRST_QUERY_KEY_LENGTH,
                               &outValue,
                               &outValueLength,
                               &outType );
    TEST_ASSERT_EQUAL( JSONSuccess, jsonStatus );
    TEST_ASSERT_EQUAL( FIRST_QUERY_KEY_ANSWER_TYPE, outType );
    TEST_ASSERT_EQUAL( FIRST_QUERY_KEY_ANSWER_LENGTH, outValueLength );
    TEST_ASSERT_EQUAL_STRING_LEN( FIRST_QUERY_KEY_ANSWER,
                                  outValue,
                                  FIRST_QUERY_KEY_ANSWER_LENGTH );

    jsonStatus = JSON_Search( JSON_DOC_MULTIPLE_VALID_ESCAPES,
                              JSON_DOC_MULTIPLE_VALID_ESCAPES_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONSuccess, jsonStatus );
    TEST_ASSERT_EQUAL( MULTIPLE_VALID_ESCAPES_LENGTH, outValueLength );
    TEST_ASSERT_EQUAL_STRING_LEN( MULTIPLE_VALID_ESCAPES,
                                  outValue,
                                  MULTIPLE_VALID_ESCAPES_LENGTH );

    jsonStatus = JSON_Search( JSON_DOC_LEGAL_UTF8_BYTE_SEQUENCES,
                              JSON_DOC_LEGAL_UTF8_BYTE_SEQUENCES_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONSuccess, jsonStatus );
    TEST_ASSERT_EQUAL( LEGAL_UTF8_BYTE_SEQUENCES_LENGTH, outValueLength );
    TEST_ASSERT_EQUAL_STRING_LEN( LEGAL_UTF8_BYTE_SEQUENCES,
                                  outValue,
                                  LEGAL_UTF8_BYTE_SEQUENCES_LENGTH );

    jsonStatus = JSON_Search( JSON_DOC_LEGAL_UNICODE_ESCAPE_SURROGATES,
                              JSON_DOC_LEGAL_UNICODE_ESCAPE_SURROGATES_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONSuccess, jsonStatus );
    TEST_ASSERT_EQUAL( LEGAL_UNICODE_ESCAPE_SURROGATES_LENGTH, outValueLength );
    TEST_ASSERT_EQUAL_STRING_LEN( LEGAL_UNICODE_ESCAPE_SURROGATES,
                                  outValue,
                                  LEGAL_UNICODE_ESCAPE_SURROGATES_LENGTH );

    jsonStatus = JSON_Search( JSON_DOC_UNICODE_ESCAPE_SEQUENCES_BMP,
                              JSON_DOC_UNICODE_ESCAPE_SEQUENCES_BMP_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONSuccess, jsonStatus );
    TEST_ASSERT_EQUAL( UNICODE_ESCAPE_SEQUENCES_BMP_LENGTH, outValueLength );
    TEST_ASSERT_EQUAL_STRING_LEN( UNICODE_ESCAPE_SEQUENCES_BMP,
                                  outValue,
                                  UNICODE_ESCAPE_SEQUENCES_BMP_LENGTH );
}

/**
 * @brief Test that JSON_Search can find the right value given a query key using arrays.
 */
void test_JSON_Search_Legal_Array_Documents( void )
{
    JSONStatus_t jsonStatus;
    char * outValue;
    size_t outValueLength;
    JSONTypes_t outType;

#define doSearch( query, type, answer )                            \
    jsonStatus = JSON_SearchT( JSON_DOC_LEGAL_ARRAY,               \
                               JSON_DOC_LEGAL_ARRAY_LENGTH,        \
                               ( query ),                          \
                               ( sizeof( query ) - 1 ),            \
                               &outValue,                          \
                               &outValueLength,                    \
                               &outType );                         \
    TEST_ASSERT_EQUAL( JSONSuccess, jsonStatus );                  \
    TEST_ASSERT_EQUAL( type, outType );                            \
    TEST_ASSERT_EQUAL( outValueLength, ( sizeof( answer ) - 1 ) ); \
    TEST_ASSERT_EQUAL_STRING_LEN( ( answer ),                      \
                                  outValue,                        \
                                  outValueLength );

    doSearch( "[0]", ARRAY_ELEMENT_0_TYPE, ARRAY_ELEMENT_0 );
    doSearch( "[1]", ARRAY_ELEMENT_1_TYPE, ARRAY_ELEMENT_1 );
    doSearch( "[2]." FIRST_QUERY_KEY, ARRAY_ELEMENT_2_SUB_0_TYPE, ARRAY_ELEMENT_2_SUB_0 );
    doSearch( "[2]." SECOND_QUERY_KEY, ARRAY_ELEMENT_2_SUB_1_TYPE, ARRAY_ELEMENT_2_SUB_1 );
    doSearch( "[2]." SECOND_QUERY_KEY "[0]", ARRAY_ELEMENT_2_SUB_1_SUB_0_TYPE, ARRAY_ELEMENT_2_SUB_1_SUB_0 );
    doSearch( "[2]." SECOND_QUERY_KEY "[1]", ARRAY_ELEMENT_2_SUB_1_SUB_1_TYPE, ARRAY_ELEMENT_2_SUB_1_SUB_1 );
    doSearch( "[3]", ARRAY_ELEMENT_3_TYPE, ARRAY_ELEMENT_3 );
    doSearch( "[4]", ARRAY_ELEMENT_4_TYPE, ARRAY_ELEMENT_4 );
    doSearch( "[5]", ARRAY_ELEMENT_5_TYPE, ARRAY_ELEMENT_5 );
}

/**
 * @brief Test that JSON_Iterate returns the given values from a JSON array.
 */
void test_JSON_Iterate_Legal_Array_Documents( void )
{
    JSONStatus_t jsonStatus;
    size_t start = 0, next = 0;
    JSONPair_t pair = { 0 };

#define iterateArray( type, answer )                                 \
    jsonStatus = JSON_Iterate( JSON_DOC_LEGAL_ARRAY,                 \
                               JSON_DOC_LEGAL_ARRAY_LENGTH,          \
                               &start,                               \
                               &next,                                \
                               &pair );                              \
    TEST_ASSERT_EQUAL( JSONSuccess, jsonStatus );                    \
    TEST_ASSERT_EQUAL( NULL, pair.key );                             \
    TEST_ASSERT_EQUAL( 0, pair.keyLength );                          \
    TEST_ASSERT_EQUAL( type, pair.jsonType );                        \
    TEST_ASSERT_EQUAL( ( sizeof( answer ) - 1 ), pair.valueLength ); \
    TEST_ASSERT_EQUAL_STRING_LEN( ( answer ),                        \
                                  pair.value,                        \
                                  pair.valueLength );

    iterateArray( ARRAY_ELEMENT_0_TYPE, ARRAY_ELEMENT_0 );
    iterateArray( ARRAY_ELEMENT_1_TYPE, ARRAY_ELEMENT_1 );
    iterateArray( ARRAY_ELEMENT_2_TYPE, ARRAY_ELEMENT_2 );
    iterateArray( ARRAY_ELEMENT_3_TYPE, ARRAY_ELEMENT_3 );
    iterateArray( ARRAY_ELEMENT_4_TYPE, ARRAY_ELEMENT_4 );
    iterateArray( ARRAY_ELEMENT_5_TYPE, ARRAY_ELEMENT_5 );

    jsonStatus = JSON_Iterate( JSON_DOC_LEGAL_ARRAY,
                               JSON_DOC_LEGAL_ARRAY_LENGTH,
                               &start,
                               &next,
                               &pair );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );
}

/**
 * @brief Test that JSON_Iterate returns the given keys and values from a JSON object.
 */
void test_JSON_Iterate_Legal_Object_Documents( void )
{
    JSONStatus_t jsonStatus;
    size_t start = 0, next = 0;
    JSONPair_t pair = { 0 };

#define iterateObject( key_, type, answer )                          \
    jsonStatus = JSON_Iterate( JSON_NESTED_OBJECT,                   \
                               JSON_NESTED_OBJECT_LENGTH,            \
                               &start,                               \
                               &next,                                \
                               &pair );                              \
    TEST_ASSERT_EQUAL( JSONSuccess, jsonStatus );                    \
    TEST_ASSERT_EQUAL( ( sizeof( key_ ) - 1 ), pair.keyLength );     \
    TEST_ASSERT_EQUAL_STRING_LEN( ( key_ ),                          \
                                  pair.key,                          \
                                  pair.keyLength );                  \
    TEST_ASSERT_EQUAL( type, pair.jsonType );                        \
    TEST_ASSERT_EQUAL( ( sizeof( answer ) - 1 ), pair.valueLength ); \
    TEST_ASSERT_EQUAL_STRING_LEN( ( answer ),                        \
                                  pair.value,                        \
                                  pair.valueLength );

    iterateObject( FIRST_QUERY_KEY, ARRAY_ELEMENT_2_SUB_0_TYPE, ARRAY_ELEMENT_2_SUB_0 );
    iterateObject( SECOND_QUERY_KEY, ARRAY_ELEMENT_2_SUB_1_TYPE, ARRAY_ELEMENT_2_SUB_1 );

    jsonStatus = JSON_Iterate( JSON_NESTED_OBJECT,
                               JSON_NESTED_OBJECT_LENGTH,
                               &start,
                               &next,
                               &pair );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );
}

/**
 * @brief Test that JSON_Iterate returns an error for an invalid collection.
 */
void test_JSON_Iterate_Illegal_Documents( void )
{
    JSONStatus_t jsonStatus;
    size_t start = 0, next = 0;
    JSONPair_t pair = { 0 };

    jsonStatus = JSON_Iterate( FIRST_QUERY_KEY,
                               FIRST_QUERY_KEY_LENGTH,
                               &start,
                               &next,
                               &pair );

    TEST_ASSERT_EQUAL( JSONIllegalDocument, jsonStatus );
}

/**
 * @brief Test that JSON_Search can returns JSONNotFound when a query key does
 * not apply to a JSON document.
 */
void test_JSON_Search_Query_Key_Not_Found( void )
{
    JSONStatus_t jsonStatus;
    char * outValue;
    size_t outValueLength;

    jsonStatus = JSON_Search( JSON_DOC_QUERY_KEY_NOT_FOUND,
                              JSON_DOC_QUERY_KEY_NOT_FOUND_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( JSON_DOC_QUERY_KEY_NOT_FOUND,
                              JSON_DOC_QUERY_KEY_NOT_FOUND_LENGTH,
                              "[0]",
                              ( sizeof( "[0]" ) - 1 ),
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( JSON_DOC_LEGAL_ARRAY,
                              JSON_DOC_LEGAL_ARRAY_LENGTH,
                              "[9]",
                              ( sizeof( "[9]" ) - 1 ),
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( WHITE_SPACE,
                              WHITE_SPACE_LENGTH,
                              "[0]",
                              ( sizeof( "[0]" ) - 1 ),
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( "[" WHITE_SPACE,
                              WHITE_SPACE_LENGTH + 1,
                              "[0]",
                              ( sizeof( "[0]" ) - 1 ),
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( ILLEGAL_SCALAR_IN_ARRAY2,
                              ILLEGAL_SCALAR_IN_ARRAY2_LENGTH,
                              "[1]",
                              ( sizeof( "[1]" ) - 1 ),
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );
}

/**
 * @brief Test that JSON_Search can find the right value given an incorrect query
 * key or Illegal JSON string.
 */
void test_JSON_Search_Illegal_Documents( void )
{
    JSONStatus_t jsonStatus;
    char * outValue;
    size_t outValueLength;

    jsonStatus = JSON_Search( WHITE_SPACE,
                              WHITE_SPACE_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( PADDED_OPENING_CURLY_BRACKET,
                              PADDED_OPENING_CURLY_BRACKET_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( CUT_AFTER_OBJECT_OPEN_BRACE,
                              CUT_AFTER_OBJECT_OPEN_BRACE_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( CLOSING_CURLY_BRACKET,
                              CLOSING_CURLY_BRACKET_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( OPENING_CURLY_BRACKET,
                              OPENING_CURLY_BRACKET_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( CLOSING_SQUARE_BRACKET,
                              CLOSING_SQUARE_BRACKET_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( INCORRECT_OBJECT_SEPARATOR,
                              INCORRECT_OBJECT_SEPARATOR_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( ILLEGAL_KEY_NOT_STRING,
                              ILLEGAL_KEY_NOT_STRING_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( WRONG_KEY_VALUE_SEPARATOR,
                              WRONG_KEY_VALUE_SEPARATOR_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( CUT_AFTER_KEY,
                              CUT_AFTER_KEY_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( TRAILING_COMMA_IN_ARRAY,
                              TRAILING_COMMA_IN_ARRAY_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( CUT_AFTER_COMMA_SEPARATOR,
                              CUT_AFTER_COMMA_SEPARATOR_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( TRAILING_COMMA_AFTER_VALUE,
                              TRAILING_COMMA_AFTER_VALUE_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( NUL_ESCAPE,
                              NUL_ESCAPE_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( SPACE_CONTROL_CHAR,
                              SPACE_CONTROL_CHAR_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( LT_ZERO_CONTROL_CHAR,
                              LT_ZERO_CONTROL_CHAR_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( CLOSING_CURLY_BRACKET,
                              CLOSING_CURLY_BRACKET_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( MISSING_ENCLOSING_ARRAY_MARKER,
                              MISSING_ENCLOSING_ARRAY_MARKER_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( LETTER_AS_EXPONENT,
                              LETTER_AS_EXPONENT_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( CUT_AFTER_DECIMAL_POINT,
                              CUT_AFTER_DECIMAL_POINT_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( LEADING_ZEROS_IN_NUMBER,
                              LEADING_ZEROS_IN_NUMBER_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( ILLEGAL_SCALAR_IN_ARRAY,
                              ILLEGAL_SCALAR_IN_ARRAY_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( ESCAPE_CHAR_ALONE,
                              ESCAPE_CHAR_ALONE_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( UNESCAPED_CONTROL_CHAR,
                              UNESCAPED_CONTROL_CHAR_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( ILLEGAL_UTF8_NEXT_BYTE,
                              ILLEGAL_UTF8_NEXT_BYTE_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( ILLEGAL_UTF8_START_C1,
                              ILLEGAL_UTF8_START_C1_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( ILLEGAL_UTF8_START_F5,
                              ILLEGAL_UTF8_START_F5_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( CUT_AFTER_UTF8_FIRST_BYTE,
                              CUT_AFTER_UTF8_FIRST_BYTE_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( ILLEGAL_UTF8_NEXT_BYTES,
                              ILLEGAL_UTF8_NEXT_BYTES_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );


    jsonStatus = JSON_Search( ILLEGAL_UTF8_GT_MIN_CP_FOUR_BYTES,
                              ILLEGAL_UTF8_GT_MIN_CP_FOUR_BYTES_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( ILLEGAL_UTF8_GT_MIN_CP_THREE_BYTES,
                              ILLEGAL_UTF8_GT_MIN_CP_THREE_BYTES_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( ILLEGAL_UTF8_LT_MAX_CP_FOUR_BYTES,
                              ILLEGAL_UTF8_LT_MAX_CP_FOUR_BYTES_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( ILLEGAL_UTF8_SURROGATE_RANGE_MIN,
                              ILLEGAL_UTF8_SURROGATE_RANGE_MIN_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( ILLEGAL_UTF8_SURROGATE_RANGE_MAX,
                              ILLEGAL_UTF8_SURROGATE_RANGE_MAX_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( ILLEGAL_UTF8_SURROGATE_RANGE_MAX,
                              ILLEGAL_UTF8_SURROGATE_RANGE_MAX_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( ILLEGAL_UNICODE_LITERAL_HEX,
                              ILLEGAL_UNICODE_LITERAL_HEX_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( UNICODE_VALID_HIGH_NO_LOW_SURROGATE,
                              UNICODE_VALID_HIGH_NO_LOW_SURROGATE_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( UNICODE_WRONG_ESCAPE_AFTER_HIGH_SURROGATE,
                              UNICODE_WRONG_ESCAPE_AFTER_HIGH_SURROGATE_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( UNICODE_STRING_END_AFTER_HIGH_SURROGATE,
                              UNICODE_STRING_END_AFTER_HIGH_SURROGATE_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( UNICODE_PREMATURE_LOW_SURROGATE,
                              UNICODE_PREMATURE_LOW_SURROGATE_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( UNICODE_INVALID_LOWERCASE_HEX,
                              UNICODE_INVALID_LOWERCASE_HEX_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( UNICODE_INVALID_UPPERCASE_HEX,
                              UNICODE_INVALID_UPPERCASE_HEX_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( UNICODE_NON_LETTER_OR_DIGIT_HEX,
                              UNICODE_NON_LETTER_OR_DIGIT_HEX_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( UNICODE_BOTH_SURROGATES_HIGH,
                              UNICODE_BOTH_SURROGATES_HIGH_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( UNICODE_ESCAPE_SEQUENCE_ZERO_CP,
                              UNICODE_ESCAPE_SEQUENCE_ZERO_CP_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( UNICODE_VALID_HIGH_INVALID_LOW_SURROGATE,
                              UNICODE_VALID_HIGH_INVALID_LOW_SURROGATE_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );
}

/**
 * @brief Test that JSON_Search is able to classify any null or bad parameters.
 */
void test_JSON_Search_Invalid_Params( void )
{
    JSONStatus_t jsonStatus;
    char * outValue;
    size_t outValueLength;

    jsonStatus = JSON_Search( NULL,
                              0,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNullParameter, jsonStatus );

    jsonStatus = JSON_Search( JSON_DOC_VARIED_SCALARS,
                              JSON_DOC_VARIED_SCALARS_LENGTH,
                              NULL,
                              0,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNullParameter, jsonStatus );

    jsonStatus = JSON_Search( JSON_DOC_VARIED_SCALARS,
                              JSON_DOC_VARIED_SCALARS_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              NULL,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNullParameter, jsonStatus );

    jsonStatus = JSON_Search( JSON_DOC_VARIED_SCALARS,
                              JSON_DOC_VARIED_SCALARS_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              NULL );
    TEST_ASSERT_EQUAL( JSONNullParameter, jsonStatus );

    jsonStatus = JSON_Search( JSON_DOC_VARIED_SCALARS,
                              0,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONBadParameter, jsonStatus );

    jsonStatus = JSON_Search( JSON_DOC_VARIED_SCALARS,
                              JSON_DOC_VARIED_SCALARS_LENGTH,
                              COMPLETE_QUERY_KEY,
                              0,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONBadParameter, jsonStatus );

    jsonStatus = JSON_Search( JSON_DOC_VARIED_SCALARS,
                              JSON_DOC_VARIED_SCALARS_LENGTH,
                              QUERY_KEY_TRAILING_SEPARATOR,
                              QUERY_KEY_TRAILING_SEPARATOR_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONBadParameter, jsonStatus );

    jsonStatus = JSON_Search( JSON_DOC_VARIED_SCALARS,
                              JSON_DOC_VARIED_SCALARS_LENGTH,
                              QUERY_KEY_EMPTY,
                              QUERY_KEY_EMPTY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONBadParameter, jsonStatus );

    jsonStatus = JSON_Search( JSON_DOC_VARIED_SCALARS,
                              JSON_DOC_VARIED_SCALARS_LENGTH,
                              "[",
                              ( sizeof( "[" ) - 1 ),
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONBadParameter, jsonStatus );

    jsonStatus = JSON_Search( JSON_DOC_VARIED_SCALARS,
                              JSON_DOC_VARIED_SCALARS_LENGTH,
                              "[0",
                              ( sizeof( "[0" ) - 1 ),
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONBadParameter, jsonStatus );

    jsonStatus = JSON_Search( JSON_DOC_VARIED_SCALARS,
                              JSON_DOC_VARIED_SCALARS_LENGTH,
                              "[0}",
                              ( sizeof( "[0}" ) - 1 ),
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONBadParameter, jsonStatus );
}

/**
 * @brief Test that JSON_Iterate is able to classify any null or bad parameters.
 */
void test_JSON_Iterate_Invalid_Params( void )
{
    JSONStatus_t jsonStatus;
    size_t start = 0, next = 0;
    JSONPair_t pair = { 0 };

    jsonStatus = JSON_Iterate( NULL,
                               JSON_DOC_LEGAL_ARRAY_LENGTH,
                               &start,
                               &next,
                               &pair );
    TEST_ASSERT_EQUAL( JSONNullParameter, jsonStatus );

    jsonStatus = JSON_Iterate( JSON_DOC_LEGAL_ARRAY,
                               0,
                               &start,
                               &next,
                               &pair );
    TEST_ASSERT_EQUAL( JSONBadParameter, jsonStatus );

    jsonStatus = JSON_Iterate( JSON_DOC_LEGAL_ARRAY,
                               JSON_DOC_LEGAL_ARRAY_LENGTH,
                               NULL,
                               &next,
                               &pair );
    TEST_ASSERT_EQUAL( JSONNullParameter, jsonStatus );

    jsonStatus = JSON_Iterate( JSON_DOC_LEGAL_ARRAY,
                               JSON_DOC_LEGAL_ARRAY_LENGTH,
                               &start,
                               NULL,
                               &pair );
    TEST_ASSERT_EQUAL( JSONNullParameter, jsonStatus );

    jsonStatus = JSON_Iterate( JSON_DOC_LEGAL_ARRAY,
                               JSON_DOC_LEGAL_ARRAY_LENGTH,
                               &start,
                               &next,
                               NULL );
    TEST_ASSERT_EQUAL( JSONNullParameter, jsonStatus );

    start = JSON_DOC_LEGAL_ARRAY_LENGTH + 1;
    jsonStatus = JSON_Iterate( JSON_DOC_LEGAL_ARRAY,
                               JSON_DOC_LEGAL_ARRAY_LENGTH,
                               &start,
                               &next,
                               &pair );
    TEST_ASSERT_EQUAL( JSONBadParameter, jsonStatus );

    start = 0;
    next = JSON_DOC_LEGAL_ARRAY_LENGTH + 1;
    jsonStatus = JSON_Iterate( JSON_DOC_LEGAL_ARRAY,
                               JSON_DOC_LEGAL_ARRAY_LENGTH,
                               &start,
                               &next,
                               &pair );
    TEST_ASSERT_EQUAL( JSONBadParameter, jsonStatus );
}

/**
 * @brief Test that JSON_Search is able to classify a partial JSON document correctly.
 *
 * @note JSON_Search returns JSONIllegalDocument when it finds a partial document.
 */
void test_JSON_Search_Partial_Documents( void )
{
    JSONStatus_t jsonStatus;
    char * outValue;
    size_t outValueLength;

    jsonStatus = JSON_Search( CUT_AFTER_NUMBER,
                              CUT_AFTER_NUMBER_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( CUT_AFTER_ARRAY_START_MARKER,
                              CUT_AFTER_ARRAY_START_MARKER_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( CUT_AFTER_OBJECT_START_MARKER,
                              CUT_AFTER_OBJECT_START_MARKER_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );

    jsonStatus = JSON_Search( CUT_AFTER_KEY,
                              CUT_AFTER_KEY_LENGTH,
                              COMPLETE_QUERY_KEY,
                              COMPLETE_QUERY_KEY_LENGTH,
                              &outValue,
                              &outValueLength );
    TEST_ASSERT_EQUAL( JSONNotFound, jsonStatus );
}

/**
 * @brief Test that a nested collection can only have up to JSON_MAX_DEPTH levels
 * of nesting.
 */
void test_JSON_Max_Depth( void )
{
    JSONStatus_t jsonStatus;
    char * maxNestedObject, * maxNestedArray;

    maxNestedArray = allocateMaxDepthArray();
    jsonStatus = JSON_Validate( maxNestedArray,
                                strlen( maxNestedArray ) );
    TEST_ASSERT_EQUAL( JSONMaxDepthExceeded, jsonStatus );

    maxNestedObject = allocateMaxDepthObject();
    jsonStatus = JSON_Validate( maxNestedObject,
                                strlen( maxNestedObject ) );
    TEST_ASSERT_EQUAL( JSONMaxDepthExceeded, jsonStatus );

    free( maxNestedArray );
    free( maxNestedObject );
}

/**
 * @brief Trip all asserts in internal functions.
 */
void test_JSON_asserts( void )
{
    char buf[] = "x", queryKey[] = "y";
    size_t start = 1, max = 1, length = 1, next = 0;
    uint16_t u = 0;
    size_t key, keyLength, value, valueLength;
    int32_t queryIndex = 0;

    catch_assert( skipSpace( NULL, &start, max ) );
    catch_assert( skipSpace( buf, NULL, max ) );
    /* assert: max != 0 */
    catch_assert( skipSpace( buf, &start, 0 ) );

    /* first argument is length; assert: 2 <= length <= 4 */
    catch_assert( shortestUTF8( 1, u ) );
    catch_assert( shortestUTF8( 5, u ) );

    catch_assert( skipUTF8MultiByte( NULL, &start, max ) );
    catch_assert( skipUTF8MultiByte( buf, NULL, max ) );
    catch_assert( skipUTF8MultiByte( buf, &start, 0 ) );
    /* assert: start < max */
    catch_assert( skipUTF8MultiByte( buf, &start, max ) );
    /* assert: buf[0] < '\0' */
    catch_assert( skipUTF8MultiByte( buf, &start, ( start + 1 ) ) );

    catch_assert( skipUTF8( NULL, &start, max ) );
    catch_assert( skipUTF8( buf, NULL, max ) );
    catch_assert( skipUTF8( buf, &start, 0 ) );

    catch_assert( skipOneHexEscape( NULL, &start, max, &u ) );
    catch_assert( skipOneHexEscape( buf, NULL, max, &u ) );
    catch_assert( skipOneHexEscape( buf, &start, 0, &u ) );
    catch_assert( skipOneHexEscape( buf, &start, max, NULL ) );

    catch_assert( skipHexEscape( NULL, &start, max ) );
    catch_assert( skipHexEscape( buf, NULL, max ) );
    catch_assert( skipHexEscape( buf, &start, 0 ) );

    catch_assert( skipEscape( NULL, &start, max ) );
    catch_assert( skipEscape( buf, NULL, max ) );
    catch_assert( skipEscape( buf, &start, 0 ) );

    catch_assert( skipString( NULL, &start, max ) );
    catch_assert( skipString( buf, NULL, max ) );
    catch_assert( skipString( buf, &start, 0 ) );

    catch_assert( strnEq( NULL, buf, max ) );
    catch_assert( strnEq( buf, NULL, max ) );

    catch_assert( skipLiteral( NULL, &start, max, "lit", length ) );
    catch_assert( skipLiteral( buf, NULL, max, "lit", length ) );
    catch_assert( skipLiteral( buf, &start, 0, "lit", length ) );
    catch_assert( skipLiteral( buf, &start, max, NULL, length ) );

    catch_assert( skipDigits( NULL, &start, max, NULL ) );
    catch_assert( skipDigits( buf, NULL, max, NULL ) );
    catch_assert( skipDigits( buf, &start, 0, NULL ) );

    catch_assert( skipDecimals( NULL, &start, max ) );
    catch_assert( skipDecimals( buf, NULL, max ) );
    catch_assert( skipDecimals( buf, &start, 0 ) );

    catch_assert( skipExponent( NULL, &start, max ) );
    catch_assert( skipExponent( buf, NULL, max ) );
    catch_assert( skipExponent( buf, &start, 0 ) );

    catch_assert( skipNumber( NULL, &start, max ) );
    catch_assert( skipNumber( buf, NULL, max ) );
    catch_assert( skipNumber( buf, &start, 0 ) );

    catch_assert( skipSpaceAndComma( NULL, &start, max ) );
    catch_assert( skipSpaceAndComma( buf, NULL, max ) );
    catch_assert( skipSpaceAndComma( buf, &start, 0 ) );

    catch_assert( skipArrayScalars( NULL, &start, max ) );
    catch_assert( skipArrayScalars( buf, NULL, max ) );
    catch_assert( skipArrayScalars( buf, &start, 0 ) );

    catch_assert( skipObjectScalars( NULL, &start, max ) );
    catch_assert( skipObjectScalars( buf, NULL, max ) );
    catch_assert( skipObjectScalars( buf, &start, 0 ) );

    /* assert: mode is '[' or '{' */
    catch_assert( skipScalars( buf, &start, max, '(' ) );

    catch_assert( skipCollection( NULL, &start, max ) );
    catch_assert( skipCollection( buf, NULL, max ) );
    catch_assert( skipCollection( buf, &start, 0 ) );

    catch_assert( nextValue( NULL, &start, max, &value, &valueLength ) );
    catch_assert( nextValue( buf, NULL, max, &value, &valueLength ) );
    catch_assert( nextValue( buf, &start, 0, &value, &valueLength ) );
    catch_assert( nextValue( buf, &start, max, NULL, &valueLength ) );
    catch_assert( nextValue( buf, &start, max, &value, NULL ) );

    catch_assert( nextKeyValuePair( NULL, &start, max, &key, &keyLength, &value, &valueLength ) );
    catch_assert( nextKeyValuePair( buf, NULL, max, &key, &keyLength, &value, &valueLength ) );
    catch_assert( nextKeyValuePair( buf, &start, 0, &key, &keyLength, &value, &valueLength ) );
    catch_assert( nextKeyValuePair( buf, &start, max, NULL, &keyLength, &value, &valueLength ) );
    catch_assert( nextKeyValuePair( buf, &start, max, &key, NULL, &value, &valueLength ) );
    catch_assert( nextKeyValuePair( buf, &start, max, &key, &keyLength, NULL, &valueLength ) );
    catch_assert( nextKeyValuePair( buf, &start, max, &key, &keyLength, &value, NULL ) );

    catch_assert( objectSearch( NULL, max, queryKey, keyLength, &value, &valueLength ) );
    catch_assert( objectSearch( buf, max, NULL, keyLength, &value, &valueLength ) );
    catch_assert( objectSearch( buf, max, queryKey, keyLength, NULL, &valueLength ) );
    catch_assert( objectSearch( buf, max, queryKey, keyLength, &value, NULL ) );

    catch_assert( arraySearch( NULL, max, queryIndex, &value, &valueLength ) );
    catch_assert( arraySearch( buf, max, queryIndex, NULL, &valueLength ) );
    catch_assert( arraySearch( buf, max, queryIndex, &value, NULL ) );

    catch_assert( skipQueryPart( NULL, &start, max, &valueLength ) );
    catch_assert( skipQueryPart( buf, NULL, max, &valueLength ) );
    catch_assert( skipQueryPart( buf, &start, 0, &valueLength ) );
    catch_assert( skipQueryPart( buf, &start, max, NULL ) );

    catch_assert( multiSearch( NULL, max, queryKey, keyLength, &value, &valueLength ) );
    catch_assert( multiSearch( buf, 0, queryKey, keyLength, &value, &valueLength ) );
    catch_assert( multiSearch( buf, max, NULL, keyLength, &value, &valueLength ) );
    catch_assert( multiSearch( buf, max, queryKey, 0, &value, &valueLength ) );
    catch_assert( multiSearch( buf, max, queryKey, keyLength, NULL, &valueLength ) );
    catch_assert( multiSearch( buf, max, queryKey, keyLength, &value, NULL ) );

    catch_assert( iterate( NULL, max, &start, &next, &key, &keyLength, &value, &valueLength ) );
    catch_assert( iterate( buf, 0, &start, &next, &key, &keyLength, &value, &valueLength ) );
    catch_assert( iterate( buf, max, NULL, &next, &key, &keyLength, &value, &valueLength ) );
    catch_assert( iterate( buf, max, &start, NULL, &key, &keyLength, &value, &valueLength ) );
    catch_assert( iterate( buf, max, &start, &next, NULL, &keyLength, &value, &valueLength ) );
    catch_assert( iterate( buf, max, &start, &next, &key, NULL, &value, &valueLength ) );
    catch_assert( iterate( buf, max, &start, &next, &key, &keyLength, NULL, &valueLength ) );
    catch_assert( iterate( buf, max, &start, &next, &key, &keyLength, &value, NULL ) );
}

/**
 * @brief These checks are not otherwise reached.
 */
void test_JSON_unreached( void )
{
    char buf[ 2 ];
    size_t start, max;

    /* return false when start >= max */
    start = max = 1;
    TEST_ASSERT_EQUAL( false, skipUTF8( "abc", &start, max ) );

    /* return false when buf[ 0 ] != '\\' */
    buf[ 0 ] = 'x';
    start = 0;
    TEST_ASSERT_EQUAL( false, skipEscape( buf, &start, sizeof( buf ) ) );

    /* set output value to -1 when integer conversion exceeds max */
    {
#define TOO_BIG    "100000000000"
        int32_t out;
        start = 0;
        TEST_ASSERT_EQUAL( true, skipDigits( TOO_BIG, &start, ( sizeof( TOO_BIG ) - 1 ), &out ) );
        TEST_ASSERT_EQUAL( -1, out );
    }

    /* return JSONNotFound when start >= max */
    {
        size_t next, key, keyLength, value, valueLength;
        start = max = 1;
        TEST_ASSERT_EQUAL( JSONNotFound,
                           iterate( buf, max, &start, &next, &key, &keyLength, &value, &valueLength ) );
    }
}

/**
 * @brief Test overflows.
 */
void test_JSON_overflows( void )
{
    char buf[] = UNICODE_ESCAPE_SEQUENCES_BMP;
    size_t start;
    uint16_t u;

    start = SIZE_MAX;
    TEST_ASSERT_EQUAL( false, skipOneHexEscape( buf, &start, SIZE_MAX, &u ) );
}
