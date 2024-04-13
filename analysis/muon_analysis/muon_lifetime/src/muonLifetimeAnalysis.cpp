// Copyright 2024 Andrea De Vita - Nicolò Salimbeni

// muonLifetimeAnalysis.cpp
// Created on: Mar 11, 2024

// cpp libraries
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using std::string;

// root libraries
#include "TCanvas.h"
#include "TF1.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TH1D.h"
#include "TLegend.h"
#include "TMath.h"
#include "TMultiGraph.h"
#include "TPaveStats.h"

#include "./../../../../include/AnUtil.h"
#include "./../../../../include/StateFile.h"
#include "./../../../../src/GrUtil.cc"
#include "./../../../../src/StateFile.cc"

TH1D *h;
TF1 *func;

/*

 - summaryPlot: it generates a comprehensive plot encompassing the fitted tau values, facilitating a side-by-side
                comparison with the theoretical value. Additionally, the weighted average fit value is calculated,
                incorporating its corresponding sigma;

 - residualsAnalysis: it fits the absolute value of the residuals w.r.t. lifetime between 10 ns to 2500 ns,
                      which is the region of negative muon lifetime regime;

 - biExponentialFit: it performs a fitting operation on the data histogram within the time range of 2000 to 15000 ns,
                     employing a negative exponential model. Subsequently, the obtained fit results serve as fixed
                     parameters for a second fitting process conducted within the 200 to 16000 ns range. This second fit
                     involves a combination of two exponentials, where one set of parameters is constrained based on the
                     outcomes of the previous fit, while the other set remains free.
                     The objective of this analysis is to investigate the initial segment of the histogram, aiming to
                     establish evidence for an additional contribution to the exponential decay, attributed to the decay
                     rate of negative muons absorbed in muonic states;

 - createFitFile: it creates a histogram from a data file, perform fits in different time ranges and
                  save results to a file.txt;

 - chi_sqr: chi squared definition with a bi-exponential fit function

 - noise_parameters_correlation: it creates a colormap of chi_sqr as a function of backgound normalization and backgound
 lifetime.

*/

//////////// MAIN FUNCTION DECLARATIONS ///////////////////
void summaryPlot(const char *resultFile = "../results/fitResult.txt");
void residualsAnalysis(const char *datafile);
void noise_parameters_correlation(const char *datafile);

//////////// MINOR FUNCTION DECLARATIONS ///////////////////
void createFitFile(const char *datafile);
void biExponentialFit(const char *datafile);
Double_t chi_sqr(Double_t *val, Double_t *par);

