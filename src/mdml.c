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

#define BACE_IMPLEMENTATION
#include "include/bace.h" // IWYU pragma: keep

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
    TOKEN_NEWLINE,
    TOKEN_TEXT,
    TOKEN_H1,
    TOKEN_H2,
    TOKEN_H3,
    TOKEN_H4,
    TOKEN_H5,
    TOKEN_H6,
    TOKEN_OL,
    TOKEN_UL,
} token_kind_t;

typedef struct {
    token_kind_t type;
    span_t operand;
    span_t operand_extra;
} token_t;

typedef struct {
    token_t* arr;
    size_t cap;
    size_t len;
} tokens_arr_t;

/*
 * Takes in any kind of span and matches it to a token
 */
token_kind_t mdml_span_tokenize(span_t raw) 
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

const char* mdml_token_to_html(token_kind_t tk) 
{
    switch (tk) {
        case TOKEN_NEWLINE: return "br";
        case TOKEN_TEXT:    return "p";
        case TOKEN_H1:      return "h1";
        case TOKEN_H2:      return "h2";
        case TOKEN_H3:      return "h3";
        case TOKEN_H4:      return "h4";
        case TOKEN_H5:      return "h5";
        case TOKEN_H6:      return "h6";
        case TOKEN_OL:      return "ol";
        case TOKEN_UL:      return "ul";
        default:            return "div";
    }
}

int mdml_lex(span_t stream, tokens_arr_t* dest) 
{
    while (stream.length != 0) {
        span_t line = span_chop_by(&stream, '\n');

        token_kind_t kind;
        span_t       token_raw  = {0},
                     op_extra   = SPAN_EMPTY,
                     op         = SPAN_EMPTY;
        
        if (line.length == 0) {
            kind = TOKEN_NEWLINE;
            goto next;
        }

        span_ltrim(&line);
        token_raw.ptr = line.ptr;

        for (int i = 0; i < line.length; i++) {
            if (line.ptr[i] == ' ')
                break;
            token_raw.length++;
        }

        kind = mdml_span_tokenize(token_raw);

        if (kind == TOKEN_TEXT) {
            op.ptr = line.ptr;
            op.length = line.length;
        }
        
        else if (TOKEN_IS_PRIMARY(kind)) {
            op.ptr = line.ptr + token_raw.length;
            op.length = line.length - token_raw.length;
            span_ltrim(&op);
        }

next:
        DA_APPEND(dest, (token_t){kind, op, op_extra});
    }

    return 0;
}

int mdml_convert(tokens_arr_t* tokens) 
{
    token_kind_t stack[STACK_DEPTH];
    size_t       stack_top = 0;

    printf("TODO: mdml_convert\n");

    return 0;
}

int mdml_parse(const char* input) 
{
    tokens_arr_t tokens;
    span_t       stream;
    size_t       input_size;
    char*        input_buf;

    DA_INIT(&tokens, token_t, 20);
    
    if ((input_buf = readfile(input, &input_size)) == NULL)
        error_exit("Could not read file\n");

    stream = span_from_cstr(input_buf);
    mdml_lex(stream, &tokens);
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