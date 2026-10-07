#ifndef LIBRARIES_OPENLAYOUT_H
#define LIBRARIES_OPENLAYOUT_H

#include <exec/types.h>
#include <openlayout.h>

#define OPENLAYOUTLIB_NAME "openlayout.library"
#define OPENLAYOUTLIB_VERSION 2
#define OPENLAYOUTLIB_REVISION 0

struct OLNodeAppend {
    ol_document *document;
    ol_node *parent;
    ULONG role;
    CONST_STRPTR text;
};

struct OLNodeString {
    ol_node *node;
    CONST_STRPTR text;
};

struct OLNodeStyle {
    ol_node *node;
    const ol_style *style;
};

struct OLNodeImage {
    ol_node *node;
    LONG image_id;
    ol_unit width;
    ol_unit height;
};

struct OLLayoutRequest {
    ol_document *document;
    ol_unit viewport_width;
    ol_measure_text_fn measure;
    APTR measure_userdata;
};

struct OLFindRoleName {
    const ol_document *document;
    ULONG role;
    CONST_STRPTR name;
};

struct OLHitAction {
    const ol_document *document;
    ol_unit x;
    ol_unit y;
    ULONG action;
};

struct OLNodeBounds {
    const ol_node *node;
    ol_rect *bounds;
};

#endif
