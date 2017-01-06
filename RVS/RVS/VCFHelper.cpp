#include "stdafx.h"
#include "RVS.h"
#include "MemoryMapped.h"

#include <iostream>  
#include <string>
#include <unordered_map>
#include <vector>
#include <set>
#include <algorithm>

/*
Seperates the case and control IDs

@param vcfDir Full directory path of the VCF file.
@param vcfDir Full directory path of the caseID file.
@param ncolID The number of columns before the sample IDs start in the last line of the headers in the VCF file.
@return A map with sample names as keys and true (case) or false (control) as values.
*/
std::unordered_map<std::string, bool> getIDs(std::string vcfDir, std::string caseIDDir, int ncolID) {

	//open VCF file and find the line with column names
	MemoryMapped vcf(vcfDir);
	int pos = 0;
	std::vector<std::string> IDs = parseHeader(vcf, pos);

	int lineCounter = 0;

	//open caseID file
	MemoryMapped caseID(caseIDDir);
	pos = 0;
	int startPos = pos;
	std::vector<std::string> caseIDs;

	//extract every ID from caseID file, assumes one ID per line
	std::string ID;

	while (pos < caseID.mappedSize()) {
		if (caseID[pos] == '\n') {

			ID = trim(getString(caseID, startPos, pos));
			if (ID.size() > 0) {
				caseIDs.push_back(ID);
				lineCounter++;
				startPos = pos + 1;
			}
		}
		pos++;
	}

	//last line may not end with return character
	std::string lastLine = trim(getString(caseID, startPos, pos));
	if (lastLine.size() > 0) {
		caseIDs.push_back(lastLine);
		lineCounter++;
	}

	//map each ID to true or false:
	//control -> false
	//case -> true
	std::unordered_map<std::string, bool> IDmap;
	int countControl = 0;

	for (size_t i = 0; i < IDs.size(); i++) {

		int index = findIndex(IDs[i], caseIDs);
		if (index > -1) {
			IDmap.insert(std::pair<std::string, bool>(IDs[i], false));
			countControl++;
		}
		else {
			IDmap.insert(std::pair<std::string, bool>(IDs[i], true));
		}
	}

	int countCase = IDmap.size() - countControl;

	std::cout << "This VCF includes " + std::to_string(IDmap.size()) + " samples with " + std::to_string(countControl) +
		" controls and " + std::to_string(countCase) + " cases.\n";

	if (lineCounter != countCase)
		std::cout << "Warning: " + std::to_string(lineCounter) + " lines were counted in case ID file but only " +
		std::to_string(countCase) + " were found to correspond to columns in .vcf file.\n";

	return IDmap;
}


/*
Extracts the last header row from a VCF file

@param vcf MemoryMapped object containing a VCF file.
@param pos Index of a character in MemoryMapped file.
@return The names of each column in the last header row from vcf.
@effect pos Is the index of the first character after the header upon return.
*/
std::vector<std::string> parseHeader(MemoryMapped &vcf, int &pos) {
	int lastPos = 0;

	//look for last header line
	//assumes header and only header begins with '#'
	while (true) {

		if (vcf[pos] == '\n') {
			if (vcf[pos + 1] != '#')
				break;
			lastPos = pos + 2;
		}
		pos++;
	}

	pos = lastPos;
	std::vector<std::string> colNames;

	//parse the header
	//assumes column names are tab-separated
	//assumes last line of header and first variant are separated by return character
	while (true) {
		if (vcf[pos] == '\t') {
			colNames.push_back(getString(vcf, lastPos, pos));
			lastPos = pos + 1;
		}
		else if (vcf[pos] == '\n') {
			colNames.push_back(getString(vcf, lastPos, pos));
			pos++;
			break;
		}
		pos++;
	}

	return colNames;
}