//////////// FUNCTION DEFINITIONS ///////////////////
void summaryPlot(const char *resultFile = "../results/fitResult.txt")
{
    /////////////////
    // IMPORT DATA //
    /////////////////

    // Open the file
    std::ifstream infile(resultFile);

    // Define vectors for each column
    std::vector<string> range;
    std::vector<double> val, pos, err, norm, chi, ndf;

    // Read the file line by line and fill the vectors
    double x, y, z, nor, c, dof;
    string r, columnNames;
    getline(infile, columnNames); // Read and discard column names
    while (infile >> y >> r >> x >> z >> nor >> c >> dof)
    {
        val.push_back(x);
        range.push_back(r);
        pos.push_back(y);
        err.push_back(z);

        norm.push_back(nor);
        chi.push_back(c);
        ndf.push_back(dof);
    }

    // Close the file
    infile.close();

    double fit_val = 0.;
    double err_val = 0.;
    // Print all values
    for (size_t i = 0; i < val.size(); ++i)
    {
        std::cout << "range: " << range[i] << "\t"
                  << "tau: " << val[i] << "\t"
                  << "sigma: " << err[i] << std::endl;
        if (i == 0)
        {
            fit_val = val[i];
            err_val = err[i];
        }
    }

    // Convert std::vectors to arrays
    double *x_arr = const_cast<double *>(val.data());
    double *y_arr = const_cast<double *>(pos.data());
    double *err_x_arr = const_cast<double *>(err.data());

    ////////////////
    // STATISTICS //
    ////////////////

    // // Compute mean and sigma
    // double mean = 0;
    // double norm = 0;

    // for (int i = 0; i < val.size(); ++i)
    // {
    //     mean += x_arr[i] / pow(err_x_arr[i], 2);
    //     norm += pow(1.0 / err_x_arr[i], 2);
    // }

    // mean = mean / norm;
    // double sigma = pow(norm, -0.5);

    //////////////
    // GRAPHICS //
    //////////////

    // Create TGraphErrors
    auto *c1 = new TCanvas("c1", "c1", 1200, 600);
    c1->SetGrid();

    TGraphErrors *graph = new TGraphErrors(val.size(), x_arr, y_arr, err_x_arr);
    graph->SetMarkerStyle(kFullSquare);

    // Create a vertical line with a coloured region
    TLine *verticalLine = new TLine(fit_val, graph->GetYaxis()->GetBinLowEdge(graph->GetYaxis()->GetFirst()), fit_val,
                                    graph->GetYaxis()->GetBinUpEdge(graph->GetYaxis()->GetLast()));
    verticalLine->SetLineColor(42);
    verticalLine->SetLineWidth(2);

    // Create a vertical line with a coloured region
    double theory_val = 2196.9811;
    double err_theory = 0.0022;
    TLine *theory = new TLine(theory_val, graph->GetYaxis()->GetBinLowEdge(graph->GetYaxis()->GetFirst()), theory_val,
                              graph->GetYaxis()->GetBinUpEdge(graph->GetYaxis()->GetLast()));
    theory->SetLineColor(kRed);
    theory->SetLineWidth(2);
    theory->SetLineStyle(9);

    // Create the coloured region
    Double_t x1[5] = {fit_val - err_val, fit_val + err_val, fit_val + err_val, fit_val - err_val, fit_val - err_val};
    Double_t y1[5] = {graph->GetYaxis()->GetBinLowEdge(graph->GetYaxis()->GetFirst()),
                      graph->GetYaxis()->GetBinLowEdge(graph->GetYaxis()->GetFirst()),
                      graph->GetYaxis()->GetBinUpEdge(graph->GetYaxis()->GetLast()),
                      graph->GetYaxis()->GetBinUpEdge(graph->GetYaxis()->GetLast()),
                      graph->GetYaxis()->GetBinLowEdge(graph->GetYaxis()->GetFirst())};

    auto excl1 = new TGraph(5, x1, y1);
    excl1->SetFillColorAlpha(41, 0.5);
    excl1->SetFillStyle(1001);

    // Create multigraph
    TMultiGraph *multigraph = new TMultiGraph();
    multigraph->SetTitle("");

    multigraph->Add(excl1, "F");
    multigraph->Add(graph, "PE");

    multigraph->GetXaxis()->SetTitle("#tau (ns)");

    // Hide the tick labels on the TAxis
    multigraph->GetYaxis()->SetLabelSize(0);
    multigraph->GetYaxis()->SetTickLength(0);

    multigraph->Draw("A*");

    verticalLine->Draw("same");
    theory->Draw("same");

    // Draw text near each point
    for (int i = 0; i < val.size(); ++i)
    {
        // Adjust the position and content of the text as needed
        double textX = x_arr[i];
        double textY = y_arr[i] + 0.1; // Offset from the point

        TText *text = new TText(textX, textY, range[i].c_str());
        text->SetTextSize(0.035);
        text->Draw();
    }

    // Draw the Legend
    TLegend leg(.57, .15, .87, .35, "Fit results");
    leg.SetFillColor(0);
    leg.AddEntry(graph, "Fitted points");
    leg.AddEntry(theory, Form("Theoretical value:   %.3f #pm %.3f [ns]", theory_val, err_theory), "l");
    leg.AddEntry(verticalLine, Form("Fit value within the full range:   %.3f #pm %.3f [ns]", fit_val, err_val), "l");
    leg.AddEntry(excl1, "1#sigma region", "f");

    leg.DrawClone("Same");

    c1->SaveAs("../figures/rangeFit.pdf");
}

