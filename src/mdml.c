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

int mdml_convert(lines_arr_t* lines) 
{
    for (size_t i = 0; i < lines->len; i++) {
        line_t l = lines->arr[i];
        printf("Type %i, indent (%lu):", l.kind, l.indent);

        if (l.kind == KIND_NEWLINE) {
            printf("NEWLINE\n");
        } else {
            printf(SPAN_FMT"\n", SPAN_ARG(l.operand));
        }
    }

    return 0;
}

int mdml_parse(const char* input) 
{
    lines_arr_t lines;
    span_t      stream;
    size_t      input_size;
    char*       input_buf;

    input_buf = readfile(input, &input_size);
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

    mdml_parse(argv[1]);
}