#include "openlayout.h"

#include <stdlib.h>
#include <string.h>

struct ol_node {
    struct ol_document *doc;
    ol_id id;
    ol_role role;
    struct ol_node *parent;
    struct ol_node *first_child;
    struct ol_node *last_child;
    struct ol_node *next_sibling;

    char *text;
    char *name;
    char *value;
    char *href;

    ol_style style;
    ol_rect bounds;
    uint32_t actions;
    uint32_t states;

    int image_id;
    ol_unit image_width;
    ol_unit image_height;
};

struct ol_document {
    ol_node *root;
    ol_node **nodes;
    size_t node_count;
    size_t node_capacity;

    ol_display_op *ops;
    size_t op_count;
    size_t op_capacity;

    ol_unit content_height;
    ol_unit viewport_width;
    uint32_t generation;
    ol_id next_id;
};

typedef struct {
    ol_document *doc;
    ol_measure_text_fn measure;
    void *measure_userdata;

    ol_unit x0;
    ol_unit right;
    ol_unit cursor_x;
    ol_unit line_top;
    ol_unit line_height;
    int has_content;
} ol_inline_flow;

static char *ol_strdup0(const char *s)
{
    size_t n;
    char *p;
    if (!s) s = "";
    n = strlen(s);
    p = (char *)malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n + 1);
    return p;
}

static int reserve_nodes(ol_document *doc, size_t extra)
{
    size_t need, cap;
    ol_node **p;
    need = doc->node_count + extra;
    if (need <= doc->node_capacity) return 1;
    cap = doc->node_capacity ? doc->node_capacity : 16;
    while (cap < need) cap *= 2;
    p = (ol_node **)realloc(doc->nodes, cap * sizeof(*p));
    if (!p) return 0;
    doc->nodes = p;
    doc->node_capacity = cap;
    return 1;
}

static int reserve_ops(ol_document *doc, size_t extra)
{
    size_t need, cap;
    ol_display_op *p;
    need = doc->op_count + extra;
    if (need <= doc->op_capacity) return 1;
    cap = doc->op_capacity ? doc->op_capacity : 64;
    while (cap < need) cap *= 2;
    p = (ol_display_op *)realloc(doc->ops, cap * sizeof(*p));
    if (!p) return 0;
    doc->ops = p;
    doc->op_capacity = cap;
    return 1;
}

static ol_display_op *append_op(ol_document *doc, ol_op_type type)
{
    ol_display_op *op;
    if (!reserve_ops(doc, 1)) return NULL;
    op = &doc->ops[doc->op_count++];
    memset(op, 0, sizeof(*op));
    op->type = type;
    return op;
}

static int rect_empty(ol_rect r)
{
    return r.width <= 0 || r.height <= 0;
}

static void rect_add(ol_rect *dst, ol_rect src)
{
    ol_unit x1, y1, x2, y2;
    if (!dst || rect_empty(src)) return;
    if (rect_empty(*dst)) {
        *dst = src;
        return;
    }
    x1 = dst->x < src.x ? dst->x : src.x;
    y1 = dst->y < src.y ? dst->y : src.y;
    x2 = dst->x + dst->width > src.x + src.width
        ? dst->x + dst->width : src.x + src.width;
    y2 = dst->y + dst->height > src.y + src.height
        ? dst->y + dst->height : src.y + src.height;
    dst->x = x1;
    dst->y = y1;
    dst->width = x2 - x1;
    dst->height = y2 - y1;
}

static int point_in_rect(ol_rect r, ol_unit x, ol_unit y)
{
    return !rect_empty(r) &&
           x >= r.x && y >= r.y &&
           x < r.x + r.width &&
           y < r.y + r.height;
}

static int node_depth(const ol_node *node)
{
    int d = 0;
    while (node && node->parent) {
        ++d;
        node = node->parent;
    }
    return d;
}

static ol_display default_display_for_role(ol_role role)
{
    switch (role) {
    case OL_ROLE_TEXT:
    case OL_ROLE_LINK:
    case OL_ROLE_IMAGE:
    case OL_ROLE_BUTTON:
    case OL_ROLE_INPUT:
        return OL_DISPLAY_INLINE;
    default:
        return OL_DISPLAY_BLOCK;
    }
}

void ol_style_init(ol_style *style)
{
    if (!style) return;
    memset(style, 0, sizeof(*style));
    style->display = OL_DISPLAY_BLOCK;
    style->text_align = OL_ALIGN_START;
    style->font_size = OL_CSSPX(16);
    style->line_height = OL_CSSPX(20);
    style->foreground = 0xff000000u;
    style->background = 0x00000000u;
}

