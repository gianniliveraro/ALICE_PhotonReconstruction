
#include "Pythia8/Pythia.h"
#include "Pythia8/HeavyIons.h"
#include "FairGenerator.h"
#include "FairPrimaryGenerator.h"
#include "Generators/GeneratorPythia8.h"
#include "TRandom3.h"
#include "TParticlePDG.h"
#include "TDatabasePDG.h"

#include <map>
#include <unordered_set>


// Default pythia8 minimum bias generator
// Please do not change

class GeneratorPythia8ExtraStrangeness : public o2::eventgen::GeneratorPythia8
{
public:
  /// Constructor
  GeneratorPythia8ExtraStrangeness() {
    genMinPt=0.0;
    genMaxPt=20.0;
    genminY=-1.5;
    genmaxY=1.5;
    genminEta=-1.5;
    genmaxEta=1.5;
    
    pdg=0;
    E=0;
    px=0;
    py=0;
    pz=0;
    p=0;
    y=0;
    eta=0;
    xProd=0;
    yProd=0;
    zProd=0;
    xProd=0.; yProd=0.; zProd=0.;
    
    fLVHelper = std::make_unique<TLorentzVector>();
    //lutGen = new o2::eventgen::FlowMapper();
    lutGen = std::make_unique<o2::eventgen::FlowMapper>();
    
  }
  
  Double_t y2eta(Double_t pt, Double_t mass, Double_t y){
    Double_t mt = TMath::Sqrt(mass * mass + pt * pt);
    return TMath::ASinH(mt / pt * TMath::SinH(y));
  }
  
  /// set 4-momentum
  void set4momentum(double input_px, double input_py, double input_pz){
    px = input_px;
    py = input_py;
    pz = input_pz;
    E  = sqrt( m*m+px*px+py*py+pz*pz );
    fourMomentum.px(px);
    fourMomentum.py(py);
    fourMomentum.pz(pz);
    fourMomentum.e(E);
    p   = sqrt( px*px+py*py+pz*pz );
    y   = 0.5*log( (E+pz)/(E-pz) );
    eta = 0.5*log( (p+pz)/(p-pz) );
  }
   
  //__________________________________________________________________
  Pythia8::Particle createParticle(){
    //std::cout << "createParticle() mass " << m << " pdgCode " << pdg << std::endl;
    Pythia8::Particle myparticle;
    myparticle.id(pdg);
    myparticle.status(11);
    myparticle.px(px);
    myparticle.py(py);
    myparticle.pz(pz);
    myparticle.e(E);
    myparticle.m(m);
    myparticle.xProd(xProd);
    myparticle.yProd(yProd);
    myparticle.zProd(zProd);
    
    return myparticle;
  }
  
  //_________________________________________________________________________________
  /// generate uniform eta and uniform momentum
  void genSpectraMomentumEtaXi(double minP, double maxP, double minY, double maxY){
    // random generator
    std::unique_ptr<TRandom3> ranGenerator { new TRandom3() };
    ranGenerator->SetSeed(0);
    
    // generate transverse momentum
    const double gen_pT = ranGenerator->Uniform(0,5);
    
    //Actually could be something else without loss of generality but okay
    const double gen_phi = ranGenerator->Uniform(0,2*TMath::Pi());
    
    // sample flat in rapidity, calculate eta
    Double_t gen_Y=10, gen_eta=10;
    
    while( gen_eta>genmaxEta || gen_eta<genminEta ){
      gen_Y = ranGenerator->Uniform(minY,maxY);
      //(Double_t pt, Double_t mass, Double_t y)
      gen_eta = y2eta(gen_pT, m, gen_Y);
    }
    
    fLVHelper->SetPtEtaPhiM(gen_pT, gen_eta, gen_phi, m);
    set4momentum(fLVHelper->Px(),fLVHelper->Py(),fLVHelper->Pz());
  }
  
  
  //__________________________________________________________________
  Bool_t generateEvent() override {
    
    // Generate PYTHIA event
    Bool_t lPythiaOK = kFALSE;
    while (!lPythiaOK){
      lPythiaOK = mPythia.next();      
    }
       
    
    //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    // For enrichment
    // m = 1.192642;
    // pdg = 3212;
    // for(Int_t ii=0; ii<3; ii++){
    //   pdg *= gRandom->Uniform()>0.5?+1:-1;
    //   xProd=0.0;
    //   yProd=0.0;
    //   zProd=0.0;
    //   genSpectraMomentumEtaXi(genMinPt,genMaxPt,genminY,genmaxY);
    //   Pythia8::Particle lAddedParticle = createParticle();
    //   mPythia.event.append(lAddedParticle);
    //   //lAddedParticles++;
    // }
    //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    
    return true;
  }
  
private:
  
  double genMinPt;      /// minimum 3-momentum for generated particles
  double genMaxPt;      /// maximum 3-momentum for generated particles
  double genminY;    /// minimum pseudorapidity for generated particles
  double genmaxY;    /// maximum pseudorapidity for generated particles
  double genminEta;
  double genmaxEta;
  
  Pythia8::Vec4   fourMomentum;  /// four-momentum (px,py,pz,E)
  std::unique_ptr<o2::eventgen::FlowMapper> lutGen;
  
  double E;        /// energy: sqrt( m*m+px*px+py*py+pz*pz ) [GeV/c]
  double m;        /// particle mass [GeV/c^2]
  int    pdg;        /// particle pdg code
  double px;        /// x-component momentum [GeV/c]
  double py;        /// y-component momentum [GeV/c]
  double pz;        /// z-component momentum [GeV/c]
  double p;        /// momentum
  double y;        /// rapidity
  double eta;        /// pseudorapidity
  double xProd;      /// x-coordinate position production vertex [cm]
  double yProd;      /// y-coordinate position production vertex [cm]
  double zProd;      /// z-coordinate position production vertex [cm]
  
  std::unique_ptr<TLorentzVector> fLVHelper;
};

 FairGenerator *generator_extraStrangeness()
 {
   return new GeneratorPythia8ExtraStrangeness();
 }