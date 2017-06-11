/**
 * Copyright (C) 2016 Felix Wang
 *
 * Simulation Tool for Asynchronous Cortical Streams (stacs)
 */

#include "audio-yarp.h"
#include <cmath>
#include <inttypes.h>
#include <fftw3.h>

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

  for (std::size_t i = 0; i < wavfile.size(); ++i) {
    //printf("%s\n", wavfile[i].c_str());
    if (wavfile[i][0] == '%') {
      continue;
    }

    // Test open sound file
    yarp::sig::file::soundStreamReader sReader;
    if (sReader.open(wavfile[i].c_str()) == false) {
      printf("Error: cannot open file %s\n", wavfile[i].c_str());
      return -1;
    }
    
    // Send the data to the network
    //int CHUNK_SIZE = 4096;
    int CHUNK_SIZE = 8192;
    //int CHUNK_SIZE = 16384;
    double PLAY_RATE = 2;
    yarp::sig::Sound ssig;
    printf("Analyzing file %s\n", wavfile[i].c_str());

    bool complete = false;
    sReader.rewind();

    do {
      int current_pos  = sReader.getIndex(); 
      int read_samples = sReader.readBlock(ssig,CHUNK_SIZE);
      if (read_samples<CHUNK_SIZE) complete=true;

      //printf("from sample %d to sample %d\n", current_pos, current_pos+read_samples); 

      std::size_t noldsample = samplebuffer.size();
      std::size_t nnewsample = ssig.getSamples();
      samplebuffer.resize(noldsample + nnewsample);
      for (std::size_t s = 0; s < nnewsample; ++s) {
        samplebuffer[noldsample+s] = ((double)((int16_t)ssig.get(s)))/32768.0;
      }
      // Split samples into windows
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
      // Find max and average
      for (std::size_t i = 0; i < mels.size(); ++i) {
        for (int m = 0; m < nmel; ++m) {
          maxmel[m] = std::max(mels[i][m], maxmel[m]);
        }
      }
      for (int m = 0; m < nmel; ++m) {
        avemaxmel[m] = (avemaxmel[m]*avemaxcount + maxmel[m])/(avemaxcount+1);
        maxmel[m] = -16.0;
      }
      ++avemaxcount;
      mels.clear();
      // Delete to the end of the processed sound samples
      samplebuffer.erase(samplebuffer.begin(), samplebuffer.begin() + nwindow * (ninput - noverlap));

    } while(!complete);

    // Close sound file
    //printf("Closing sound file\n");
    //sReader.close();
  }
  for (int m = 0; m < nmel; ++m) {
    printf("Average max for %d = %f\n", m, avemaxmel[m]);
  }

  // exit successfully
  return 0;
}