void residualsAnalysis(const char *datafile)
{
    // Open data file
    std::ifstream infile(datafile);

    // Open Arietta calibration parameters file
    StateFile *calibration_parameters = new StateFile("../../../detector/arietta/results.txt");

    // Create a new canvas
    auto *c1 = new TCanvas();

    // Create a histogram for muon lifetime
    TH1D *histo = new TH1D("hist", "Muon lifetime", 200, 0, 16000);
    double x;

    double a = std::stod(calibration_parameters->ValueOf("m"));
    double b = std::stod(calibration_parameters->ValueOf("q"));

    // Fill the histogram with data from the file
    while (infile >> x)
    {
        histo->Fill(a * x + b);
    }
    infile.close();

    // Define a fit function
    TF1 *f = new TF1("f", "[0]+[1]*exp(-x/[2])", 2000, 15000);
    f->SetParNames("constant", "normalization", "decay rate [ns]");
    f->SetParLimits(0, 0, 200);
    f->SetParameter(2, 2200);

    // Perform the fit on the histogram
    histo->Fit(f, "RMN");

    // Create a graph for residuals
    TGraph *graph = new TGraph(30);
    graph->SetMarkerStyle(kFullCircle);
    graph->SetMarkerSize(0.5);

    // Fill the graph with residuals
    for (int i = 1; i <= 30; ++i)
    {
        double xValue = histo->GetBinCenter(i);
        double yValueData = histo->GetBinContent(i);

        // Calculate the fitted value and residual
        double yValueFit = f->Eval(xValue);
        double residual = yValueData - yValueFit;

        // Add point to the graph
        graph->AddPoint(xValue, TMath::Abs(residual));
    }

    // Set titles for the graph axes
    graph->SetTitle("Residuals Analysis");
    graph->GetXaxis()->SetTitle("Lifetime [ns]");
    graph->GetYaxis()->SetTitle("Residuals");

    // Draw the graph
    graph->Draw("APL");

    // Define another fit function for a subrange
    TF1 *fitfunc = new TF1("fitfunc", "[0]+[1]*exp(-x/[2])", 20, 2500);
    fitfunc->SetParNames("constant", "normalization", "decay rate [ns]");
    fitfunc->SetParameter(2, 10000);
    fitfunc->SetParLimits(0, 0, 1000);

    // Perform the fit on the graph with the new fit function
    graph->Fit(fitfunc, "R+");

    // Draw default fit parameters legend
    gStyle->SetOptFit(1);

    delete calibration_parameters;
}

