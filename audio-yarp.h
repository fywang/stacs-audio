/**
 * Copyright (C) 2016 Felix Wang
 *
 * Simulation Tool for Asynchronous Cortical Streams (stacs)
 *
 * Reading audio files and making it available to YARP
 */

#ifndef __STACS_TIMIT_YARP_H__
#define __STACS_TIMIT_YARP_H__

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <random>
#include <vector>

#include <yarp/os/Time.h>
#include <yarp/os/Network.h>
#include <yarp/os/Port.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/sig/SoundFile.h>
#include <yarp/sig/Sound.h>

#include <yarp/dev/PolyDriver.h>
#include <yarp/dev/AudioGrabberInterfaces.h>

#endif
