# `tvterm`

A terminal emulator that runs in your terminal. Powered by Turbo Vision.

![htop, turbo and notcurses-demo running in tvterm](https://user-images.githubusercontent.com/20713561/137407902-27538f99-dc0e-47a8-9bf2-0705733d8a8c.png)

`tvterm` is an experimental terminal emulator widget and application based on the [Turbo Vision](https://github.com/magiblot/tvision) framework. It was created for the purpose of demonstrating new features in Turbo Vision such as 24-bit color support.

`tvterm` relies on Paul Evan's [libvterm](http://www.leonerd.org.uk/code/libvterm/) terminal emulator, also used by [Neovim](https://github.com/neovim/libvterm) and [Emacs](https://github.com/akermu/emacs-libvterm).

As of now, `tvterm` can only be compiled for Unix systems.

The original location of this project is https://github.com/magiblot/tvterm.

# Building

In order to build `tvterm` you must have the following things installed:

* CMake 3.5 or newer.
* A compiler supporting C++14.
* `libvterm` (preferably an up-to-date version such as [libvterm-bzr](https://aur.archlinux.org/packages/libvterm-bzr/) from the AUR).
* Any dependencies required by [Turbo Vision](https://github.com/magiblot/tvision#build-environment):
    * `libncursesw` (Unix only).
    * `libgpm` (optional, Linux only).
* Turbo Vision itself. You may do this in two different ways:
    * Use the `--recursive` option of `git clone` when cloning this repository (or run `git submodule init && git submodule update` if you have already cloned it). This way, Turbo Vision will be built along `tvterm`.
    * Clone [Turbo Vision](https://github.com/magiblot/tvision) separately and follow its [build](https://github.com/magiblot/tvision#build-environment) and [install](https://github.com/magiblot/tvision#build-cmake) instructions. Make sure you don't use a version of Turbo Vision older than the one required by `tvterm` (specified in the [`tvision` submodule](https://github.com/magiblot/tvterm/tree/master/deps)). When building `tvterm`, enable the CMake option `-DTVTERM_USE_SYSTEM_TVISION=ON`.

Then build `tvterm` with CMake:

```sh
cmake . -B ./build -DCMAKE_BUILD_TYPE=Release && # Could also be 'Debug', 'MinSizeRel' or 'RelWithDebInfo'.
cmake --build ./build
```

CMake versions older than 3.13 may not support the `-B` option. You can try the following instead:

```sh
mkdir -p build; cd build
cmake .. -DCMAKE_BUILD_TYPE=Release &&
cmake --build .
```

# Features

This project is still WIP. Some features it may achieve at some point are:

- [x] UTF-8 support.
- [x] fullwidth and zero-width character support.
- [x] 24-bit color support.
- [ ] Scrollback.
- [ ] Text selection.
- Typical `Edit` menu actions:
    - [ ] Find text.
    - [ ] Send signal.
- [ ] Text reflow on resize.
- [ ] Having other terminal emulator implementations to choose from.
