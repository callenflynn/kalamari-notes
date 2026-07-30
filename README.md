# Kalamari Notes

A fast, cross-platform markdown notebook built with **Tauri**, **React**, and **TypeScript**.

## Features

- Vault-based markdown note taking
- Split-pane editor with live Markdown preview
- Wiki-links (`[[Note]]`) and backlinks
- Interactive graph view of note relationships
- Daily notes and quick search
- Plugin API for commands and panels
- Light / dark theme

## Prerequisites

- [Node.js](https://nodejs.org/) 20+
- [Rust](https://www.rust-lang.org/tools/install)
- (Linux only) `libgtk-3-dev`, `libwebkit2gtk-4.0-dev`, `libappindicator3-dev`, `librsvg2-dev`, `patchelf`

## Development

```bash
cd tauri
npm install
npm run tauri dev
```

## Build

```bash
cd tauri
npm run tauri build
```

Build artifacts appear in `tauri/src-tauri/target/release/bundle/`.

## Release

Pushes and pull requests to `main` trigger `.github/workflows/build-tauri.yml`, which builds for Linux, macOS, and Windows.

To create a manual release, push a tag:

```bash
git tag v0.1.0
git push origin v0.1.0
```
