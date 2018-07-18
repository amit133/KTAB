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
		LOG(INFO) << "Optimizing for base policy";
		auto policyVariables = xmlParser.getPolicyVariables();
		for (auto policy : policyVariables) {
			LOG(INFO) << policy.first << " = " << policy.second;
			symbol_table_initvals.add_constant(policy.first, policy.second);
			initValuesList[policy.first] = policy.second;
		}

		auto policyConsts = xmlParser.getPolicyConstants();
		for (auto policy : policyConsts) {
			LOG(INFO) << policy.first << " = " << policy.second;
			symbol_table_initvals.add_constant(policy.first, policy.second);
		}

		isBasePolicy = false;
	}
	else { // This is a case of randomly generated policy
		LOG(INFO) << "Optimizing for generated policy";
		auto policyVarNames = xmlParser.getPolicyVariableNames();
		auto policyVarCount = policyVarNames.size();
		for (auto j = 0; j < policyVarCount; ++j) {
			symbol_table_initvals.add_constant(policyVarNames[j], genPolicy(0, j));
			LOG(INFO) << policyVarNames[j] << " = " << genPolicy(0, j);
			initValuesList[policyVarNames[j]] = genPolicy(0, j);
		}

		// Use policy constants
		auto policyConsts = xmlParser.getPolicyConstantNames();
		for (auto j = policyVarCount; j < policyVarCount + policyConsts.size(); ++j) {
			symbol_table_initvals.add_constant(policyConsts[j- policyVarCount], genPolicy(0, j));
			LOG(INFO) << policyConsts[j - policyVarCount] << " = " << genPolicy(0, j);
		}
	}

	LOG(INFO) << "";

    parser_t parser;

    expression_t expr;
    expr.register_symbol_table(symbol_table_initvals);

    for(auto var : listOfVars) {
        parser.compile(initValExpressions.at(var) , expr);

        // evaluate the expression to find the initial value of a variable
        auto varValue = expr.value();

        // Add each constant name with its value in the symbol table
        symbol_table_initvals.add_constant(var, varValue);

		initValuesList[var] = varValue;

    }

    // Replace the last comma with a square bracket
	//for (auto inival : initValuesList) {
	//	LOG(INFO) << inival.first << inival.second;
	//}
}

string EconOptimzer::getInitVals() {
	initValuesStr = "[";

	for (auto var : listOfVars) {
		initValuesStr += std::to_string(initValuesList.at(var)) + ",";
	}

	auto policyVarNames = xmlParser.getPolicyVariableNames();
	for (auto policyName : policyVarNames) {
		initValuesStr += std::to_string(initValuesList.at(policyName)) + ",";
	}

	// Replace the last comma with a square bracket
	initValuesStr.back() = ']';
	//LOG(INFO) << initValuesStr;

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
    LOG(INFO) << "Initial values of variables: " << initValuesStr;
	LOG(INFO) << "";

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

    LOG(INFO) << "Optimized solution set: ";

    //cout << fixed;
    //streamsize ss = cout.precision();
    //cout << setprecision(4);
    for(auto var: listOfVars) {
        LOG(INFO) << KBase::getFormattedString("%s : %.4f", var.c_str(), optimumSolution.at(var));
    }

    for(auto var: xmlParser.getPolicyVariableNames()) {
		LOG(INFO) << KBase::getFormattedString("%s : %.4f", var.c_str(), optimumSolution.at(var));
	}

	LOG(INFO) << "";
    //cout << setprecision(ss);
    //cout << endl;
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

	auto policyVars = xmlParser.getPolicyVariableNames();
	for (auto var : policyVars) {
		symbol_table_utils.remove_variable(var);
		symbol_table_utils.add_constant(var, optimumSolution.at(var));
	}
	
	auto equationFunctions = xmlParser.getEquationFunctions();

    LOG(INFO) << "Contribution of different factors: ";
    for(auto func : equationFunctions) {
        parser.compile(func.second, expr);
        double funcValue = expr.value();

		// Add the computed value to the symbol table
		symbol_table_utils.remove_variable(func.first);
		symbol_table_utils.add_constant(func.first, funcValue);
        LOG(INFO) << func.first << ": " << funcValue;
    }
    //cout << endl;
	LOG(INFO) << "";

    auto actorUtilFunctions = xmlParser.getActorUtilities();

    LOG(INFO) << "Utility of each actor: ";
    for(auto actorUtil : actorUtilFunctions) {
        parser.compile(actorUtil.second, expr);
        double aUtil = expr.value();
        LOG(INFO) << "Util of " << actorUtil.first << ": " << aUtil;
    }
    //cout << endl;
	LOG(INFO) << "";
}

KMatrix EconOptimzer::createPolicies(unsigned int policyCount, PRNG * rng)
{
	auto policyVarNames = xmlParser.getPolicyVariableNames();
	auto policyConstNames = xmlParser.getPolicyConstantNames();

	unsigned int policyElementsCount = policyVarNames.size() + policyConstNames.size();
	auto policyRanges = xmlParser.getPolicyRanges();

	auto getPolicyRange = [&](string policyName, double &lowerLimit, double &upperLimit) {
		// Get the allowed range of this policy element
		auto policyRange = policyRanges.at(policyName);
		lowerLimit = std::get<0>(policyRange);
		upperLimit = std::get<1>(policyRange);
		LOG(INFO) << "policy element: " << policyName << ", policyRange; [" << lowerLimit << ":" << upperLimit << "]";
	};

	// A policy is a combination of different factors or policyElements and we need to generate 2500 or more such combinations
	auto policies = KMatrix(policyCount, policyElementsCount);

	// Generate a policy
	auto makeRand = [rng](double lowerLimit, double upperLimit) {
		double ti = rng->uniform(lowerLimit, upperLimit);
		return ti;
	};

	unsigned int j = 0;
	unsigned int policyVarCount = policyVarNames.size();

	// Fill policy matrix column by column instead of row by row
	for (; j < policyVarCount; ++j) { // Fill for all the policy variables
		// Get the allowed range of this policy element
		double lowerLimit = 1E-10;
		double upperLimit = 1E-10;

		getPolicyRange(policyVarNames[j], lowerLimit, upperLimit);

		for (unsigned int i = 0; i < policyCount; ++i) {
			policies(i, j) = makeRand(lowerLimit, upperLimit);
		}
	}

	for (; j < policyElementsCount; ++j) { // Fill for all the policy constants
		// Get the allowed range of this policy element
		double lowerLimit = 1E-10;
		double upperLimit = 1E-10;

		getPolicyRange(policyConstNames[j - policyVarCount], lowerLimit, upperLimit);

		for (unsigned int i = 0; i < policyCount; ++i) {
			policies(i, j) = makeRand(lowerLimit, upperLimit);
		}
	}

	generatedPolicies = policies;

	LOG(INFO) << "";
	return policies;
}
