/**
 * Copyright (C) 2016 Felix Wang
 *
 * Simulation Tool for Asynchronous Cortical Streams (stacs)
 */

#include "audio-yarp.h"
#include <cmath>
#include <inttypes.h>
#include <fftw3.h>
#include <unordered_map>

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
    printf("Usage: ./audio-fftw3stats <filename.txt>\n");
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
  std::ofstream mfccstats;
  mfccstats.open("mfccstats.txt");

  // Set up parameters
  // Sound samples
  double samplefreq = 16000.0; // Hz
  std::vector<double> samplebuffer;
  samplebuffer.clear();
  // Power spectral density
  int ninput = 512; // 32ms
  int noverlap = 256; // 50%
  int noutput = ninput/2+1;
  // Hamming window
  std::vector<double> window;
  window.resize(ninput);
  double windownormsquare = 0.0;
  for (int n = 0; n < ninput; ++n) {
    window[n] = 0.54 - 0.46 * cos(6.28318530718 * n / (ninput - 1));
    windownormsquare += window[n] * window[n];
  }
  std::vector<double> psd;
  psd.resize(noutput);
  // Mel frequencies
  int nmel = 30;
  std::vector<double> mel;
  mel.resize(nmel);
  std::vector<std::vector<double> > mels;
  mels.clear();
  std::vector<double> maxmel;
  maxmel.resize(nmel);
  std::vector<double> avemaxmel;
  avemaxmel.resize(nmel);
  int avemaxcount = 0;
  // Filterbank
  std::vector<std::vector<double> > filters;
  filters.clear();
  double mellow = 1125.0 * log(1.0 + 300.0/700.0);
  double melhigh = 1125.0 * log(1.0 + 8000.0/700.0);
  std::vector<int> freqcenters;
  freqcenters.clear();
  freqcenters.push_back(0);
  for (int m = 0; m < nmel; ++m) {
    double melcenter = mellow + m * ((melhigh - mellow)/(nmel));
    double freqcenter = 700.0 * (exp(melcenter/1125) -1);
    freqcenters.push_back(std::floor((ninput+1)*freqcenter/samplefreq));
    avemaxmel[m] = -16.0;
    maxmel[m] = -16.0;
  }
  freqcenters.push_back(noutput-1);
  for (int m = 1; m <= nmel; ++m) {
    std::vector<double> filter;
    filter.resize(noutput);
    for (int k = 0; k < freqcenters[m-1]; ++k) {
      filter[k] = 0.0;
    }
    for (int k = freqcenters[m-1]; k < freqcenters[m]; ++k) {
      filter[k] = ((double)(k - freqcenters[m-1]))/(freqcenters[m] - freqcenters[m-1]);
    }
    for (int k = freqcenters[m]; k <= freqcenters[m+1]; ++k) {
      filter[k] = ((double)(freqcenters[m+1] - k))/(freqcenters[m+1] - freqcenters[m]);
    }
    for (int k = freqcenters[m+1]+1; k < noutput; ++k) {
      filter[k] = 0.0;
    }
    filters.push_back(filter);
  }
  
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

  int nphones = 0;
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
    yarp::sig::Sound ssig;
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
        int read_samples = sReader.readBlock(ssig,PHONE_SIZE);
        printf("  skipping '%s': sample %d to sample %d\n", phone.c_str(), current_pos, current_pos+read_samples); 
        continue;
      }

      // Prepare samples
      int current_pos  = sReader.getIndex(); 
      int read_samples = sReader.readBlock(ssig,PHONE_SIZE);

      printf("  analyzing '%s' sample %d to sample %d\n", phone.c_str(), current_pos, current_pos+read_samples); 
      
      std::size_t nsample = ssig.getSamples();
      samplebuffer.resize(nsample);
      for (std::size_t s = 0; s < nsample; ++s) {
        samplebuffer[s] = ((double)((int16_t)ssig.get(s)))/32768.0;
      }
      // Split samples into windows
      mels.clear();
      int nwindow = (((int)samplebuffer.size()) - noverlap)/(ninput - noverlap);
      for (int w = 0; w < nwindow; ++w) {
        // Find power spectral density of window
        double* inputbuffer = static_cast<double*>(fftw_malloc(ninput * sizeof(double)));
        fftw_complex* outputbuffer = static_cast<fftw_complex*>(fftw_malloc(noutput * sizeof(fftw_complex)));
        for (int n = 0; n < ninput; ++n) {
          inputbuffer[n] = samplebuffer[w * (ninput - noverlap) + n] * window[n];
        }
        fftw_plan plan = fftw_plan_dft_r2c_1d(ninput, inputbuffer, outputbuffer, FFTW_ESTIMATE);
        fftw_execute(plan);
        for (int n = 0; n < noutput; ++n) {
          psd[n] = (outputbuffer[n][0] * outputbuffer[n][0] + outputbuffer[n][1] * outputbuffer[n][1]) / (samplefreq * windownormsquare);
        }
        // Filtering to mel scale
        for (int m = 0; m < nmel; ++m) {
          mel[m] = 0.0;
          for (int n = 0; n < noutput; ++n) {
            mel[m] += filters[m][n] * psd[n];
          }
          mel[m] = (mel[m] == 0.0) ? -16.0 : log10(mel[m]);
        }
        mels.push_back(mel);
        // Free allocated things
        fftw_free(inputbuffer);
        fftw_free(outputbuffer);
        fftw_destroy_plan(plan);
        //mels.pop_front();
      }
      // Delete to the end of the processed sound samples
      samplebuffer.clear();

      // Find distribution
      std::vector<double> melcomp = mels.front();
      for (std::size_t ms = 1; ms < mels.size(); ++ms) {
        for (std::size_t m = 0; m < melcomp.size(); ++m) {
          melcomp[m] = (melcomp[m]*ms + mels[ms][m])/(ms+1);
        }
      }
      // Distribution too hard to do in c++,
      // writing to file to let MATLAB compute
      for (std::size_t m = 0; m < melcomp.size()-1; ++m) {
        mfccstats << melcomp[m] << ", ";
      }
      mfccstats << melcomp[melcomp.size()-1] << std::endl;

      if (++nphones > 10000) {
        finish = true;
        break;
      }
    }

    // Close phoneme list
    phnlist.close();

    if (finish) {
      printf("Analyzed requested phonemes\n");
      break;
    }
  }
  
  // Close writing out
  mfccstats.close();

  // exit successfully
  return 0;
}
