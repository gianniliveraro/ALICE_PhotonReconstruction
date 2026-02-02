#include <TFile.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TLatex.h>
#include <TStyle.h>
#include <TSystem.h>
#include <iostream>

TH1F* Project2D_Comb(TH2F* h, char axis, double xmin, double xmax, int rebin, int number) {
    if (!h) return nullptr;
    
    int x1 = h->GetXaxis()->FindBin(xmin);
    int x2 = h->GetXaxis()->FindBin(xmax);
    
    TH1F* h1 = nullptr;
    if (axis == 'Y')
        h1 = (TH1F*)h->ProjectionY(Form("hComb_%d", number), x1, x2);
    else if (axis == 'X')
        h1 = (TH1F*)h->ProjectionX(Form("hComb_%d", number), x1, x2);
    else {
        std::cerr << "Invalid axis!" << std::endl;
        return nullptr;
    }
    
    if (rebin > 1) h1->Rebin(rebin);
    return h1;
}

void GetAllRatios_Comb(const char* filename, int rebin, double centralityMin, double centralityMax, int uniqueID,
                       TH1F*& hFindableOverGen, TH1F*& hFoundOverFindable, TH1F*& hTrackQOverFound, TH1F*& hTopOverTrackQ) {
    
    hFindableOverGen = nullptr;
    hFoundOverFindable = nullptr;
    hTrackQOverFound = nullptr;
    hTopOverTrackQ = nullptr;
    
    TFile* file = TFile::Open(filename);
    if (!file || file->IsZombie()) {
        std::cerr << "Could not open file: " << filename << std::endl;
        return;
    }
    
    TH2F* hFindable = (TH2F*)file->Get("findable-study/h2dPtVsCentrality_Findable");
    TH2F* hFound = (TH2F*)file->Get("findable-study/h2dPtVsCentrality_Found");
    TH2F* hPassesTrackQuality = (TH2F*)file->Get("findable-study/h2dPtVsCentrality_PassesTrackQuality");
    TH2F* hPassesThisSpecies = (TH2F*)file->Get("findable-study/h2dPtVsCentrality_PassesThisSpecies");
    TH1F* hGeneratedGamma = (TH1F*)file->Get("strangederivedbuilder/hGeneratedGamma");
    
    if (!hFindable || !hFound || !hPassesTrackQuality || !hPassesThisSpecies || !hGeneratedGamma) {
        std::cerr << "Error: Histograms not found in " << filename << std::endl;
        file->Close();
        return;
    }
    
    // Project 2D histograms
    TH1F* hFindablePt = Project2D_Comb(hFindable, 'Y', centralityMin, centralityMax, rebin, uniqueID * 100 + 1);
    TH1F* hFoundPt = Project2D_Comb(hFound, 'Y', centralityMin, centralityMax, rebin, uniqueID * 100 + 2);
    TH1F* hTrackQPt = Project2D_Comb(hPassesTrackQuality, 'Y', centralityMin, centralityMax, rebin, uniqueID * 100 + 3);
    TH1F* hTopPt = Project2D_Comb(hPassesThisSpecies, 'Y', centralityMin, centralityMax, rebin, uniqueID * 100 + 4);
    
    // Clone and rebin generated
    TH1F* hGenClone = (TH1F*)hGeneratedGamma->Clone(Form("hGenComb_%d", uniqueID));
    hGenClone->SetDirectory(0);
    if (rebin > 1) hGenClone->Rebin(rebin);
    
    hFindablePt->Sumw2();
    hFoundPt->Sumw2();
    hTrackQPt->Sumw2();
    hTopPt->Sumw2();
    hGenClone->Sumw2();
    
    // Findable / Generated
    hFindableOverGen = (TH1F*)hFindablePt->Clone(Form("hFindableOverGenComb_%d", uniqueID));
    hFindableOverGen->SetDirectory(0);
    hFindableOverGen->Divide(hGenClone);
    
    // Found / Findable
    hFoundOverFindable = (TH1F*)hFoundPt->Clone(Form("hFoundOverFindableComb_%d", uniqueID));
    hFoundOverFindable->SetDirectory(0);
    hFoundOverFindable->Divide(hFindablePt);
    
    // TrackQuality / Found
    hTrackQOverFound = (TH1F*)hTrackQPt->Clone(Form("hTrackQOverFoundComb_%d", uniqueID));
    hTrackQOverFound->SetDirectory(0);
    hTrackQOverFound->Divide(hFoundPt);
    
    // Topological / TrackQuality
    hTopOverTrackQ = (TH1F*)hTopPt->Clone(Form("hTopOverTrackQComb_%d", uniqueID));
    hTopOverTrackQ->SetDirectory(0);
    hTopOverTrackQ->Divide(hTrackQPt);
}

