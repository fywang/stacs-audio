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

  // Writing order of phones
  std::ofstream phnorder;
  phnorder.open("phnorder.txt");
  
  // Phones
  std::ifstream phnlist;
  std::string phnfile;
  std::string phnline;

  bool finish = false;

  std::random_device r;
  std::srand(r());

  // Stream until receive stop signal
  for (std::size_t i = 0;; ++i) {
    std::size_t ii = i % wavfile.size();
    if (ii == 0) {
      std::random_shuffle (wavfile.begin(), wavfile.end());
    }
    //printf("%s\n", wavfile[ii].c_str());
    phnfile = wavfile[ii].substr(0,wavfile[ii].size()-3) + "phn";
    printf("%s\n", phnfile.c_str());
    if (wavfile[ii][0] == '%') {
      continue;
    }

    // Open sound file
    printf("Opening sound file");
    yarp::sig::file::soundStreamReader sReader;
    if (sReader.open(wavfile[ii].c_str()) == false) {
      printf("Error: cannot open file %s\n", wavfile[ii].c_str());
      return -1;
    } 
    
    // Send the data to the network
    yarp::sig::Sound s;
    printf("Streaming file %s\n", wavfile[ii].c_str());

    bool complete = false;
    sReader.rewind();

    phnlist.open(phnfile.c_str());

    // Send one phoneme at a time
    while (std::getline(phnlist, phnline)) {
      // Get phone size
      std::stringstream phnstream(phnline);
      int tstart, tfinish;
      std::string phone;
      phnstream >> tstart;
      phnstream >> tfinish;
      phnstream >> phone;
      int PHONE_SIZE = tfinish - tstart;
     
      // Check if requested
      yarp::os::Bottle *b = pRequest.read();
      printf("Requesting samples: %d entries left\n", b->get(0).asInt());
      if (b->get(0).asInt() < 0) {
        finish = true;
        break;
      }

      int current_pos  = sReader.getIndex(); 
      int read_samples = sReader.readBlock(s,PHONE_SIZE);
    
      // Write what was played to file
      phnorder << PHONE_SIZE << " " << phone << std::endl;

      printf("  from sample %d to sample %d\n", current_pos, current_pos+read_samples); 

      pAudio.write(s); //use ports
    }

    // Close phoneme list
    phnlist.close();

    if (finish) {
      printf("Stacs no longer requesting data\n");
      break;
    }
  }

  // Close writing out
  phnorder.close();
  
  // exit successfully
  return 0;
}
