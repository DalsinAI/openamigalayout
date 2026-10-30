/* OpenLayout UI on the host: markup and CSS to a laid out window, the
 * widgets' events through C callbacks, the keyboard, the theme as CSS.
 * With an argument, writes the window's draw ops there as lines of text
 * (tests/ui_render.py draws them as a picture).
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#include "openlayout_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    printf("FAIL line=%d expr=%s\n", __LINE__, #x); return 20; \
} } while (0)

/* every character 7 px wide (8 bold), ascent 10, descent 3, at 13 px */
static int measure(void *userdata, const char *utf8, size_t length,
                   const ol_style *style, ol_text_metrics *m)
{
    size_t i, glyphs = 0;
    (void)userdata;
    for (i = 0; i < length; ++i)
        if (((unsigned char)utf8[i] & 0xc0u) != 0x80u) ++glyphs;
    m->width = (ol_unit)glyphs * style->font_size * ((style->text_flags & OL_TEXT_BOLD) ? 8 : 7) / 13;
    m->ascent = style->font_size * 10 / 13;
    m->descent = style->font_size * 3 / 13;
    return 1;
}

static const char markup[] =
    "<style>#title{font-size:16px}</style>\n"
    "<h1 id=title>Mail folders</h1>\n"
    "<row>\n"
    "  <label style='width:60px'>Search</label>\n"
    "  <input id=search placeholder='Type to search' style='flex:1'>\n"
    "  <button id=go>Go</button>\n"
    "</row>\n"
    "<row style='flex:1'>\n"
    "  <tree id=folders style='width:180px;height:auto'>\n"
    "    <item id=inbox value=INBOX open>Inbox\n"
    "      <item id=work>Work</item>\n"
    "      <item id=family>Family</item>\n"
    "    </item>\n"
    "    <item id=sent>Sent</item>\n"
    "    <item id=archive>Archive<item id=y2025>2025</item><item id=y2026>2026</item></item>\n"
    "  </tree>\n"
    "  <tabs id=tabs style='flex:1'>\n"
    "    <tab title=Message id=t1>\n"
    "      <checkbox id=html checked>Show HTML</checkbox>\n"
    "      <radio name=sort id=bydate value=date checked>By date</radio>\n"
    "      <radio name=sort id=byname value=name>By sender</radio>\n"
    "      <p>Some <b>bold</b> text &amp; more.</p>\n"
    "    </tab>\n"
    "    <tab title=Settings id=t2><label>Settings page</label></tab>\n"
    "  </tabs>\n"
    "</row>\n"
    "<progress id=bar max=10></progress>\n"
    "<row style='justify-content:end'><button id=ok>OK</button><button id=cancel disabled>Cancel</button></row>\n";

static int clicks, changes, submits, selects, activates, expands, collapses, tab_selects;
static char last_value[64];

static void on_event(olui *ui, const olui_event *ev, void *userdata)
{
    (void)ui;
    (void)userdata;
    switch (ev->type) {
    case OLUI_CLICK: ++clicks; break;
    case OLUI_CHANGE: ++changes; break;
    case OLUI_SUBMIT: ++submits; break;
    case OLUI_SELECT: if (!strcmp(ev->id, "tabs")) ++tab_selects; else ++selects; break;
    case OLUI_ACTIVATE: ++activates; break;
    case OLUI_EXPAND: ++expands; break;
    case OLUI_COLLAPSE: ++collapses; break;
    default: break;
    }
    if (ev->value) {
        strncpy(last_value, ev->value, sizeof(last_value) - 1);
        last_value[sizeof(last_value) - 1] = 0;
    }
}

static int ok_clicks;
static void on_ok(olui *ui, const olui_event *ev, void *userdata)
{
    (void)ui;
    (void)ev;
    *(int *)userdata += 1;
}

typedef struct {
    FILE *out;
    int boxes, texts, marks;
    int saw_text_ok;
} sink;

