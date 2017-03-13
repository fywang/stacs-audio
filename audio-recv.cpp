/**
 * Copyright (C) 2016 Felix Wang
 *
 * Simulation Tool for Asynchronous Cortical Streams (stacs)
 */

#include "audio-yarp.h"

/**************************************************************************
* Main entry point
**************************************************************************/

class SoundPort : public yarp::os::BufferedPort<yarp::sig::Sound> {
  public:
    SoundPort();
    //using yarp::os::BufferedPort<yarp::sig::Sound>::onRead;
    virtual void onRead (yarp::sig::Sound& s) {
      // Receive sound and render
      printf("Received %d samples\n", s.getSamples());
      put->renderSound(s);
    }
  private:
    yarp::os::Property conf;
    yarp::dev::PolyDriver *poly;
    yarp::dev::IAudioRender *put;
};

SoundPort::SoundPort() {
    // Get an audio write device
    conf.put("device", "portaudio");
    conf.put("samples", 4096);
    conf.put("channels", 2);
    conf.put("rate", 16000);
    conf.put("write", 1);
    poly = new yarp::dev::PolyDriver(conf);

    // Check that we can write sound
    poly->view(put);
    if (put == NULL) {
      printf("Cannot open interface\n");
    }
    printf("Playing to default speakers\n");
}

// Main
//
int main(int argc, char ** argv) {
  // Open the network
  yarp::os::Network yarp;

  // Open the input port
  SoundPort p;
  p.useCallback();
  p.open("/audio/recv");

  // Connect to yarp port
  yarp::os::Network::connect("/audio/send", "/audio/recv");

  while(true);

  // exit successfully
  return 0;
}
