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

#define TAB     "    "
#define TAB_LEN 4

typedef enum {
    TOKEN_UNKNOWN = -1,
    TOKEN_NEWLINE,
    TOKEN_TEXT,
    TOKEN_TAB,
    TOKEN_NUM,
    TOKEN_DASH,
    TOKEN_H1,
    TOKEN_H2,
    TOKEN_H3,
    TOKEN_H4,
    TOKEN_H5,
    TOKEN_H6,
} token_kind_t;

typedef struct {
    token_kind_t kind;
    span_t entry;
} token_t;

typedef struct {
    token_t* arr;
    size_t cap;
    size_t len;
} tokens_arr_t;

static const char* token_name(token_kind_t t) 
{
    switch (t) {
        case TOKEN_UNKNOWN: return "TOKEN_UNKNOWN";
        case TOKEN_NEWLINE: return "TOKEN_NEWLINE";
        case TOKEN_TEXT:    return "TOKEN_TEXT";
        case TOKEN_TAB:     return "TOKEN_TAB";
        case TOKEN_H1:      return "TOKEN_H1";
        case TOKEN_H2:      return "TOKEN_H2";
        case TOKEN_H3:      return "TOKEN_H3";
        case TOKEN_H4:      return "TOKEN_H4";
        case TOKEN_H5:      return "TOKEN_H5";
        case TOKEN_H6:      return "TOKEN_H6";
        case TOKEN_NUM:     return "TOKEN_NUM";
        case TOKEN_DASH:    return "TOKEN_DASH";
    }
}

token_kind_t mdml_match_token(span_t raw) 
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
        return TOKEN_DASH;

    if (isdigit(raw.ptr[0]) && raw.ptr[raw.length-1] == '.')
        return TOKEN_NUM;

    return TOKEN_TEXT;
}

int mdml_lex(span_t stream, tokens_arr_t* dest) 
{
    while (stream.length != 0) {
        span_t line = span_chop_by(&stream, '\n');

        token_kind_t  kind;
        span_t        raw = {0};

        if (line.length == 0) {
            kind = TOKEN_NEWLINE;
            DA_APPEND(dest, (token_t){kind, raw});
            continue;
        }
        
        // Strip tabs and put them as tokens
        while (true) {
            int len;

            if (span_starts_with(line, SPAN(TAB)))
                len = TAB_LEN;
            else if (span_starts_with(line, SPAN("\t")))
                len = 1;
            else 
                break;

            line.ptr += len;
            line.length -= len;
            DA_APPEND(dest, (token_t){TOKEN_TAB, raw});
        }

        raw = span_chop_by(&line, ' ');
        kind = mdml_match_token(raw);
        DA_APPEND(dest, (token_t){kind, raw});
        
        if (line.length > 0)
            DA_APPEND(dest, (token_t){TOKEN_TEXT, line});
    }

    for (int i = 0; i < dest->len; i++) {
        printf("%s :"SPAN_FMT":\n", token_name(dest->arr[i].kind),
                                    SPAN_ARG(dest->arr[i].entry));
    }

    return 0;
}

int mdml_convert(tokens_arr_t* tokens)
{
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
    // mdml_convert(&tokens);

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