static void draw(void *userdata, const olui_draw_op *op)
{
    sink *s = (sink *)userdata;
    switch (op->kind) {
    case OLUI_DRAW_BOX: {
        int i;
        ++s->boxes;
        if (s->out) {
            fprintf(s->out, "box %ld %ld %ld %ld %d %08lx %d %d", (long)op->box.x, (long)op->box.y,
                    (long)op->box.w, (long)op->box.h, op->border, (unsigned long)op->border_argb, op->radius,
                    op->stops);
            for (i = 0; i < op->stops; ++i)
                fprintf(s->out, " %08lx %d", (unsigned long)op->stop_argb[i], op->stop_pos[i]);
            fputc('\n', s->out);
        }
        break;
    }
    case OLUI_DRAW_TEXT:
        ++s->texts;
        if (op->length == 2 && !memcmp(op->text, "OK", 2)) s->saw_text_ok = 1;
        if (s->out)
            fprintf(s->out, "text %ld %ld %d %08lx %lu %.*s\n", (long)op->box.x, (long)op->baseline,
                    op->font_pixels, (unsigned long)op->argb, (unsigned long)op->text_flags,
                    (int)op->length, op->text);
        break;
    case OLUI_DRAW_MARK:
        ++s->marks;
        if (s->out)
            fprintf(s->out, "mark %ld %ld %ld %ld %d %08lx\n", (long)op->box.x, (long)op->box.y,
                    (long)op->box.w, (long)op->box.h, op->mark, (unsigned long)op->argb);
        break;
    case OLUI_DRAW_CLIP:
        if (s->out)
            fprintf(s->out, "clip %ld %ld %ld %ld\n", (long)op->clip.x, (long)op->clip.y,
                    (long)op->clip.w, (long)op->clip.h);
        break;
    default:
        break;
    }
}

static olui_rect box_of(olui *ui, const char *id);

/* The centre of an element, found by drawing and catching its box */
typedef struct { const olui_el *want; olui_rect r; int found; } finder;
static void find_box(void *userdata, const olui_draw_op *op)
{
    finder *f = (finder *)userdata;
    if (!f->found && op->el == f->want && (op->kind == OLUI_DRAW_BOX || op->kind == OLUI_DRAW_TEXT)) {
        f->r = op->box;
        f->found = 1;
    }
}

static olui_rect box_of(olui *ui, const char *id)
{
    finder f;
    memset(&f, 0, sizeof(f));
    f.want = olui_find(ui, id);
    olui_draw(ui, NULL, find_box, &f);
    return f.r;
}

static void click(olui *ui, olui_rect r)
{
    olui_mouse(ui, r.x + r.w / 2, r.y + r.h / 2, OLUI_MOUSE_MOVE);
    olui_mouse(ui, r.x + r.w / 2, r.y + r.h / 2, OLUI_MOUSE_DOWN);
    olui_mouse(ui, r.x + r.w / 2, r.y + r.h / 2, OLUI_MOUSE_UP);
}

static char *read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    long n;
    char *s;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    s = (char *)malloc((size_t)n + 1);
    if (s && fread(s, 1, (size_t)n, f) != (size_t)n) { free(s); s = NULL; }
    if (s) s[n] = 0;
    fclose(f);
    return s;
}

