# Yocto build environment

## Upstream baseline

- **Poky branch:** `scarthgap`
- **Poky commit:** `77d1feb37e280733684ae8a9449fb031d5d7ff40`
- **Checkout path:** `yocto/poky/` (ignored by this repository)

## Local generated directories

The following directories are intentionally ignored because they contain downloaded or generated data:

- `yocto/build/`
- `yocto/downloads/`
- `yocto/sstate-cache/`

## Host resource policy

This development host has four CPU threads and limited RAM. Yocto builds are configured later to use at most two BitBake tasks and two compiler jobs.