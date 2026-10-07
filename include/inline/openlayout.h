#ifndef INLINE_OPENLAYOUT_H
#define INLINE_OPENLAYOUT_H
#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif
#ifndef OPENLAYOUT_BASE_NAME
#define OPENLAYOUT_BASE_NAME OpenLayoutBase
#endif

#define OL_Version() LP0(0x1e, ULONG, OL_Version, , OPENLAYOUT_BASE_NAME)
#define OL_VersionString() LP0(0x24, CONST_STRPTR, OL_VersionString, , OPENLAYOUT_BASE_NAME)
#define OL_DocumentNew() LP0(0x2a, ol_document *, OL_DocumentNew, , OPENLAYOUT_BASE_NAME)
#define OL_DocumentFree(doc) LP1NR(0x30, OL_DocumentFree, ol_document *, doc, a0, , OPENLAYOUT_BASE_NAME)
#define OL_DocumentRoot(doc) LP1(0x36, ol_node *, OL_DocumentRoot, ol_document *, doc, a0, , OPENLAYOUT_BASE_NAME)
#define OL_NodeAppend(req) LP1(0x3c, ol_node *, OL_NodeAppend, struct OLNodeAppend *, req, a0, , OPENLAYOUT_BASE_NAME)
#define OL_NodeID(node) LP1(0x42, ULONG, OL_NodeID, const ol_node *, node, a0, , OPENLAYOUT_BASE_NAME)
#define OL_NodeSetText(req) LP1(0x48, BOOL, OL_NodeSetText, struct OLNodeString *, req, a0, , OPENLAYOUT_BASE_NAME)
#define OL_NodeSetName(req) LP1(0x4e, BOOL, OL_NodeSetName, struct OLNodeString *, req, a0, , OPENLAYOUT_BASE_NAME)
#define OL_NodeSetValue(req) LP1(0x54, BOOL, OL_NodeSetValue, struct OLNodeString *, req, a0, , OPENLAYOUT_BASE_NAME)
#define OL_NodeSetHref(req) LP1(0x5a, BOOL, OL_NodeSetHref, struct OLNodeString *, req, a0, , OPENLAYOUT_BASE_NAME)
#define OL_NodeSetStyle(req) LP1NR(0x60, OL_NodeSetStyle, struct OLNodeStyle *, req, a0, , OPENLAYOUT_BASE_NAME)
#define OL_NodeSetActions(node,actions) LP2NR(0x66, OL_NodeSetActions, ol_node *, node, a0, ULONG, actions, d0, , OPENLAYOUT_BASE_NAME)
#define OL_NodeActions(node) LP1(0x6c, ULONG, OL_NodeActions, const ol_node *, node, a0, , OPENLAYOUT_BASE_NAME)
#define OL_NodeSetImage(req) LP1NR(0x72, OL_NodeSetImage, struct OLNodeImage *, req, a0, , OPENLAYOUT_BASE_NAME)
#define OL_NodeHref(node) LP1(0x78, CONST_STRPTR, OL_NodeHref, const ol_node *, node, a0, , OPENLAYOUT_BASE_NAME)
#define OL_NodeBounds(req) LP1(0x7e, BOOL, OL_NodeBounds, struct OLNodeBounds *, req, a0, , OPENLAYOUT_BASE_NAME)
#define OL_FindRoleName(req) LP1(0x84, const ol_node *, OL_FindRoleName, struct OLFindRoleName *, req, a0, , OPENLAYOUT_BASE_NAME)
#define OL_HitAction(req) LP1(0x8a, const ol_node *, OL_HitAction, struct OLHitAction *, req, a0, , OPENLAYOUT_BASE_NAME)
#define OL_Layout(req) LP1(0x90, BOOL, OL_Layout, struct OLLayoutRequest *, req, a0, , OPENLAYOUT_BASE_NAME)
#define OL_DocumentContentHeight(doc) LP1(0x96, LONG, OL_DocumentContentHeight, const ol_document *, doc, a0, , OPENLAYOUT_BASE_NAME)
#define OL_DocumentGeneration(doc) LP1(0x9c, ULONG, OL_DocumentGeneration, const ol_document *, doc, a0, , OPENLAYOUT_BASE_NAME)
#define OL_DisplayCount(doc) LP1(0xa2, ULONG, OL_DisplayCount, const ol_document *, doc, a0, , OPENLAYOUT_BASE_NAME)
#define OL_DisplayGet(doc,index) LP2(0xa8, const ol_display_op *, OL_DisplayGet, const ol_document *, doc, a0, ULONG, index, d0, , OPENLAYOUT_BASE_NAME)
#define OL_StyleInit(style) LP1NR(0xae, OL_StyleInit, ol_style *, style, a0, , OPENLAYOUT_BASE_NAME)
#define OL_NodeByID(doc,id) LP2(0xb4, const ol_node *, OL_NodeByID, const ol_document *, doc, a0, ULONG, id, d0, , OPENLAYOUT_BASE_NAME)
#define OL_NodeRole(node) LP1(0xba, ULONG, OL_NodeRole, const ol_node *, node, a0, , OPENLAYOUT_BASE_NAME)
#define OL_NodeText(node) LP1(0xc0, CONST_STRPTR, OL_NodeText, const ol_node *, node, a0, , OPENLAYOUT_BASE_NAME)
#define OL_NodeName(node) LP1(0xc6, CONST_STRPTR, OL_NodeName, const ol_node *, node, a0, , OPENLAYOUT_BASE_NAME)
#define OL_NodeValue(node) LP1(0xcc, CONST_STRPTR, OL_NodeValue, const ol_node *, node, a0, , OPENLAYOUT_BASE_NAME)

#endif
