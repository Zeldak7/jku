#include "presettings.h"

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct BigInteger {
	size_t length;
	bool negative;

	char *data;
} BigInteger;

typedef struct Matrix {
	size_t rows;
	size_t columns;

	// data[row][col]
	BigInteger ***data;
} Matrix;


// Counterpart to create_big_integer. Destroys a BigInteger struct.
void destroy_big_integer(BigInteger *bigint) {
	if (!bigint) {
		return;
	}

	if (bigint->data) {
		free(bigint->data);
	}

	free(bigint);

	return;
}

// Reverses the content of the data field of the given bigint.
void reverse_big_integer_data(BigInteger *bigint) {
	for (size_t i = 0; i < bigint->length / 2; i++) {
		size_t reverse_i = bigint->length - 1 - i;
		char temp = bigint->data[reverse_i];

		bigint->data[reverse_i] = bigint->data[i];
		bigint->data[i] = temp;
	}
}

// Parses a string containing a number into a bigint.
error_t parse_raw_integer(char *raw_integer, BigInteger *bigint) {
	char c;
	size_t i = 0;
	while ((c = *raw_integer++)) {
		if (!isdigit(c)) {
			return EXIT_INVALID_NUMBER;
		}

		if (i > MAX_LENGTH) {
			return EXIT_NUMBER_TOO_BIG;
		}

		bigint->data[i++] = c - '0';
	}

	bigint->length = i;

	if (!bigint->length) {
		return EXIT_INVALID_NUMBER;
	}

	return EXIT_OK;
}

// Creates a BigInteger struct from a string. Guarantees that digits are zeroed.
// The data in the struct will be reversed. E. g. "123" -> [ 3, 2, 1 ]. Caller
// is responsible for cleanup in all cases
BigInteger *create_big_integer(char *raw_integer, error_t *error) {
	// Use calloc to make fields (length, negative, ...) default to zero
	BigInteger *bigint = calloc(1, sizeof(BigInteger));

	if (!bigint) {
		*error = EXIT_OUT_OF_MEM;

		return NULL;
	}

	// Make sure that unset digits are zeroed
	bigint->data = calloc(1, MAX_LENGTH);

	if (!bigint->data) {
		*error = EXIT_OUT_OF_MEM;

		return bigint;
	}

	if (*raw_integer == '-') {
		bigint->negative = true;
		raw_integer++;
	}

	if ((*error = parse_raw_integer(raw_integer, bigint)) != EXIT_OK) {
		return bigint;
	}

	reverse_big_integer_data(bigint);

	return bigint;
}

// Analogous to strcmp, returns 0 if a == b, -1 if a < b and 1 if a > b. If 
// absolute is true, ignores the sign.
int compare_big_integers(BigInteger *a, BigInteger *b, bool absolute) {
	if (a->length > b->length) {
		return ((absolute || !a->negative) ? 1 : -1);
	} else if (a->length < b->length) {
		return ((absolute || !b->negative) ? -1 : 1);
	}

	// We know a->length == b->length

	if (!absolute) {
		if (!a->negative && b->negative) {
			return 1;
		} else if (a->negative && !b->negative) {
			return -1;
		}
	}

	for (size_t i = 0; i < a->length; i++) {
		if (a->data[i] > b->data[i]) {
			return (absolute || !a->negative) ? 1 : -1;
		} else if (a->data[i] < b->data[i]) {
			return (absolute || !a->negative) ? -1 : 1;
		}
	}

	return 0;
}

// Calculate src + dest and stores it in dest. Exactly one of src or dest has to
// be negative.
error_t subtract_big_integers(BigInteger *src, BigInteger *dest) {
	const size_t max_length = (
		src->length > dest->length ? src->length : dest->length
	);

	bool swapped_sign = false;

	// Wrapping this in a scope because the variables would be potentially 
	// confusing otherwise
	{
		BigInteger *negative = src->negative ? src : dest;
		BigInteger *positive = negative == src ? dest : src;

		// If the negative value is larger than the positive one (in absolute terms)...
		if (compare_big_integers(negative, positive, true) > 0) {
			// Swap their signedness to make the naiive algorithm work
			negative->negative = false;
			positive->negative = true;

			swapped_sign = true;
		}
	}

	const int src_factor = src->negative ? -1 : 1;
	const int dest_factor = dest->negative ? -1 : 1;

	size_t i;
	int carry = 0;

	for (i = 0; i < max_length; i++) {
		const int sum = (src->data[i] * src_factor)
			+ (dest->data[i] * dest_factor) + carry;

		if (sum < 0) {
			carry = -1;
			dest->data[i] = sum + 10;
		} else {
			carry = 0;
			dest->data[i] = sum;
		}
	}

	dest->length = i;

	// We can never have a carry at this point because the negative value is 
	// guaranteed to be smaller than the positive one.

	if (swapped_sign) {
		// If we previously swapped the signs of the operands, make sure 
		// to change them back. Dest is now definitely negative though.
		dest->negative = true;
		src->negative = !src->negative;
	} else {
		// And if we didn't swap them dest is sure to be positive.
		dest->negative = false;
	}

	return EXIT_OK;
}

