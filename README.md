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

Amiga static target:

```sh
./build-amiga.sh
```

Resident Amiga library and ABI test:

```sh
./build-library.sh
```

The resident build produces `build-amiga/lib/openlayout.library` plus the public `libraries/`, `proto/` and `inline/` headers. It uses a process-free Exec allocation runtime so documents owned by the resident library do not depend on a caller's libc startup.

The first OS 3.2.3 / AC090 68040 guest qualification opened the library from `LIBS:` and passed document creation, layout, display-list access, semantic lookup, geometry and actionable-link hit testing.

The Amiga builds use the AmigaChrome os32-gcc16 stove by default.

## Geometry

- 1 point = 80 OpenLayout units
- 1 CSS pixel = 60 units
- 1 twip = 4 units
- 1 inch = 5760 units

See `DESIGN.md` and `docs/LIGHTWEIGHT_BROWSER.md`.

## Licence

MIT. Copyright (c) 2026 Dalsin Limited.