/*
Creates a new SNP object from one row in a VCF file

@param vcf MemoryMapped object containing a VCF file.
@param ind Index of the first character in every column of the MemoryMapped file.
@param ncolID The number of columns before the sample IDs start in the last line of the headers in the VCF file.
@return A SNP object with chr, loc and genotype likelihoods initialized.
*/
SNP initSNP(MemoryMapped &vcf, std::vector<int> ind, int ncolID) {

	SNP snp;

	//get chromosome and location from VCF file
	//assumes CHR is first column and LOC is second column
	snp.chr = getString(vcf, ind[0], ind[1] - 1);
	snp.loc = std::stoi(getString(vcf, ind[1], ind[2] - 1));

	//finds the index of "PL" from the FORMAT column
	//assumes FORMAT column is the 9th column
	int indexPL = 0;
	int pos = ind[8];
	while (true) {
		if (vcf[pos] == ':')
			indexPL++;
		else if (vcf[pos] == 'P' && vcf[pos + 1] == 'L')
			break;
		else if (vcf[pos] == '\t') {
			std::cout << "ERROR: Phred-scaled likelihoods (PL) not found in FORMAT column ";
			std::cout << "or FORMAT is not the 9th column in the .vcf file.";
		}
		pos++;
	}

	std::vector<genotypeLikelihood> likelihoods;
	int startPos;
	int counter;
	double l00;
	double l01;
	double l11;

	//get genotype likelihood for every sample
	for (int i = ncolID; i < ind.size(); i++) {
		pos = ind[i];
		counter = 0;

		if (vcf[pos] == '.') {
			genotypeLikelihood gl;
			gl.L00 = NULL;
			gl.L01 = NULL;
			gl.L11 = NULL;
			likelihoods.push_back(gl);

			continue;
		}

		while (true) {
			if (vcf[pos] == ':') {
				counter++;
				if (counter == indexPL) {
					pos++;
					break;
				}
			}
			pos++;
		}

		startPos = pos;

		while (vcf[pos] != ',')
			pos++;
		pos++;
		l00 = stod(getString(vcf, startPos, pos));
		startPos = pos;

		while (vcf[pos] != ',')
			pos++;
		pos++;
		l01 = stod(getString(vcf, startPos, pos));
		startPos = pos;

		while (vcf[pos] != '\t' && vcf[pos] != ':' && vcf[pos] != '\n')
			pos++;
		l11 = stod(getString(vcf, startPos, pos));

		//convert from Phred-scaled likelihoods
		genotypeLikelihood gl;
		gl.L00 = pow(10, -l00*0.1);
		gl.L01 = pow(10, -l01*0.1);
		gl.L11 = pow(10, -l11*0.1);
		likelihoods.push_back(gl);
	}

	snp.gl = likelihoods;

	return snp;
}

