#include "EconOptimizer.h"
#include <stdlib.h>
#include <math.h>
#include <cassert>
#include <iomanip>
#include <functional>
#include <easylogging++.h>

using namespace std;
using namespace alglib;

// Initialize the static members of class
vector<EconOptimzer::expression_t> EconOptimzer::expressions;
vector<double> EconOptimzer::z;

void EconOptimzer::setInitValuesOfVars(KMatrix genPolicy) {
	initValuesList.clear();
	listOfVars = xmlParser.getVariables();
	auto initValExpressions = xmlParser.getInitComputeExpressions();

    symbol_table_t symbol_table_initvals;
    symbol_table_initvals.add_constants();

	// TODO Find out if a generated policy would be used to compute the init values before calling the optimizer
    auto params = xmlParser.getParameters();
    for(auto param : params) {
        symbol_table_initvals.add_constant(param.first, param.second);
    }

	// Base policy details come from the input xml file and only one run is needed to get the base actor utilities.
	if (isBasePolicy) {
		auto policyVariables = xmlParser.getPolicyVariables();
		for (auto policy : policyVariables) {
			symbol_table_initvals.add_constant(policy.first, policy.second);
			initValuesList[policy.first] = policy.second;
		}

		auto policyConsts = xmlParser.getPolicyConstants();
		for (auto policy : policyConsts) {
			symbol_table_initvals.add_constant(policy.first, policy.second);
		}

		isBasePolicy = false;
	}
	else { // This is a case of randomly generated policy
		auto policyVarNames = xmlParser.getPolicyVariableNames();
		auto policyVarCount = policyVarNames.size();
		for (auto j = 0; j < policyVarCount; ++j) {
			symbol_table_initvals.add_constant(policyVarNames[j], genPolicy(0, j));
			cout << "Random policy " << policyVarNames[j] << " = " << genPolicy(0, j);
			initValuesList[policyVarNames[j]] = genPolicy(0, j);
		}

		// Use policy constants
		auto policyConsts = xmlParser.getPolicyConstantNames();
		for (auto j = policyVarCount; j < policyVarCount + policyConsts.size(); ++j) {
			symbol_table_initvals.add_constant(policyConsts[j- policyVarCount], genPolicy(0, j));
		}
	}

    parser_t parser;

    expression_t expr;
    expr.register_symbol_table(symbol_table_initvals);

//    initValuesStr += "[";

    for(auto var : listOfVars) {
        parser.compile(initValExpressions.at(var) , expr);

        // evaluate the expression to find the initial value of a variable
        auto varValue = expr.value();

        // Add each constant name with its value in the symbol table
        symbol_table_initvals.add_constant(var, varValue);

		initValuesList[var] = varValue;

//        initValuesStr += std::to_string(varValue) + ",";
    }

  //  auto policyVarNames = xmlParser.getPolicyVariableNames();
  //  for(auto policyName : policyVarNames) {
		//initValuesStr += std::to_string(policyVariables.at(policyName)) + ",";
  //  }

    // Replace the last comma with a square bracket
    //initValuesStr.back() = ']';
	for (auto inival : initValuesList) {
		LOG(INFO) << inival.first << inival.second;
	}
}

string EconOptimzer::getInitVals() {
	LOG(INFO) << "In getInitVals";
	initValuesStr = "[";

	for (auto var : listOfVars) {
		initValuesStr += std::to_string(initValuesList.at(var)) + ",";
		LOG(INFO) << initValuesStr;
	}

	auto policyVarNames = xmlParser.getPolicyVariableNames();
//	auto policyVariables = xmlParser.getPolicyVariables();
	for (auto policyName : policyVarNames) {
		initValuesStr += std::to_string(initValuesList.at(policyName)) + ",";
		LOG(INFO) << initValuesStr;
	}

	// Replace the last comma with a square bracket
	initValuesStr.back() = ']';
	LOG(INFO) << initValuesStr;

	return initValuesStr;
}

