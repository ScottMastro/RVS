#include "stdafx.h"
#include "RVS.h"
#include <vector>
#include <random>

VectorXd extractRows(VectorXd &v, VectorXd &where, double equals) {
	VectorXd subset(v.rows());
	int c = 0;

	for (int i = 0; i < where.rows(); i++) {
		if (where[i] == equals) {
			subset[c] = v[i];
			c++;
		}
	}

	return subset.block(0, 0, c, 1);
}

MatrixXd extractRows(MatrixXd &m, VectorXd &where, double equals) {
	MatrixXd subset(m.rows(), m.cols());
	int c = 0;
	int i, j;

	for (i = 0; i < where.rows(); i++) {
		if (where[i] == equals) {
			for (j = 0; j < m.cols(); j++)
				subset(c, j) = m(i, j);
			c++;
		}
	}

	return subset.block(0, 0, c, m.cols());
}

VectorXd whereNAN(VectorXd &X, VectorXd &Y, MatrixXd &Z) {
	int nobs = Y.rows();
	int ncov = Z.cols();

	VectorXd toRemove(nobs);
	
	for (int i = 0; i < nobs; i++) {
		toRemove[i] = 0;

		if (isnan(X[i]) || isnan(Y[i]))
			toRemove[i] = 1;
		else {
			for (int j = 0; j < ncov; j++)
				if (isnan(Z(i, j)))
					toRemove[i] = 1;
		}
	}

	return toRemove;
}

VectorXd whereNAN(VectorXd &Y, MatrixXd &Z) {
	int nobs = Y.rows();
	int ncov = Z.cols();

	VectorXd toRemove(nobs);

	for (int i = 0; i < nobs; i++) {
		toRemove[i] = 0;

		if (isnan(Y[i]))
			toRemove[i] = 1;
		else {
			for (int j = 0; j < ncov; j++)
				if (isnan(Z(i, j)))
					toRemove[i] = 1;
		}
	}

	return toRemove;
}

VectorXd whereNAN(VectorXd &X, VectorXd &Y) {
	int nobs = X.rows();
	int ncov = Y.cols();
	VectorXd toRemove(nobs);

	for (int i = 0; i < nobs; i++) {
		toRemove[i] = 0;
		if (isnan(X[i]) || isnan(Y[i]))
			toRemove[i] = 1;
	}
	return toRemove;
}

VectorXd whereNAN(VectorXd &X) {
	VectorXd toRemove(X.rows());
	for (int i = 0; i < X.rows(); i++) {
		toRemove[i] = 0;

		if (isnan(X[i]))
			toRemove[i] = 1;
		else 
			toRemove[i] = 0;
	}

	return toRemove;
}


inline VectorXd CovariateRegression(VectorXd &Y, MatrixXd &Z) {
	return Z.householderQr().solve(Y);
}

VectorXd getBeta(VectorXd &X, VectorXd &Y, MatrixXd &Z) {
	size_t i, j, k;
	int ncov = Z.cols();
	int nobs = Y.rows();

	VectorXd toRemove = whereNAN(X, Y, Z);
	VectorXd y_filtered = extractRows(Y, toRemove, 0);
	MatrixXd z_filtered = extractRows(Z, toRemove, 0);

	return CovariateRegression(y_filtered, z_filtered);
}

std::vector<VectorXd> fitModel(VectorXd &beta, std::vector<VectorXd> &y, std::vector<MatrixXd> &z, std::string distribution) {
	std::vector<VectorXd> ycenter;

	for (int i = 0; i < y.size(); i++) {

		VectorXd meanValue = z[i] * beta;

		if (distribution == "binom")
			meanValue = 1 / (1 + exp(-meanValue.array()));

		ycenter.push_back(y[i] - meanValue);
	}
	return ycenter;
}

double average(std::vector<VectorXd> v) {
	double sum = 0;
	double n = 0;
	for (int i = 0; i < v.size(); i++) {
		n += v[i].size();
		sum += v[i].sum();
	}
	return sum / n;
}

double variance(VectorXd &v) {

	double mean = v.mean();
	double var = (v.array() - mean).pow(2).sum();

	return var/(v.rows()-1);
}


double center(VectorXd &v) {

	double mean = v.mean();
	double var = (v.array() - mean).pow(2).sum();

	return var / (v.rows() - 1);
}

/*
Calculates the robust variance of E(G | D). var(x) = E(x^2) - E(x)^2

@param p Genotype frequency from EM algorithm
@return Robust variance of E(G | D)
*/
double calcRobustVar(VectorXd p) {
	return (4 * p[2] + p[1]) - pow(2 * p[2] + p[1], 2);
}

std::random_device random;
std::mt19937 generate(random());