// Calculate src + dest and store it in dest. If absolute is true, ignores the sign.
error_t add_big_integers(BigInteger *src, BigInteger *dest, bool absolute) {
	// If either src or dest are zero immediately return the other one
	if (src->length == 1 && !src->data[src->length - 1]) {
		// src is zero so dest remains unchanged
		return EXIT_OK;
	} else if (dest->length == 1 && !dest->data[dest->length - 1]) {
		// dest is zero so just copy src into it
		dest->length = src->length;
		dest->negative = absolute ? dest->negative : src->negative;

		memmove(dest->data, src->data, src->length);

		return EXIT_OK;
	}

	// If exactly one of the numbers is negative, perform a subtraction
	if (!absolute && (src->negative ^ dest->negative)) {
		return subtract_big_integers(src, dest);
	}

	// We know that either none or both of the numbers are negative now. If 
	// both are negative we can just perform addition as if none of them were.
	
	const size_t max_length = (
		src->length > dest->length ? src->length : dest->length
	);

	size_t i;
	int carry = 0;

	for (i = 0; i < max_length; i++) {
		// It's fine if we technically go over one of the bigints lengths
		// here because the buffers are zero-initialized

		// Sum is in [0, 19]
		const int sum = src->data[i] + dest->data[i] + carry;

		// Carry is either 0 or 1
		carry = sum / 10;
		// Final digit is in [0, 9]
		dest->data[i] = sum % 10;

		// If we made a contentful calculation just now...
		if (dest->data[i]) {
			dest->length = i + 1;
		}
	}

	// If we still have a leftover carry...
	if (carry) {
		// Make sure it will fit
		if (i + 1 > MAX_LENGTH) {
			return EXIT_NUMBER_TOO_BIG;
		}

		dest->length = i + 1;
		dest->data[i] = carry;
	}

	return EXIT_OK;
}

// Returns a copy of a given biginteger, scaled by a value in [0, 9]. Caller is 
// responsible for freeing the created bigint in all cases.
BigInteger *scale_big_integer(BigInteger *src, char scale, error_t *error) {
	BigInteger *dest = create_big_integer("0", error);

	if (*error != EXIT_OK || scale == 0) {
		return dest;
	}

	// This is not particularly efficient but adds at most a constant factor
	// to the runtime so it'll be fine
	for (char i = 0; i < scale; i++) {
		error_t temp_error = EXIT_OK;

		if ((temp_error = add_big_integers(src, dest, true)) != EXIT_OK) {
			*error = temp_error;

			return dest;
		}
	}

	return dest;
}

// Returns a new bigintiger containing the result of a * b. Caller is resposible for 
// freeing the created bigint in all cases.
BigInteger *multiply_big_integers(BigInteger *a, BigInteger *b, error_t *error) {
	BigInteger *dest = create_big_integer("0", error);

	if (*error != EXIT_OK) {
		return dest;
	}

	for (size_t i = 0; i < a->length; i++) {
		BigInteger *scaled = scale_big_integer(b, a->data[i], error);

		if (*error != EXIT_OK) {
			destroy_big_integer(scaled);

			return dest;
		}

		if ((scaled->length + i) > MAX_LENGTH) {
			*error = EXIT_NUMBER_TOO_BIG;

			return dest;
		}

		// Multiply `scaled` by 10^i by adding i zeroes to the end
		memmove(scaled->data + i, scaled->data, scaled->length);
		memset(scaled->data, 0, i);
		scaled->length += i;

		error_t temp_error = EXIT_OK;

		if ((temp_error = add_big_integers(scaled, dest, true)) != EXIT_OK) {
			*error = temp_error;

			destroy_big_integer(scaled);

			return dest;
		}

		destroy_big_integer(scaled);
	}

	const bool one_negative = !(a->negative && b->negative)
		&& (a->negative || b->negative);

	if (one_negative) {
		dest->negative = true;
	}

	return dest;
}