ol_document *ol_document_new(void)
{
    ol_document *doc = (ol_document *)calloc(1, sizeof(*doc));
    ol_node *root;
    if (!doc) return NULL;
    doc->next_id = 1;
    if (!reserve_nodes(doc, 1)) {
        free(doc);
        return NULL;
    }
    root = (ol_node *)calloc(1, sizeof(*root));
    if (!root) {
        free(doc->nodes);
        free(doc);
        return NULL;
    }
    root->doc = doc;
    root->id = doc->next_id++;
    root->role = OL_ROLE_DOCUMENT;
    root->image_id = -1;
    ol_style_init(&root->style);
    root->style.display = OL_DISPLAY_BLOCK;
    doc->nodes[doc->node_count++] = root;
    doc->root = root;
    return doc;
}

static void free_node(ol_node *node)
{
    if (!node) return;
    free(node->text);
    free(node->name);
    free(node->value);
    free(node->href);
    free(node);
}

void ol_document_free(ol_document *doc)
{
    size_t i;
    if (!doc) return;
    for (i = 0; i < doc->node_count; ++i) free_node(doc->nodes[i]);
    free(doc->nodes);
    free(doc->ops);
    free(doc);
}

ol_node *ol_document_root(ol_document *doc)
{
    return doc ? doc->root : NULL;
}

const ol_node *ol_document_root_const(const ol_document *doc)
{
    return doc ? doc->root : NULL;
}

ol_node *ol_node_append(ol_document *doc, ol_node *parent, ol_role role,
                        const char *utf8)
{
    ol_node *node;
    if (!doc) return NULL;
    if (!parent) parent = doc->root;
    if (parent->doc != doc) return NULL;
    if (!reserve_nodes(doc, 1)) return NULL;
    node = (ol_node *)calloc(1, sizeof(*node));
    if (!node) return NULL;
    node->doc = doc;
    node->id = doc->next_id++;
    node->role = role;
    node->parent = parent;
    node->image_id = -1;
    ol_style_init(&node->style);
    node->style.display = default_display_for_role(role);
    node->text = ol_strdup0(utf8);
    if (!node->text) {
        free(node);
        return NULL;
    }
    if (role == OL_ROLE_LINK) node->actions |= OL_ACTION_ACTIVATE;
    if (!parent->first_child) parent->first_child = node;
    else parent->last_child->next_sibling = node;
    parent->last_child = node;
    doc->nodes[doc->node_count++] = node;
    return node;
}

ol_id ol_node_id(const ol_node *node) { return node ? node->id : 0; }
ol_role ol_node_role(const ol_node *node) { return node ? node->role : OL_ROLE_DOCUMENT; }
const char *ol_node_text(const ol_node *node) { return node ? node->text : NULL; }
const char *ol_node_name(const ol_node *node) { return node ? node->name : NULL; }
const char *ol_node_value(const ol_node *node) { return node ? node->value : NULL; }
const char *ol_node_href(const ol_node *node) { return node ? node->href : NULL; }
const ol_style *ol_node_style(const ol_node *node) { return node ? &node->style : NULL; }
ol_rect ol_node_bounds(const ol_node *node)
{
    ol_rect r = {0, 0, 0, 0};
    return node ? node->bounds : r;
}

static int set_string(char **dst, const char *s)
{
    char *p = ol_strdup0(s);
    if (!p) return 0;
    free(*dst);
    *dst = p;
    return 1;
}

int ol_node_set_text(ol_node *node, const char *utf8)
{
    return node ? set_string(&node->text, utf8) : 0;
}

int ol_node_set_name(ol_node *node, const char *utf8)
{
    return node ? set_string(&node->name, utf8) : 0;
}

int ol_node_set_value(ol_node *node, const char *utf8)
{
    return node ? set_string(&node->value, utf8) : 0;
}

int ol_node_set_href(ol_node *node, const char *utf8)
{
    return node ? set_string(&node->href, utf8) : 0;
}

void ol_node_set_style(ol_node *node, const ol_style *style)
{
    if (node && style) node->style = *style;
}

void ol_node_set_actions(ol_node *node, uint32_t actions)
{
    if (node) node->actions = actions;
}

void ol_node_set_states(ol_node *node, uint32_t states)
{
    if (node) node->states = states;
}

uint32_t ol_node_actions(const ol_node *node) { return node ? node->actions : 0; }
uint32_t ol_node_states(const ol_node *node) { return node ? node->states : 0; }