// Function for bi-exponential fit
void biExponentialFit(const char *datafile)
{
    // Open Arietta calibration parameters file
    StateFile *calibration_parameters = new StateFile("../../../detector/arietta/results.txt");

    // Open data file
    std::ifstream infile(datafile);

    // Initialize ROOT canvas
    auto *c1 = new TCanvas();
    c1->SetGrid();

    // Create a histogram for muon lifetime

    int xmin = 0;
    int xmax = 16000;
    int nbins = 75;
    TH1D *histo = new TH1D("", "", nbins, xmin, xmax);

    // Read calibration parameters
    double x;
    double a = std::stod(calibration_parameters->ValueOf("m"));
    double b = std::stod(calibration_parameters->ValueOf("q"));

    // Fill the histogram using the calibrated values
    while (infile >> x)
    {
        histo->Fill(a * x + b);
    }
    infile.close();

    // Define and fit a single exponential function in the range [2000, 15000]
    TF1 *f = new TF1("f", "[0]+[1]*exp(-x/[2])", 2000, 15000);
    f->SetParNames("constant", "normalization", "decay rate [ns]");
    f->SetParLimits(0, 0, 2000);
    f->SetParameter(2, 2200);
    histo->Fit(f, "RMN");

    // Define a combined function with fixed and free parameters for the second fit
    TF1 *combF = new TF1("combF", "[0]+[1]*exp(-x/[2])+[3]*exp(-x/[4])", 200, 15000);

    combF->SetParName(0, "Const");
    combF->SetParName(1, "NormMuonExp");
    combF->SetParName(2, "MuonLifetime [ns]");
    combF->SetParName(3, "NormBkg");
    combF->SetParName(4, "BkgTime [ns]");

    combF->SetParameter(1, f->GetParameter(1));
    combF->FixParameter(2, f->GetParameter(2));

    // Set limits and initial values for background parameters
    combF->SetParLimits(0, 0, 2500);
    combF->SetParLimits(3, 0, 2500);
    combF->SetParameter(4, 1000);

    // // Fit the histogram with the combined function
    histo->Fit(combF, "RM");
    // gStyle->SetOptFit(01111);

    // Set x and y-axis titles
    histo->GetXaxis()->SetTitle("time [ns]");

    std::string ylabel = "counts / (" + std::to_string((xmax - xmin) / nbins) + " ns)";
    histo->GetYaxis()->SetTitle(ylabel.c_str());

    GrUtil::SetHTextSize(histo);
    GrUtil::SetCountDigits(histo);

    // Draw histogram
    histo->Draw("E1");

    // graphics
    gStyle->SetOptStat("rm");
    gStyle->SetOptFit(01111);
    TPaveStats *pv = (TPaveStats *)histo->GetListOfFunctions()->FindObject("stats");
    pv->SetX1NDC(0.45);
    pv->SetX2NDC(0.9);
    pv->SetY1NDC(0.55);
    pv->SetY2NDC(0.9);
    GrUtil::SetHTextSize(histo);
    GrUtil::SetCountDigits(histo);

    gPad->Update();
}

Double_t chi_sqr(Double_t *val, Double_t *par)
{

    Float_t x = val[0];
    Float_t y = val[1];

    Double_t sum = 0;

    double xMin, xMax;
    func->GetRange(xMin, xMax);
    int bin_min = h->FindBin(xMin);
    int bin_max = h->FindBin(xMax) - 1;
    for (int i = bin_min; i <= bin_max; i++)
    {
        double expected =
            par[0] + par[1] * exp(-1 * h->GetBinCenter(i) / par[2]) + x * exp(-1 * h->GetBinCenter(i) / y);
        sum += pow(h->GetBinContent(i) - expected, 2) / expected;
    }
    return sum;
}

