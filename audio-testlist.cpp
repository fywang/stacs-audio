/**
 * Copyright (C) 2016 Felix Wang
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

  // Get the filename
  std::string filename;
  if (argc == 2) {
    filename = argv[1];
  }
  else {
    printf("Usage: ./audio-testlist <filename.txt>\n");
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

  for (std::size_t i = 0; i < wavfile.size(); ++i) {
    //printf("%s\n", wavfile[i].c_str());
    if (wavfile[i][0] == '%') {
      continue;
    }

    // Test open sound file
    yarp::sig::file::soundStreamReader sReader;
    if (sReader.open(wavfile[i].c_str()) == false) {
      printf("Error: cannot open file %s\n", wavfile[i].c_str());
    }
  }

  // exit successfully
  return 0;
}
