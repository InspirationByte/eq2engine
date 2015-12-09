To compile linux version of core you need:

- CodeBlocks IDE (because no makefile at the moment)

- ia32-libs package if you use x32_64 version

- install i386 libs

sudo apt-get install libgtk2.0-dev:i386 libglib2.0-dev:i386 libdbus-1-dev:i386 \
     mesa-common-dev:i386 libgdk-pixbuf2.0-dev:i386 \
     libpango1.0-dev:i386 libatk1.0-dev:i386 \
     pkg-config:i386
