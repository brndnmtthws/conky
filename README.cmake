Hello, there.

Using CMake to build Conky is pretty easy, and here is how I do it:

1. From the top level source dir, create a build working dir, and cd into it
  $ mkdir build
  $ cd build
2. Run the cmake configuration process
  $ cmake ../ # pass the path to the sources to cmake
  OR
  $ ccmake ../ # you can also use the fance curses interface, or try cmake-gui
3. Compile as usual, and enjoy the out-of-source goodness
  $ make
  # make install # if you want

There are a number of build options for Conky, and the best way to discover
them is to use the ccmake (or cmake-gui) CMake tool for browsing them.

Certain Conky build time features (such as doc generation) require third-party
applications, which you should be notified of via CMake.  In the case of doc
generation, you'll need the docbook2X package (available on most
distributions).
