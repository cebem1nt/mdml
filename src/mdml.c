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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#define SPAN_IS_EQ(span, str) (strncmp(span.ptr, str, span.length) == 0)
#define TOKEN_IS_HEADER(t) ((t) == TOKEN_H1 || \
                            (t) == TOKEN_H2 || \
                            (t) == TOKEN_H3 || \
                            (t) == TOKEN_H4 || \
                            (t) == TOKEN_H5 || \
                            (t) == TOKEN_H6)

#define SPAN_EMPTY ((span_t){0, NULL})
#define TOKENS_ARR_CAP 20

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
    size_t length;
    char* ptr;
} span_t;

typedef struct {
    token_type_t type;
    span_t operand;
} token_t;

typedef struct {
    token_t* arr;
    size_t cap;
    size_t len;
} tokens_arr_t;

void perror_aboard(const char* reason) 
{
    perror(reason);
    exit(EXIT_FAILURE);
}

void error_aboard(const char* reason) 
{
    fprintf(stderr, "%s\n", reason);
    exit(EXIT_FAILURE);
} 

tokens_arr_t* tokens_arr_init() 
{
    tokens_arr_t* out = malloc(sizeof(tokens_arr_t));
    if (!out)
        error_aboard("No memory");

    out->arr = malloc(sizeof(token_t) * TOKENS_ARR_CAP);
    if (!out->arr)
        error_aboard("No memory");

    out->cap = TOKENS_ARR_CAP;
    out->len = 0;

    return out;
}

void tokens_arr_append(tokens_arr_t* tokens, token_t val) 
{
    if (tokens->len >= tokens->cap) {
        tokens->cap *= 2;
        tokens->arr = realloc(tokens->arr, tokens->cap * sizeof(token_t));

        if (!tokens->arr) 
            error_aboard("No memory");
    }
    
    tokens->arr[tokens->len++] = val;
}

char* readfile(int fd, size_t* out_size) 
{
    size_t fsize;
    char*  out;

    fsize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    if ((out = malloc(fsize)) == NULL)
        error_aboard("No memory");

    if (read(fd, out, fsize) == -1)
        perror_aboard("read");

    if (out_size)
        *out_size = fsize;

    return out;
}

token_type_t match_token_type(span_t view) 
{
    if (SPAN_IS_EQ(view, "#")) 
        return TOKEN_H1;

    if (SPAN_IS_EQ(view, "##")) 
        return TOKEN_H2;    
    
    if (SPAN_IS_EQ(view, "###")) 
        return TOKEN_H3;   
    
    if (SPAN_IS_EQ(view, "####")) 
        return TOKEN_H4;    
    
    if (SPAN_IS_EQ(view, "#####")) 
        return TOKEN_H5;

    if (SPAN_IS_EQ(view, "######")) 
        return TOKEN_H6;

    return TOKEN_TEXT;
}

const char* match_token_html(token_t t) 
{
    switch (t.type) {
        case TOKEN_H1:      return "h1";
        case TOKEN_H2:      return "h2";
        case TOKEN_H3:      return "h3";
        case TOKEN_H4:      return "h4";
        case TOKEN_H5:      return "h5";
        case TOKEN_H6:      return "h6";
        case TOKEN_OL:      return "ol";
        case TOKEN_UL:      return "ul";
        case TOKEN_TEXT:    return "p";
        case TOKEN_NEWLINE: return "br";
        default:            return "div";
    }
}

int mdml_convert(tokens_arr_t* tokens, const char* out) 
{
    int out_fd;

    if ((out_fd = open(out, O_RDWR | O_CREAT, 0644)) == -1)
        perror_aboard("open");

    for (int i = 0; i < tokens->len; i++) {
        token_t token = tokens->arr[i];
        const char* html = match_token_html(token);

        printf("<%s>%.*s</%s>\n", html, 
                (int)token.operand.length, token.operand.ptr, 
            html);
    }

    return 0;
}

int mdml_parse(const char* input, const char* output) 
{
    tokens_arr_t* tokens;
    int           input_fd;
    size_t        input_size;
    char*         input_buf;

    if ((input_fd = open(input, O_RDONLY)) == -1)
        perror_aboard("open");

    input_buf = readfile(input_fd, &input_size);
    tokens = tokens_arr_init();

    char*  newline; 
    span_t line = {
        0, input_buf
    };

    while ((newline = strchr(line.ptr, '\n')) != NULL) {
        line.length = newline - line.ptr;
        token_type_t token_type = {0};
        span_t       token_raw = {0};
        span_t       operand = {0};
        
        if (line.length == 0) {
            token_type = TOKEN_NEWLINE;
            operand = SPAN_EMPTY;
            goto next;
        } 

        int i = 0;
        token_raw.ptr = line.ptr;

        while (line.ptr[i] != ' ' && i < line.length) {
            token_raw.length++;
            i++;            
        }

        token_type = match_token_type(token_raw);

        if (token_type == TOKEN_TEXT) {
            operand.ptr = line.ptr;
            operand.length = line.length;
        }
        
        // In futue we should just check if token is of primary type
        else if (TOKEN_IS_HEADER(token_type)) {
            operand.ptr = line.ptr + token_raw.length;
            operand.length = line.length - token_raw.length;
        }

next:
        tokens_arr_append(tokens, (token_t){token_type, operand});
        line.ptr = newline + 1;
    }

    mdml_convert(tokens, output);
    free(input_buf);

    return 0;
}

int main(int argc, char** argv) 
{
    if (argc < 2) {
        printf("usage: %s input.md [output.html]\n", argv[0]);
        return 1;
    }

    char* output = "output.html";

    if (argc == 3)
        output = argv[2];
    
    mdml_parse(argv[1], output);
}