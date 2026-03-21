/*
 * Some notes.
 * We can differ md formatting in two pieces.
 *   - line
 *   - formatting
 * 
 * In md if line is something special, like header, text, list,
 * you can understand that by matching the first token of the line
 * By token i mean the first word after splitting by ' '.
 * We can qualify them by different "Kinds"
 *
 * So line has a kind and optionally text.
 * By text I mean any sequence of chars (or a single one)
 *
 * Example: 
 * - Lorem ipsum
 *
 * This is a line, kind is "Unordered List" and text is "Lorem ipsum"
 *
 * Links
 * links are a kind of special formatting, they look like:
 *
 * [any text](link/to/something)
 * 
 */

#define BACE_IMPLEMENTATION
#include "include/bace.h" // IWYU pragma: keep

#define TAB "    "
#define TAB_LEN 4
#define STACK_DEPTH 20 // Definitely no more than 20 nested lists

typedef enum {
    KIND_UNKNOWN = -1,
    KIND_TEXT,
    KIND_H1,
    KIND_H2,
    KIND_H3,
    KIND_H4,
    KIND_H5,
    KIND_H6,
    KIND_OL,
    KIND_UL,
    KIND_PRE_OPENED,
    KIND_PRE_CLOSED,
    KIND_BARE,
    KIND_NEWLINE,
} line_kind_t;

typedef struct {
    line_kind_t kind;
    span_t operand;
    size_t indent; // Line indentation
} line_t;

typedef struct {
    line_t* arr;
    size_t  cap, len;
} line_da_t;

typedef struct {
    size_t length; 
    char*  marker;
    char*  otag;
    char*  ctag; 
} fmt_marker_t;

fmt_marker_t markers[] = {
    {3, "***", "<b><i>", "</i></b>"},
    {3, "___", "<b><i>", "</i></b>"},
    {2, "**",  "<b>",    "</b>"},
    {2, "__",  "<b>",    "</b>"},
    {2, "~~",  "<del>",  "</del>"},
    {2, "==",  "<u>",    "</u>"},
    {1, "*",   "<i>",    "</i>"},
    {1, "_",   "<i>",    "</i>"},
    {1, "`",   "<code>", "</code>"},
};

const char* mdml_kind_to_html(line_kind_t k) 
{
    switch (k) {
        case KIND_UNKNOWN:        return "div";
        case KIND_TEXT:           return "p"; 
        case KIND_H1:             return "h1";
        case KIND_H2:             return "h2";
        case KIND_H3:             return "h3";
        case KIND_H4:             return "h4";
        case KIND_H5:             return "h5";
        case KIND_H6:             return "h6";
        case KIND_OL:             return "ol";
        case KIND_UL:             return "ul";
        case KIND_NEWLINE:        return "br"; // for now, might be another thing
        case KIND_PRE_CLOSED:
        case KIND_PRE_OPENED:     return "pre";
        case KIND_BARE:           return NULL;
    }
}

line_kind_t mdml_line_get_kind(span_t* line)
{
    if (line->length == 0)
        return KIND_NEWLINE;

    if (span_ltrims(line, SPAN("######")) != 0)
        return KIND_H6;

    if (span_ltrims(line, SPAN("#####")) != 0)
        return KIND_H5;

    if (span_ltrims(line, SPAN("####")) != 0)
        return KIND_H4;

    if (span_ltrims(line, SPAN("###")) != 0)
        return KIND_H3;

    if (span_ltrims(line, SPAN("##")) != 0)
        return KIND_H2;

    if (span_ltrims(line, SPAN("#")) != 0)
        return KIND_H1;

    if (span_ltrims(line, SPAN("-")) != 0 ||
        span_ltrims(line, SPAN("*")) != 0 ||
        span_ltrims(line, SPAN("+")) != 0 )
        return KIND_UL;

    if (line->length >= 2 &&
        isdigit(line->ptr[0]) && line->ptr[1] == '.') {
        span_advance(line, 2);
        return KIND_OL;
    }

    return KIND_TEXT;
}