void ol_node_set_image(ol_node *node, int image_id, ol_unit width, ol_unit height)
{
    if (!node) return;
    node->image_id = image_id;
    node->image_width = width;
    node->image_height = height;
}

const ol_node *ol_node_by_id(const ol_document *doc, ol_id id)
{
    size_t i;
    if (!doc || !id) return NULL;
    for (i = 0; i < doc->node_count; ++i)
        if (doc->nodes[i]->id == id) return doc->nodes[i];
    return NULL;
}

const ol_node *ol_find_role_name(const ol_document *doc, ol_role role,
                                 const char *name)
{
    size_t i;
    if (!doc || !name) return NULL;
    for (i = 0; i < doc->node_count; ++i) {
        const ol_node *n = doc->nodes[i];
        if (n->role == role && n->name && strcmp(n->name, name) == 0)
            return n;
    }
    return NULL;
}

const ol_node *ol_hit_test(const ol_document *doc, ol_unit x, ol_unit y)
{
    const ol_node *best = NULL;
    int best_depth = -1;
    size_t i;
    if (!doc) return NULL;
    for (i = 0; i < doc->node_count; ++i) {
        const ol_node *n = doc->nodes[i];
        int d;
        if (n->style.display == OL_DISPLAY_NONE || !point_in_rect(n->bounds, x, y))
            continue;
        d = node_depth(n);
        if (d >= best_depth) {
            best = n;
            best_depth = d;
        }
    }
    return best;
}

static int default_measure(const char *utf8, size_t length,
                           const ol_style *style, ol_text_metrics *m)
{
    size_t i, glyphs = 0;
    for (i = 0; i < length; ++i)
        if (((unsigned char)utf8[i] & 0xc0u) != 0x80u) ++glyphs;
    m->width = (ol_unit)((int64_t)style->font_size * 3 * (int64_t)glyphs / 5);
    m->ascent = style->font_size * 4 / 5;
    m->descent = style->font_size - m->ascent;
    return 1;
}

static int measure_text(ol_inline_flow *flow, const char *text, size_t len,
                        const ol_style *style, ol_text_metrics *m)
{
    if (flow->measure)
        return flow->measure(flow->measure_userdata, text, len, style, m);
    return default_measure(text, len, style, m);
}

static void flow_newline(ol_inline_flow *flow)
{
    flow->cursor_x = flow->x0;
    flow->line_top += flow->line_height > 0 ? flow->line_height : OL_CSSPX(20);
    flow->line_height = 0;
    flow->has_content = 0;
}

static int token_is_spaces(const char *s, size_t n)
{
    size_t i;
    for (i = 0; i < n; ++i)
        if (s[i] != ' ' && s[i] != '\t') return 0;
    return 1;
}

static int emit_text_piece(ol_inline_flow *flow, ol_node *node,
                           size_t offset, size_t length)
{
    ol_text_metrics m;
    ol_display_op *op;
    ol_unit line_height, baseline;
    const char *text = node->text + offset;

    if (!length) return 1;
    if (!measure_text(flow, text, length, &node->style, &m)) return 0;
    line_height = node->style.line_height;
    if (line_height < m.ascent + m.descent)
        line_height = m.ascent + m.descent;
    if (line_height <= 0) line_height = OL_CSSPX(20);

    if (flow->has_content && flow->cursor_x + m.width > flow->right)
        flow_newline(flow);

    if (!flow->has_content && token_is_spaces(text, length))
        return 1;

    if (line_height > flow->line_height) flow->line_height = line_height;
    baseline = flow->line_top + (line_height - (m.ascent + m.descent)) / 2 + m.ascent;

    op = append_op(flow->doc, OL_OP_TEXT);
    if (!op) return 0;
    op->u.text.node_id = node->id;
    op->u.text.bounds.x = flow->cursor_x;
    op->u.text.bounds.y = flow->line_top;
    op->u.text.bounds.width = m.width;
    op->u.text.bounds.height = line_height;
    op->u.text.baseline = baseline;
    op->u.text.text_offset = offset;
    op->u.text.text_length = length;
    op->u.text.style = node->style;

    rect_add(&node->bounds, op->u.text.bounds);
    flow->cursor_x += m.width;
    flow->has_content = 1;
    return 1;
}

static int layout_inline_node(ol_inline_flow *flow, ol_node *node);

