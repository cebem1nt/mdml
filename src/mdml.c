/*
 * Some notes.
 * We can differ md formatting in three pieces.
 *  Primary: The main formating options that come at the
 *           beginning of each line. Fo example: headers,
 *           bulleted / numbered lists, code blocks
 *  
 * Secondary (or formating): 
 *      Is any kind of formatting text. like **bold**, _italic_ 
 *
 * Special:
 *  Weird md things, like image notations [something]() or hyprlinks,
 *  possibly sections like >
 *
 * We'll focus primary on first two groups, the special 
 * one will come somewhere in the future 
 */

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#define BACE_IMPLEMENTATION
#include "include/bace.h"

#define STACK_DEPTH 100
#define TOKEN_IS_PRIMARY(t) ((t) == TOKEN_H1 || \
                             (t) == TOKEN_H2 || \
                             (t) == TOKEN_H3 || \
                             (t) == TOKEN_H4 || \
                             (t) == TOKEN_H5 || \
                             (t) == TOKEN_OL || \
                             (t) == TOKEN_UL || \
                             (t) == TOKEN_H6)

typedef enum {
    TOKEN_UNKNOWN = -1,
    TOKEN_H1,
    TOKEN_H2,
    TOKEN_H3,
    TOKEN_H4,
    TOKEN_H5,
    TOKEN_H6,
    TOKEN_OL,
    TOKEN_UL,
    TOKEN_TEXT,
    TOKEN_NEWLINE,
} token_type_t;

typedef struct {
    token_type_t type;
    span_t operand;
    span_t operand_extra;
} token_t;

typedef struct {
    token_t* arr;
    size_t cap;
    size_t len;
} tokens_arr_t;

static char* readfile(int fd, size_t* out_size) 
{
    size_t fsize;
    char*  out;

    fsize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    if ((out = malloc(fsize)) == NULL)
        error_abort("%s", "No memory");

    if (read(fd, out, fsize) == -1)
        perror_abort("read");

    if (out_size)
        *out_size = fsize;

    return out;
}

/*
 * Takes in any kind of span and matches it to a token
 */
token_type_t mdml_span_tokenize(span_t raw) 
{
    if (span_iseq(raw, SPAN("#"))) 
        return TOKEN_H1;

    if (span_iseq(raw, SPAN("##"))) 
        return TOKEN_H2;    
    
    if (span_iseq(raw, SPAN("###"))) 
        return TOKEN_H3;
    
    if (span_iseq(raw, SPAN("####"))) 
        return TOKEN_H4;
    
    if (span_iseq(raw, SPAN("#####"))) 
        return TOKEN_H5;

    if (span_iseq(raw, SPAN("######"))) 
        return TOKEN_H6;

    if (span_iseq(raw, SPAN("-"))) 
        return TOKEN_UL;

    if (isdigit(raw.ptr[0]) && raw.ptr[raw.length-1] == '.')
        return TOKEN_OL;

    return TOKEN_TEXT;
}

const char* mdml_token_to_html(token_type_t t) 
{
    switch (t) {
        case TOKEN_H1:      return "h1";
        case TOKEN_H2:      return "h2";
        case TOKEN_H3:      return "h3";
        case TOKEN_H4:      return "h4";
        case TOKEN_H5:      return "h5";
        case TOKEN_H6:      return "h6";
        case TOKEN_TEXT:    return "p";
        case TOKEN_NEWLINE: return "br";
        case TOKEN_OL:      return "ol";
        case TOKEN_UL:      return "ul";
        default:            return "div";
    }
}

int mdml_convert(tokens_arr_t* tokens) 
{
    token_type_t stack[STACK_DEPTH];
    size_t       stack_top = 0;

    for (size_t i = 0; i < tokens->len; i++) {
        token_t t = tokens->arr[i];
        const char* html = mdml_token_to_html(t.type);

        if (t.type == TOKEN_OL || t.type == TOKEN_UL) {            
            if (stack_top == 0 || stack[stack_top-1] != t.type) {
                stack[stack_top++] = t.type;
                printf("<%s>\n", html);
            } 

            printf("    <li>%.*s</li>\n", (int)t.operand.length, t.operand.ptr);
        } else if (stack_top > 0) {
            token_type_t mode = stack[--stack_top];
            const char* closing = mdml_token_to_html(mode);
            printf("</%s>\n", closing);
        } else if (t.type == TOKEN_NEWLINE) {
            printf("<%s>\n", html);
        } else {
            printf("<%s>%.*s</%s>\n", html, 
                (int)t.operand.length, t.operand.ptr, 
            html);
        }
    }

    if (stack_top > 0) {
        token_type_t mode = stack[--stack_top];
        const char* closing = mdml_token_to_html(mode);
        printf("</%s>\n", closing);
    }

    return 0;
}

int mdml_parse(const char* input) 
{
    tokens_arr_t  tokens;
    span_t        line, raw;
    int           input_fd;
    size_t        input_size;
    char*         input_buf;

    if ((input_fd = open(input, O_RDONLY)) == -1)
        perror_abort("open");

    DA_INIT(&tokens, token_t, 20);
    
    input_buf = readfile(input_fd, &input_size);
    raw = span_from_cstr(input_buf);

    while (raw.length != 0) {
        line = span_chop_by(&raw, '\n');

        token_type_t token_type = {0};
        span_t       token_raw  = {0},
                     op_extra   = SPAN_EMPTY,
                     op         = SPAN_EMPTY;
        
        if (line.length == 0) {
            token_type = TOKEN_NEWLINE;
            goto next;
        }

        span_ltrim(&line);
        token_raw.ptr = line.ptr;

        for (int i = 0; i < line.length; i++) {
            if (line.ptr[i] == ' ')
                break;
            token_raw.length++;
        }

        token_type = mdml_span_tokenize(token_raw);

        if (token_type == TOKEN_TEXT) {
            op.ptr = line.ptr;
            op.length = line.length;
        }
        
        else if (TOKEN_IS_PRIMARY(token_type)) {
            // HTML won't render trailing and leading whitespaces, no need to care
            op.ptr = line.ptr + token_raw.length;
            op.length = line.length - token_raw.length;
        }

next:
        DA_APPEND(&tokens, (token_t){token_type, op, op_extra});
    }

    mdml_convert(&tokens);
    free(input_buf);

    return 0;
}

int main(int argc, char** argv) 
{
    if (argc < 2) {
        printf("usage: %s input.md\n", argv[0]);
        return 1;
    }

    mdml_parse(argv[1]);
}