int main(int argc, char **argv)
{
    olui *ui = olui_new();
    char err[128];
    sink s;
    olui_el *search, *folders, *tabs, *html, *bydate, *byname, *bar, *inbox, *work, *ok, *cancel;
    olui_rect r;
    const char *theme_path = getenv("OLUI_THEME_FILE");

    CHECK(ui);
    olui_set_measure(ui, measure, NULL);
    if (theme_path) {
        char *text = read_file(theme_path), *css;
        CHECK(text);
        css = olui_theme_css(text, 0, err, sizeof(err));
        if (!css) printf("theme: %s\n", err);
        CHECK(css);
        CHECK(strstr(css, "button{background:linear-gradient("));
        CHECK(olui_add_css(ui, css));
        free(css);
        free(text);
    }
    CHECK(olui_load(ui, markup, "item{padding:2px 4px}", err, sizeof(err)));
    CHECK(olui_on(ui, NULL, 0, on_event, NULL));
    CHECK(olui_on(ui, "ok", OLUI_CLICK, on_ok, &ok_clicks));
    CHECK(olui_layout(ui, 520, 340));

    search = olui_find(ui, "search");
    folders = olui_find(ui, "folders");
    tabs = olui_find(ui, "tabs");
    html = olui_find(ui, "html");
    bydate = olui_find(ui, "bydate");
    byname = olui_find(ui, "byname");
    bar = olui_find(ui, "bar");
    inbox = olui_find(ui, "inbox");
    work = olui_find(ui, "work");
    ok = olui_find(ui, "ok");
    cancel = olui_find(ui, "cancel");
    CHECK(search && folders && tabs && html && bydate && byname && bar && inbox && work && ok && cancel);
    CHECK(!strcmp(olui_tag(folders), "tree"));
    CHECK(olui_get_checked(html) && olui_get_checked(bydate) && !olui_get_checked(byname));

    memset(&s, 0, sizeof(s));
    if (argc > 2) {
        /* the window as it opens */
        s.out = fopen(argv[2], "w");
        CHECK(s.out);
        fprintf(s.out, "size 520 340\n");
    }
    olui_draw(ui, NULL, draw, &s);
    if (s.out) fclose(s.out);
    s.out = NULL;
    CHECK(s.boxes > 10 && s.texts > 10 && s.marks >= 3);
    CHECK(s.saw_text_ok);

    /* the window is laid out to its size: OK's row is at the bottom */
    r = box_of(ui, "ok");
    CHECK(r.w > 0 && r.y + r.h <= 340 - 6 && r.y + r.h >= 340 - 8);
    CHECK(r.x + r.w < 520 - 6);                     /* justify-content:end, Cancel after it */
    /* the tree is 180 px wide and as tall as its row */
    r = box_of(ui, "folders");
    CHECK(r.w == 180 && r.h > 100);

    /* a click on OK: the handler for its id, and the one for all */
    click(ui, box_of(ui, "ok"));
    CHECK(ok_clicks == 1 && clicks == 1);
    /* a disabled button does nothing */
    click(ui, box_of(ui, "cancel"));
    CHECK(clicks == 1);

    /* the checkbox and the radio buttons */
    r = box_of(ui, "html");
    if (getenv("OLUI_DEBUG")) printf("html box %ld %ld %ld %ld checked %d changes %d\n", (long)r.x, (long)r.y, (long)r.w, (long)r.h, olui_get_checked(html), changes);
    click(ui, box_of(ui, "html"));
    if (getenv("OLUI_DEBUG")) printf("after: checked %d changes %d\n", olui_get_checked(html), changes);
    CHECK(!olui_get_checked(html) && changes == 1);
    click(ui, box_of(ui, "byname"));
    CHECK(olui_get_checked(byname) && !olui_get_checked(bydate) && !strcmp(last_value, "name"));

    /* typing in the field */
    click(ui, box_of(ui, "search"));
    CHECK(olui_key(ui, 'h', 0) && olui_key(ui, 'i', 0) && olui_key(ui, 0xe9, 0));
    CHECK(!strcmp(olui_get_text(search), "hi\xc3\xa9"));
    CHECK(olui_key(ui, OLUI_KEY_BACKSPACE, 0));
    CHECK(olui_key(ui, OLUI_KEY_HOME, 0) && olui_key(ui, '>', 0));
    CHECK(!strcmp(olui_get_text(search), ">hi"));
    CHECK(olui_key(ui, OLUI_KEY_RETURN, 0) && submits == 1 && !strcmp(last_value, ">hi"));

    /* the tree: a click selects, the arrow keys move and open */
    click(ui, box_of(ui, "work"));
    CHECK(olui_list_selected(folders) == work && selects == 1);
    CHECK(olui_key(ui, OLUI_KEY_DOWN, 0));
    CHECK(olui_list_selected(folders) == olui_find(ui, "family"));
    CHECK(olui_key(ui, OLUI_KEY_DOWN, 0) && olui_key(ui, OLUI_KEY_DOWN, 0));
    CHECK(olui_list_selected(folders) == olui_find(ui, "archive"));
    CHECK(!olui_item_is_open(olui_find(ui, "archive")));
    CHECK(olui_key(ui, OLUI_KEY_RIGHT, 0) && olui_item_is_open(olui_find(ui, "archive")) && expands == 1);
    CHECK(olui_key(ui, OLUI_KEY_DOWN, 0) && olui_list_selected(folders) == olui_find(ui, "y2025"));
    CHECK(olui_key(ui, OLUI_KEY_LEFT, 0) && olui_list_selected(folders) == olui_find(ui, "archive"));
    CHECK(olui_key(ui, OLUI_KEY_LEFT, 0) && collapses == 1);
    CHECK(olui_key(ui, OLUI_KEY_RETURN, 0) && activates == 1 && !strcmp(last_value, "archive"));
    /* closing the inbox with the program's call; its selected child goes to it */
    olui_list_select(ui, folders, work);
    olui_item_open(ui, inbox, 0);
    CHECK(olui_list_selected(folders) == inbox);
    /* items added by the program */
    {
        olui_el *more = olui_list_add(ui, folders, NULL, "Drafts", "DRAFTS");
        CHECK(more && olui_get_value(more) && !strcmp(olui_get_value(more), "DRAFTS"));
        olui_list_select(ui, folders, more);
        CHECK(olui_list_selected(folders) == more);
    }
    /* many items: the list scrolls to keep the selected one in view */
    {
        int i;
        olui_el *last = NULL;
        char name[24];
        for (i = 0; i < 40; ++i) {
            sprintf(name, "Folder %d", i);
            last = olui_list_add(ui, folders, NULL, name, NULL);
            CHECK(last);
        }
        olui_list_select(ui, folders, last);
        r = box_of(ui, "folders");
        {
            finder f;
            memset(&f, 0, sizeof(f));
            f.want = last;
            olui_draw(ui, NULL, find_box, &f);
            CHECK(f.found && f.r.y >= r.y && f.r.y + f.r.h <= r.y + r.h);
        }
    }

    /* the tabs: a click on the second's button shows its page */
    CHECK(olui_tabs_current(tabs) == 0);
    {
        finder f;
        memset(&f, 0, sizeof(f));
        f.want = NULL;
        /* the strip's second button: Tab from the tree's focus reaches it */
        CHECK(olui_key(ui, OLUI_KEY_TAB, 0));
        CHECK(olui_key(ui, OLUI_KEY_TAB, 0));
        CHECK(olui_key(ui, OLUI_KEY_RIGHT, 0));
        CHECK(olui_tabs_current(tabs) == 1 && tab_selects == 1);
        olui_tabs_select(ui, tabs, 0);
    }

    olui_set_progress(ui, bar, 7);
    CHECK(olui_damaged(ui, &r) && r.w == 520);

    /* a bad markup says where */
    {
        olui *bad = olui_new();
        CHECK(bad && !olui_load(bad, "<row><button>x</row>", NULL, err, sizeof(err)));
        CHECK(strstr(err, "line 1"));
        olui_free(bad);
    }

    if (argc > 1) {
        memset(&s, 0, sizeof(s));
        s.out = fopen(argv[1], "w");
        CHECK(s.out);
        fprintf(s.out, "size 520 340\n");
        olui_draw(ui, NULL, draw, &s);
        fclose(s.out);
    }

    olui_free(ui);
    printf("OLUI PASS version=%s\n", OLUI_VERSION);
    return 0;
}