static int layout_inline_text(ol_inline_flow *flow, ol_node *node)
{
    const char *s = node->text ? node->text : "";
    size_t len = strlen(s), pos = 0, start;

    while (pos < len) {
        if (s[pos] == '\r') {
            ++pos;
            continue;
        }
        if (s[pos] == '\n') {
            flow_newline(flow);
            ++pos;
            continue;
        }

        start = pos;
        if (s[pos] == ' ' || s[pos] == '\t') {
            while (pos < len && (s[pos] == ' ' || s[pos] == '\t')) ++pos;
        } else {
            while (pos < len && s[pos] != ' ' && s[pos] != '\t' &&
                   s[pos] != '\r' && s[pos] != '\n')
                ++pos;
            while (pos < len && (s[pos] == ' ' || s[pos] == '\t')) ++pos;
        }
        if (!emit_text_piece(flow, node, start, pos - start)) return 0;
    }
    return 1;
}

static int layout_inline_image(ol_inline_flow *flow, ol_node *node)
{
    ol_unit w = node->image_width > 0 ? node->image_width : OL_CSSPX(16);
    ol_unit h = node->image_height > 0 ? node->image_height : OL_CSSPX(16);
    ol_display_op *op;

    if (flow->has_content && flow->cursor_x + w > flow->right)
        flow_newline(flow);

    op = append_op(flow->doc, OL_OP_IMAGE);
    if (!op) return 0;
    op->u.image.node_id = node->id;
    op->u.image.bounds.x = flow->cursor_x;
    op->u.image.bounds.y = flow->line_top;
    op->u.image.bounds.width = w;
    op->u.image.bounds.height = h;
    op->u.image.image_id = node->image_id;
    rect_add(&node->bounds, op->u.image.bounds);

    flow->cursor_x += w;
    if (h > flow->line_height) flow->line_height = h;
    flow->has_content = 1;
    return 1;
}

static int layout_inline_node(ol_inline_flow *flow, ol_node *node)
{
    ol_node *child;
    ol_rect before = node->bounds;
    if (!node || node->style.display == OL_DISPLAY_NONE) return 1;

    if (node->role == OL_ROLE_IMAGE) {
        if (!layout_inline_image(flow, node)) return 0;
    } else if (node->text && node->text[0]) {
        if (!layout_inline_text(flow, node)) return 0;
    }

    for (child = node->first_child; child; child = child->next_sibling) {
        if (child->style.display == OL_DISPLAY_BLOCK) break;
        if (!layout_inline_node(flow, child)) return 0;
        rect_add(&node->bounds, child->bounds);
    }

    if (rect_empty(node->bounds)) node->bounds = before;
    return 1;
}

static int layout_block(ol_document *doc, ol_node *node,
                        ol_unit x, ol_unit y, ol_unit width,
                        ol_measure_text_fn measure, void *measure_userdata,
                        ol_unit *bottom_out);

static int flush_flow(ol_inline_flow *flow, ol_unit *bottom)
{
    ol_unit b = flow->line_top;
    if (flow->has_content)
        b += flow->line_height > 0 ? flow->line_height : OL_CSSPX(20);
    *bottom = b;
    return 1;
}

static int layout_block(ol_document *doc, ol_node *node,
                        ol_unit x, ol_unit y, ol_unit width,
                        ol_measure_text_fn measure, void *measure_userdata,
                        ol_unit *bottom_out)
{
    ol_unit box_x, box_y, inner_x, inner_w, current_y, content_bottom;
    ol_node *child;
    ol_inline_flow flow;
    size_t background_index = (size_t)-1;

    if (!node || node->style.display == OL_DISPLAY_NONE) {
        *bottom_out = y;
        return 1;
    }

    box_x = x + node->style.margin_left;
    box_y = y + node->style.margin_top;
    inner_x = box_x + node->style.padding_left;
    inner_w = width - node->style.margin_left - node->style.margin_right
                    - node->style.padding_left - node->style.padding_right;
    if (inner_w < OL_CSSPX(1)) inner_w = OL_CSSPX(1);

    memset(&node->bounds, 0, sizeof(node->bounds));

    if ((node->style.background >> 24) != 0) {
        ol_display_op *bg = append_op(doc, OL_OP_FILL_RECT);
        if (!bg) return 0;
        background_index = doc->op_count - 1;
        bg->u.fill_rect.node_id = node->id;
        bg->u.fill_rect.argb = node->style.background;
    }

    current_y = box_y + node->style.padding_top;
    memset(&flow, 0, sizeof(flow));
    flow.doc = doc;
    flow.measure = measure;
    flow.measure_userdata = measure_userdata;
    flow.x0 = inner_x;
    flow.right = inner_x + inner_w;
    flow.cursor_x = inner_x;
    flow.line_top = current_y;

    if (node->text && node->text[0]) {
        if (!layout_inline_text(&flow, node)) return 0;
    }

    for (child = node->first_child; child; child = child->next_sibling) {
        if (child->style.display == OL_DISPLAY_NONE) continue;
        if (child->style.display == OL_DISPLAY_INLINE) {
            if (!layout_inline_node(&flow, child)) return 0;
            rect_add(&node->bounds, child->bounds);
        } else {
            ol_unit child_bottom;
            flush_flow(&flow, &current_y);
            flow.cursor_x = inner_x;
            flow.line_top = current_y;
            flow.line_height = 0;
            flow.has_content = 0;
            if (!layout_block(doc, child, inner_x, current_y, inner_w,
                              measure, measure_userdata, &child_bottom))
                return 0;
            current_y = child_bottom;
            flow.line_top = current_y;
            rect_add(&node->bounds, child->bounds);
        }
    }

    flush_flow(&flow, &content_bottom);
    if (content_bottom < current_y) content_bottom = current_y;
    content_bottom += node->style.padding_bottom;

    node->bounds.x = box_x;
    node->bounds.y = box_y;
    node->bounds.width = width - node->style.margin_left - node->style.margin_right;
    if (node->bounds.width < 0) node->bounds.width = 0;
    node->bounds.height = content_bottom - box_y;
    if (node->bounds.height < 0) node->bounds.height = 0;

    if (background_index != (size_t)-1)
        doc->ops[background_index].u.fill_rect.bounds = node->bounds;

    *bottom_out = content_bottom + node->style.margin_bottom;
    return 1;
}

