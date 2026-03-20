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
} lines_arr_t;

const char* mdml_kind_to_html(line_kind_t k) 
{
    switch (k) {
        case KIND_UNKNOWN: return "div";
        case KIND_TEXT:    return "p"; 
        case KIND_H1:      return "h1";
        case KIND_H2:      return "h2";
        case KIND_H3:      return "h3";
        case KIND_H4:      return "h4";
        case KIND_H5:      return "h5";
        case KIND_H6:      return "h6";
        case KIND_OL:      return "ol";
        case KIND_UL:      return "ul";
        case KIND_NEWLINE: return "br"; // for now, might be another thing
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

    if (span_ltrims(line, SPAN("-")) != 0)
        return KIND_UL;

    if (line->length >= 2 &&
        isdigit(line->ptr[0]) && line->ptr[1] == '.') {
        
        line->ptr += 2;
        line->length -= 2;
        return KIND_OL;
    }

    return KIND_TEXT;
}

static void _indent(size_t n) 
{
    for (int i = 0; i < n; i++)
        printf("  ");
}

int mdml_convert(lines_arr_t* lines) 
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
                _indent(stack_top);
                stack[stack_top++] = ln.kind;
                printf("<%s>\n", html);
            }

            _indent(ln.indent);
            printf("<li>"SPAN_FMT"</li>\n", SPAN_ARG(ln.operand));
            break;

        default:
            while (stack_top > 0) {
                _indent(--stack_top);
                printf("</%s>\n", mdml_kind_to_html(stack[stack_top]));
            }

            printf("<%s>", html);
            printf(SPAN_FMT, SPAN_ARG(ln.operand));
            printf("</%s>\n", html);
        }
    }

    // Empty the stack
    while (stack_top > 0) {
        _indent(--stack_top);
        printf("</%s>\n", mdml_kind_to_html(stack[stack_top]));
    }

    return 0;
}

int mdml_parse(const char* input) 
{
    lines_arr_t lines;
    span_t      stream;
    size_t      input_size;
    char*       input_buf;

    if ((input_buf = readfile(input, &input_size)) == NULL)
        return 1;

    stream.ptr = input_buf;
    stream.length = input_size;

    DA_INIT(&lines, line_t, 20);

    while (stream.length > 0) {
        span_t line = span_chop_by(&stream, '\n');
        line_t parsed = {0};

        // \t is ignored on purpose.
        while (span_starts_with(line, SPAN(TAB))) {
            parsed.indent++;
            line.ptr += TAB_LEN;
            line.length -= TAB_LEN;
        }

        parsed.kind = mdml_line_get_kind(&line);
        span_ltrim(&line);

        parsed.operand = line;
        DA_APPEND(&lines, parsed);
    }

    mdml_convert(&lines);
    return 0;
}

int main(int argc, char** argv) 
{
    if (argc < 2) {
        printf("usage: %s input.md\n", argv[0]);
        return 1;
    }

    return mdml_parse(argv[1]);
}