void EconOptimzer::setSymbolTable() {
    auto params = xmlParser.getParameters();
    for(auto param : params) {
        symbol_table.add_constant(param.first, param.second);
    }

    auto policyConsts = xmlParser.getPolicyConstants();
    for(auto policy : policyConsts) {
        symbol_table.add_constant(policy.first, policy.second);
    }

    auto policyVarNames = xmlParser.getPolicyVariableNames();
    z.resize(listOfVars.size() + policyVarNames.size());
    int i=0;
    for( ; i < listOfVars.size(); ++i) {
        //cout << "z is set to: " << listOfVars[i].substr(0, listOfVars[i].length()-1) << " " << endl;
        // symbol_table.add_variable(listOfVars[i].substr(0, listOfVars[i].length() - 1),     z[i]);
        symbol_table.add_variable(listOfVars[i],     z[i]);
    }

    for( ; i < z.size(); ++i) {
        auto policyVarName = policyVarNames[i-listOfVars.size()];
        //cout << "z is set to: " << policyVarName.substr(0, policyVarName.length()-1) << " " << endl;
        // symbol_table.add_variable(policyVarName.substr(0, policyVarName.length() - 1),     z[i]);
        symbol_table.add_variable(policyVarName,     z[i]);
    }
    
    symbol_table.add_constants();
}

void EconOptimzer::resetPolicyConstsInSymTable(KMatrix &genPolicy) {
	auto policyVarCount = xmlParser.getPolicyVariableNames().size();
	// Use policy constants
	auto policyConsts = xmlParser.getPolicyConstantNames();
	for (auto j = policyVarCount; j < policyVarCount + policyConsts.size(); ++j) {
		symbol_table.remove_variable(policyConsts[j - policyVarCount]);
		symbol_table.add_constant(policyConsts[j - policyVarCount], genPolicy(0, j));
	}
}

void EconOptimzer::setMathExpressions() {
    parser_t parser;

    auto systemOfEquations = xmlParser.getOptimizeFunctions();

    size_t equationsCount =  systemOfEquations.size();

    for(int i = 0; i < equationsCount /* Note: count of equations under <optimize> tag of xml*/; ++i) {
        expressions.push_back(expression_t());
        expressions[i].register_symbol_table(symbol_table);
        parser.compile(systemOfEquations[i] , expressions[i]);
    }
}

void EconOptimzer::optimize_fvec(const real_1d_array &x, real_1d_array &fi, void *ptr)
{

    //
    // this callback calculates the values of expressions as per the inputs passed by optimization algorithm
    //

    // Confirm that the length of the number of expressions is equal to the length of input real_1d_array x
    assert(expressions.size() == x.length());

    // A non-const vector z is required because input parameter x is a const which causes c++ error in code compilation
    // Size of this vector needs to be fixed and equal to that of the number of variables ( which is also equal to number of equations
    // and the size of the input real_1d_array x in this callback method.
    // Keeping it dynamic causes runtime trouble (neither error nor crash) with symbol table of exprtk library.
    // I think it is due to change in location of vector in memory if the vector undergoes a change in its size due to push back operation.
    // We can't use push_back() method as the referenece to individual members of z are given to the symbol table.
    for(int i=0 ; i < x.length() ; ++i) {
        z[i] = x[i]; // Values of z are used to evaluate the expression values
    }

    size_t equationsCount =  expressions.size();
    for(size_t i = 0; i < equationsCount /* Note: count of equations under <optimize> tag of xml*/; ++i) {
        fi[i] = expressions[i].value(); // this evaluation uses the vector z which was mapped in the symbol table of expressions
        //cout << "fi[" << i <<  "] = " << fi[i] << endl;
    }
    //cout << endl;
}

