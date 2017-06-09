/**
 * Copyright (C) 2017 Felix Wang
 *
 * Simulation Tool for Asynchronous Cortical Streams (stacs)
 */

#include "audio-yarp.h"

/**************************************************************************
* Main entry point
**************************************************************************/



// Main
//
int main(int argc, char ** argv) {
  // Open the network
  yarp::os::Network yarp;

  // Open the output port
  yarp::os::Port pAudio;
  pAudio.open("/audio/send");
  // Open an input port
  yarp::os::BufferedPort<yarp::os::Bottle> pRequest;
  pRequest.open("/audio/request");

  // Connect to yarp port
  yarp::os::Network::connect("/audio/send", "/stacs/audio/recv");
  yarp::os::Network::connect("/stacs/audio/request", "/audio/request");

  // Get the filename
  std::string filename;
  if (argc == 2) {
    filename = argv[1];
  }
  else {
    printf("Usage: ./stacs-audio <filename.txt>\n");
    return -1;
  }

  // Get wavfile from list
  std::ifstream wavlist(filename.c_str());
  std::string wavline;
  std::vector<std::string> wavfile;
  wavfile.clear();
  while (std::getline(wavlist, wavline)) {
    wavfile.push_back(wavline);
  }
  // Close list
  wavlist.close();

  std::random_shuffle (wavfile.begin(), wavfile.end());

  bool finish = false;

  for (std::size_t i = 0; i < wavfile.size(); ++i) {
    printf("%s\n", wavfile[i].c_str());

    // Open sound file
    printf("Opening sound file");
    yarp::sig::file::soundStreamReader sReader;
    if (sReader.open(wavfile[i].c_str()) == false) {
      printf("Error: cannot open file %s\n", wavfile[i].c_str());
      return -1;
    } 

    // Send the data to the network
    int CHUNK_SIZE = 4096;
    double PLAY_RATE = 2;
    yarp::sig::Sound s;
    printf("Streaming file %s\n", wavfile[i].c_str());

    bool complete = false;
    sReader.rewind();

    do {
      // Check if requested
      yarp::os::Bottle *b = pRequest.read();
      printf("Requesting samples: %d entries left\n", b->get(0).asInt());
      if (b->get(0).asInt() < 0) {
        finish = true;
        break;
      }

      int current_pos  = sReader.getIndex(); 
      int read_samples = sReader.readBlock(s,CHUNK_SIZE);
      if (read_samples<CHUNK_SIZE) complete=true;

      static double old = yarp::os::Time::now();
      printf("from sample %d to sample %d, time %.3f\n", current_pos, current_pos+read_samples, yarp::os::Time::now()-old); 
      old = yarp::os::Time::now();
      double del = (((double) read_samples) * PLAY_RATE)/16000;
      //yarp::os::Time::delay(del);

      pAudio.write(s); //use ports
    } while(!complete);

    if (finish) {
      printf("Stacs no longer requesting data\n");
      printf("Closing sound file\n");
      sReader.close();
      break;
    }

    // Close sound file
    printf("Closing sound file\n");
    sReader.close();
  }
  
  // exit successfully
  return 0;
}
