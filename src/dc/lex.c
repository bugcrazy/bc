/*
 * *****************************************************************************
 *
 * Copyright 2018 Gavin D. Howard
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * *****************************************************************************
 *
 * The lexer for dc.
 *
 */

#include <ctype.h>

#include <status.h>
#include <lex.h>
#include <dc.h>
#include <vm.h>

#if DC_ENABLED
BcStatus dc_lex_register(BcLex *l) {

	BcStatus s = BC_STATUS_SUCCESS;

	if (isspace(l->buf[l->i - 1])) {
		bc_lex_whitespace(l);
		++l->i;
		if (!DC_X) s = BC_STATUS_LEX_EXTENDED_REG;
		else s = bc_lex_name(l);
	}
	else {
		bc_vec_npop(&l->t.v, l->t.v.len);
		bc_vec_pushByte(&l->t.v, l->buf[l->i - 1]);
		bc_vec_pushByte(&l->t.v, '\0');
		l->t.t = BC_LEX_NAME;
	}

	return s;
}

BcStatus dc_lex_string(BcLex *l) {

	size_t depth = 1, nls = 0, i = l->i;
	char c;

	l->t.t = BC_LEX_STR;
	bc_vec_npop(&l->t.v, l->t.v.len);

	for (c = l->buf[i]; c != 0 && depth; c = l->buf[++i]) {

		depth += (c == '[' && (i == l->i || l->buf[i - 1] != '\\'));
		depth -= (c == ']' && (i == l->i || l->buf[i - 1] != '\\'));
		nls += (c == '\n');

		if (depth) bc_vec_push(&l->t.v, &c);
	}

	if (c == '\0') {
		l->i = i;
		return BC_STATUS_LEX_NO_STRING_END;
	}

	bc_vec_pushByte(&l->t.v, '\0');
	if (i - l->i > BC_MAX_STRING) return BC_STATUS_EXEC_STRING_LEN;

	l->i = i;
	l->line += nls;

	return BC_STATUS_SUCCESS;
}

BcStatus dc_lex_token(BcLex *l) {

	BcStatus s = BC_STATUS_SUCCESS;
	char c = l->buf[l->i++], c2;
	size_t i;

	for (i = 0; i < dc_lex_regs_len; ++i) {
		if (l->t.last == dc_lex_regs[i]) return dc_lex_register(l);
	}

	if (c >= '%' && c <= '~' &&
	    (l->t.t = dc_lex_tokens[(c - '%')]) != BC_LEX_INVALID)
	{
		return s;
	}

	// This is the workhorse of the lexer.
	switch (c) {

		case '\0':
		{
			l->t.t = BC_LEX_EOF;
			break;
		}

		case '\n':
		case '\t':
		case '\v':
		case '\f':
		case '\r':
		case ' ':
		{
			l->newline = (c == '\n');
			bc_lex_whitespace(l);
			break;
		}

		case '!':
		{
			c2 = l->buf[l->i];

			if (c2 == '=') l->t.t = BC_LEX_OP_REL_NE;
			else if (c2 == '<') l->t.t = BC_LEX_OP_REL_LE;
			else if (c2 == '>') l->t.t = BC_LEX_OP_REL_GE;
			else return BC_STATUS_LEX_BAD_CHAR;

			++l->i;
			break;
		}

		case '#':
		{
			bc_lex_lineComment(l);
			break;
		}

		case '.':
		{
			if (isdigit(l->buf[l->i])) s = bc_lex_number(l, c);
			else s = BC_STATUS_LEX_BAD_CHAR;
			break;
		}

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		{
			s = bc_lex_number(l, c);
			break;
		}

		case '[':
		{
			s = dc_lex_string(l);
			break;
		}

		default:
		{
			l->t.t = BC_LEX_INVALID;
			s = BC_STATUS_LEX_BAD_CHAR;
			break;
		}
	}

	return s;
}
#endif // DC_ENABLED
