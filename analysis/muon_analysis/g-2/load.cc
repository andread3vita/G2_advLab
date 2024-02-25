// Copyright 2023 nicolò salimbeni andrea de vita
/*
  this script is made to load properly all the libraries for
  the analysis
  TO DO
*/
#include <RtypesCore.h>

#include <string>

#include "TROOT.h"
#include "TSystem.h"

void load()
{
  // load usefull libraries
  std::string lib_path     = "./../../../lib/";
  std::string include_path = "./../../../include/";
  std::string src_path     = "./../../../src/";

  gROOT->ProcessLine((".L " + src_path + "AnUtil.cc").c_str());
  gROOT->ProcessLine((".L " + src_path + "Event.cc").c_str());
  gROOT->ProcessLine((".L " + src_path + "InfoAcq.cc").c_str());
  gROOT->ProcessLine((".L " + src_path + "StateFile.cc").c_str());
  gROOT->ProcessLine((".L " + src_path + "FolderManager.cc").c_str());
  gROOT->ProcessLine((".L " + src_path + "GrUtil.cc").c_str());

  // load macros for this analysis
  gROOT->ProcessLine(".L ./quick_check_plot.cc");
  gROOT->ProcessLine(".L ./g-2_fit/g-2_fit.cc");
}
