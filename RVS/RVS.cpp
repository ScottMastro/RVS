#include "stdafx.h"
#include "RVS.h"

#include <iostream>  
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>


void generateForR(MatrixXd X, VectorXd Y, MatrixXd Z, VectorXd G, MatrixXd P, std::map<int, int> readGroup) {
	std::ofstream Xfile("C:/Users/Scott/Desktop/RVS-master/example/X.txt");
	std::ofstream Yfile("C:/Users/Scott/Desktop/RVS-master/example/Y.txt");
	std::ofstream Pfile("C:/Users/Scott/Desktop/RVS-master/example/P.txt");
	std::ofstream Mfile("C:/Users/Scott/Desktop/RVS-master/example/M.txt");
	std::ofstream Zfile("C:/Users/Scott/Desktop/RVS-master/example/Z.txt");

	int precise = 18;

	if (Mfile.is_open())
	{
		Mfile << "ID\thrg\n";

		for (size_t i = 0; i < G.rows(); i++) {

			Mfile << G[i];
			Mfile << '\t';
			Mfile << readGroup[G[i]];
			Mfile << '\n';
		}
		Mfile.close();
	}


	if (Yfile.is_open())
	{
		for (size_t i = 0; i < Y.rows(); i++) {
			Yfile << std::setprecision(precise) << Y[i];
			Yfile << '\n';
		}
		Yfile.close();
	}

	if (Xfile.is_open())
	{
		for (size_t i = 0; i < X.cols(); i++) {
			for (size_t j = 0; j < X.rows(); j++) {

				if (isnan(X(j, i)))
					Xfile << "NA";
				else
					Xfile << std::setprecision(precise) << X(j, i);

				if (j < X.rows() - 1)
					Xfile << '\t';
			}
			Xfile << '\n';
		}

		Xfile.close();
	}

	if (Pfile.is_open())
	{
		for (size_t i = 0; i < P.rows(); i++) {
			Pfile << std::setprecision(precise) << P(i, 0);
			Pfile << '\t';
			Pfile << std::setprecision(precise) << P(i, 1);
			Pfile << '\t';
			Pfile << std::setprecision(precise) << P(i, 2);
			Pfile << '\n';
		}

		Pfile.close();
	}

	if (Zfile.is_open())
	{
		for (size_t i = 0; i < Z.rows(); i++) {
			for (size_t j = 0; j < Z.cols(); j++) {
				Zfile << std::setprecision(precise) << Z(i, j);

				if (j < Z.cols() - 1)
					Zfile << '\t';
			}

			Zfile << '\n';
		}

		Zfile.close();
	}
}

SimulationRequestGroup newSimulationRequestGroup(int groupID,
	std::string n, std::string caseControl, std::string highLow, 
	std::string meanDepth, std::string sdDepth) {

	std::string id = std::to_string(groupID);

	SimulationRequestGroup requestGroup;

	try { requestGroup.n = std::abs(std::stoi(n)); }
	catch (...) { throw std::invalid_argument("group " + id + " sample size,integer"); }
	if (caseControl != "case" && caseControl != "control")
		throw std::invalid_argument("group " + id + " cohort,one of 'case' or 'control'");
	requestGroup.isCase = (caseControl == "case"); 
	if (highLow != "high" && highLow != "low")
		throw std::invalid_argument("group " + id + " read depth,one of 'high' or 'low'");
	requestGroup.isHrg = (highLow == "high"); 

	try { requestGroup.meanDepth = std::abs(std::stod(meanDepth)); }
	catch (...) { throw std::invalid_argument("group " + id + " mean read depth,decimal"); }
	try { requestGroup.sdDepth = std::abs(std::stod(sdDepth)); }
	catch (...) { throw std::invalid_argument("group " + id + " read depth standard deviation,decimal"); }
	
	return requestGroup;
}

