# OpenLayout Design

Version 0.1 — 7 October 2026

OpenLayout is the small shared layout and semantic-display engine for the Open Amiga applications. It is deliberately not a browser engine and deliberately not an OpenWrite document model.

Its job is:

```text
semantic/content tree
        |
computed styles
        |
line breaking + inline flow
        |
block placement
        |
pagination or viewport
        |
display list + semantic bounds
        |
renderer (pixman.library / printer / PDF / other)
```

## 1. Design goals

OpenLayout must provide one deterministic geometry engine for:

- OpenWrite screen, PDF and print layout;
- static HTML and html.datatype;
- the lightweight OpenBrowser path;
- future OpenPage/DTP work;
- headless rendering and automation.

It must remain small enough to be an ordinary Amiga library/static library, not a second WebCore.

The first build is pure C99, has no external dependencies, and cross-compiles for the AmigaChrome 68040 + FPU target.

## 2. Exact geometry

OpenLayout uses signed 32-bit logical units.

The unit scale is chosen so all three existing coordinate systems convert exactly:

- 1 point = 80 OpenLayout units;
- 1 CSS pixel = 60 OpenLayout units;
- 1 twip = 4 OpenLayout units;
- 1 inch = 5760 OpenLayout units.

This is intentional. OpenWrite's twips, browser CSS pixels and print/PDF points can all use integer conversion with no cumulative rounding drift.

Rasterisation is the presentation boundary. Layout is not recomputed merely because a different screen DPI or output device is used.

## 3. Semantic tree

Every node has a stable 32-bit ID that survives reflow.

A node carries:

- role;
- parent/children relationship;
- optional text;
- optional semantic name;
- optional value;
- optional link target;
- state bits;
- action bits;
- computed style;
- post-layout bounds.

Roles begin with document, block, paragraph, heading, text, link, image, list, list item, table, row, cell, button and input.

The tree is intentionally richer than a paint tree. Automation, hit-testing, accessibility-like inspection and UI actions use the same identities that generated the pixels.

## 4. Stable IDs and automation

A relayout may move a node but does not change its ID.

This gives OpenAutomation a reliable object model. The intended higher-level API is Playwright-inspired rather than DOM-dependent:

- find by stable ID;
- find by role + name;
- read bounds, text, value and state;
- hit-test a coordinate;
- enumerate children;
- activate/focus/edit through the consumer application's action callback;
- take a semantic snapshot;
- render headlessly.

OpenLayout itself does not synthesize mouse clicks or mutate an HTML DOM. It identifies the semantic object and the owning adapter performs the action.

## 5. Styles

OpenLayout consumes computed styles. It does not parse CSS.

The first style structure covers:

- block / inline / none;
- margins;
- padding;
- font size;
- line height;
- foreground/background;
- bold/italic/underline/mono flags;
- simple text alignment.

An HTML adapter may implement CSS. OpenWrite may derive styles from libowf. Both hand the same computed style to OpenLayout.

This separation avoids teaching a word processor about CSS selectors or teaching a browser about ODT styles.

## 6. Text

Font ownership stays outside OpenLayout.

The layout call accepts a text-measurement callback. A consumer may back that with:

- FreeType + HarfBuzz;
- Amiga graphics/diskfont;
- a printer font provider;
- a deterministic test provider.

Display-list text operations retain the semantic node ID and UTF-8 byte range. A renderer can shape/rasterise the same run while hit-testing and automation still refer to the originating semantic object.

The first build wraps on whitespace and supports explicit newlines. Future work adds shaped cluster boundaries, tabs, justification, bidi and language-aware line breaking without changing the tree or display-list identity model.

## 7. Display list

Layout produces a retained display list. Initial operation types are:

- text run;
- fill rectangle;
- image;
- rule.

Planned operations include clip push/pop, transformed image, glyph run and path primitives where evidence requires them.

The display list is the rendering boundary.

For AmigaChrome the preferred raster path is:

```text
OpenLayout display list
        |
OpenAmigaPixman / pixman.library
        |
68040 fast paths / AC090 AC_MAGIC / OpenGPU
        |
OpenRTG or presentation buffer
```

Cairo is not required by OpenLayout.

## 8. Headless operation

No screen, intuition object or browser window is required.

A headless caller can:

1. build a semantic tree;
2. layout at a chosen width/page geometry;
3. inspect semantic bounds;
4. inspect or serialise the display list;
5. render to an offscreen target;
6. drive semantic actions through the owning adapter.

This is the basis for deterministic regression tests and OpenAutomation.

## 9. Incremental reflow

The public identity model is designed for incremental reflow from the start.

Version 0.1 currently performs a full relayout. Later versions will add dirty-node generations and nearest-layout-root invalidation. Stable IDs and retained semantic nodes mean this optimisation does not change consumers.

Correctness comes before incremental complexity.

## 10. OpenWrite relationship

OpenWrite 1.0 is the regression baseline.

OpenWrite already has:

- deterministic layout entry point;
- page geometry;
- renderer callbacks for text, rules and images;
- run identity used by selection/caret/hit testing.

The extraction path is:

1. represent OpenWrite paragraphs/runs/tables/images as OpenLayout semantic nodes;
2. map libowf formatting to computed OpenLayout styles;
3. replace the direct renderer walk with an OpenLayout display list;
4. compare geometry and output against OpenWrite 1.0;
5. move selection/caret hit-testing to stable OpenLayout IDs.

OpenWrite remains the editor and document owner. It does not become an HTML application.

## 11. HTML relationship

HTML is an adapter, not OpenLayout's native document format.

The static HTML path is expected to support an HTML 4 / CSS 2-ish useful subset first. It builds a semantic tree, computes styles and hands that tree to OpenLayout.

OpenLayout does not implement:

- networking;
- JavaScript;
- DOM mutation;
- browser security;
- fetch/WebSockets;
- Canvas/WebGL/WebGPU;
- Grid/Flexbox;
- media.

Those remain the full OpenBrowser/WebKit lane when required.

## 12. First build

The 0.1 foundation implements:

- exact point/CSS-pixel/twip units;
- stable semantic IDs;
- block and inline nodes;
- whitespace wrapping and explicit line breaks;
- images as inline display objects;
- backgrounds;
- display-list generation;
- role/name lookup;
- semantic bounds;
- hit testing;
- generation counter;
- host regression test;
- m68k 68040 + FPU cross-build.

The first m68k build is deliberately tiny; size is part of the regression budget.

## 13. Next milestones

### L0 — Foundation
Current 0.1 core and tests.

### L1 — OpenWrite adapter
Reproduce OpenWrite 1.0 paragraph/run geometry and display output without changing document semantics.

### L2 — HTML Lite adapter
HTML parser + small CSS cascade producing OpenLayout nodes and styles. Images remain external handles decoded by datatypes.

### L3 — Pixman renderer
Render the display list through pixman.library with deterministic pixel regression tests.

### L4 — Automation
Semantic snapshots, role/name selectors, action dispatch and headless page tests.

### L5 — Advanced layout
Tables, lists, floats where needed, pagination, tabs, shaped cluster line breaking and incremental reflow.

The governing rule is: add layout capability because a real Open app needs it, not because a web standard contains it.
