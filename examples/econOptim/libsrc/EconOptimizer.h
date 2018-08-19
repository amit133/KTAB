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

#include <string>
#include <map>
#include <vector>
#include <optimization.h>
#include <exprtk.hpp>
#include "XMLParser.h"
#include <kmatrix.h>
#include <prng.h>


using KBase::KMatrix;
using KBase::PRNG;

class EconOptimzer {
private:
    KXml &xmlParser;

    typedef exprtk::expression<double>     expression_t;
    static std::vector<expression_t> expressions;

    typedef exprtk::symbol_table<double> symbol_table_t;
    symbol_table_t symbol_table;

    // An intermediate vector z is required because input parameter x in the callback function of optimization algorithm is a const which causes error in compilation
    // Size of this vector needs to be fixed. Keeping it dynamic causes runtime trouble (neither error nor crash) with symbol table of exprtk library.
    // I think it is due to change in location of vector in memory if the vector undergoes a change in its size due to push back operation.
    static std::vector<double> z; // Rename z to some meaningful name

    typedef std::string varName;
    std::vector<varName> listOfVars;
	typedef double varValue;

    typedef exprtk::parser<double> parser_t;

    // The string would contain a comma separated list of initial values of all equation variables and policy variables
    std::string initValuesStr;
	std::map<varName, varValue> initValuesList;

    // data structure to store the optimized solution
    typedef std::map<std::string, double> Solution;
    Solution optimumSolution;

    // We need a static member function as it would serve as the callback function for the optimzer algorithm
    static void optimize_fvec(const alglib::real_1d_array &x, alglib::real_1d_array &fi, void *ptr);

	bool isBasePolicy = true;

	KMatrix generatedPolicies;

public:
    EconOptimzer(KXml &xmlParser): xmlParser(xmlParser) {
    }
	void setInitValuesOfVars(KMatrix genPolicy = KMatrix{});
    std::string getInitVals();
    void setSymbolTable();
    void setMathExpressions();
    void optimize();
    void calcActorUtils();

	KMatrix createPolicies(unsigned int policyCount, PRNG* rng);
	void resetPolicyConstsInSymTable(KMatrix &policy);
};