int ol_layout(ol_document *doc, ol_unit viewport_width,
              ol_measure_text_fn measure, void *measure_userdata)
{
    ol_node *child;
    size_t i;
    ol_unit y = 0;

    if (!doc || viewport_width <= 0) return 0;
    doc->op_count = 0;
    doc->viewport_width = viewport_width;
    for (i = 0; i < doc->node_count; ++i)
        memset(&doc->nodes[i]->bounds, 0, sizeof(doc->nodes[i]->bounds));

    for (child = doc->root->first_child; child; child = child->next_sibling) {
        ol_unit bottom;
        if (child->style.display == OL_DISPLAY_NONE) continue;
        if (child->style.display == OL_DISPLAY_INLINE) {
            ol_inline_flow flow;
            memset(&flow, 0, sizeof(flow));
            flow.doc = doc;
            flow.measure = measure;
            flow.measure_userdata = measure_userdata;
            flow.x0 = 0;
            flow.right = viewport_width;
            flow.cursor_x = 0;
            flow.line_top = y;
            if (!layout_inline_node(&flow, child)) return 0;
            flush_flow(&flow, &y);
        } else {
            if (!layout_block(doc, child, 0, y, viewport_width,
                              measure, measure_userdata, &bottom))
                return 0;
            y = bottom;
        }
        rect_add(&doc->root->bounds, child->bounds);
    }

    doc->root->bounds.x = 0;
    doc->root->bounds.y = 0;
    doc->root->bounds.width = viewport_width;
    doc->root->bounds.height = y;
    doc->content_height = y;
    ++doc->generation;
    return 1;
}

ol_unit ol_document_content_height(const ol_document *doc)
{
    return doc ? doc->content_height : 0;
}

uint32_t ol_document_generation(const ol_document *doc)
{
    return doc ? doc->generation : 0;
}

size_t ol_display_count(const ol_document *doc)
{
    return doc ? doc->op_count : 0;
}

const ol_display_op *ol_display_get(const ol_document *doc, size_t index)
{
    if (!doc || index >= doc->op_count) return NULL;
    return &doc->ops[index];
}

const char *ol_role_name(ol_role role)
{
    switch (role) {
    case OL_ROLE_DOCUMENT: return "document";
    case OL_ROLE_BLOCK: return "block";
    case OL_ROLE_PARAGRAPH: return "paragraph";
    case OL_ROLE_HEADING: return "heading";
    case OL_ROLE_TEXT: return "text";
    case OL_ROLE_LINK: return "link";
    case OL_ROLE_IMAGE: return "image";
    case OL_ROLE_LIST: return "list";
    case OL_ROLE_LIST_ITEM: return "listitem";
    case OL_ROLE_TABLE: return "table";
    case OL_ROLE_ROW: return "row";
    case OL_ROLE_CELL: return "cell";
    case OL_ROLE_BUTTON: return "button";
    case OL_ROLE_INPUT: return "input";
    default: return "unknown";
    }
}
