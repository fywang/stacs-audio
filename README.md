# STACS Audio

This program reads a text file containing locations of audio files (e.g. `.wav`) and sends the audio signal to STACS through YARP.

There are also some testing programs (adapted from the YARP examples) to send and receive audio data.

Building using CMake:

	mkdir build
	cd build
	ccmake ../
	make
