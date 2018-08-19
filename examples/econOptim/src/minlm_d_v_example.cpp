// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2015 King Abdullah Petroleum Studies and Research Center
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software
// and associated documentation files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom
// the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
// BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// --------------------------------------------

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

	using KBase::dSeed;

	auto showHelp = []() {
		printf("Usage: \n");
		printf("demoEconOptim [options]\n");
		printf("specify one or more of these options\n");
		printf("--help           print this message\n");
		printf("--xml <f>        read a scenario from XML\n");
		printf("--seed <n>       set a 64bit seed; default is %020llu; 0 means truly random\n", dSeed);
	};

	bool run = true;
	uint64_t seed = -1;
	string inputXML = "";

	if (argc > 1) {
		for (int i = 1; i < argc; i++) {
			if (strcmp(argv[i], "--seed") == 0) {
				i++;
				seed = std::stoull(argv[i]);
			}
			else if (strcmp(argv[i], "--xml") == 0) {
				i++;
				if (argv[i] != NULL)
				{
					inputXML = argv[i];
				}
				else
				{
					run = false;
					break;
				}
			}
			else if (strcmp(argv[i], "--help") == 0) {
				run = false;
			}
			else {
				run = false;
				printf("Unrecognized argument %s\n", argv[i]);
			}
		}
	}
	else {
		run = false; // no arguments supplied
	}

	if (!run) {
		showHelp();
		return 0;
	}

	KBase::Model::configLogger("./econoptim-logger.conf");

	//std::string xmlFile("sampleinput.xml");
    //KXml xmlParser(xmlFile);
	KXml xmlParser(inputXML);

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
	if (0 == seed) {
		PRNG * rng = new PRNG();
		seed = rng->setSeed(seed); // 0 == get a random number
	}

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

	for (unsigned int i = 0; i < policyCount; ++i) {
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

	delete rng;
	rng = nullptr;
	return 0;
}