// Creates an empty matrix (2d array) of the given size. Might still return a 
// pointer to an incomplete matrix if *err is EXIT_OUT_OF_MEM, make sure to 
// destroy it. Doesn't initialize matrix data items.
Matrix *create_matrix(size_t rows, size_t columns, error_t *err) {
	// Make sure to always use calloc here, otherwise destroy_matrix might 
	// run into garbage values when called immediately after a partially failed
	// run of create_matrix. (In other cases the unitialized memory will always 
	// be overwritten anyways so it really only matters for destroy_matrix.)

	Matrix *matrix = calloc(1, sizeof(Matrix));

	if (!matrix) {
		*err = EXIT_OUT_OF_MEM;

		return NULL;
	}

	matrix->rows = rows;
	matrix->columns = columns;

	matrix->data = calloc(rows, sizeof(BigInteger **));

	if (!matrix) {
		*err = EXIT_OUT_OF_MEM;

		return matrix;
	}

	for (size_t row = 0; row < rows; row++) {
		matrix->data[row] = calloc(columns, sizeof(BigInteger *));

		if (!matrix->data[row]) {
			*err = EXIT_OUT_OF_MEM;

			return matrix;
		}
	}

	return matrix;
}

// Counterpart to create_matrix. Can also destroy matrices that were only partially
// allocated and will destroy any BigIntegers.
void destroy_matrix(Matrix *matrix) {
	if (!matrix) {
		return;
	}

	if (matrix->data) {
		for (size_t row = 0; row < matrix->rows; row++) {
			if (!matrix->data[row]) {
				continue;
			}

			for (size_t col = 0; col < matrix->columns; col++) {
				if (!matrix->data[row][col]) {
					continue;
				}

				destroy_big_integer(matrix->data[row][col]);
			}

			free(matrix->data[row]);
		}

		free(matrix->data);
	}

	free(matrix);
}

// Read and discard characters until a non-isspace() character is read. That
// character will be the first character on the stream after the call.
void skip_space(FILE *input_file) {
	char c;

	// Skip space characters and newlines etc.
	while (isspace(c = fgetc(input_file)))
		;

	// `c` isn't a space so add it to the front again
	ungetc(c, input_file);
}

// Read a word, i. e. a sequence of characters which are !isspace() until the first
// isspace() character is read. That character will be the first character on the
// stream after the call.
error_t read_word(FILE *input_file, char *word, size_t max_length) {
	skip_space(input_file);

	char c;
	size_t i = 0;

	while ((c = fgetc(input_file))) {
		if (isspace(c)) {
			break;
		}

		if (i >= (max_length - 1)) {
			return EXIT_NUMBER_TOO_BIG;
		}

		word[i++] = c;
	}

	ungetc(c, input_file);

	word[i] = '\0';

	return EXIT_OK;
}

int32_t read_int32(FILE *input_file, error_t *error) {
	// Maximum amount of digits for int32 is 10, plus one for \0
	const size_t max_length = 11;

	char *word = malloc(max_length);
	if ((*error = read_word(input_file, word, max_length)) != EXIT_OK) {
		free(word);

		return 0;
	}

	int32_t value = strtol(word, NULL, 10);

	free(word);

	return value;
}

void parse_matrix_block(FILE *input_file, Matrix *m, error_t *error) {
	const size_t max_length = MAX_LENGTH + 1;
	char *raw_integer = malloc(max_length);

	for (size_t row = 0; row < m->rows; row++) {
		for (size_t col = 0; col < m->columns; col++) {
			if (feof(input_file)) {
				free(raw_integer);

				*error = EXIT_INVALID_MATRIX;

				return;
			}

			if ((*error = read_word(input_file, raw_integer, max_length)) != EXIT_OK) {
				free(raw_integer);

				return;
			}

			m->data[row][col] = create_big_integer(raw_integer, error);

			if (*error != EXIT_OK) {
				free(raw_integer);

				return;
			}
		}
	}

	free(raw_integer);
}

