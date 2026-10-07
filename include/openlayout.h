#ifndef OPENLAYOUT_H
#define OPENLAYOUT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OL_VERSION "0.1"

/*
 * OpenLayout logical units are exact across the three worlds we care about:
 *   1 point     = 80 OL units
 *   1 CSS pixel = 60 OL units (96 CSS px/in)
 *   1 twip      = 4 OL units (20 twips/point)
 *   1 inch      = 5760 OL units
 */
typedef int32_t ol_unit;
#define OL_UNITS_PER_POINT 80
#define OL_UNITS_PER_CSSPX 60
#define OL_UNITS_PER_TWIP 4
#define OL_UNITS_PER_INCH 5760

#define OL_POINT(v) ((ol_unit)((v) * OL_UNITS_PER_POINT))
#define OL_CSSPX(v) ((ol_unit)((v) * OL_UNITS_PER_CSSPX))
#define OL_TWIP(v) ((ol_unit)((v) * OL_UNITS_PER_TWIP))

typedef uint32_t ol_id;

typedef struct ol_document ol_document;
typedef struct ol_node ol_node;

typedef struct {
    ol_unit x;
    ol_unit y;
    ol_unit width;
    ol_unit height;
} ol_rect;

typedef enum {
    OL_ROLE_DOCUMENT = 0,
    OL_ROLE_BLOCK,
    OL_ROLE_PARAGRAPH,
    OL_ROLE_HEADING,
    OL_ROLE_TEXT,
    OL_ROLE_LINK,
    OL_ROLE_IMAGE,
    OL_ROLE_LIST,
    OL_ROLE_LIST_ITEM,
    OL_ROLE_TABLE,
    OL_ROLE_ROW,
    OL_ROLE_CELL,
    OL_ROLE_BUTTON,
    OL_ROLE_INPUT
} ol_role;

typedef enum {
    OL_DISPLAY_BLOCK = 0,
    OL_DISPLAY_INLINE,
    OL_DISPLAY_NONE
} ol_display;

typedef enum {
    OL_ALIGN_START = 0,
    OL_ALIGN_CENTER,
    OL_ALIGN_END
} ol_text_align;

enum {
    OL_TEXT_BOLD      = 1u << 0,
    OL_TEXT_ITALIC    = 1u << 1,
    OL_TEXT_UNDERLINE = 1u << 2,
    OL_TEXT_MONO      = 1u << 3
};

enum {
    OL_ACTION_ACTIVATE = 1u << 0,
    OL_ACTION_FOCUS    = 1u << 1,
    OL_ACTION_EDIT     = 1u << 2,
    OL_ACTION_SELECT   = 1u << 3
};

enum {
    OL_STATE_DISABLED = 1u << 0,
    OL_STATE_CHECKED  = 1u << 1,
    OL_STATE_SELECTED = 1u << 2,
    OL_STATE_FOCUSED  = 1u << 3
};

typedef struct {
    ol_display display;
    ol_text_align text_align;

    ol_unit margin_top;
    ol_unit margin_right;
    ol_unit margin_bottom;
    ol_unit margin_left;

    ol_unit padding_top;
    ol_unit padding_right;
    ol_unit padding_bottom;
    ol_unit padding_left;

    ol_unit font_size;
    ol_unit line_height;

    uint32_t foreground;  /* AARRGGBB */
    uint32_t background;  /* AARRGGBB, alpha 0 means transparent */
    uint32_t text_flags;
} ol_style;

typedef struct {
    ol_unit width;
    ol_unit ascent;
    ol_unit descent;
} ol_text_metrics;

typedef int (*ol_measure_text_fn)(void *userdata,
                                  const char *utf8,
                                  size_t length,
                                  const ol_style *style,
                                  ol_text_metrics *metrics);

typedef enum {
    OL_OP_TEXT = 1,
    OL_OP_FILL_RECT,
    OL_OP_IMAGE,
    OL_OP_RULE
} ol_op_type;

typedef struct {
    ol_id node_id;
    ol_rect bounds;
    ol_unit baseline;
    size_t text_offset;
    size_t text_length;
    ol_style style;
} ol_text_op;

typedef struct {
    ol_id node_id;
    ol_rect bounds;
    uint32_t argb;
} ol_fill_rect_op;

typedef struct {
    ol_id node_id;
    ol_rect bounds;
    int image_id;
} ol_image_op;

typedef struct {
    ol_id node_id;
    ol_unit x1, y1, x2, y2;
    ol_unit thickness;
    uint32_t argb;
} ol_rule_op;

typedef struct {
    ol_op_type type;
    union {
        ol_text_op text;
        ol_fill_rect_op fill_rect;
        ol_image_op image;
        ol_rule_op rule;
    } u;
} ol_display_op;

ol_document *ol_document_new(void);
void ol_document_free(ol_document *doc);
ol_node *ol_document_root(ol_document *doc);
const ol_node *ol_document_root_const(const ol_document *doc);

void ol_style_init(ol_style *style);
ol_node *ol_node_append(ol_document *doc, ol_node *parent, ol_role role,
                        const char *utf8);
ol_id ol_node_id(const ol_node *node);
ol_role ol_node_role(const ol_node *node);
const char *ol_node_text(const ol_node *node);
const char *ol_node_name(const ol_node *node);
const char *ol_node_value(const ol_node *node);
const char *ol_node_href(const ol_node *node);
const ol_style *ol_node_style(const ol_node *node);
ol_rect ol_node_bounds(const ol_node *node);

int ol_node_set_text(ol_node *node, const char *utf8);
int ol_node_set_name(ol_node *node, const char *utf8);
int ol_node_set_value(ol_node *node, const char *utf8);
int ol_node_set_href(ol_node *node, const char *utf8);
void ol_node_set_style(ol_node *node, const ol_style *style);
void ol_node_set_actions(ol_node *node, uint32_t actions);
void ol_node_set_states(ol_node *node, uint32_t states);
uint32_t ol_node_actions(const ol_node *node);
uint32_t ol_node_states(const ol_node *node);
void ol_node_set_image(ol_node *node, int image_id, ol_unit width, ol_unit height);

const ol_node *ol_node_by_id(const ol_document *doc, ol_id id);
const ol_node *ol_find_role_name(const ol_document *doc, ol_role role,
                                 const char *name);
const ol_node *ol_hit_test(const ol_document *doc, ol_unit x, ol_unit y);

int ol_layout(ol_document *doc, ol_unit viewport_width,
              ol_measure_text_fn measure, void *measure_userdata);
ol_unit ol_document_content_height(const ol_document *doc);
uint32_t ol_document_generation(const ol_document *doc);

size_t ol_display_count(const ol_document *doc);
const ol_display_op *ol_display_get(const ol_document *doc, size_t index);

const char *ol_role_name(ol_role role);

#ifdef __cplusplus
}
#endif
#endif
