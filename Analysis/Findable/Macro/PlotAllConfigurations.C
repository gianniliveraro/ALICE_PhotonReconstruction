#include <TFile.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TLatex.h>
#include <TStyle.h>
#include <iostream>
#include <vector>

TH1F* Project2D(TH2F* h, char axis, double xmin, double xmax, int rebin, int number) {
    if (!h) return nullptr;
    
    int x1 = h->GetXaxis()->FindBin(xmin);
    int x2 = h->GetXaxis()->FindBin(xmax);
    
    TH1F* h1 = nullptr;
    if (axis == 'Y')
        h1 = (TH1F*)h->ProjectionY(Form("h_%d", number), x1, x2);
    else if (axis == 'X')
        h1 = (TH1F*)h->ProjectionX(Form("h_%d", number), x1, x2);
    else {
        std::cerr << "Invalid axis!" << std::endl;
        return nullptr;
    }
    
    if (rebin > 1) h1->Rebin(rebin);
    return h1;
}

void ProcessFile(const char* filename, int chi2Value, int tpcValue, int uniqueID) {
    
    TFile* file = TFile::Open(filename);
    if (!file || file->IsZombie()) {
        std::cerr << "Could not open file: " << filename << std::endl;
        return;
    }
    
    // Load histograms from findable-study directory
    TH2F* hFindable = (TH2F*)file->Get("findable-study/h2dPtVsCentrality_Findable");
    TH2F* hFound = (TH2F*)file->Get("findable-study/h2dPtVsCentrality_Found");
    TH2F* hPassesTrackQuality = (TH2F*)file->Get("findable-study/h2dPtVsCentrality_PassesTrackQuality");
    TH2F* hPassesTrackTopological = (TH2F*)file->Get("findable-study/h2dPtVsCentrality_PassesTopological");
    TH2F* hPassesThisSpecies = (TH2F*)file->Get("findable-study/h2dPtVsCentrality_PassesThisSpecies");
    
    // Load histograms from strangederivedbuilder directory
    TH1F* hGeneratedGamma = (TH1F*)file->Get("strangederivedbuilder/hGeneratedGamma");
    
    // Check if histograms are loaded
    if (!hFindable || !hFound || !hPassesTrackQuality || !hPassesTrackTopological || !hPassesThisSpecies || !hGeneratedGamma) {
        std::cerr << "Error: Some histograms not found in " << filename << ". Skipping." << std::endl;
        file->Close();
        return;
    }
    
    // Create output directory
    TString outDir = Form("Results/Findable/Chi2%d/Tpc%d", chi2Value, tpcValue);
    gSystem->Exec(Form("mkdir -p %s", outDir.Data()));
    
    // Configuration label for plots
    TString configLabel = Form("cutMatchingChi2 = %d, minTPCRows = %d", chi2Value, tpcValue);
    
    // Settings
    int rebin = 5;
    double centralityMin = 0.0;
    double centralityMax = 100.0;
    double ptMin = 0.0;
    double ptMax = 5.0;
    
    // Project all 2D histograms onto pT axis (Y-axis)
    TH1F* hFindablePt = Project2D(hFindable, 'Y', centralityMin, centralityMax, rebin, uniqueID * 10 + 1);
    TH1F* hFoundPt = Project2D(hFound, 'Y', centralityMin, centralityMax, rebin, uniqueID * 10 + 2);
    TH1F* hPassesTrackQualityPt = Project2D(hPassesTrackQuality, 'Y', centralityMin, centralityMax, rebin, uniqueID * 10 + 3);
    TH1F* hPassesTrackTopologicalPt = Project2D(hPassesTrackTopological, 'Y', centralityMin, centralityMax, rebin, uniqueID * 10 + 4);
    TH1F* hPassesThisSpeciesPt = Project2D(hPassesThisSpecies, 'Y', centralityMin, centralityMax, rebin, uniqueID * 10 + 5);
    
    // Clone and rebin hGeneratedGamma to match
    TH1F* hGeneratedClone = (TH1F*)hGeneratedGamma->Clone(Form("hGeneratedClone_%d", uniqueID));
    hGeneratedClone->SetDirectory(0);
    if (rebin > 1) hGeneratedClone->Rebin(rebin);
    
    // Set Sumw2 for all histograms
    hFindablePt->Sumw2();
    hFoundPt->Sumw2();
    hPassesTrackQualityPt->Sumw2();
    hPassesTrackTopologicalPt->Sumw2();
    hPassesThisSpeciesPt->Sumw2();
    hGeneratedClone->Sumw2();
    
    // ===== PLOT 1: Findable / hGeneratedGamma =====
    TH1F* hEffFindableOverGenerated = (TH1F*)hFindablePt->Clone(Form("hEffFindableOverGenerated_%d", uniqueID));
    hEffFindableOverGenerated->SetDirectory(0);
    hEffFindableOverGenerated->Divide(hGeneratedClone);
    
    TCanvas* c1 = new TCanvas(Form("c1_%d", uniqueID), "Findable / Generated", 800, 600);
    c1->SetTicks(1, 1);
    c1->SetLeftMargin(0.12);
    
    hEffFindableOverGenerated->GetXaxis()->SetRangeUser(ptMin, ptMax);
    hEffFindableOverGenerated->SetLineWidth(2);
    hEffFindableOverGenerated->SetMarkerSize(0.8);
    hEffFindableOverGenerated->SetTitle(configLabel);
    hEffFindableOverGenerated->GetXaxis()->SetTitle("p_{T} (GeV/#it{c})");
    hEffFindableOverGenerated->GetYaxis()->SetTitle("Findable / Generated");
    hEffFindableOverGenerated->GetYaxis()->SetRangeUser(0.0, 1.2);
    hEffFindableOverGenerated->SetStats(0);
    hEffFindableOverGenerated->Draw("E1");
    
    TLatex latex1;
    latex1.SetNDC();
    latex1.SetTextSize(0.035);
    latex1.DrawLatex(0.17, 0.85, "#font[62]{ALICE}");
    latex1.DrawLatex(0.17, 0.80, "#font[42]{Findable / Generated}");
    
    c1->SaveAs(Form("%s/Findable_Gen.png", outDir.Data()));
    delete c1;
    
    // ===== PLOT 2: Found / Findable =====
    TH1F* hEffFoundOverFindable = (TH1F*)hFoundPt->Clone(Form("hEffFoundOverFindable_%d", uniqueID));
    hEffFoundOverFindable->SetDirectory(0);
    hEffFoundOverFindable->Divide(hFindablePt);
    
    TCanvas* c2 = new TCanvas(Form("c2_%d", uniqueID), "Found / Findable", 800, 600);
    c2->SetTicks(1, 1);
    c2->SetLeftMargin(0.12);
    
    hEffFoundOverFindable->GetXaxis()->SetRangeUser(ptMin, ptMax);
    hEffFoundOverFindable->SetLineWidth(2);
    hEffFoundOverFindable->SetMarkerSize(0.8);
    hEffFoundOverFindable->SetTitle(configLabel);
    hEffFoundOverFindable->GetXaxis()->SetTitle("p_{T} (GeV/#it{c})");
    hEffFoundOverFindable->GetYaxis()->SetTitle("Found / Findable");
    hEffFoundOverFindable->GetYaxis()->SetRangeUser(0.0, 1.2);
    hEffFoundOverFindable->SetStats(0);
    hEffFoundOverFindable->Draw("E1");
    
    TLatex latex2;
    latex2.SetNDC();
    latex2.SetTextSize(0.035);
    latex2.DrawLatex(0.17, 0.85, "#font[62]{ALICE}");
    latex2.DrawLatex(0.17, 0.80, "#font[42]{Found / Findable}");
    
    c2->SaveAs(Form("%s/Found_Findable.png", outDir.Data()));
    delete c2;
    
    // ===== PLOT 3: PassesTrackQuality / Found =====
    TH1F* hEffQualityOverFound = (TH1F*)hPassesTrackQualityPt->Clone(Form("hEffQualityOverFound_%d", uniqueID));
    hEffQualityOverFound->SetDirectory(0);
    hEffQualityOverFound->Divide(hFoundPt);
    
    TCanvas* c3 = new TCanvas(Form("c3_%d", uniqueID), "PassesTrackQuality / Found", 800, 600);
    c3->SetTicks(1, 1);
    c3->SetLeftMargin(0.12);
    
    hEffQualityOverFound->GetXaxis()->SetRangeUser(ptMin, ptMax);
    hEffQualityOverFound->SetLineWidth(2);
    hEffQualityOverFound->SetMarkerSize(0.8);
    hEffQualityOverFound->SetTitle(configLabel);
    hEffQualityOverFound->GetXaxis()->SetTitle("p_{T} (GeV/#it{c})");
    hEffQualityOverFound->GetYaxis()->SetTitle("PassesTrackQuality / Found");
    hEffQualityOverFound->GetYaxis()->SetRangeUser(0.0, 1.2);
    hEffQualityOverFound->SetStats(0);
    hEffQualityOverFound->Draw("E1");
    
    TLatex latex3;
    latex3.SetNDC();
    latex3.SetTextSize(0.035);
    latex3.DrawLatex(0.17, 0.85, "#font[62]{ALICE}");
    latex3.DrawLatex(0.17, 0.80, "#font[42]{PassesTrackQuality / Found}");
    
    c3->SaveAs(Form("%s/TrackQ_Found.png", outDir.Data()));
    delete c3;
    
    // ===== PLOT 4: PassesTopological / PassesTrackQuality =====
    TH1F* hEffTopologicalOverQuality = (TH1F*)hPassesThisSpeciesPt->Clone(Form("hEffTopologicalOverQuality_%d", uniqueID));
    hEffTopologicalOverQuality->SetDirectory(0);
    hEffTopologicalOverQuality->Divide(hPassesTrackQualityPt);
    
    TCanvas* c4 = new TCanvas(Form("c4_%d", uniqueID), "PassesTopological / PassesTrackQuality", 800, 600);
    c4->SetTicks(1, 1);
    c4->SetLeftMargin(0.12);
    
    hEffTopologicalOverQuality->GetXaxis()->SetRangeUser(ptMin, ptMax);
    hEffTopologicalOverQuality->SetLineWidth(2);
    hEffTopologicalOverQuality->SetMarkerSize(0.8);
    hEffTopologicalOverQuality->SetTitle(configLabel);
    hEffTopologicalOverQuality->GetXaxis()->SetTitle("p_{T} (GeV/#it{c})");
    hEffTopologicalOverQuality->GetYaxis()->SetTitle("PassesTopological / PassesTrackQuality");
    hEffTopologicalOverQuality->GetYaxis()->SetRangeUser(0.0, 1.2);
    hEffTopologicalOverQuality->SetStats(0);
    hEffTopologicalOverQuality->Draw("E1");
    
    TLatex latex4;
    latex4.SetNDC();
    latex4.SetTextSize(0.035);
    latex4.DrawLatex(0.17, 0.85, "#font[62]{ALICE}");
    latex4.DrawLatex(0.17, 0.80, "#font[42]{PassesTopological / PassesTrackQuality}");
    
    c4->SaveAs(Form("%s/Top_TrackQ.png", outDir.Data()));
    delete c4;
    
    file->Close();
    
    std::cout << "Processed: " << filename << " -> " << outDir << std::endl;
}

