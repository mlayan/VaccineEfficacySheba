#include <Rcpp.h>
#include "simulate_epidemic_fun.h"
using namespace std;
using namespace Rcpp;

// [[Rcpp::plugins(cpp11)]]


double mIncub = 1.63;
double sdIncub = 0.5;
double maxPCRDetectability = 10.0;


//////////////////////////////////////////////
// [[Rcpp::export]]
double incubPeriod() 
  {
  // Truncated incubation period for symptomatic cases
  double d = rlnorm(1, mIncub, sdIncub)[0];
  while(d < 3 || d > 30) d = rlnorm(1, mIncub, sdIncub)[0];
  return d;
}


//////////////////////////////////////////////
// [[Rcpp::export]]
double rIncub(double d) 
  {
  // Truncated incubation period for symptomatic cases
  double pIncub = rlnorm(1, mIncub, sdIncub)[0];
  while (pIncub < 3.0 || pIncub > 30.0) {
    pIncub = rlnorm(1, mIncub, sdIncub)[0];  
  }
  
  return d-pIncub;
}


//////////////////////////////////////////////
// [[Rcpp::export]]
double detectionPeriod() 
{
	// Time period between infection and detection by PCR for asymptomatic cases
	return runif(1, 0, maxPCRDetectability)[0];
}


//////////////////////////////////////////////
// [[Rcpp::export]]
double rInfectionAsymptomatic(double d) 
{
	// Time period between infection and detection by PCR for asymptomatic cases
	return runif(1, d-maxPCRDetectability, d)[0];
}


//////////////////////////////////////////////
// [[Rcpp::export]]
NumericVector foi(
	double t, 
	double dt, 
	double lastDate,  
	NumericVector symptomOnset, 
	NumericVector infection, 
	IntegerVector vaccinatedInfectors,
	IntegerVector infStatus, 
	double beta, 
	double alpha, 
	double delta,
	double rInfVac, 
	double hhsize, 
	double mainHHSize
  ) {
	// Force of infection from 
	// 	0: community
	// 	1 to #infected: infected household contacts
	
	// Initialize foi vector
	NumericVector fois(symptomOnset.size() + 1);

	//Infection by the community
	fois[0] = alpha*dt;

	// Relative infectivity of vaccinated cases at time t
	IntegerVector allVaccinated = rep(1, vaccinatedInfectors.size());
	NumericVector relativeInfectivity = ifelse(vaccinatedInfectors == allVaccinated, rInfVac, 1.0);

	// Parameters of the infectivity profile
	// Shifted gamma distribution from Aschcroft et al., 2020 (DOI:10.4414/smw.2020.20336)
	double mBeta = 26.1;
	double vBeta = 7;
	double shift = 25.6;
	double shapeBeta = pow(mBeta,2) / vBeta;
	double scaleBeta = vBeta / mBeta;
  
  	// Infection by infected individuals within the same household
	for (int index = 0; index < symptomOnset.size(); ++index) {
		double k = 0.0;

		if (infStatus[index] == 1) { // Symptomatic infector
			if (t >= symptomOnset[index] - 3) {
				k += ( R::pgamma(shift + (t + dt - symptomOnset[index]), shapeBeta, scaleBeta, true, false) - R::pgamma(shift + (t - symptomOnset[index]), shapeBeta, scaleBeta, true, false) ) /
				( R::pgamma(shift + (lastDate - symptomOnset[index]), shapeBeta, scaleBeta, true, false) - R::pgamma(shift - 3, shapeBeta, scaleBeta, true, false) );
			} 

		} else if (infStatus[index] == 2){ // Asymptomatic infector 
			if ( t >= infection[index] + 2 ) {
				k += ( R::pgamma(shift - 3.0 + (t + dt - infection[index] - 2.0 ), shapeBeta, scaleBeta, true, false) - R::pgamma(shift - 3.0 + (t - infection[index] - 2.0), shapeBeta, scaleBeta, true, false) ) /
				( R::pgamma(shift - 3.0 + (lastDate - infection[index] - 2.0), shapeBeta, scaleBeta, true, false) - R::pgamma(shift - 3.0, shapeBeta, scaleBeta, true, false) );

				k *= 0.6; // Relative infectivity of asymptomatic cases compared to symptomatic cases
			}
		} 

		fois[index + 1] = (beta * k * relativeInfectivity[index]) / pow(hhsize / mainHHSize, delta);
	} 
  
  return fois;
}