void noise_parameters_correlation(const char *datafile)
{

    // Open Arietta calibration parameters file
    StateFile *calibration_parameters = new StateFile("../../../detector/arietta/results.txt");

    // Open data file
    std::ifstream infile(datafile);

    // Initialize ROOT canvas
    auto *c1 = new TCanvas();
    c1->SetGrid();

    // Create a histogram for muon lifetime
    h = new TH1D("", "", 75, 0, 16000);

    // Read calibration parameters
    double x;
    double a = std::stod(calibration_parameters->ValueOf("m"));
    double b = std::stod(calibration_parameters->ValueOf("q"));

    // Fill the histogram using the calibrated values
    while (infile >> x)
    {
        h->Fill(a * x + b);
    }
    infile.close();

    // Define and fit a single exponential function in the range [2000, 15000]
    TF1 *f = new TF1("f", "[0]+[1]*exp(-x/[2])", 2000, 15000);
    f->SetParNames("constant", "normalization", "decay rate [ns]");
    f->SetParLimits(0, 0, 2000);
    f->SetParameter(2, 2200);
    h->Fit(f, "RMN");

    // Define a combined function with fixed and free parameters for the second fit
    func = new TF1("combF", "[0]+[1]*exp(-x/[2])+[3]*exp(-x/[4])", 200, 15000);

    func->SetParName(0, "Const");
    func->SetParName(1, "NormMuonExp");
    func->SetParName(2, "MuonLifetime [ns]");
    func->SetParName(3, "NormBkg");
    func->SetParName(4, "BkgTime [ns]");

    func->SetParameter(1, f->GetParameter(1));
    func->FixParameter(2, f->GetParameter(2));

    // Set limits and initial values for background parameters
    func->SetParLimits(0, 0, 2500);
    func->SetParLimits(3, 0, 2000);
    func->SetParameter(4, 500);

    // // Fit the histogram with the combined function
    h->Fit(func, "RM");

    delete c1;
    delete calibration_parameters;

    // ############### DRAW THE CHI² ###########################
    TCanvas *c_chi = new TCanvas("c_chi", "c_chi", 1000, 650);

    c_chi->SetLeftMargin(0.13);
    c_chi->SetRightMargin(0.13);
    c_chi->SetBottomMargin(0.11);
    c_chi->cd();
    c_chi->SetGrid();
    c_chi->SetLogz();

    TF2 *f_chi = new TF2("f_chi", chi_sqr, 0, func->GetParameter(3) + 3.2 * func->GetParError(3), 0, 500, 3);

    f_chi->SetNpx(200);
    f_chi->SetNpy(200);

    f_chi->SetParameter(0, func->GetParameter(0));
    f_chi->SetParameter(1, func->GetParameter(1));
    f_chi->SetParameter(2, func->GetParameter(2));

    f_chi->GetXaxis()->SetLabelSize(0.047);
    f_chi->GetXaxis()->SetLabelOffset(0.015);
    f_chi->GetXaxis()->SetTitleSize(0.05);
    f_chi->GetXaxis()->SetTitleOffset(1.01);

    f_chi->GetYaxis()->SetLabelSize(0.047);
    f_chi->GetYaxis()->SetTitleSize(0.05);
    f_chi->GetYaxis()->SetNdivisions(5, 5, 0, true);
    f_chi->GetYaxis()->SetTitleOffset(1.3);

    f_chi->GetZaxis()->SetLabelSize(0.047);
    f_chi->GetZaxis()->SetTitleSize(0.05);
    f_chi->GetZaxis()->SetNdivisions(5, 10, 20, true);
    f_chi->GetZaxis()->SetTitleOffset(0.5);

    f_chi->GetXaxis()->SetTitle("bkg normalization");
    f_chi->GetYaxis()->SetTitle("bkg #tau [ns]");
    f_chi->GetZaxis()->SetTitle("#chi^{2}");
    f_chi->SetTitle("");

    f_chi->Draw("CONT4Z");
    c_chi->SaveAs("../figures/bifit_background_correlation.png");
}

void createFitFile(const char *datafile)
{

    // Open the data file for reading
    std::ifstream infile(datafile);

    // Create a histogram for muon lifetime
    TH1D *histo = new TH1D("hist", "Muon lifetime", 80, 0, 16000);
    double x;

    // Open Arietta calibration parameters file
    StateFile *calibration_parameters = new StateFile("../../../detector/arietta/results.txt");

    // Retrieve calibration parameters
    double a = std::stod(calibration_parameters->ValueOf("m"));
    double b = std::stod(calibration_parameters->ValueOf("q"));

    std::cout << "Fit is starting:" << std::endl;
    std::cout << "Calibration parameters:\t"
              << "m: " << a << "   q: " << b << std::endl;

    // Fill the histogram with calibrated data
    while (infile >> x)
    {
        histo->Fill(a * x + b);
    }
    infile.close();

    // Open a file to store fit results
    std::ofstream resultFile("./../results/fitResult.txt");

    resultFile << "Fit_number\tRange\tTau\tSigma\tNorm\tChi2\tnDOF" << std::endl;
    // Loop through different fit configurations
    for (int i = 1; i < 5; ++i)
    {
        // Create a fit function
        TF1 *f = new TF1("f", "[0]+[1]*exp(-x/[2])", 200, 15000);
        f->SetParNames("constant", "normalization", "decay rate [ns]");

        // Set fit range based on iteration
        if (i == 1)
        {
            f->SetRange(200, 15000);
        }
        else if (i == 2)
        {
            f->SetRange(200, 2000);
        }
        else if (i == 3)
        {
            f->SetRange(2000, 3700);
        }
        else if (i == 4)
        {
            f->SetRange(3700, 15000);
        }

        // Set parameter limits and initial values
        f->SetParLimits(0, 0, 200);
        f->SetParameter(1, 2000);
        f->SetParameter(2, 2200);

        // Print fit results to the result file
        Double_t xmin, xmax;
        f->GetRange(xmin, xmax);
        std::cout << "\nFitting range: " << xmin << " - " << xmax << std::endl;

        // Fit the function to the histogram
        histo->Fit(f, "RML");

        resultFile << i << "\t";
        resultFile << Form("%.0f-%.0f", xmin, xmax) << "\t";
        resultFile << f->GetParameter(2) << "\t";
        resultFile << f->GetParError(2) << "\t";
        resultFile << f->GetParameter(1) << "\t";
        resultFile << f->GetChisquare() << "\t";
        resultFile << f->GetNDF() << std::endl;

        std::cout << "_________________________________\n\n" << std::endl;
        // Clean up memory by deleting the fit function object
        delete f;
    }
    resultFile.close();
}