void initialize_matrix(FILE *input_file, Matrix **m, error_t *error) {
	size_t rows = read_int32(input_file, error);
	size_t cols = read_int32(input_file, error);

	if (*error) {
		return;
	} else if (!rows || !cols) {
		*error = EXIT_INVALID_NUMBER;

		return ;
	}

	*m = create_matrix(rows, cols, error);
}

error_t parse_input_file(FILE *input_file, Matrix **m1, Matrix **m2) {
	error_t error = EXIT_OK;

	initialize_matrix(input_file, m1, &error);
	initialize_matrix(input_file, m2, &error);

	if (error != EXIT_OK) {
		return error;
	}

	if (!*m1 || !*m2) {
		return EXIT_INTERNAL;
	}

	if ((*m1)->columns != (*m2)->rows) {
		return EXIT_INCOMPATIBLE_DIM;
	}

	parse_matrix_block(input_file, *m1, &error);
	parse_matrix_block(input_file, *m2, &error);

	skip_space(input_file);

	if (!feof(input_file)) {
		return EXIT_INVALID_MATRIX;
	}

	return error;
}

void print_matrix(Matrix *m) {
	printf("matrix @ %p\n", m);

	if (!m) {
		return;
	}

	printf("\trows = %ld\n", m->rows);
	printf("\tcolumns = %ld\n", m->columns);
	printf("\tdata = %p\n", m->data);

	if (!m->data) {
		return;
	}

	for (size_t row = 0; row < m->rows; row++) {
		if (!m->data[row]) {
			continue;
		}

		for (size_t col = 0; col < m->columns; col++) {
			BigInteger *bigint = m->data[row][col];

			if (!bigint) {
				fputs("\tnull ", stdout);

				continue;
			}

			putchar('\t');

			if (bigint->negative) {
				putchar('-');
			}

			size_t i;
			for (i = bigint->length; i > 0; i--) {
				printf("%d", bigint->data[i - 1]);
			}
		}

		putchar('\n');
	}

	putchar('\n');
}

// Returns the result of a * b. Caller is resposible for freeing the return value
// in all cases.
Matrix *multiply_matrices(Matrix *a, Matrix *b, error_t *error) {
	Matrix *result = create_matrix(a->rows, b->columns, error);

	if (*error != EXIT_OK) {
		return result;
	}

	for (size_t row = 0; row < result->rows; row++) {
		for (size_t col = 0; col < result->columns; col++) {
			BigInteger *bigint = create_big_integer("0", error);

			if (*error != EXIT_OK) {
				destroy_big_integer(bigint);

				return result;
			}

			for (size_t i = 0; i < a->columns; i++) {
				BigInteger *product = multiply_big_integers(
					a->data[row][i], b->data[i][col], error
				);

				if (*error != EXIT_OK) {
					destroy_big_integer(bigint);
					destroy_big_integer(product);

					return result;
				}

				if ((*error = add_big_integers(product, bigint, false)) != EXIT_OK) {
					destroy_big_integer(bigint);
					destroy_big_integer(product);

					return result;
				}

				destroy_big_integer(product);
			}

			result->data[row][col] = bigint;
		}
	}

	return result;
}

int main(int argc, char *argv[]) {
	error_t error = EXIT_OK;

	if (argc != 2) {
		fputs(invalid_arg_num_str, stderr);

		return EXIT_ARGS;
	}

	char *input_file_name = argv[1];
	FILE *input_file = fopen(input_file_name, "r");

	if (!input_file) {
		fprintf(stderr, open_infile_err_str, input_file_name);

		return EXIT_IO;
	}

	Matrix *m1 = NULL, *m2 = NULL;

	if ((error = parse_input_file(input_file, &m1, &m2)) != EXIT_OK) {
		if (error == EXIT_OUT_OF_MEM) {
			fputs(out_of_mem_str, stderr);
		} else if (error == EXIT_NUMBER_TOO_BIG) {
			fprintf(stderr, number_too_big_str, MAX_LENGTH);
		} else {
			// Should be a dead code path
			fprintf(stderr, unknown_error_str, error);
		}

		// This is C after all
		goto cleanup;
	}

	print_matrix(m1);
	print_matrix(m2);

	Matrix *result = multiply_matrices(m1, m2, &error);

	print_matrix(result);

	destroy_matrix(result);

cleanup:
	destroy_matrix(m1);
	destroy_matrix(m2);

	if (fclose(input_file)) {
		fputs(close_file_err_str, stderr);

		return EXIT_IO;
	}

	return error;
}
