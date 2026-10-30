/* OpenLayout UI in an Intuition window: draws olui's ops through
 * graphics.library (OpenGfx), true-colour boxes with cybergraphics'
 * FillPixelArray as OpenGadTools does, text in the screen's font at each
 * size through diskfont.library, all into a buffer the size of the window's
 * inside that is copied to the window when it changes; and turns the
 * window's IDCMP messages into olui_mouse and olui_key.
 *
 *     struct Window *w = OpenWindowTags(..., WA_IDCMP, olui_amiga_idcmp(), WA_ReportMouse, TRUE, ...);
 *     olui_amiga *a = olui_amiga_new(ui, w);
 *     olui_amiga_theme(a, NULL);              (the theme OpenPrefs Look chose)
 *     olui_amiga_layout(a);
 *     for (;;) { WaitPort(w->UserPort); while ((m = GT_GetIMsg / GetMsg(...))) {
 *         int r = olui_amiga_event(a, m); ReplyMsg(m); if (r == OLUI_AMIGA_CLOSE) ...; }
 *         olui_amiga_draw(a); }
 *
 * MIT, Copyright (c) 2026 AmigaChrome Team. */
#ifndef OPENLAYOUT_UI_AMIGA_H
#define OPENLAYOUT_UI_AMIGA_H

#include <exec/types.h>
#include "openlayout_ui.h"

struct Window;
struct IntuiMessage;

typedef struct olui_amiga olui_amiga;

/* What olui_amiga_event saw */
enum {
    OLUI_AMIGA_NONE = 0,
    OLUI_AMIGA_USED,        /* olui took it (its handlers have run) */
    OLUI_AMIGA_CLOSE        /* the close gadget */
};

/* The window's IDCMP for olui: the mouse, keys, size, refresh and close. */
ULONG olui_amiga_idcmp(void);

/* Draws ui in win, measuring with the window's screen font (olui_set_measure
 * is set here). NULL when memory or a library is missing. */
olui_amiga *olui_amiga_new(olui *ui, struct Window *win);
void olui_amiga_free(olui_amiga *a);

/* The theme as CSS before the program's own: a theme file's path, or NULL
 * for the one OpenPrefs Look chose (ENV:OpenGadTools/Look, from
 * SYS:Prefs/Presets/Themes). 1 when one was read. */
int olui_amiga_theme(olui_amiga *a, const char *path);

/* Lays out at the window's inner size, and draws it all. */
int olui_amiga_layout(olui_amiga *a);
/* Draws what changed, if anything did. */
void olui_amiga_draw(olui_amiga *a);
/* One IDCMP message, before it is replied. */
int olui_amiga_event(olui_amiga *a, struct IntuiMessage *msg);

#endif