void validateSimulationRequest(SimulationRequest request) {

	int nsample = 0;
	for (SimulationRequestGroup group : request.groups)
		nsample += group.n;

	if (nsample > request.npop)
		throw std::domain_error("Population size (value given: " +
			std::to_string(request.npop) +
			") should be greater than sample size (summed across groups: " +
			std::to_string(nsample) + ").");

	if (request.npop <= 0)
		throw std::domain_error("Population size (value given: " +
			std::to_string(request.npop) +
			") should be greater than zero.");

	if (request.nsnp <= 0)
		throw std::domain_error("Number of variants (value given: " +
			std::to_string(request.nsnp) +
			") should be greater than zero.");

	if (request.prevalence > 1)
		throw std::domain_error("Revelance (value given: " +
			std::to_string(request.prevalence) +
			") should be a value between 0 and 1.");

	if (request.maf > 0.5)
		throw std::domain_error("Minor allele frequency (value given: " +
			std::to_string(request.maf) +
			") should be a value between 0 and 0.5.");

	if (request.groups.size() < 2)
		throw std::domain_error("Simulation requires at least two groups (" +
			std::to_string(request.groups.size()) + " given).");


	//if (!request.useCommonTest && request.nsnp < 5)
		//throw std::domain_error("Rare test requires number of variants to be at least 5 (value given: " +
			//std::to_string(request.nsnp) + ").");

	if (request.useBootstrap && request.nboot < 1)
		throw std::domain_error("Nuber of bootstrap iterations should be at least 1 (value given: " +
			std::to_string(request.nboot) + ").");

	int ncase = 0;
	int ncontrol = 0;
	for (SimulationRequestGroup g : request.groups) {
		if (g.isCase)
			ncase++;
		if (!g.isCase)
			ncontrol++;

		if(g.n < 1)
			throw std::domain_error("Each group should have a sample size of at least 1.");
	}

	if (ncase < 1 || ncontrol < 1) 		
		throw std::domain_error("Case/control simulation requires at least one case and one control group (" +
			std::to_string(ncase) + " case and " +
			std::to_string(ncontrol) + " control groups given).");

}


SimulationRequest newSimulationRequest(std::string npop, std::string prevalence,
	std::string nsnp, std::string me, std::string sde, std::string oddsRatio,
	std::string maf, std::vector<SimulationRequestGroup> groups,
	std::string test, bool useBootstrap, std::string nboot) {

	SimulationRequest request;

	try { request.npop = std::abs(std::stoi(npop)); }
	catch (...) { throw std::invalid_argument("population size,integer"); }
	try { request.prevalence = std::abs(std::stod(prevalence)); }
	catch (...) { throw std::invalid_argument("prevalence,decimal"); }
	try { request.nsnp = std::abs(std::stoi(nsnp)); }
	catch (...) { throw std::invalid_argument("number of variants,integer"); }
	try { request.me = std::abs(std::stod(me)); }
	catch (...) { throw std::invalid_argument("sequencing error rate,decimal"); }
	try { request.sde = std::abs(std::stod(sde)); }
	catch (...) { throw std::invalid_argument("sequencing error standard deviation,integer"); }	
	try { request.oddsRatio = std::abs(std::stod(oddsRatio)); }
	catch (...) { throw std::invalid_argument("odds ratio,decimal"); }
	try { request.maf = std::abs(std::stod(maf)); }
	catch (...) { throw std::invalid_argument("Minor allele frequency,decimal"); }

	request.test = test;
	request.useBootstrap = useBootstrap;

	if (useBootstrap) {
		try { request.nboot = std::abs(std::stoi(nboot)); }
		catch (...) { throw std::invalid_argument("# bootstrap iterations,integer"); }
	}

	request.groups = groups;

	try { validateSimulationRequest(request); }
	catch (...) { throw; }

	return request;
}

