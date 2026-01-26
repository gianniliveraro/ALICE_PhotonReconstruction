#if !defined(__CLING__) || defined(__ROOTCLING__)
#include <memory>
#include <string>
#include <TCanvas.h>
#include <TChain.h>
#include <TFile.h>
#include <TH2D.h>
#include <TTree.h>
#include <TGeoGlobalMagField.h>
#include <TMatrix.h>
#include <TMatrixD.h>

#include <TStopwatch.h>

#include <FairEventHeader.h>
#include <FairGeoParSet.h>
#include <FairLogger.h>
#include <FairMCEventHeader.h>

#include "DetectorsCommonDataFormats/DetID.h"
#include "DataFormatsITSMFT/CompCluster.h"
#include "DataFormatsITSMFT/TopologyDictionary.h"
#include "DataFormatsITSMFT/ROFRecord.h"
#include "DataFormatsParameters/GRPObject.h"
#include "DetectorsBase/GeometryManager.h"
#include "DetectorsBase/Propagator.h"
#include "Field/MagneticField.h"
#include "ITSBase/GeometryTGeo.h"
#include "ITSReconstruction/CookedTracker.h"
#include "MathUtils/Utils.h"
#include "SimulationDataFormat/MCCompLabel.h"
#include "SimulationDataFormat/MCTruthContainer.h"
#include "ReconstructionDataFormats/Vertex.h"
#include "CommonUtils/NameConf.h"
#include "ReconstructionDataFormats/TrackTPCITS.h"
#include "ReconstructionDataFormats/PrimaryVertex.h"
#include "ReconstructionDataFormats/V0.h"
#include "ReconstructionDataFormats/Cascade.h"
#include "CommonDataFormat/RangeReference.h"
#include "ITStracking/Configuration.h"
#include "ITStracking/IOUtils.h"
#include "ITStracking/Tracker.h"
//#include "ITStracking/TrackerTraitsCPU.h"
#include "ITStracking/Vertexer.h"
#include "ITStracking/VertexerTraits.h"
#include "DataFormatsTPC/TrackTPC.h"
#include "ReconstructionDataFormats/MatchInfoTOF.h"
#include "ReconstructionDataFormats/TrackTPCTOF.h"
#include "ReconstructionDataFormats/GlobalTrackID.h"
#include "CCDB/BasicCCDBManager.h"
#include "CCDB/CCDBTimeStampUtils.h"

#include "StrangenessTracking/StrangenessTracker.h"

#include "SimulationDataFormat/MCTrack.h"
#include "SimulationDataFormat/MCEventHeader.h"
#include "SimulationDataFormat/MCEventLabel.h"
#include "SimulationDataFormat/MCTruthContainer.h"

#endif
#include "DCAFitter/DCAFitterN.h"
#include "RecoDecay.h"

void resetTrackParCov(o2::track::TrackParCov& track){
  //resets parameters to avoid confusion. 
  track.setX(0);
  track.setY(0);
  track.setZ(0);
  track.setSnp(0);
  track.setTgl(0);
  track.setQ2Pt(1e-6);
}

bool propagateToReference(o2::track::TrackParCov& track, float refX = 70.0){ 
    ///----------- aux stuff --------------///
  static constexpr float MaxSnp = 0.9;                 // max snp of ITS or TPC track at xRef to be matched
  
  // Prepare track to match conditions found in the ITSTPC matching
  o2::base::Propagator::MatCorrType matCorr = o2::base::Propagator::MatCorrType::USEMatCorrNONE;
  //o2::base::Propagator::MatCorrType matCorr = o2::base::Propagator::MatCorrType::USEMatCorrLUT;
  //o2::base::Propagator::MatCorrType matCorr = o2::base::Propagator::MatCorrType::USEMatCorrTGeo;

  return o2::base::Propagator::Instance()->PropagateToXBxByBz(track, refX, MaxSnp, 2., matCorr);

  // rotate alpha 
  auto alphaNew = o2::math_utils::angle2Alpha(track.getPhiPos());
  if (!track.rotate(alphaNew) != 0) return false; 
}

