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
  char labimg_delaymax = 20;
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
  */
  labmap["b"] = 0;
  labmap["d"] = 0;
  labmap["g"] = 0;
  labmap["p"] = 0;
  labmap["t"] = 0;
  labmap["k"] = 0;
  labmap["dx"] = 0;
  labmap["q"] = 0;
  labmap["jh"] = 1;
  labmap["ch"] = 1;
  labmap["s"] = 1;
  labmap["sh"] = 1;
  labmap["zh"] = 1;
  labmap["z"] = 1;
  labmap["f"] = 1;
  labmap["th"] = 1;
  labmap["v"] = 1;
  labmap["dh"] = 1;
  labmap["m"] = 2;
  labmap["em"] = 2;
  labmap["n"] = 2;
  labmap["en"] = 2;
  labmap["nx"] = 2;
  labmap["ng"] = 2;
  labmap["eng"] = 2;
  labmap["l"] = 3;
  labmap["el"] = 3;
  labmap["r"] = 3;
  labmap["w"] = 3;
  labmap["y"] = 3;
  labmap["hh"] = 3;
  labmap["hv"] = 3;
  labmap["iy"] = 4;
  labmap["ih"] = 4;
  labmap["ix"] = 4;
  labmap["eh"] = 4;
  labmap["ey"] = 4;
  labmap["ae"] = 4;
  labmap["aa"] = 4;
  labmap["ao"] = 4;
  labmap["aw"] = 4;
  labmap["ay"] = 4;
  labmap["ah"] = 4;
  labmap["ax"] = 4;
  labmap["ax-h"] = 4;
  labmap["er"] = 4;
  labmap["axr"] = 4;
  labmap["oy"] = 4;
  labmap["ow"] = 4;
  labmap["uh"] = 4;
  labmap["uw"] = 4;
  labmap["ux"] = 4;
  // Need 5 label images
  std::vector<char> labimage;
  labimage.resize(labimg_size);
  for (unsigned int labelrng = 2001; labelrng <= 2005; ++labelrng) {
    std::srand(labelrng);
    for (int i = 0; i < labimg_size; ++i) {
      labimage[i] = ((char)(std::rand() % labimg_delaymax)) + 5;
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
      labimg.resize(1, labimg_size);
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
