#include <string>
#include <map>
#include <set>
#include <iostream>
#include <utility>
#include <vector>
#include <cstdlib>
#include "CombineHarvester/CombineTools/interface/CombineHarvester.h"
#include "CombineHarvester/CombineTools/interface/Observation.h"
#include "CombineHarvester/CombineTools/interface/Process.h"
#include "CombineHarvester/CombineTools/interface/Utilities.h"
#include "CombineHarvester/CombineTools/interface/Systematics.h"
#include "CombineHarvester/CombineTools/interface/BinByBin.h"
#include "TRegexp.h"

using namespace std;

int main(int argc, char * argv[]) {
  //! [part1]
  // First define the location of the "auxiliaries" directory where we can
  // source the input files containing the datacard shapes
  string aux_shapes = "./InputHisto_ToBeFitted/";

  // Create an empty CombineHarvester instance that will hold all of the
  // datacard configuration and histograms etc.
  ch::CombineHarvester cb;
  // Uncomment this next line to see a *lot* of debug information
  // cb.SetVerbosity(3);

  // Here we will just define two categories for an 8TeV analysis. Each entry in
  // the vector below specifies a bin name and corresponding bin_id.
  ch::Categories cats = {
      {1, "ETauFR_pass"},
      {2, "ETauFR_fail"}
  };
  // ch::Categories is just a typedef of vector<pair<int, string>>
  //! [part1]


  //! [part2]
  //vector<string> masses = ch::MassesFromRange("120-135:5");
  // Or equivalently, specify the mass points explicitly:
    vector<string> masses = {"90"};
  //! [part2]

  //! [part3]
  cb.AddObservations({"*"}, {"ETauFR"}, {"13TeV"}, {"et"}, cats);
  //! [part3]

  //! [part4]
  vector<string> bkg_procs = {"ZJ","ZEE", "W", "QCD", "TT","VV"};
  cb.AddProcesses({"*"}, {"ETauFR"}, {"13TeV"}, {"et"}, bkg_procs, cats, false);

  vector<string> sig_procs = {"ZTT"};
  cb.AddProcesses(masses, {"ETauFR"}, {"13TeV"}, {"et"}, sig_procs, cats, true);
  //! [part4]


  //Some of the code for this is in a nested namespace, so
  // we'll make some using declarations first to simplify things a bit.
  using ch::syst::SystMap;
  using ch::syst::era;
  using ch::syst::bin_id;
  using ch::syst::process;

  //! [part5]
    cb.cp().process(ch::JoinStr({sig_procs,{"ZJ"},{"ZEE"},{"VV"},{"TT"}})).AddSyst(cb, "lumi_$ERA", "lnN", SystMap<era>::init({"13TeV"}, 1.026));
    
  //! [part5]

  //! [part6]
    
    cb.cp().process({"W"}).AddSyst(cb, "normalizationW", "lnN", SystMap<>::init(1.20));
    cb.cp().process({"ZJ"},{"ZTT"}).AddSyst(cb, "normalizationDY", "lnN", SystMap<>::init(1.03));
    cb.cp().process({"ZEE"}).AddSyst(cb, "normalizationDYEE", "lnN", SystMap<>::init(1.06));
    cb.cp().process({"QCD"}).AddSyst(cb, "normalizationQCD", "lnN", SystMap<>::init(1.2));

    cb.cp().process({"VV"}).AddSyst(cb, "normalizationVV", "lnN", SystMap<>::init(1.15));
    cb.cp().process({"TT"}).AddSyst(cb, "normalizationTT", "lnN", SystMap<>::init(1.10));
    
    cb.cp().process(ch::JoinStr({sig_procs,{"ZJ"},{"ZEE"},{"VV"},{"TT"}})).AddSyst(cb, "CMS_eff_e", "lnN", SystMap<>::init(1.05));
    cb.cp().process(ch::JoinStr({sig_procs,{"ZJ"},{"ZEE"},{"VV"},{"TT"}})).AddSyst(cb, "CMS_eff_t", "lnN", SystMap<>::init(1.03));
    
    cb.cp().process({"ZEE"}).AddSyst(cb, "tagele_", "shape", SystMap<>::init(1));
    cb.cp().process({"ZEE"}).AddSyst(cb, "probeele_", "shape", SystMap<>::init(1));
    
    cb.cp().process({"ZTT"}).AddSyst(cb, "probetau_", "shape", SystMap<>::init(1));
    
    cb.cp().process({"ZEE"}).AddSyst(cb, "reso_", "shape", SystMap<>::init(1));
    
  //! [part6]

  //! [part7]
  cb.cp().backgrounds().ExtractShapes(
      aux_shapes + argv[1],
      "$BIN/$PROCESS",
      "$BIN/$PROCESS_$SYSTEMATIC");
  cb.cp().signals().ExtractShapes(
      aux_shapes + argv[1],
      "$BIN/$PROCESS",
      "$BIN/$PROCESS_$SYSTEMATIC");
  //! [part7]

  //! [part8]
    auto bbb = ch::BinByBinFactory().SetAddThreshold(0.1).SetFixNorm(true);
    
    bbb.AddBinByBin(cb.cp().backgrounds(),cb);
    
    TString outputdcname = (TString) argv[1];
    TRegexp re(".root");
    outputdcname(re) = ".txt";
  //! [part8]

  //! [part9]
  // First we generate a set of bin names:
  set<string> bins = cb.bin_set();
  // This method will produce a set of unique bin names by considering all
  // Observation, Process and Systematic entries in the CombineHarvester
  // instance.

  // We create the output root file that will contain all the shapes.
  TFile output("htt_et.input.root", "RECREATE");

  // Finally we iterate through each bin,mass combination and write a
  // datacard.
    cb.cp().WriteDatacard((string)outputdcname, output);
  //! [part9]
    cout << "pre-fit eff.: "
    << cb.cp().bin({"ETauFR_pass"}).process({"ZTT"}).GetRate()/((cb.cp().bin({"ETauFR_pass"}).process({"ZTT"}).GetRate()+cb.cp().bin({"ETauFR_fail"}).process({"ZTT"}).GetRate())) << "\n";
    float sigRatePassPre = cb.cp().bin({"ETauFR_pass"}).process({"ZTT"}).GetRate();
    float sigRateFailPre = cb.cp().bin({"ETauFR_fail"}).process({"ZTT"}).GetRate();
    float sigErrPassPre = cb.cp().bin({"ETauFR_pass"}).process({"ZTT"}).GetUncertainty();
    float sigErrFailPre = cb.cp().bin({"ETauFR_fail"}).process({"ZTT"}).GetUncertainty();
    
    float dfdxPre = sigRateFailPre/((sigRatePassPre+sigRateFailPre)*(sigRatePassPre+sigRateFailPre));
    float dfdyPre = - sigRatePassPre/((sigRatePassPre+sigRateFailPre)*(sigRatePassPre+sigRateFailPre));
    float errfakeratePrefit= sqrt((dfdxPre*sigErrPassPre)*(dfdxPre*sigErrPassPre)+(dfdyPre*sigErrFailPre)*(dfdyPre*sigErrFailPre));
    
    cout << "pre-fit eff. errors:" << errfakeratePrefit << endl;
    
    
}