std::vector<double> startVikNGS(Request req) {
	
	std::vector<double> p;

	VectorXd Y, G; MatrixXd X, Z, P;
	std::map<int, int> readGroup;
	std::vector<std::vector<int>> interval;

	TestInput input = parseAndFilter(req);

	bool useCovariates = Z.rows() > 0;

	std::vector<double> pval;

	if (req.useCommon())
		pval = runCommonTest(req, input);
	else
		pval = runRareTest(req, input);

	/*
		if (req.useBootstrap && useCovariates)
			pval = runCommonTest(X, Y, Z, G, readGroup, P, req.nboot);
		else if(req.useBootstrap && !useCovariates)
			pval = runCommonTest(X, Y, G, readGroup, P, req.nboot);
		else if (!req.useBootstrap && useCovariates)
			pval = runCommonTest(X, Y, Z, G, readGroup, P);
		else
			pval = runCommonTest(X, Y, G, readGroup, P);

	}
	else {
		if (useCovariates)
			pval = runRareTest(X, Y, Z, G, readGroup, P, req.nboot);
		else
			pval = runRareTest(X, Y, G, readGroup, P, req.nboot);
	}
	*/
	std::string outputDir = "C:/Users/Scott/Desktop/out.txt";
	//outputPvals(pval, outputDir);

	return pval;
}

std::vector<double> startSimulation(SimulationRequest req) {

	VectorXd Y, G; MatrixXd X, P;
	std::map<int, int> readGroup;

	simulate(req, X, Y, G, readGroup, P);

	std::vector<double> pval;

	if (req.test == "common"){
		
		//if(req.useBootstrap)
	//		pval = runCommonTest(X, Y, G, readGroup, P, req.nboot);
	//	else
	//		pval = runCommonTest(X, Y, G, readGroup, P);

	}
//	else
//		pval = runRareTest(X, Y, G, readGroup, P, req.nboot, req.test);

    std::string outputDir = "C:/Users/Scott/Desktop/out.txt";
  //  outputPvals(pval, outputDir);

    return pval;
}

SimulationRequest testSimulationRequest() {
	std::vector<SimulationRequestGroup> groups;
	groups.push_back(newSimulationRequestGroup(0, "300", "control", "low", "10", "5"));
	groups.push_back(newSimulationRequestGroup(0, "200", "case", "high", "40", "7"));
	groups.push_back(newSimulationRequestGroup(0, "100", "control", "high", "50", "6"));

    return newSimulationRequest("3000", "0.1", "100", "0.01", "0.025", "1.4", "0.1", groups, "common", false, 0);
}



#include "CLI11.hpp"