int generateRandomInteger(int from, int to) {
	std::uniform_int_distribution<> sample(from, to);
	return sample(generate);
}
double generateRandomDouble(double from, double to) {
	std::uniform_real_distribution<double> sample(from, to);
	return sample(generate);
}
double randomNormal(double mean, double sd) {
	std::normal_distribution<> sample(mean, sd);
	return sample(generate);
}

//same as doing pairwise.complete.obs in R
MatrixXd covariance(MatrixXd &M) {
	int n = M.cols();
	int m = M.rows();

	MatrixXd cov(n, n);
	size_t i, j, k;

	double count;
	double meani;
	double meanj;
	double sum;

	for (i = 0; i < n; i++) {
		for (j = i; j < n; j++) {

			sum = 0;
			count = 0;
			meani = 0;
			meanj = 0;

			for (k = 0; k < m; k++) {
				if (!isnan(M(k, i)) && !isnan(M(k, j))) {
					count++;
					meani += M(k, i);
					meanj += M(k, j);
				}
			}

			meani /= count;
			meanj /= count;

			for (k = 0; k < m; k++)
				if (!isnan(M(k, i)) && !isnan(M(k, j)))
					sum += (M(k, i) - meani) * (M(k, j) - meanj);

			sum /= count - 1;
			cov(i, j) = sum;
			cov(j, i) = sum;
		}
	}

	return cov;
}


MatrixXd correlation(MatrixXd &M) {
	int n = M.cols();
	int m = M.rows();

	MatrixXd cor(n, n);
	size_t i, j, k;

	double count;
	double vari;
	double varj;
	double meani;
	double meanj;
	double sum;

	for (i = 0; i < n; i++) {
		for (j = i; j < n; j++) {
			if (i == j) {
				cor(i, j) = 1;
				cor(j, i) = 1;
			}
			else {
				sum = 0;
				count = 0;
				vari = 0;
				varj = 0;
				meani = 0;
				meanj = 0;

				for (k = 0; k < m; k++) {
					if (!isnan(M(k, i)) && !isnan(M(k, j))) {
						count++;
						meani += M(k, i);
						meanj += M(k, j);
					}
				}

				meani /= count;
				meanj /= count;

				for (k = 0; k < m; k++) {
					if (!isnan(M(k, i)) && !isnan(M(k, j))) {
						vari += pow(M(k, i) - meani, 2);
						varj += pow(M(k, j) - meanj, 2);
						sum += (M(k, i) - meani) * (M(k, j) - meanj);
					}
				}

				sum /= sqrt(vari * varj);
				cor(i, j) = sum;
				cor(j, i) = sum;
			}
		}
	}

	return cor;
}

/*
Approximates the p-value from the pdf of the normal distribution where x is a Z-score

@param x Z-score.
@return p-value.
*/
double pnorm(double x)
{
	// constants
	double a1 = 0.254829592;
	double a2 = -0.284496736;
	double a3 = 1.421413741;
	double a4 = -1.453152027;
	double a5 = 1.061405429;
	double p = 0.3275911;

	x = fabs(x) / sqrt(2.0);

	// A&S formula 7.1.26
	double t = 1.0 / (1.0 + p*x);
	return (((((a5*t + a4)*t) + a3)*t + a2)*t + a1)*t*exp(-x*x);
}

std::vector<double> randomSample(std::vector<double> &vec, int nsample) {
	std::vector<double> rvec;
	std::uniform_int_distribution<> sample(0, vec.size()-1);

	for (size_t i = 0; i < nsample; i++)
		rvec.push_back(vec[sample(generate)]);
	
	return rvec;
}



/*
Finds p-value for test statistic using a chi-squared distribution with one degree of freedom
using chi-squared probability density function.

@param statistic Test statistic.
@return p-value.
*/
double chiSquareOneDOF(double statistic) {
	double z = statistic * 0.5;
	double sc = 2 * sqrt(z) * exp(-z);

	double sum = 1;
	double prevSum = sum;
	double nom = 1;
	double dnom = 1;
	double s = 0.5;

	for (int i = 0; i < 200; i++)
	{
		nom *= z;
		s++;
		dnom *= s;
		sum += nom / dnom;
		if (prevSum == sum) break;
		prevSum = sum;
	}

	double p = sum * sc;
	if (isnan(p) || isinf(p) || p <= 1e-8)
		return 1e-14;

	p /= tgamma(0.5);

	return 1 - p;
}

MatrixXd nanToZero(MatrixXd &M) {
	std::cout << "\n";	std::cout << "\n";
	std::cout << "\n";
	std::cout << "\n";
	std::cout << "\n";

	std::cout << M;

	for (size_t i = 0; i < M.cols(); i++) 
		for (size_t j = 0; j < M.rows(); j++) 
			if (isnan(M(j, i)))
				M(j, i) = 0;

	return  M;
}

VectorXd nanToZero(VectorXd &V) {

	for (size_t i = 0; i < V.rows(); i++)
			if (isnan(V[i]))
				V[i] = 0;

	return V;
}