# OpenLayout

OpenLayout is the small semantic layout and display-list engine shared by the Open Amiga applications.

It is intentionally independent of OpenWrite document formats and HTML parsing. Consumers provide a semantic tree and computed styles; OpenLayout provides deterministic geometry, stable object IDs, hit-testing and a renderer-neutral display list.

## First light

The 0.1 foundation includes:

- exact units shared by CSS pixels, print points and OpenWrite twips;
- stable semantic IDs;
- block/inline flow and word wrapping;
- text measurement callback;
- image nodes;
- background rectangles;
- display list;
- semantic bounds;
- role/name lookup;
- hit testing;
- headless host tests;
- AmigaOS 3.x 68040 + FPU cross-build.

## Build and test

Host:

```sh
make test
```

Amiga target:

```sh
./build-amiga.sh
```

The Amiga build uses the AmigaChrome os32-gcc16 stove by default.

## Geometry

- 1 point = 80 OpenLayout units
- 1 CSS pixel = 60 units
- 1 twip = 4 units
- 1 inch = 5760 units

See `DESIGN.md` and `docs/LIGHTWEIGHT_BROWSER.md`.

## Licence

MIT. Copyright (c) 2026 Dalsin Limited.