int main(int argc, char* argv[]) {

	CLI::App app{ "vikNGS Variant Association Tool" };

	std::string filename = "default";
	
	// -------------------------------------
	bool common;
	CLI::Option *c = app.add_flag("-c,--common", common, "Perform a common variant association test (default)");
	bool rare;
	CLI::Option *r = app.add_flag("-r,--rare", rare, "Perform a rare variant association test");
	r->excludes(c);
	// -------------------------------------

	// -------------------------------------
	int threads;
	CLI::Option *t = app.add_option("-t,--threads", threads, "Number of threads", 1);
	t->check(CLI::Range(1, 2147483647));

	int nboot;
	CLI::Option *n = app.add_option("-n,--boot", nboot, "Number of bootstrap iterations to calculate");
	n->check(CLI::Range(1, 2147483647));

	bool stopEarly;
	CLI::Option *s = app.add_flag("-s,--stop", stopEarly, "Stop bootstrapping if p-value looks to be > 0.05");
	// -------------------------------------

	// -------------------------------------
	std::string bedDir;
	CLI::Option *b = app.add_option("-b,--bed", bedDir, "Specify a directory of a BED file for collapsing variants");
	b->check(CLI::ExistingFile);

	bool byGene;
	CLI::Option *gene = app.add_option("--gene", byGene, "Collapse variants by gene if BED file specified (default)");
	bool byExon;
	CLI::Option *exon = app.add_option("--exon", byExon, "Collapse variants by exon if BED file specified");
	bool byCoding;
	CLI::Option *coding = app.add_option("--coding", byCoding, "Collapse variants by coding if BED file specified");
	gene->excludes(exon);
	gene->excludes(coding);
	exon->excludes(coding);
	gene->requires(b);
	exon->requires(b);
	coding->requires(b);
	// -------------------------------------

	// -------------------------------------
	double maf;
	CLI::Option *m = app.add_option("-m,--maf", maf, "Minor allele frequency cut-off (common-rare threshold)", 0.05);
	m->check(CLI::Range(0.0, 1.0));

	int depth;
	CLI::Option *d = app.add_option("-d,--depth", depth, "Read depth cut-off (low-high read depth threshold)", 30);
	d->check(CLI::Range(1, 2147483647));

	double missing;
	CLI::Option *x = app.add_option("-x,--missing", missing, "Missing data cut-off (variants with a proportion of missing data more than this threshold will not be tested)", 0.1);
	x->check(CLI::Range(0.0, 0.5));

	bool all;
	CLI::Option *a = app.add_flag("-a,--all", all, "Include variants which do not have PASS in the FILTER column");
	// -------------------------------------

	// -------------------------------------
	std::string vcfDir;
	CLI::Option *i = app.add_option("vcf,-i,--vcf", vcfDir, "Specify a directory of a multisample VCF file (required)");
	i->required();
	i->check(CLI::ExistingFile);

	std::string sampleDir;
	CLI::Option *g = app.add_option("sample,-g,--sample", sampleDir, "Specify a directory of a TXT file containing sample information (required)");
	g->required();
	g->check(CLI::ExistingFile);

	std::string outputDir;
	CLI::Option *o = app.add_option("-o,--out", outputDir, "Specify a directory for output (default = current directory)", ".");
	g->check(CLI::ExistingDirectory);

	// -------------------------------------

	CLI11_PARSE(app, argc, argv);

	std::string newline = "\n";

	std::cout << "Starting vikNGS..." << newline;
	std::cout << "VCF file: " << vcfDir << newline;
	std::cout << "Sample info file: " << sampleDir << newline;
	std::cout << "Output directory: " << sampleDir << newline;




	std::cout << sampleDir;

	initializeRequest(vcfDir, sampleDir);
/*
	for (int i = 1; i < argc; ++i) {

		if (std::string(argv[i]) == "-m" || std::string(argv[i]) == "--maf") {
			double maf = std::stod(argv[i++]);
			setMAFCutoff(maf);
		}
		if (std::string(argv[i]) == "-d" || std::string(argv[i]) == "--depth") {
			int depth = std::stoi(argv[i++]);
			setHighLowCutOff(depth);
		}
		if (std::string(argv[i]) == "-x" || std::string(argv[i]) == "--missing") {
			double missing = std::stoi(argv[i++]);
			setNumberThreads(missing);
		}
		if (std::string(argv[i]) == "-a" || std::string(argv[i]) == "--all") {
			setMustPASS(false);
		}


		if (std::string(argv[i]) == "-t" || std::string(argv[i]) == "--threads") {
			int nthreads = std::stoi(argv[i++]);
			setNumberThreads(nthreads);
		}

		if (std::string(argv[i]) == "-r" || std::string(argv[i]) == "--rare") {
			useRareTest();
		}
		if (std::string(argv[i]) == "-c" || std::string(argv[i]) == "--common") {
			useRareTest();
		}
		if (std::string(argv[i]) == "-n" || std::string(argv[i]) == "--nboot") {
			int nboot = std::stoi(argv[i++]);
			useBootstrap(nboot);
		}
		if (std::string(argv[i]) == "-s" || std::string(argv[i]) == "--stop") {
			double stopParam = std::stod(argv[i++]);
			setStopEarly(stopParam);
		}
		
		if (std::string(argv[i]) == "-b" || std::string(argv[i]) == "--bed") {
			setCollapseFile(bedDir);
		}		std::string bedDir = std::string(argv[i++]);
	





		if (std::string(argv[i]) == "-h" || std::string(argv[i]) == "--help") {
			//todo
		}


		if (std::string(argv[i]) == "--destination") {

			if (i + 1 < argc) { // Make sure we aren't at the end of argv!
				destination = argv[i++]; // Increment 'i' so we don't get the argument as the next argv[i].
			}
			else { // Uh-oh, there was no argument to the destination option.
				std::cerr << "--destination option requires one argument." << std::endl;
				return 1;
			}
		}
		else {
			sources.push_back(argv[i]);
		}
	}
	return move(sources, destination);
	
}


*/
	//keep console open while debugging
	//TODO: be sure to remove eventually!
	std::cout << "\ndone...>";
	while (true) {}
	return 0;
}

