/**
 * Copyright (C) 2017 Felix Wang
 *
 * Simulation Tool for Asynchronous Cortical Streams (stacs)
 */

#include "audio-yarp.h"

#include <yarp/sig/ImageFile.h>
#include <yarp/sig/Image.h>
#include <unordered_map>

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
  yarp::os::Port pLabel;
  pLabel.open("/label/send");
  // Open an input port
  yarp::os::BufferedPort<yarp::os::Bottle> pRequest;
  pRequest.open("/audio/request");
  yarp::os::BufferedPort<yarp::os::Bottle> pLabRequest;
  pLabRequest.open("/label/request");

  // Connect to yarp port
  yarp::os::Network::connect("/audio/send", "/stacs/audio/recv");
  yarp::os::Network::connect("/stacs/audio/request", "/audio/request");
  yarp::os::Network::connect("/label/send", "/stacs/label/recv");
  yarp::os::Network::connect("/stacs/label/request", "/label/request");

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
  
  // Labels
  std::unordered_map<std::string, int> labmap;
  int labimg_size = 20;
  char labimg_delaymax = 25;
  std::vector<std::vector<char>> labimgs;
  // go through phone list
  /*
  labmap["pau"] = -1;
  labmap["epi"] = -1;
  labmap["h#"] = -1;
  labmap["bcl"] = -1;
  labmap["dcl"] = -1;
  labmap["gcl"] = -1;
  labmap["pcl"] = -1;
  labmap["tcl"] = -1;
  labmap["kcl"] = -1;
  labmap["q"] = 7;
  */
  labmap["b"] = 0;
  labmap["d"] = 1;
  labmap["g"] = 2;
  labmap["p"] = 3;
  labmap["t"] = 4;
  labmap["k"] = 5;
  labmap["dx"] = 6;
  labmap["jh"] = 7;
  labmap["ch"] = 8;
  labmap["s"] = 9;
  labmap["sh"] = 10;
  labmap["zh"] = 10;
  labmap["z"] = 11;
  labmap["f"] = 12;
  labmap["th"] = 13;
  labmap["v"] = 14;
  labmap["dh"] = 15;
  labmap["m"] = 16;
  labmap["em"] = 16;
  labmap["n"] = 17;
  labmap["en"] = 17;
  labmap["nx"] = 17;
  labmap["ng"] = 18;
  labmap["eng"] = 18;
  labmap["l"] = 19;
  labmap["el"] = 19;
  labmap["r"] = 20;
  labmap["w"] = 21;
  labmap["y"] = 22;
  labmap["hh"] = 23;
  labmap["hv"] = 23;
  labmap["iy"] = 24;
  labmap["ih"] = 25;
  labmap["ix"] = 25;
  labmap["eh"] = 26;
  labmap["ey"] = 27;
  labmap["ae"] = 28;
  labmap["aa"] = 29;
  labmap["ao"] = 29;
  labmap["aw"] = 30;
  labmap["ay"] = 31;
  labmap["ah"] = 32;
  labmap["ax"] = 32;
  labmap["ax-h"] = 32;
  labmap["er"] = 33;
  labmap["axr"] = 33;
  labmap["oy"] = 34;
  labmap["ow"] = 35;
  labmap["uh"] = 36;
  labmap["uw"] = 37;
  labmap["ux"] = 37;
  // Need 38 label images
  std::vector<char> labimage;
  labimage.resize(labimg_size);
  for (unsigned int labelrng = 2001; labelrng <= 2038; ++labelrng) {
    std::srand(labelrng);
    for (int i = 0; i < labimg_size; ++i) {
      labimage[i] = ((char)(std::rand() % labimg_delaymax)) + 10;
    }
    labimgs.push_back(labimage);
  }

  bool finish = false;
  std::srand(std::time(0));

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
      // Only send subset of phones
      std::unordered_map<std::string, int>::iterator labmapped = labmap.find(phone);
      if (labmapped == labmap.end()) {
        int current_pos  = sReader.getIndex(); 
        int read_samples = sReader.readBlock(s,PHONE_SIZE);
        printf("  skipping '%s': sample %d to sample %d\n", phone.c_str(), current_pos, current_pos+read_samples); 
        continue;
      }

      // Prepare samples
      int current_pos  = sReader.getIndex(); 
      int read_samples = sReader.readBlock(s,PHONE_SIZE);
      // Prepare labels
      yarp::sig::ImageOf<yarp::sig::PixelMono> labimg;
      labimg.resize(1,labimg_size);
      std::memcpy(labimg.getRawImage(), labimgs[labmapped->second].data(), labimg_size);
    
      // Write what was played to file
      phnorder << PHONE_SIZE << " " << phone << std::endl;

      printf("  sending '%s' sample %d to sample %d\n", phone.c_str(), current_pos, current_pos+read_samples); 

      pAudio.write(s); // send samples
      pLabel.write(labimg); // send labels
      
      // Check if requested
      yarp::os::Bottle *b = pRequest.read();
      yarp::os::Bottle *blab = pLabRequest.read();
      printf("Requesting samples: %d entries left\n", b->get(0).asInt());
      printf("Requesting labels: %d entries left\n", blab->get(0).asInt());
      if (b->get(0).asInt() < 0 || blab->get(0).asInt() < 0) {
        finish = true;
        break;
      }
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