void PlotCombinedCanvas_Comb(TH1F* histos[], int tpcRowValues[], int nHistos,
                              int chi2Value, const char* yTitle, const char* plotName,
                              const char* outDir, double ptMin, double ptMax) {
    
    int colors[] = {kBlack, kRed+1, kBlue+1, kGreen+2, kMagenta+1};
    int markers[] = {20, 21, 22, 23, 33};
    
    TCanvas* c = new TCanvas(Form("cComb_%s_%d", plotName, chi2Value), plotName, 800, 600);
    c->SetTicks(1, 1);
    c->SetLeftMargin(0.12);
    c->SetRightMargin(0.05);
    
    TLegend* leg = new TLegend(0.55, 0.15, 0.90, 0.45);
    leg->SetBorderSize(0);
    leg->SetFillStyle(0);
    leg->SetTextSize(0.033);
    leg->SetHeader(Form("cutMatchingChi2 = %d", chi2Value));
    
    TH1F* hFirst = nullptr;
    for (int i = 0; i < nHistos; i++) {
        if (!histos[i]) continue;
        
        histos[i]->SetLineColor(colors[i]);
        histos[i]->SetMarkerColor(colors[i]);
        histos[i]->SetMarkerStyle(markers[i]);
        histos[i]->SetLineWidth(2);
        histos[i]->SetMarkerSize(0.8);
        histos[i]->GetXaxis()->SetRangeUser(ptMin, ptMax);
        histos[i]->GetYaxis()->SetRangeUser(0.0, 1.2);
        histos[i]->SetTitle("");
        histos[i]->GetXaxis()->SetTitle("p_{T} (GeV/#it{c})");
        histos[i]->GetYaxis()->SetTitle(yTitle);
        histos[i]->SetStats(0);
        
        if (!hFirst) {
            histos[i]->Draw("E1");
            hFirst = histos[i];
        } else {
            histos[i]->Draw("E1 SAME");
        }
        leg->AddEntry(histos[i], Form("minTPCRows = %d", tpcRowValues[i]), "lep");
    }
    
    leg->Draw();
    
    TLatex latex;
    latex.SetNDC();
    latex.SetTextSize(0.035);
    latex.DrawLatex(0.17, 0.85, "#font[62]{ALICE}");
    latex.DrawLatex(0.17, 0.80, Form("#font[42]{%s}", yTitle));
    latex.DrawLatex(0.17, 0.75, Form("#font[42]{cutMatchingChi2 = %d}", chi2Value));
    
    c->SaveAs(Form("%s/%s.png", outDir, plotName));
    delete c;
}

void CombinedPlot() {
    
    // Settings
    int rebin = 5;
    double centralityMin = 0.0;
    double centralityMax = 100.0;
    double ptMin = 0.0;
    double ptMax = 5.0;
    
    // Base path - adjust as needed
    TString basePath = "~/O2WorkingDirectory/ALICE_PhotonReconstruction/FindableStudy/Macro/";
    
    // Configuration arrays
    const int nChi2 = 2;
    const int nTpc = 5;
    int chi2Values[nChi2] = {30, 100};
    int tpcRowValues[nTpc] = {15, 25, 35, 50, 100};
    
    // File index mapping: Test0-4 -> chi2=30, Test5-9 -> chi2=100
    
    for (int iChi2 = 0; iChi2 < nChi2; iChi2++) {
        
        int chi2 = chi2Values[iChi2];
        int startIndex = iChi2 * 5;
        
        // Create output directory
        TString outDir = Form("Results/Findable/Chi2%d/Combined", chi2);
        gSystem->Exec(Form("mkdir -p %s", outDir.Data()));
        
        // Arrays to hold histograms for each ratio type
        TH1F* hFindableOverGen[nTpc];
        TH1F* hFoundOverFindable[nTpc];
        TH1F* hTrackQOverFound[nTpc];
        TH1F* hTopOverTrackQ[nTpc];
        
        // Load all 5 files for this chi2 value
        for (int iTpc = 0; iTpc < nTpc; iTpc++) {
            int fileIndex = startIndex + iTpc;
            TString filename = Form("%sTest%d_AnalysisResults.root", basePath.Data(), fileIndex);
            
            GetAllRatios_Comb(filename.Data(), rebin, centralityMin, centralityMax, fileIndex,
                              hFindableOverGen[iTpc], hFoundOverFindable[iTpc], 
                              hTrackQOverFound[iTpc], hTopOverTrackQ[iTpc]);
        }
        
        // Create 4 combined plots for this chi2 value
        PlotCombinedCanvas_Comb(hFindableOverGen, tpcRowValues, nTpc, chi2, 
                                "Findable / Generated", "Findable_Gen", outDir.Data(), ptMin, ptMax);
        
        PlotCombinedCanvas_Comb(hFoundOverFindable, tpcRowValues, nTpc, chi2, 
                                "Found / Findable", "Found_Findable", outDir.Data(), ptMin, ptMax);
        
        PlotCombinedCanvas_Comb(hTrackQOverFound, tpcRowValues, nTpc, chi2, 
                                "PassesTrackQuality / Found", "TrackQ_Found", outDir.Data(), ptMin, ptMax);
        
        PlotCombinedCanvas_Comb(hTopOverTrackQ, tpcRowValues, nTpc, chi2, 
                                "PassesTopological / PassesTrackQuality", "Top_TrackQ", outDir.Data(), ptMin, ptMax);
        
        std::cout << "Completed chi2 = " << chi2 << " -> " << outDir << std::endl;
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Done! All combined plots saved to:" << std::endl;
    std::cout << "  Results/Findable/Chi230/Combined/" << std::endl;
    std::cout << "  Results/Findable/Chi2100/Combined/" << std::endl;
    std::cout << "Each contains:" << std::endl;
    std::cout << "  - Findable_Gen.png" << std::endl;
    std::cout << "  - Found_Findable.png" << std::endl;
    std::cout << "  - TrackQ_Found.png" << std::endl;
    std::cout << "  - Top_TrackQ.png" << std::endl;
    std::cout << "========================================" << std::endl;
}