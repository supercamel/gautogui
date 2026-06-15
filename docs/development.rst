Development
===========

Build checks
------------

The normal local validation loop is:

.. code-block:: sh

   meson setup build --reconfigure
   meson compile -C build
   meson test -C build --print-errorlogs

For binding checks from the build tree:

.. code-block:: sh

   GI_TYPELIB_PATH=$PWD/build/src \
   LD_LIBRARY_PATH=$PWD/build/src \
   sqgi -e 'local G = import("Gautogui"); print(G.Controller + "\n")'

Documentation
-------------

Read the Docs uses ``.readthedocs.yaml`` and ``docs/conf.py``. To build the
HTML documentation locally without changing your system Python:

.. code-block:: sh

   python3 -m venv /tmp/gautogui-docs-venv
   . /tmp/gautogui-docs-venv/bin/activate
   pip install -r docs/requirements.txt
   sphinx-build -W -b html docs docs/_build/html

Design notes
------------

The implementation design notes are kept in ``docs/DESIGN.md``. They describe
the backend contract and the current Windows/X11 architecture.