void EconOptimzer::optimize() {
	getInitVals();
    cout << "Initial values of variables: " << initValuesStr << endl << endl;
    real_1d_array x = initValuesStr.c_str();
    double epsx = 0.0000000001;
    ae_int_t maxits = 0;
    minlmstate state;
    minlmreport rep;

    minlmcreatev(25, 25, x, 0.000001, state);
    minlmsetcond(state, epsx, maxits);

    alglib::minlmoptimize(state, &EconOptimzer::optimize_fvec);

    minlmresults(state, x, rep);

    size_t x_index = 0;

    for(auto var: listOfVars) {
        optimumSolution[var] = x[x_index];
        ++x_index;
    }

    for(auto var: xmlParser.getPolicyVariableNames()) {
        optimumSolution[var] = x[x_index];
        ++x_index;
    }

    cout << "Optimized solution set: " << endl;

    cout << fixed;
    streamsize ss = cout.precision();
    cout << setprecision(4);
    for(auto var: listOfVars) {
        cout << var << ":       " << optimumSolution.at(var) << endl;
    }

    for(auto var: xmlParser.getPolicyVariableNames()) {
        cout << var << ": " << optimumSolution.at(var) << endl;
    }

    cout << setprecision(ss);
    cout << endl;
}

void EconOptimzer::calcActorUtils() {
    parser_t parser;

    expression_t expr;

	// Make a copy of the symbol table as the old symbol would be used in another optimization operation
	symbol_table_t symbol_table_utils(symbol_table);
	expr.register_symbol_table(symbol_table_utils);

	// The variables used in the optimized equations have got some definite values after the optimization process.
	// Use them in the symbol table as constants for further computes.
	for (auto var : listOfVars) {
		symbol_table_utils.remove_variable(var);
		symbol_table_utils.add_constant(var, optimumSolution.at(var));
	}

	for (auto var : xmlParser.getPolicyVariableNames()) {
		symbol_table_utils.remove_variable(var);
		symbol_table_utils.add_constant(var, optimumSolution.at(var));
	}
	
	auto equationFunctions = xmlParser.getEquationFunctions();

    cout << "Contribution of different factors: " << endl;
    for(auto func : equationFunctions) {
        parser.compile(func.second, expr);
        double funcValue = expr.value();

		// Add the computed value to the symbol table
        //symbol_table.add_constant(func.first, funcValue);
		symbol_table_utils.add_constant(func.first, funcValue);
		// cout << func.first << " (" << func.second << "): " << funcValue << endl;
        cout << func.first << ": " << funcValue << endl;
    }
    cout << endl;

    auto actorUtilFunctions = xmlParser.getActorUtilities();

    cout << "Utility of each actor: " << endl;
    for(auto actorUtil : actorUtilFunctions) {
        parser.compile(actorUtil.second, expr);
        double aUtil = expr.value();
        // cout << "Util of " << actorUtil.first << " (" << actorUtil.second << "): " << aUtil << endl;
        cout << "Util of " << actorUtil.first << ": " << aUtil << endl;
    }
    cout << endl;
}

KMatrix EconOptimzer::createPolicies(unsigned int policyCount, PRNG * rng)
{
	unsigned int policyElementsCount = xmlParser.getPolicyVariableNames().size() + xmlParser.getPolicyConstantNames().size();
	//unsigned int policyElementsCount = xmlParser.getPolicyVariableNames().size();

	// A policy is a combination of different factors or policyElements and we need to generate 2500 or more such combinations
	auto policies = KMatrix(policyCount, policyElementsCount);

	// Generate a policy
	auto makeRand = [rng]() {
		// Amit: Is it fine to provide range for a policy value in the input xml?
		double ti = rng->uniform(0, +1);
		//if (ti < 0) {
		//	ti = ti * maxSub;
		//}
		//if (0 < ti) {
		//	ti = ti * maxTax;
		//}
		return ti;
	};

	for (unsigned int i = 0; i < policyCount; ++i) {
		// Generate a policy. The order of the policy elements matches with that of xml input
		for (unsigned int j = 0; j < policyElementsCount; j++) {
			policies(i, j) = makeRand();
		}
	}

	generatedPolicies = policies;
	return policies;
}