static void indent(size_t n) 
{
    for (int i = 0; i < n; i++)
        printf("  ");
}

void mdml_convert_text(span_t text, const char* html) 
{
    if (html == NULL) {
        printf(SPAN_FMT"\n", SPAN_ARG(text));
        return;
    }

    int_da_t stack;
    DA_INIT(&stack, 20);
    
    printf("<%s>", html);
    while (text.length > 0) {
        bool is_matched = false;

        for (int i = 0; i < sizeof(markers)/sizeof(markers[0]); i++) {
            fmt_marker_t* m = &markers[i];

            span_t m_raw = {
                m->marker,
                m->length
            };

            if (!span_starts_with(text, m_raw))
                continue;

            if (stack.len > 0 && stack.arr[stack.len - 1] == i) {
                printf("%s", m->ctag);
                DA_POP(&stack);
            } else {
                printf("%s", m->otag);
                DA_APPEND(&stack, i);
            }

            is_matched = true;
            span_advance(&text, m->length);
            break;
        }

        if (!is_matched) {
            putchar(*text.ptr);
            span_advance(&text, 1);
        }
    }

    while (stack.len > 0) {
        int i = DA_POP(&stack);
        printf("%s", markers[i].ctag);
    }

    printf("</%s>\n", html);
}

int mdml_convert(line_da_t* lines) 
{
    line_kind_t stack[STACK_DEPTH];
    size_t      stack_top = 0;

    for (size_t i = 0; i < lines->len; i++) {
        line_t ln = lines->arr[i];
        const char* html = mdml_kind_to_html(ln.kind);

        switch (ln.kind) {
        case KIND_OL:
        case KIND_UL:
            ln.indent += 1;

            if (ln.indent > stack_top) {
                if (stack_top >= STACK_DEPTH) 
                    return 2;

                indent(stack_top);
                stack[stack_top++] = ln.kind;
                printf("<%s>\n", html);
            }

            indent(ln.indent);
            mdml_convert_text(ln.operand, "li");
            break;

        default:
            while (stack_top > 0) {
                indent(--stack_top);
                printf("</%s>\n", mdml_kind_to_html(stack[stack_top]));
            }

            if (ln.kind == KIND_NEWLINE)
                continue; // Just skip, might do other thing 

            if (ln.kind == KIND_PRE_OPENED)
                printf("<pre>\n");
            else if (ln.kind == KIND_PRE_CLOSED)
                printf("</pre>\n");
            else
                mdml_convert_text(ln.operand, html);
        }
    }

    // Empty the stack
    while (stack_top > 0) {
        indent(--stack_top);
        printf("</%s>\n", mdml_kind_to_html(stack[stack_top]));
    }

    return 0;
}

int mdml_parse(const char* input) 
{
    line_da_t   lines;
    span_t      stream;
    size_t      input_size;
    char*       input_buf;

    if ((input_buf = readfile(input, &input_size)) == NULL)
        return 1;

    stream.ptr = input_buf;
    stream.length = input_size;

    DA_INIT(&lines);

    // Lex
    bool has_fence = false;
    while (stream.length > 0) {
        span_t line = span_chop_by(&stream, '\n');
        line_t parsed = {0};

        if (span_ltrims(&line, SPAN("```")) > 0) {
            has_fence = !has_fence;
            parsed.kind = has_fence ? KIND_PRE_OPENED
                                    : KIND_PRE_CLOSED;
            goto next;
        }

        while (span_starts_with(line, SPAN(TAB)) && !has_fence) {
            parsed.indent++;
            span_advance(&line, TAB_LEN);
        }
        
        if (has_fence) {
            parsed.kind = KIND_BARE;
        } else {
            parsed.kind = mdml_line_get_kind(&line);
            span_ltrim(&line);
        }

next:
        parsed.operand = line;
        DA_APPEND(&lines, parsed);
    }

    return mdml_convert(&lines);
}

int main(int argc, char** argv) 
{
    if (argc < 2) {
        printf("usage: %s input.md\n", argv[0]);
        return 1;
    }

    return mdml_parse(argv[1]);
}