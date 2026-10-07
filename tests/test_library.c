#include <exec/types.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <proto/openlayout.h>

#include <stdio.h>
#include <string.h>

struct Library *OpenLayoutBase;

#define CHECK(x) do { if (!(x)) {     printf("LAYOUTLIB FAIL line=%d expr=%s\n", __LINE__, #x); rc = 20; goto done; } } while (0)

int main(void)
{
    int rc = 0;
    ol_document *doc = NULL;
    ol_node *root, *p, *text, *link;
    struct OLNodeAppend append;
    struct OLNodeStyle style_req;
    struct OLNodeString string_req;
    struct OLLayoutRequest layout_req;
    struct OLFindRoleName find_req;
    struct OLNodeBounds bounds_req;
    struct OLHitAction hit_req;
    ol_style style;
    ol_rect box;
    const ol_node *found, *hit;

    OpenLayoutBase = OpenLibrary((CONST_STRPTR)OPENLAYOUTLIB_NAME, OPENLAYOUTLIB_VERSION);
    CHECK(OpenLayoutBase != NULL);
    CHECK(OL_Version() == 1);
    CHECK(!strcmp((const char *)OL_VersionString(), "0.1"));

    doc = OL_DocumentNew();
    CHECK(doc != NULL);
    root = OL_DocumentRoot(doc);
    CHECK(root != NULL);

    append.document = doc;
    append.parent = root;
    append.role = OL_ROLE_PARAGRAPH;
    append.text = NULL;
    p = OL_NodeAppend(&append);
    CHECK(p != NULL);

    OL_StyleInit(&style);
    style.display = OL_DISPLAY_BLOCK;
    style.margin_bottom = OL_CSSPX(8);
    style_req.node = p;
    style_req.style = &style;
    OL_NodeSetStyle(&style_req);

    append.parent = p;
    append.role = OL_ROLE_TEXT;
    append.text = (CONST_STRPTR)"Resident OpenLayout can lay out ";
    text = OL_NodeAppend(&append);
    CHECK(text != NULL);
    style.display = OL_DISPLAY_INLINE;
    style_req.node = text;
    OL_NodeSetStyle(&style_req);

    append.role = OL_ROLE_LINK;
    append.text = (CONST_STRPTR)"this link";
    link = OL_NodeAppend(&append);
    CHECK(link != NULL);
    style_req.node = link;
    OL_NodeSetStyle(&style_req);

    string_req.node = link;
    string_req.text = (CONST_STRPTR)"example-link";
    CHECK(OL_NodeSetName(&string_req));
    string_req.text = (CONST_STRPTR)"https://example.com/";
    CHECK(OL_NodeSetHref(&string_req));
    OL_NodeSetActions(link, OL_ACTION_ACTIVATE);

    layout_req.document = doc;
    layout_req.viewport_width = OL_CSSPX(240);
    layout_req.measure = NULL;
    layout_req.measure_userdata = NULL;
    CHECK(OL_Layout(&layout_req));
    CHECK(OL_DocumentContentHeight(doc) > 0);
    CHECK(OL_DocumentGeneration(doc) == 1);
    CHECK(OL_DisplayCount(doc) >= 2);

    find_req.document = doc;
    find_req.role = OL_ROLE_LINK;
    find_req.name = (CONST_STRPTR)"example-link";
    found = OL_FindRoleName(&find_req);
    CHECK(found != NULL);
    CHECK(OL_NodeID(found) == OL_NodeID(link));
    CHECK(!strcmp((const char *)OL_NodeHref(found), "https://example.com/"));

    bounds_req.node = found;
    bounds_req.bounds = &box;
    CHECK(OL_NodeBounds(&bounds_req));
    CHECK(box.width > 0 && box.height > 0);

    hit_req.document = doc;
    hit_req.x = box.x + 1;
    hit_req.y = box.y + 1;
    hit_req.action = OL_ACTION_ACTIVATE;
    hit = OL_HitAction(&hit_req);
    CHECK(hit != NULL);
    CHECK(OL_NodeID(hit) == OL_NodeID(link));

    printf("LAYOUTLIB PASS version=%s height=%ld ops=%lu link=%lu\n",
           (const char *)OL_VersionString(),
           (long)(OL_DocumentContentHeight(doc) / OL_UNITS_PER_CSSPX),
           (unsigned long)OL_DisplayCount(doc),
           (unsigned long)OL_NodeID(link));

done:
    if (doc) OL_DocumentFree(doc);
    if (OpenLayoutBase) CloseLibrary(OpenLayoutBase);
    return rc;
}
