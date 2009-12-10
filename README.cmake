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

When switching back from cmake to autotools, you need to call 'make distclean'.

NOTE: I haven't actually finished the CMake build system yet, because I'm lazy.  I'll add all the different options eventually though.
