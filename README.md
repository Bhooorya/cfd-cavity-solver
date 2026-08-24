# CFD Cavity Solver

An interactive lid-driven cavity flow simulator. The steady-state incompressible
Navier–Stokes equations are solved in the **stream function–vorticity formulation**,
with the numerical core written in C++ and compiled to WebAssembly via Emscripten so
the whole solver runs in the browser.

## Method

- **Formulation:** stream function–vorticity (ψ–ω), which eliminates the pressure term
  from the momentum equations and enforces incompressibility by construction.
- **Discretization:** second-order central finite differences on a uniform grid.
- **Solver:** successive over-relaxation (SOR) with a tunable relaxation factor.
- **Pressure:** recovered in a separate post-processing Poisson solve.

Adjustable parameters: grid size, cavity aspect ratio, Reynolds number, and the SOR
relaxation factor.

## Running it

You need Python (for a local static server) — WebAssembly will not load from a
`file://` URL, so the page must be served over HTTP.

**Windows**

```bash
launcher-windows.bat
```

**macOS / Linux**

```bash
./launcher-mac.sh
```

Either script serves the folder and opens `project-webpage.html` in your browser.
Keep the terminal window open while using the app; Ctrl+C stops it.

To use a different port, pass it as an argument:

```bash
launcher-windows.bat 8080
```

## Rebuilding the WebAssembly module

`solver.js` and `solver.wasm` are committed, so you only need this if you change
`project-code.cpp`. Requires the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html).

```bash
emcc project-code.cpp -O3 --bind -s WASM=1 -s MODULARIZE=1 -s EXPORT_NAME=createCavityModule -s ALLOW_MEMORY_GROWTH=1 -o solver.js
```

On Windows, `run-cfd.bat` does the build, then serves and opens the page in one step
(it expects emsdk at `%USERPROFILE%\emsdk`).

## Files

| File | Purpose |
| --- | --- |
| `project-code.cpp` | C++ solver core (`CavitySolver`), exposed through Embind |
| `project-webpage.html` | Web UI — controls, plots, side-by-side comparison view |
| `solver.js`, `solver.wasm` | Prebuilt Emscripten output |
| `launcher-windows.bat`, `launcher-mac.sh` | Serve the folder and open the app |
| `run-cfd.bat` | Rebuild the WASM module, then serve and open (Windows) |
