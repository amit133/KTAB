#include "EconOptimizer.h"
#include "kmodel.h"
#include <easylogging++.h>

using namespace std;
using KBase::hSlice;

int main(int argc, char **argv)
{
    //
    // This example demonstrates minimization of F(x0,x1) = f0^2+f1^2+...+fn^2
    //
    // using "V" mode of the Levenberg-Marquardt optimizer.
    //
    // Optimization algorithm uses:
    // * function vector f[] = {f1,f2,...,fn}
    //
    // No other information (Jacobian, gradient, etc.) is needed.
    //

	KBase::Model::configLogger("./econoptim-logger.conf");
	
	std::string xmlFile("sampleinput.xml");
    KXml xmlParser(xmlFile);

    EconOptimzer econOptim(xmlParser);
    
    econOptim.setInitValuesOfVars();
    econOptim.setSymbolTable();
    econOptim.setMathExpressions();

	// Run the optimizer
    econOptim.optimize();
    
    // Calculate actor utilities based on optimized solution
    econOptim.calcActorUtils();

    //cout << endl;

	PRNG * rng = new PRNG();
	uint64_t seed = 0;
	seed = rng->setSeed(seed); // 0 == get a random number

	LOG(INFO) << KBase::getFormattedString("Using PRNG seed:  %020llu", seed);
	LOG(INFO) << KBase::getFormattedString("Same seed in hex:   0x%016llX", seed);

	unsigned int policyCount = 0;

	cout << "Enter the number of policies you need to generate: ";
	cin >> policyCount;
	// Create policies
	auto policies = econOptim.createPolicies(policyCount, rng);
	LOG(INFO) << "Generated Policies: ";
	policies.mPrintf(" %.6f ");
	LOG(INFO) << "";

	for (auto i = 0; i < policyCount; ++i) {
		auto policy = hSlice(policies, i);
		//LOG(INFO) << "Policy set: ";
		//policy.mPrintf(" %.6f ");

		econOptim.setInitValuesOfVars(policy);
		econOptim.resetPolicyConstsInSymTable(policy);

		// Run the optimizer
		econOptim.optimize();

		// Calculate actor utilities based on optimized solution
		econOptim.calcActorUtils();

		//cout << endl;
	}

	//KMatrix policy(1, 5);
	//policy(0, 0) = 0.5; // POSUB
	//policy(0, 1) = 0.6563; // PG
	//policy(0, 2) = 1.5625; // PRNW
	//policy(0, 3) = 0; // igexg
	//policy(0, 4) = 0.5; // laborShare

	//econOptim.setInitValuesOfVars(policy);
	//econOptim.resetPolicyConstsInSymTable(policy);

	//// Run the optimizer
	//econOptim.optimize();

	//// Calculate actor utilities based on optimized solution
	//econOptim.calcActorUtils();

	//cout << endl;

	return 0;
}