int main_old() {

    //TODO: take as input from command line
    //---------------------------------------
	bool simulation = false;

	///input files
    //std::string vcfDir = "C:/Users/Scott/Desktop/vcf/chr7_case_control.vcf";
    //std::string infoDir = "C:/Users/Scott/Desktop/vcf/sampleInfo.txt";

	std::string vcfDir = "C:/Users/Scott/Desktop/vcf/example_1000snps.vcf";
	std::string infoDir = "C:/Users/Scott/Desktop/vcf/sampleInfo.txt";

	//std::string vcfDir = "C:/Users/Scott/Desktop/vcf/step3_1.vcf";
	//std::string vcfDir = "C:/Users/Scott/Desktop/vcf/step5_2.vcf";

	//std::string infoDir = "C:/Users/Scott/Desktop/vcf/step3_sampleinfo.txt";

    std::string bedDir = "";
    //std::string bedDir = "C:/Users/Scott/Desktop/RVS-master/example/chr11.bed";

    std::string outputDir = "C:/Users/Scott/Desktop/out.txt";

	initializeRequest(vcfDir, infoDir);
	useCommonTest();
	setHighLowCutOff(30);
	
	//setCollapseFile(bedDir);
	//setCollapseCoding();

	setMAFCutoff(0.05);
	setMissingThreshold(0.5);
	setMustPASS(false);
	setOnlySNPs(true);
	
	useBootstrap(10000);
	setStopEarly(false);
	setRVS(true);
	Request req = getRequest();
	

    //---------------------------------------

    //TODO: check to see if file can be opened when another application is using it (excel)
    //TODO: test windows vs unix EOF characters, doesn't seem to work well with windows



    if (simulation) {
		VectorXd Y, G; MatrixXd X, Z, P;
		std::map<int, int> readGroup;
		std::vector<std::vector<int>> interval;

        simulate(testSimulationRequest(), X, Y, G, readGroup, P);
       
		std::vector<double> pvals;
		//todo = runCommonTest(req, X, Y, G, readGroup, P);

        std::cout << "Common Test p-values\n";
        for (size_t i = 0; i < pvals.size(); i++) {
            std::cout << pvals[i];
            std::cout << '\n';
        }

        //outputPvals( pvals, outputDir);
    }
    else {

		TestInput input = parseAndFilter(req);
        generateForR(input.X, input.Y, input.Z, input.G, input.P, input.readGroup);

        if (req.useCommon()) {
            std::vector<double> pvals = runCommonTest(req, input);

            std::cout << "Common Test p-values\n";
            for (size_t i = 0; i < pvals.size(); i++) {
                std::cout << pvals[i];
                std::cout << '\n';
            }

			outputPvals(input.variantInfo, pvals, outputDir);
        }
        else {

            std::vector<double> pval = runRareTest(req, input);

            std::cout << "Rare Test p-values\n";
			for (size_t i = 0; i < pval.size(); i++) {
				std::cout << input.variantInfo[i] + "\t";
				std::cout << pval[i];
				std::cout << '\n';
			}

			outputPvals(input.variantInfo, pval, outputDir);
		}
    }



    //keep console open while debugging
    //TODO: be sure to remove eventually!
    std::cout << "\ndone...>";
    while (true) {}
    return 0;
}