Double_t myFunction(Double_t *x, Double_t *params)
{
    Double_t c = params[0];
    Double_t n = params[1];
    Double_t tau = params[2];
    return c + n * exp(-x[0] / tau);
}

void test(const char *datafile, const int nbins, const int init, const int middle)
{
    // Open Arietta calibration parameters file
    StateFile *calibration_parameters = new StateFile("../../../detector/arietta/results.txt");

    // Open data file
    std::ifstream infile(datafile);

    // Initialize ROOT canvas
    auto *c1 = new TCanvas();
    c1->SetGrid();

    // Create a histogram for muon lifetime

    int xmin = 0;
    int xmax = 16000;
    TH1D *histo = new TH1D("title", "title", nbins, xmin, xmax);

    // Read calibration parameters
    double x;
    double a = std::stod(calibration_parameters->ValueOf("m"));
    double b = std::stod(calibration_parameters->ValueOf("q"));

    // Fill the histogram using the calibrated values
    while (infile >> x)
    {
        histo->Fill(a * x + b);
    }
    infile.close();

    // Define the function
    TF1 *fitFunc1 = new TF1("fitFunc", myFunction, init, middle, 3); // assuming range [0, 10]
    TF1 *fitFunc2 = new TF1("fitFunc", myFunction, middle, 9000, 3); // assuming range [0, 10]
    TF1 *fitFunc3 = new TF1("fitFunc", myFunction, init, 9000, 3);   // assuming range [0, 10]

    fitFunc1->SetParameter(0, 20);
    fitFunc1->SetParameter(1, 2500);
    fitFunc1->SetParameter(2, 2194);

    fitFunc2->SetParameter(0, 20);
    fitFunc2->SetParameter(1, 2500);
    fitFunc2->SetParameter(2, 2194);

    fitFunc3->SetParameter(0, 20);
    fitFunc3->SetParameter(1, 2500);
    fitFunc3->SetParameter(2, 2194);

    fitFunc1->SetParLimits(0, 0, 100);
    fitFunc2->SetParLimits(0, 0, 100);
    fitFunc3->SetParLimits(0, 0, 100);

    // fitFunc->SetParNames("c", "n", "tau");
    histo->Fit(fitFunc1, "RMLN");
    histo->Fit(fitFunc2, "RMLN");
    histo->Fit(fitFunc3, "RMLN");

    // Set x and y-axis titles
    histo->GetXaxis()->SetTitle("time [ns]");

    std::string ylabel = "counts / (" + std::to_string((xmax - xmin) / nbins) + " ns)";
    histo->GetYaxis()->SetTitle(ylabel.c_str());

    // fitFunc1->SetRange(0,15000);
    //  // Draw histogram
    histo->Draw("E1");
    fitFunc1->SetLineColor(kRed);
    fitFunc1->Draw("same");

    fitFunc2->SetLineColor(kBlue);
    fitFunc2->Draw("same");

    fitFunc3->SetLineColor(kGreen);
    fitFunc3->Draw("same");
    // gStyle->SetOptFit(01111);
    gStyle->SetOptFit(1);
    gPad->SetLogy();
}
