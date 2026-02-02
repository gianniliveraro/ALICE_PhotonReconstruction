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

void PlotRatio() {
    
    TFile* file = TFile::Open("~/O2WorkingDirectory/FindableStudy/AnalysisResults.root");
    if (!file || file->IsZombie()) {
        std::cerr << "Could not open file!" << std::endl;
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
    if (!hFindable) std::cerr << "Error: h2dPtVsCentrality_Findable not found." << std::endl;
    if (!hFound) std::cerr << "Error: h2dPtVsCentrality_Found not found." << std::endl;
    if (!hPassesTrackQuality) std::cerr << "Error: h2dPtVsCentrality_PassesTrackQuality not found." << std::endl;
    if (!hPassesTrackTopological) std::cerr << "Error: h2dPtVsCentrality_PassesTopological not found." << std::endl;
    if (!hPassesThisSpecies) std::cerr << "Error: h2dPtVsCentrality_PassesThisSpecies not found." << std::endl;
    if (!hGeneratedGamma) std::cerr << "Error: hGeneratedGamma not found." << std::endl;
    
    if (!hFindable || !hFound || !hPassesTrackQuality || !hPassesTrackTopological || !hPassesThisSpecies || !hGeneratedGamma) {
        std::cerr << "Error: Some histograms not found. Exiting." << std::endl;
        return;
    }
    
    // Create output directory
    system("mkdir -p Results/Efficiencies");
    
    // Settings
    int rebin = 5;  // Rebin factor - adjust as needed
    double centralityMin = 0.0;   // Centrality range for projection
    double centralityMax = 100.0; // Full centrality range
    double ptMin = 0.0;
    double ptMax = 5.0;
    
    // Project all 2D histograms onto pT axis (Y-axis)
    TH1F* hFindablePt = Project2D(hFindable, 'Y', centralityMin, centralityMax, rebin, 1);
    TH1F* hFoundPt = Project2D(hFound, 'Y', centralityMin, centralityMax, rebin, 2);
    TH1F* hPassesTrackQualityPt = Project2D(hPassesTrackQuality, 'Y', centralityMin, centralityMax, rebin, 3);
    TH1F* hPassesTrackTopologicalPt = Project2D(hPassesTrackTopological, 'Y', centralityMin, centralityMax, rebin, 4);
    TH1F* hPassesThisSpeciesPt = Project2D(hPassesThisSpecies, 'Y', centralityMin, centralityMax, rebin, 5);
    
    // Clone and rebin hGeneratedGamma to match
    TH1F* hGeneratedClone = (TH1F*)hGeneratedGamma->Clone("hGeneratedClone");
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
    TH1F* hEffFindableOverGenerated = (TH1F*)hFindablePt->Clone("hEffFindableOverGenerated");
    hEffFindableOverGenerated->SetDirectory(0);
    hEffFindableOverGenerated->Divide(hGeneratedClone);
    
    TCanvas* c1 = new TCanvas("c1", "Findable / Generated", 800, 600);
    c1->SetTicks(1, 1);
    c1->SetLeftMargin(0.12);
    
    hEffFindableOverGenerated->GetXaxis()->SetRangeUser(ptMin, ptMax);
    hEffFindableOverGenerated->SetLineWidth(2);
    hEffFindableOverGenerated->SetMarkerSize(0.8);
    hEffFindableOverGenerated->SetTitle("");
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
    
    c1->SaveAs("Results/Efficiencies/Efficiency_Findable_over_Generated.png");
    delete c1;
    
    // ===== PLOT 2: Found / Findable =====
    TH1F* hEffFoundOverFindable = (TH1F*)hFoundPt->Clone("hEffFoundOverFindable");
    hEffFoundOverFindable->SetDirectory(0);
    hEffFoundOverFindable->Divide(hFindablePt);
    
    TCanvas* c2 = new TCanvas("c2", "Found / Findable", 800, 600);
    c2->SetTicks(1, 1);
    c2->SetLeftMargin(0.12);
    
    hEffFoundOverFindable->GetXaxis()->SetRangeUser(ptMin, ptMax);
    hEffFoundOverFindable->SetLineWidth(2);
    hEffFoundOverFindable->SetMarkerSize(0.8);
    hEffFoundOverFindable->SetTitle("");
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
    
    c2->SaveAs("Results/Efficiencies/Efficiency_Found_over_Findable.png");
    delete c2;
    
    // ===== PLOT 3: PassesTrackQuality / Found =====
    TH1F* hEffQualityOverFound = (TH1F*)hPassesTrackQualityPt->Clone("hEffQualityOverFound");
    hEffQualityOverFound->SetDirectory(0);
    hEffQualityOverFound->Divide(hFoundPt);
    
    TCanvas* c3 = new TCanvas("c3", "PassesTrackQuality / Found", 800, 600);
    c3->SetTicks(1, 1);
    c3->SetLeftMargin(0.12);
    
    hEffQualityOverFound->GetXaxis()->SetRangeUser(ptMin, ptMax);
    hEffQualityOverFound->SetLineWidth(2);
    hEffQualityOverFound->SetMarkerSize(0.8);
    hEffQualityOverFound->SetTitle("");
    hEffQualityOverFound->GetXaxis()->SetTitle("p_{T} (GeV/#it{c})");
    hEffQualityOverFound->GetYaxis()->SetTitle("PassesTrackQuality / Found");
    hEffQualityOverFound->GetYaxis()->SetRangeUser(0.0, 1.0);
    hEffQualityOverFound->SetStats(0);
    hEffQualityOverFound->Draw("E1");
    
    TLatex latex3;
    latex3.SetNDC();
    latex3.SetTextSize(0.035);
    latex3.DrawLatex(0.17, 0.85, "#font[62]{ALICE}");
    latex3.DrawLatex(0.17, 0.80, "#font[42]{PassesTrackQuality / Found}");
    
    c3->SaveAs("Results/Efficiencies/Efficiency_PassesTrackQuality_over_Found.png");
    delete c3;
    
    // ===== PLOT 4: PassesTrackTopological / PassesTrackQuality =====
    TH1F* hEffTopologicalOverQuality = (TH1F*)hPassesThisSpeciesPt->Clone("hEffTopologicalOverQuality");
    hEffTopologicalOverQuality->SetDirectory(0);
    hEffTopologicalOverQuality->Divide(hPassesTrackQualityPt);
    
    TCanvas* c4 = new TCanvas("c4", "PassesTopological / PassesTrackQuality", 800, 600);
    c4->SetTicks(1, 1);
    c4->SetLeftMargin(0.12);
    
    hEffTopologicalOverQuality->GetXaxis()->SetRangeUser(ptMin, ptMax);
    hEffTopologicalOverQuality->SetLineWidth(2);
    hEffTopologicalOverQuality->SetMarkerSize(0.8);
    hEffTopologicalOverQuality->SetTitle("");
    hEffTopologicalOverQuality->GetXaxis()->SetTitle("p_{T} (GeV/#it{c})");
    hEffTopologicalOverQuality->GetYaxis()->SetTitle("PassesTopological / PassesQuality");
    hEffTopologicalOverQuality->GetYaxis()->SetRangeUser(0.0, 1.2);
    hEffTopologicalOverQuality->SetStats(0);
    hEffTopologicalOverQuality->Draw("E1");
    
    TLatex latex4;
    latex4.SetNDC();
    latex4.SetTextSize(0.035);
    latex4.DrawLatex(0.17, 0.85, "#font[62]{ALICE}");
    latex4.DrawLatex(0.17, 0.80, "#font[42]{PassesTopological / PassesQuality}");
    
    c4->SaveAs("Results/Efficiencies/Efficiency_PassesTopological_over_PassesQuality.png");
    delete c4;
    
    std::cout << "Done! Plots saved to Results/Efficiencies/" << std::endl;
}