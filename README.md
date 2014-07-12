GameMaker
=========

This is the source code and executables for Recreational Software Design's GameMaker product, released in 1994.

You can learn more about this product here:

[http://en.wikipedia.org/wiki/Game-Maker]

[http://www.aderack.com/game-maker]


Using This Repository
=====================

After cloning this repository on your machine, you will see the following directories:

cd:  The GameMaker cd image first released in 1994.  This can be used to install GameMaker and contains all the sample games
code: The GameMaker source code (and other stuff)
runtime: This is an image of a typical "c:" disk with GameMaker installed.  This makes it easy to run GameMaker inside a dosbox.
tools: Compilers

First step: install dosbox.  [http://www.dosbox.com/]

Next "cd" to the root directory of this repository.

Now run dosbox.

Inside your dosbox, run:

mount c runtime

Now you can run GameMaker with:

    c:
    cd c:\gm
    gm

To start working with the source code, run:

    c:
    dev.bat

This will mount the compiler into the "t:" directory, and the source code into "d:".

Now to compile everything
    d:
    cd gm
    del *.obj
    del *.exe
    creatall

To create a single program:

    create palchos
    create blocedit

(etc)

In today's world, I've been editing the code outside of dosbox in linux/emacs and then (in the dosbox window) hit ctrl-f4 to sync the disk inside dosbox with the external changes. YMMV.

