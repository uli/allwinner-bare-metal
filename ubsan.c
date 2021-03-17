/*-
 * Copyright (c) 2017 The FreeBSD Foundation
 * All rights reserved.
 *
 * As a reference, this code uses the software from the contrib/compiler-rt
 * implementation of the userspace UBSAN runtime, which is developed as part
 * of the LLVM project under the University of Illinois Open Source License.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 */

/*
 * Adapted from (unmerged?) FreeBSD patch for allwinner-bare-metal
 * Copyright (c) 2021 Ulrich Hecht
 */

// Avoid using stdio. Enable this if stuff goes wrong really early.
//#define EARLY_PRINTF

#include <sys/cdefs.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>


enum type_kinds {
	TK_INTEGER = 0x0000,
	TK_FLOAT = 0x0001,
	TK_UNKNOWN = 0xffff
};

struct type_descriptor {
	uint16_t type_kind;
	uint16_t type_info;
	char type_name[1];
};

struct source_location {
	char *filename;
	uint32_t line;
	uint32_t column;
};

struct overflow_data {
	struct source_location loc;
	struct type_descriptor *type;
};

struct shift_out_of_bounds_data {
	struct source_location loc;
	struct type_descriptor *lhs_type;
	struct type_descriptor *rhs_type;
};

struct out_of_bounds_data {
	struct source_location loc;
	struct type_descriptor *array_type;
	struct type_descriptor *index_type;
};

struct non_null_return_data {
	struct source_location attr_loc;
};

struct type_mismatch_data {
	struct source_location loc;
	struct type_descriptor *type;
	unsigned char log_alignment;
	unsigned char type_check_kind;
};

struct vla_bound_data {
	struct source_location loc;
	struct type_descriptor *type;
};

struct invalid_value_data {
	struct source_location loc;
	struct type_descriptor *type;
};

struct unreachable_data {
	struct source_location loc;
};

const char *type_check_kinds[] = {
    "load of", "store to", "reference binding to", "member access within",
    "member call on", "constructor call on", "downcast of", "downcast of",
    "upcast of", "cast to virtual base of", "_Nonnull binding to"
};

void __ubsan_handle_add_overflow(struct overflow_data *data, uintptr_t lhs,
    uintptr_t rhs);

void __ubsan_handle_sub_overflow(struct overflow_data *data, uintptr_t lhs,
    uintptr_t rhs);

void __ubsan_handle_mul_overflow(struct overflow_data *data, uintptr_t lhs,
    uintptr_t rhs);

void __ubsan_handle_divrem_overflow(struct overflow_data *data, uintptr_t lhs,
    uintptr_t rhs);

void __ubsan_handle_negate_overflow(struct overflow_data *data, uintptr_t old);

void __ubsan_handle_shift_out_of_bounds(struct shift_out_of_bounds_data *data,
    uintptr_t lhs, uintptr_t rhs);

void __ubsan_handle_out_of_bounds(struct out_of_bounds_data *data,
    uintptr_t index);

void __ubsan_handle_nonnull_return(struct non_null_return_data *data,
    struct source_location *loc);

void __ubsan_handle_type_mismatch_v1(struct type_mismatch_data *data,
    uintptr_t ptr);

void __ubsan_handle_vla_bound_not_positive(struct vla_bound_data *data,
    uintptr_t bound);

void __ubsan_handle_load_invalid_value(struct invalid_value_data *data,
    uintptr_t val);

void __ubsan_handle_builtin_unreachable(struct unreachable_data *data);

#ifdef EARLY_PRINTF
#include "uart.h"
#define printf(x...) do { char foo[256]; sprintf(foo, x); uart_print(foo); } while (0)
#endif

static unsigned
bit_width(struct type_descriptor *type)
{

	return (1 << (type->type_info >> 1));
}

static int
is_inline_int(struct type_descriptor *type)
{

	return (bit_width(type) <= 8*sizeof(uintptr_t));
}

static int
is_signed(struct type_descriptor *type)
{

	return (type->type_info & 1);
}

static intmax_t
signed_val(struct type_descriptor *type, uintptr_t val)
{

	if (is_inline_int(type)) {
		unsigned extra_bits = 8*sizeof(intmax_t) - bit_width(type);
		return ((intmax_t)val << extra_bits >> extra_bits);
	}
	return (*(intmax_t *)val);
}

static uintmax_t
unsigned_val(struct type_descriptor *type, uintptr_t val)
{

	if (is_inline_int(type))
		return (val);
	return (*(uintmax_t *)val);
}

static void
format_value(char *dest, size_t size, struct type_descriptor *type,
    uintptr_t value)
{

	if (type->type_kind == TK_INTEGER) {
		if (is_signed(type)) {
			snprintf(dest, size, "%lld",
			    (int64_t)signed_val(type, value));
		} else {
			snprintf(dest, size, "%llu",
			    (uint64_t)unsigned_val(type, value));
		}
	} else {
		snprintf(dest, size, "<unknown>");
	}
}

static int
ubsan_init(struct source_location *loc)
{
	uint32_t old_column;

	old_column = loc->column;
	loc->column = 0xffffffff;

	if (old_column == 0xffffffff)
		return (0);
	printf("%s:%lu:%lu: runtime error: ", strrchr(loc->filename, '/') + 1,
	    loc->line, old_column);
	return (1);
}

static void
handle_overflow(struct overflow_data *data, uintptr_t lhs, const char *operator,
    uintptr_t rhs)
{
	char lhs_str[20];
	char rhs_str[20];
	int data_signed;

	if (!ubsan_init(&data->loc))
		return;

	data_signed = is_signed(data->type);
	format_value(lhs_str, sizeof(lhs_str), data->type, lhs);
	format_value(rhs_str, sizeof(rhs_str), data->type, rhs);

	printf("%s integer overflow: %s %s %s cannot be represented"
	    "in type %s\n", data_signed ? "signed" : "unsigned", lhs_str,
	    operator, rhs_str, data->type->type_name);
}

