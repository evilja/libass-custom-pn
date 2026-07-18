# libass-custom-pn-1

A custom libass build with the `ass_get_metrics` API. It installs under a separate library name, so it can coexist with the system libass.

## Install

```sh
git clone https://github.com/evilja/libass-custom-pn.git
cd libass-custom-pn

meson setup build \
  --prefix="$HOME/.local" \
  --libdir=lib \
  --buildtype=release \
  -Ddefault_library=both \
  -Dtest=enabled \
  -Dcheckasm=disabled

meson compile -C build
meson test -C build
meson install -C build
```

The library is installed as `libass-custom-pn-1`, with headers under `~/.local/include/libass-custom-pn-1`.

For pkg-config:

```sh
export PKG_CONFIG_PATH="$HOME/.local/lib/pkgconfig:$PKG_CONFIG_PATH"
pkg-config --cflags --libs libass-custom-pn-1
```