void PlotAllConfigurations() {
    
    // Base path - adjust as needed
    TString basePath = "~/O2WorkingDirectory/ALICE_PhotonReconstruction/Findable/Macro/";
    
    // Configuration arrays
    std::vector<int> chi2Values = {30, 100};
    std::vector<int> tpcValues = {15, 25, 35, 50, 100};
    
    // Process all 15 files
    // Test0-4:   chi2 = 30,   tpc = 15, 25, 35, 50, 100
    // Test5-9:   chi2 = 100,  tpc = 15, 25, 35, 50, 100
    // Test10-14: chi2 = 1000, tpc = 15, 25, 35, 50, 100
    
    int fileIndex = 0;
    for (int iChi2 = 0; iChi2 < (int)chi2Values.size(); iChi2++) {
        for (int iTpc = 0; iTpc < (int)tpcValues.size(); iTpc++) {
            
            TString filename = Form("%sTest%d_AnalysisResults.root", basePath.Data(), fileIndex);
            
            ProcessFile(filename.Data(), chi2Values[iChi2], tpcValues[iTpc], fileIndex);
            
            fileIndex++;
        }
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Done! All plots saved to Results/Findable/" << std::endl;
    std::cout << "Directory structure:" << std::endl;
    std::cout << "  Results/Findable/Chi2[30|100|1000]/Tpc[15|25|35|50|100]/" << std::endl;
    std::cout << "    - Findable_Gen.png" << std::endl;
    std::cout << "    - Found_Findable.png" << std::endl;
    std::cout << "    - TrackQ_Found.png" << std::endl;
    std::cout << "    - Top_TrackQ.png" << std::endl;
    std::cout << "========================================" << std::endl;
}