void runMatcherStudy01( TString lPath = "..", TString outputstring = "itstpcmatching_qa.root", int lIndex = 1){
  std::cout<<"\e[1;31m***********************************************\e[0;00m"<<std::endl;
  std::cout<<"\e[1;31m     ITSTPC matcher debug study \e[0;00m"<<std::endl;
  std::cout<<"\e[1;31m***********************************************\e[0;00m"<<std::endl;
  //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  // define parameters 
  float lReferenceX = 70.0f; // from Ruben's default (is this a good idea?)

  // Connect to all relevant trees
    //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  // Open ITS
  cout<<"Opening ITS track file..."<<endl;
  TFile *fitstracks = new TFile(Form( "%s/o2trac_its.root", lPath.Data()), "READ");
  if ( !fitstracks->IsOpen() ){
    cout<<"Problem with path, stopping now"<<endl; 
    return; 
  }

  fitstracks->ls();
  TTree *fTitstracks = (TTree*) fitstracks->Get("o2sim");
  std::vector<o2::MCCompLabel>* mMCITSTrackArray = new std::vector<o2::MCCompLabel>;
  std::vector<o2::its::TrackITS>* mITSTrackArray = new std::vector<o2::its::TrackITS>;
  
  fTitstracks->SetBranchAddress("ITSTrackMCTruth", &mMCITSTrackArray);
  fTitstracks->SetBranchAddress("ITSTrack", &mITSTrackArray);
  
  fTitstracks->GetEntry(0);
  if(fTitstracks->GetEntries()>1) cout<<"MORE THAN ONE TREE ENTRY DETECTED?"<<endl;
  cout<<"Number of ITS MC refs detected = "<<mMCITSTrackArray->size()<<endl;
  //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  // Open TPC matched tracks
  cout<<"Opening TPC track file..."<<endl;
  TFile *ftpctracks = new TFile(Form( "%s/tpctracks.root", lPath.Data()), "READ");
  if ( !ftpctracks->IsOpen() ){
    cout<<"Problem with path, stopping now"<<endl; 
    return; 
  }
  ftpctracks->ls();
  TTree *fTPCTtracks = (TTree*) ftpctracks->Get("tpcrec");
  std::vector<o2::tpc::TrackTPC>* mTPCTrackArray = new std::vector<o2::tpc::TrackTPC>;
  std::vector<o2::MCCompLabel>* mMCTPCTrackArray = new std::vector<o2::MCCompLabel>;
  
  fTPCTtracks->SetBranchAddress("TPCTracks", &mTPCTrackArray);
  fTPCTtracks->SetBranchAddress("TPCTracksMCTruth", &mMCTPCTrackArray);
  
  fTPCTtracks->GetEntry(0);
  if(fTPCTtracks->GetEntries()>1) cout<<"MORE THAN ONE TREE ENTRY DETECTED?"<<endl;
  cout<<"Number of TPC tracks detected = "<<mTPCTrackArray->size()<<endl;
  cout<<"Number of TPC MC refs detected = "<<mMCTPCTrackArray->size()<<endl;
  //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  // Open ITSTPC matched tracks
  cout<<"Opening ITSTPC matched track file..."<<endl;
  TFile *ftracks = new TFile(Form( "%s/o2match_itstpc.root", lPath.Data()), "READ");
  if ( !ftracks->IsOpen() ){
    cout<<"Problem with path, stopping now"<<endl; 
    return; 
  }
  ftracks->ls();
  TTree *fTtracks = (TTree*) ftracks->Get("matchTPCITS");
  std::vector<o2::dataformats::TrackTPCITS>* mTrackArray = new std::vector<o2::dataformats::TrackTPCITS>;
  std::vector<o2::MCCompLabel>* mMCTrackArray = new std::vector<o2::MCCompLabel>;
  
  fTtracks->SetBranchAddress("TPCITS", &mTrackArray);
  fTtracks->SetBranchAddress("MatchMCTruth", &mMCTrackArray);
  
  fTtracks->GetEntry(0);
  if(fTtracks->GetEntries()>1) cout<<"MORE THAN ONE TREE ENTRY DETECTED?"<<endl;
  cout<<"Number of tracks detected = "<<mTrackArray->size()<<endl;
  cout<<"Number of MC refs detected = "<<mMCTrackArray->size()<<endl;
  //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  TFile *fkine = new TFile(Form( "%s/sgn_%i_Kine.root", lPath.Data(), lIndex),"READ") ;
  if ( !fkine->IsOpen() ){
    cout<<"Problem with path, stopping now"<<endl; 
    return; 
  }
  auto mcTree = (TTree*)fkine->Get("o2sim");
  o2::dataformats::MCEventHeader* mcHead = nullptr;
  mcTree->SetBranchStatus("*", 0); //disable all branches
  mcTree->SetBranchStatus("MCTrack*", 1);
  mcTree->SetBranchStatus("MCEventHeader.*", 1);
  mcTree->SetBranchAddress("MCEventHeader.", &mcHead);
  std::vector<o2::MCTrack>* mcArr = nullptr;
  mcTree->SetBranchAddress("MCTrack", &mcArr);
  cout<<"kine Tree entry count = "<<mcTree->GetEntries()<<endl;
  //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //Open GRP
  const auto grp = o2::parameters::GRPObject::loadFrom(Form("%s/o2sim_grp.root",lPath.Data()));
  if (!grp) {
    LOG(FATAL) << "Cannot run w/o GRP object";
  }
  
  o2::base::Propagator::initFieldFromGRP(grp);
  auto field = static_cast<o2::field::MagneticField*>(TGeoGlobalMagField::Instance()->GetField());
  if (!field) {
    LOG(FATAL) << "Failed to load magnetic field";
  }
    //Operational parameters
  const Double_t lMagneticField = field->GetBz(0,0,0);
  cout<<"Magnetic field auto-detected to be "<<lMagneticField<<endl;
  //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  //___________________________________________________________________________
  //Setup output TTree containing per-MC-particle properties 
  //of the reconstructed tracks 
  TFile *fout = new TFile(outputstring.Data(), "RECREATE");

  TTree *fTreeParticles = new TTree ( "fTreeParticles", "Reconstruction characterization tree" ) ;

  // Simple presence in file 
  Bool_t recoITS, recoTPC, recoITSTPC, recoITSTPCfake; //marks if track present in corresponding list 
  fTreeParticles->Branch ("recoITS",  &recoITS,  "recoITS/O"  );
  fTreeParticles->Branch ("recoTPC",  &recoTPC,  "recoTPC/O"  );
  fTreeParticles->Branch ("recoITSTPC",  &recoITSTPC,  "recoITSTPC/O"  );

  Bool_t refXokITS, refXokTPC, refXokITSTPC; //marks if track went to ref X
  fTreeParticles->Branch ("refXokITS",  &refXokITS,  "refXokITS/O"  );
  fTreeParticles->Branch ("refXokTPC",  &refXokTPC,  "refXokTPC/O"  );
  fTreeParticles->Branch ("refXokITSTPC",  &refXokITSTPC,  "refXokITSTPC/O"  );

  // MC information: creation vertex, momentum, PDG code
  Float_t vXmc, vYmc, vZmc, pXmc, pYmc, pZmc; 
  Int_t pdg; 
  fTreeParticles->Branch ("vXmc",  &vXmc,  "vXmc/F"  );
  fTreeParticles->Branch ("vYmc",  &vYmc,  "vYmc/F"  );
  fTreeParticles->Branch ("vZmc",  &vZmc,  "vZmc/F"  );
  fTreeParticles->Branch ("pXmc",  &pXmc,  "pXmc/F"  );
  fTreeParticles->Branch ("pYmc",  &pYmc,  "pYmc/F"  );
  fTreeParticles->Branch ("pZmc",  &pZmc,  "pZmc/F"  );
  fTreeParticles->Branch ("pdg",  &pdg,  "pdg/I"  );

  // Store corresponding TrackParCovs for posterior lighter-weight analysis of specific selections
  o2::its::TrackITS trackITS;
  o2::tpc::TrackTPC trackTPC;
  o2::dataformats::TrackTPCITS trackITSTPC;

  fTreeParticles->Branch("trackITS",&trackITS);
  fTreeParticles->Branch("trackTPC",&trackTPC);
  fTreeParticles->Branch("trackITSTPC",&trackITSTPC);
  //___________________________________________________________________________
  // Initialize some basic event counters and so on 
  TH1F *hEventCounter = new TH1F("hEventCounter", "", 1,0,1); 
  TH1F *hGenK0ShortPt = new TH1F("hGenK0ShortPt", "", 100,0,10); 

  Int_t nBinsRadius = 200; 
  Float_t maxRadius = 50; 

  // Initialize some interesting counters to determine matching probabilities 
  TH1F *hTrackCounterVsPtTPC = new TH1F("hTrackCounterVsPtTPC","",100,0,10); 
  TH1F *hTrackCounterVsPtITS = new TH1F("hTrackCounterVsPtITS","",100,0,10); 
  TH1F *hTrackCounterVsPtITSTPC = new TH1F("hTrackCounterVsPtITSTPC","",100,0,10); 
  TH1F *hTrackCounterVsPtMatched = new TH1F("hTrackCounterVsPtMatched","",100,0,10);
  TH1F *hTrackCounterVsPtMatchedFake = new TH1F("hTrackCounterVsPtMatchedFake","",100,0,10);

  TH1F *hTrackCounterVsRadiusTPC = new TH1F("hTrackCounterVsRadiusTPC","",nBinsRadius,0,maxRadius); 
  TH1F *hTrackCounterVsRadiusITS = new TH1F("hTrackCounterVsRadiusITS","",nBinsRadius,0,maxRadius); 
  TH1F *hTrackCounterVsRadiusITSTPC = new TH1F("hTrackCounterVsRadiusITSTPC","",nBinsRadius,0,maxRadius); 
  TH1F *hTrackCounterVsRadiusMatched = new TH1F("hTrackCounterVsRadiusMatched","",nBinsRadius,0,maxRadius);
  TH1F *hTrackCounterVsRadiusMatchedFake = new TH1F("hTrackCounterVsRadiusMatchedFake","",nBinsRadius,0,maxRadius);

  TH2F *hTrackCounterVsPtVsRadiusTPC = new TH2F("hTrackCounterPtVsVsRadiusTPC","",100,0,10, nBinsRadius,0,maxRadius); 
  TH2F *hTrackCounterVsPtVsRadiusITS = new TH2F("hTrackCounterPtVsVsRadiusITS","",100,0,10, nBinsRadius,0,maxRadius); 
  TH2F *hTrackCounterVsPtVsRadiusITSTPC = new TH2F("hTrackCounterPtVsVsRadiusITSTPC","",100,0,10, nBinsRadius,0,maxRadius); 
  TH2F *hTrackCounterVsPtVsRadiusMatched = new TH2F("hTrackCounterPtVsVsRadiusMatched","",100,0,10, nBinsRadius,0,maxRadius);
  TH2F *hTrackCounterVsPtVsRadiusMatchedFake = new TH2F("hTrackCounterPtVsVsRadiusMatchedFake","",100,0,10, nBinsRadius,0,maxRadius);

  TH2F *hMomentumResolutionTPC = new TH2F("hMomentumResolutionTPC", "", 100,0,10,40,-1,1); 
  TH2F *hMomentumResolutionITS = new TH2F("hMomentumResolutionITS", "", 100,0,10,40,-1,1); 
  TH2F *hMomentumResolutionMatched = new TH2F("hMomentumResolutionMatched", "", 100,0,10,40,-1,1); 
  TH2F *hMomentumResolutionMatchedFake = new TH2F("hMomentumResolutionMatchedFake", "", 100,0,10,40,-1,1); 

  //___________________________________________________________________________

  // cross-check all parameters tested in the ITSTPC matcher
  // 1) delta-tgl, delta-tgl in Nsigma
  // 2) delta-Y, delta-Y in Nsigma
  // 3) delta-Z, delta-Z in Nsigma
  // 4) delta-snp, delta-snp in Nsigma
  // 5) delta-q2pt, delta-q2pt in Nsigma
  // 6) predicted chi2

  int nBinsMatchVariables = 1000;

  TH1F *hDeltaY = new TH1F("hDeltaY", "", nBinsMatchVariables,-20,20); 
  TH1F *hDeltaZ = new TH1F("hDeltaZ", "", nBinsMatchVariables,-20,20); 
  TH1F *hDeltaTgl = new TH1F("hDeltaTgl", "", nBinsMatchVariables,-20,20); 
  TH1F *hDeltaSnp = new TH1F("hDeltaSnp", "", nBinsMatchVariables,-20,20); 
  TH1F *hDeltaQ2Pt = new TH1F("hDeltaQ2Pt", "", nBinsMatchVariables,-20,20); 
  
  TH1F *hMatchedDeltaY = new TH1F("hMatchedDeltaY", "", nBinsMatchVariables,-20,20); 
  TH1F *hMatchedDeltaZ = new TH1F("hMatchedDeltaZ", "", nBinsMatchVariables,-20,20); 
  TH1F *hMatchedDeltaTgl = new TH1F("hMatchedDeltaTgl", "", nBinsMatchVariables,-20,20); 
  TH1F *hMatchedDeltaSnp = new TH1F("hMatchedDeltaSnp", "", nBinsMatchVariables,-20,20); 
  TH1F *hMatchedDeltaQ2Pt = new TH1F("hMatchedDeltaQ2Pt", "", nBinsMatchVariables,-20,20); 
  //___________________________________________________________________________
  // Identify MC labels of particles of interest
  for (int iEvent{0}; iEvent < mcTree->GetEntriesFast(); ++iEvent) {
    mcTree->GetEvent(iEvent);
    cout<<"Looping over event number "<<iEvent<<"; Nparticles = "<<mcArr->size()<<endl;
    hEventCounter->Fill(0.5);
    for (Long_t iii=0; iii< mcArr->size(); iii++ ){
      auto part = mcArr->at(iii);
      if( part.GetPdgCode()  == 310){
        hGenK0ShortPt -> Fill( part.GetPt() );
        if( part.getFirstDaughterTrackId() < 0 || part.getLastDaughterTrackId() < 0) continue;
              
        //trick to get rid of kPDeltaRay electrons! check last daughters only

        for(Int_t idau=part.getLastDaughterTrackId()-1; idau<part.getLastDaughterTrackId()+1; idau++){ 
          auto daughter = mcArr->at(idau);
          if(daughter.getProcess()!=4) continue; //not decay product, skip
        
          //______ STORE DAUGHTER INFO ______
          //step 1: acquire MC properties
          pdg = daughter.GetPdgCode(); 
          vXmc = daughter.Vx();
          vYmc = daughter.Vy();
          vZmc = daughter.Vz();
          pXmc = daughter.Px();
          pYmc = daughter.Py();
          pZmc = daughter.Pz();

          // skip if pdg not like charged pion 
          if (TMath::Abs(pdg)<120) continue; 

          recoTPC = kFALSE;
          recoITS = kFALSE;
          recoITSTPC = kFALSE;
          resetTrackParCov(trackITS);
          resetTrackParCov(trackTPC);
          resetTrackParCov(trackITSTPC);

          //Bool_t refXokITS, refXokTPC, refXokITSTPC;
          //step 2: check for TPC track, assign if found
          for (int j = 0; j < mTPCTrackArray->size(); j++) {
            o2::MCCompLabel lLabel = mMCTPCTrackArray->at(j);
            if(iEvent!=lLabel.getEventID()) continue; //very stupid, I know, but it works
            if( lLabel.getTrackID() == idau ) {
              recoTPC = kTRUE;
              trackTPC = mTPCTrackArray->at(j);
              refXokTPC = propagateToReference(trackTPC);
            }
          }
          //step 3: check for ITS track, assign if found
          for (int j = 0; j < mMCITSTrackArray->size(); j++) {
            o2::MCCompLabel lLabel = mMCITSTrackArray->at(j);
            if(iEvent!=lLabel.getEventID()) continue; //very stupid, I know, but it works
            if( lLabel.getTrackID() == idau ) {
              recoITS = kTRUE;
              trackITS = mITSTrackArray->at(j);
              refXokITS = propagateToReference(trackITS);
            }
          }
          //step 3: check for ITSTPC matched track, assign if found
          recoITSTPCfake = false;
          for (int j = 0; j < mMCTrackArray->size(); j++) {
            o2::MCCompLabel lLabel = mMCTrackArray->at(j);
            if(iEvent!=lLabel.getEventID()) continue; //very stupid, I know, but it works
            if( lLabel.getTrackID() == idau ) {
              trackITSTPC = mTrackArray->at(j);
              o2::dataformats::GlobalTrackID globalID = trackITSTPC.getRefITS();
              if( globalID.getSource() == o2::dataformats::GlobalTrackID::ITSAB ){
                resetTrackParCov(trackITSTPC);
                continue; 
              }
              // if you're here, it's not an afterburned ITSTPC match
              recoITSTPC = kTRUE;
              if(lLabel.isFake()) recoITSTPCfake = true;
              refXokITSTPC = propagateToReference(trackITSTPC);
            }
          }
          //fTreeParticles->Fill();

          float pt = std::hypot(pXmc, pYmc);
          float radius = std::hypot(vXmc, vYmc);

          // fill some basic qa histograms 
          if(recoTPC){ 
            hTrackCounterVsPtTPC->Fill(pt);
            hTrackCounterVsRadiusTPC->Fill(radius);
            hTrackCounterVsPtVsRadiusTPC->Fill(pt,radius);
            hMomentumResolutionTPC->Fill(pt, trackTPC.getPt()-pt);
          }
          if(recoITS){ 
            hTrackCounterVsPtITS->Fill(pt);
            hTrackCounterVsRadiusITS->Fill(radius);
            hTrackCounterVsPtVsRadiusITS->Fill(pt,radius);
            hMomentumResolutionITS->Fill(pt, trackITS.getPt()-pt);
          }
          if(recoITS && recoTPC){ 
            hTrackCounterVsPtITSTPC->Fill(pt);
            hTrackCounterVsRadiusITSTPC->Fill(radius);
            hTrackCounterVsPtVsRadiusITSTPC->Fill(pt,radius);

            hDeltaY->Fill( trackTPC.getY() - trackITS.getY() );
            hDeltaZ->Fill( trackTPC.getZ() - trackITS.getZ() );
            hDeltaTgl->Fill( trackTPC.getTgl() - trackITS.getTgl() );
            hDeltaSnp->Fill( trackTPC.getSnp() - trackITS.getSnp() );
            hDeltaQ2Pt->Fill( trackTPC.getCharge2Pt() - trackITS.getCharge2Pt() );
          }
          if(recoITSTPC){ // matched
            hTrackCounterVsPtMatched->Fill(pt);
            hTrackCounterVsRadiusMatched->Fill(radius);
            hTrackCounterVsPtVsRadiusMatched->Fill(pt,radius);

            hMatchedDeltaY->Fill( trackTPC.getY() - trackITS.getY() );
            hMatchedDeltaZ->Fill( trackTPC.getZ() - trackITS.getZ() );
            hMatchedDeltaTgl->Fill( trackTPC.getTgl() - trackITS.getTgl() );
            hMatchedDeltaSnp->Fill( trackTPC.getSnp() - trackITS.getSnp() );
            hMatchedDeltaQ2Pt->Fill( trackTPC.getCharge2Pt() - trackITS.getCharge2Pt() );
            hMomentumResolutionMatched->Fill(pt, trackITSTPC.getPt()-pt);

            if(recoITSTPC && recoITSTPCfake){
              hTrackCounterVsPtMatchedFake->Fill(pt);
              hTrackCounterVsRadiusMatchedFake->Fill(radius);
              hTrackCounterVsPtVsRadiusMatchedFake->Fill(pt,radius);
              hMomentumResolutionMatchedFake->Fill(pt, trackITSTPC.getPt()-pt);
            }
          }
        }
      }
    }
  }
  cout<<"Finished populating TTree. Entries: "<<fTreeParticles->GetEntries()<<endl; 
  fTreeParticles->Write(); 
  fout->Write(); 
  fout->Close(); 
}