void
__ubsan_handle_add_overflow(struct overflow_data *data, uintptr_t lhs,
    uintptr_t rhs)
{

	handle_overflow(data, lhs, "+", rhs);
}

void
__ubsan_handle_sub_overflow(struct overflow_data *data, uintptr_t lhs,
    uintptr_t rhs)
{

	handle_overflow(data, lhs, "-", rhs);
}

void
__ubsan_handle_mul_overflow(struct overflow_data *data, uintptr_t lhs,
    uintptr_t rhs)
{

	handle_overflow(data, lhs, "*", rhs);
}

void
__ubsan_handle_divrem_overflow(struct overflow_data *data, uintptr_t lhs,
    uintptr_t rhs)
{
	struct type_descriptor *type = data->type;

	if (!ubsan_init(&data->loc))
		return;

	if (is_signed(type) && signed_val(type, rhs) == -1)
		printf("division of %lld by -1 cannot be represented"
		    "in type %s\n", signed_val(data->type, lhs),
		    data->type->type_name);
	else
		printf("division by zero\n");
}

void
__ubsan_handle_negate_overflow(struct overflow_data *data, uintptr_t old)
{
	struct type_descriptor *type = data->type;

	if (!ubsan_init(&data->loc))
		return;

	if (is_signed(type))
		printf("negation of %lld cannot be represented in type %s; "
		    "cast to an unsigned type to negate this value to itself\n",
		    signed_val(type, old), data->type->type_name);
	else
		printf("negation of %lld cannot be represented in type %s\n",
		    unsigned_val(type, old), data->type->type_name);
}

void
__ubsan_handle_shift_out_of_bounds(struct shift_out_of_bounds_data *data,
    uintptr_t lhs, uintptr_t rhs)
{

	if (!ubsan_init(&data->loc))
		return;

	if (is_signed(data->rhs_type) && signed_val(data->rhs_type, rhs) < 0) {
		printf("shift exponent %lld is negative\n",
		    signed_val(data->rhs_type, rhs));
	} else if (unsigned_val(data->rhs_type, rhs) >=
	    bit_width(data->lhs_type)) {
		printf("shift exponent %llu is too large for %u-bit type %s\n",
		    unsigned_val(data->rhs_type, rhs),
		    bit_width(data->rhs_type), data->lhs_type->type_name);
	} else if (is_signed(data->lhs_type) &&
	    signed_val(data->lhs_type, lhs) < 0) {
		printf("left shift of negative value %lld\n",
		    signed_val(data->lhs_type, lhs));
	} else {
		printf("left shift of %llu by %llu places cannot be represented"
		    "in type %s\n", unsigned_val(data->lhs_type, lhs),
		    unsigned_val(data->rhs_type, rhs),
		    data->lhs_type->type_name);
	}
}

void
__ubsan_handle_out_of_bounds(struct out_of_bounds_data *data, uintptr_t index)
{
	char index_str[20];

	if (!ubsan_init(&data->loc))
		return;

	format_value(index_str, sizeof(index_str), data->index_type, index);
	printf("index %s out of bounds for type %s\n",
	    index_str, data->array_type->type_name);
}

void
__ubsan_handle_nonnull_return(struct non_null_return_data *data,
    struct source_location *loc)
{
	struct source_location attr_loc = data->attr_loc;

	if (!ubsan_init(loc))
		return;

	printf("null pointer returned from function declared"
	    "to never return null\n");
	if (attr_loc.filename) {
		printf("%s:%lu:%lu: note: returns_nonnull attribute"
		    "specified here\n", strrchr(attr_loc.filename, '/') + 1,
		    attr_loc.line, attr_loc.column);
	}
}

void
__ubsan_handle_type_mismatch_v1(struct type_mismatch_data *data, uintptr_t ptr)
{
	unsigned alignment;

	if (!ubsan_init(&data->loc))
		return;

	alignment = 1 << data->log_alignment;
	if (!ptr)
		printf("%s null pointer of type %s\n",
		    type_check_kinds[data->type_check_kind],
		    data->type->type_name);
	else if (ptr & (alignment - 1))
		printf("%s misaligned address %p for type %s, "
		    "which requires %u byte alignment\n",
		    type_check_kinds[data->type_check_kind], (void *)ptr,
		    data->type->type_name, alignment);
	else
		printf("%s address %p with insufficient space "
		    "for an object of type %s\n",
		    type_check_kinds[data->type_check_kind], (void *)ptr,
		    data->type->type_name);
}

void
__ubsan_handle_vla_bound_not_positive(struct vla_bound_data *data,
    uintptr_t bound)
{
	char bound_str[20];

	if (!ubsan_init(&data->loc))
		return;

	format_value(bound_str, sizeof(bound_str), data->type, bound);
	printf("variable length array bound evaluates"
	    "to non-positive value %s\n", bound_str);
}

void __ubsan_handle_load_invalid_value(struct invalid_value_data *data,
    uintptr_t val)
{
	char val_str[20];

	if (!ubsan_init(&data->loc))
		return;

	format_value(val_str, sizeof(val_str), data->type, val);
	printf("load of value %s, which is not a valid value for type %s\n",
	    val_str, data->type->type_name);
}

void
__ubsan_handle_builtin_unreachable(struct unreachable_data *data)
{

	if (!ubsan_init(&data->loc))
		return;

	printf("execution reached a __builtin_unreachable() call\n");
}