/*
1)Parses data from VCF file for each SNP (chromosome, location and Phred-scaled likelihoods for each sample)
2)Filters each SNP by ignoring ones that fail the following criteria:
- FILTER column contains "PASS"
- REF and ALT columns contain exactly one base
- the proportion of missing PL values for control samples is less than missingTh
- the proportion of missing PL values for case samples is less than missingTh
- every SNP passing the above criteria must have a unique pair of CHR and LOC values (no duplications)

@param vcfDir Full directory path of the VCF file.
@param ncolID The number of columns before the sample IDs start in the last line of the headers in the VCF file.
@param missingTh Proportion of missing values tolerable.
@param IDmap A map with sample names as keys and true (case) or false (control) as values.
@return A list of SNPs that pass the filtering step.
*/
std::vector<SNP> parseAndFilter(std::string vcfDir, int ncolID, double missingTh, std::unordered_map<std::string, bool> &IDmap) {

	MemoryMapped vcf(vcfDir);
	std::vector<SNP> snps;

	int pos = 0;
	std::vector<std::string> colNames = parseHeader(vcf, pos);

	//assumes order of columns in VCF file
	int chrom = 0;
	int loc = 1;
	int ref = 3;
	int alt = 4;
	int filter = 6;

	//counts number of cases and controls
	int ncase = 0;
	int ncontrol = 0;
	for (auto& p : IDmap) {
		ncase += p.second;
		ncontrol += !p.second;
	}

	int allowedMissCase = floor(ncase * missingTh);
	int allowedMissControl = floor(ncontrol * missingTh);

	int filterPass = 0;
	int filterBase = 0;
	int filterCase = 0;
	int filterControl = 0;
	int filterDuplicate = 0;

	//parallelize if speed-up desired?
	//======================================
	int start = pos;
	int end = vcf.mappedSize();

	int currInd = 0;
	int missCase = 0;
	int missControl = 0;
	bool filtered = false;

	int counter = 0;
	int successcounter = 0;
	while (start < end) {
		std::vector<int> ind;
		ind.push_back(start);
		currInd = 1;
		missCase = 0;
		missControl = 0;
		filtered = false;

		while (true) {
			//parsing the columns before ID columns
			if (currInd <= ncolID) {
				if (vcf[start] == '\t') {
					ind.push_back(start + 1);

					//filter out SNPs where REF and ALT are not single base
					if (currInd == 3 || currInd == 4) {
						if (vcf[start + 2] != '\t') {
							filtered = true;
							filterBase++;
							break;
						}
					}

					//filter out SNPs where FILTER is not "PASS"
					else if (currInd == 6) {
						if (vcf[start + 1] != 'P' ||
							vcf[start + 2] != 'A' ||
							vcf[start + 3] != 'S' ||
							vcf[start + 4] != 'S') {
							filtered = true;
							filterPass++;
							break;
						}
					}
					currInd++;
				}
			}

			//parsing ID columns
			else {
				if (start < vcf.mappedSize() && vcf[start] == '\t') {
					ind.push_back(start + 1);
					if (vcf[start + 1] == '.') {
						if (IDmap.at(colNames[currInd])) {
							missCase++;
							if (missCase > allowedMissCase) {
								filtered = true;
								filterCase++;
								break;
							}
						}
						else {
							missControl++;
							if (missControl > allowedMissControl) {
								filtered = true;
								filterCase++;
								break;
							}
						}
					}
					currInd++;
				}

				else if (start >= vcf.mappedSize() || vcf[start] == '\n')
					break;
			}
			start++;
		}

		while (start - 1 < vcf.mappedSize() && vcf[start - 1] != '\n')
			start++;

		counter++;
		
		//if SNP looks good, keep it 
		if (!filtered) {
			successcounter++;
			snps.push_back(initSNP(vcf, ind, ncolID));
		}
	}
	//======================================


	//sort by location
	std::sort(snps.begin(), snps.end(), locCompare);

	//keep track of duplicated SNPs
	std::set<size_t> toDelete;
	size_t j;

	for (size_t i = 0; i < snps.size(); i++) {
		j = 1;
		while (i + j < snps.size() && snps[i].loc == snps[i + j].loc) {

			if (snps[i].chr == snps[i + j].chr) {
				toDelete.insert(i);
				toDelete.insert(i + j);
			}
			j++;
		}
	}

	for (auto it = toDelete.rbegin(); it != toDelete.rend(); it++) {
		snps.erase(snps.begin() + *it);
		filterDuplicate++;
	}

	std::cout << filterPass;
	std::cout << " SNPs were filtered by FILTER column\n";
	std::cout << filterBase;
	std::cout << " SNPs were filtered by ALT and REF column\n";
	std::cout << filterCase;
	std::cout << " SNPs were filtered by missing case values\n";
	std::cout << filterControl;
	std::cout << " SNPs were filtered by missing control values\n";
	std::cout << filterDuplicate;
	std::cout << " SNPs were filtered by duplication\n";
	std::cout << successcounter;
	std::cout << " SNPs remain after filtering\n";

	return snps;
}

