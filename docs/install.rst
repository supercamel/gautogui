Installation
============

Build requirements
------------------

gautogui is built with Meson and uses GLib/GObject/GIO. GObject
Introspection is required for Python, Vala, SQGI, and other binding users.

Linux/X11 builds also require X11, XInput2, and XTest development headers.
Wayland sessions are detected at runtime, but the current Linux backend is
X11-only.

Debian or Ubuntu
~~~~~~~~~~~~~~~~

.. code-block:: sh

   sudo apt install \
     build-essential meson ninja-build pkg-config \
     libglib2.0-dev libx11-dev libxi-dev libxtst-dev \
     gobject-introspection libgirepository1.0-dev valac

Fedora
~~~~~~

.. code-block:: sh

   sudo dnf install \
     gcc meson ninja-build pkgconf-pkg-config \
     glib2-devel libX11-devel libXi-devel libXtst-devel \
     gobject-introspection-devel vala

Arch Linux
~~~~~~~~~~

.. code-block:: sh

   sudo pacman -S --needed \
     base-devel meson ninja pkgconf glib2 libx11 libxi libxtst \
     gobject-introspection vala

Windows with MSYS2
~~~~~~~~~~~~~~~~~~

Open an MSYS2 UCRT64 shell and install the MinGW packages:

.. code-block:: sh

   pacman -S --needed \
     mingw-w64-ucrt-x86_64-gcc \
     mingw-w64-ucrt-x86_64-meson \
     mingw-w64-ucrt-x86_64-ninja \
     mingw-w64-ucrt-x86_64-pkgconf \
     mingw-w64-ucrt-x86_64-glib2 \
     mingw-w64-ucrt-x86_64-gobject-introspection \
     mingw-w64-ucrt-x86_64-vala

Build and test
--------------

From the repository root:

.. code-block:: sh

   meson setup build
   meson compile -C build
   meson test -C build

Run from the build tree
-----------------------

Before installation, the shared library and typelib live inside the build tree.
Set both paths when running interpreted binding examples:

.. code-block:: sh

   GI_TYPELIB_PATH=$PWD/build/src \
   LD_LIBRARY_PATH=$PWD/build/src \
   sqgi examples/monitor.nut

For Python:

.. code-block:: sh

   GI_TYPELIB_PATH=$PWD/build/src \
   LD_LIBRARY_PATH=$PWD/build/src \
   python3 examples/monitor.py

The compiled examples link directly to the build-tree library:

.. code-block:: sh

   ./build/examples/gautogui-monitor
   ./build/examples/gautogui-monitor-vala

Install system-wide
-------------------

Install from the repository root:

.. code-block:: sh

   sudo meson install -C build
   sudo ldconfig

``ldconfig`` refreshes the dynamic linker cache on Linux. Without it, tools
such as Python and SQGI may find the ``Gautogui-1.0.typelib`` but fail to load
``libgautogui-1.0.so``.

After installation, these commands should work without build-tree variables:

.. code-block:: sh

   pkg-config --cflags --libs gautogui-1.0
   sqgi examples/monitor.nut
   python3 examples/monitor.py

Install under a custom prefix
-----------------------------

For a user-local install:

.. code-block:: sh

   meson setup build --prefix=$HOME/.local
   meson compile -C build
   meson install -C build

If your system does not already search that prefix, export these variables:

.. code-block:: sh

   export PATH=$HOME/.local/bin:$PATH
   export PKG_CONFIG_PATH=$HOME/.local/lib/pkgconfig:$PKG_CONFIG_PATH
   export LD_LIBRARY_PATH=$HOME/.local/lib:$LD_LIBRARY_PATH
   export GI_TYPELIB_PATH=$HOME/.local/lib/girepository-1.0:$GI_TYPELIB_PATH

The exact library directory can be ``lib`` or ``lib64`` depending on the
distribution and Meson configuration.
