#include "EconOptimizer.h"
#include <stdlib.h>
#include <math.h>
#include <cassert>
#include <iomanip>
#include <functional>

using namespace std;
using namespace alglib;

// Initialize the static members of class
vector<EconOptimzer::expression_t> EconOptimzer::expressions;
vector<double> EconOptimzer::z;

void EconOptimzer::setInitValuesOfVars() {
	listOfVars = xmlParser.getVariables();
	auto initValExpressions = xmlParser.getInitComputeExpressions();

    symbol_table_t symbol_table_initvals;
    symbol_table_initvals.add_constants();
 
    auto params = xmlParser.getParameters();
    for(auto param : params) {
        symbol_table_initvals.add_constant(param.first, param.second);
    }

    auto policyVariables = xmlParser.getPolicyVariables();
    for(auto policy : policyVariables) {
        symbol_table_initvals.add_constant(policy.first, policy.second);
    }

    auto policyConsts = xmlParser.getPolicyConstants();
    for(auto policy : policyConsts) {
        symbol_table_initvals.add_constant(policy.first, policy.second);
    }

    parser_t parser;

    expression_t expr;
    expr.register_symbol_table(symbol_table_initvals);

    initValues += "[";

    for(auto var : listOfVars) {
        parser.compile(initValExpressions[var] , expr);

        // evaluate the expression to find the initial value of a variable
        auto varValue = expr.value();

        // Add each constant name with its value in the symbol table
        symbol_table_initvals.add_constant(var, varValue);

        initValues += std::to_string(varValue) + ",";
    }

    auto policyVarNames = xmlParser.getPolicyVariableNames();
    for(auto policyName : policyVarNames) {
        initValues += std::to_string(policyVariables.at(policyName)) + ",";
    }

    // Replace the last comma with a square bracket
    initValues.back() = ']';
}

string EconOptimzer::getInitVals() const {
    return initValues;
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
    cout << "Initial values of variables: " << initValues << endl << endl;
    real_1d_array x = initValues.c_str();
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
        // symbol_table.remove_variable(var.substr(0, var.length()-1));
        symbol_table.remove_variable(var);
        // symbol_table.add_constant(var.substr(0, var.length()-1), x[x_index]);
        symbol_table.add_constant(var, x[x_index]);
        ++x_index;
    }

    for(auto var: xmlParser.getPolicyVariableNames()) {
        optimumSolution[var] = x[x_index];
        // symbol_table.remove_variable(var.substr(0, var.length()-1));
        symbol_table.remove_variable(var);
        // symbol_table.add_constant(var.substr(0, var.length()-1), x[x_index]);
        symbol_table.add_constant(var, x[x_index]);
        ++x_index;
    }

    cout << "Optimized solution set: " << endl;

    cout << fixed;
    streamsize ss = cout.precision();
    cout << setprecision(4);
    for(auto var: listOfVars) {
        // cout << var.substr(0, var.length()-1) << ":       " << optimumSolution.at(var) << endl;
        cout << var << ":       " << optimumSolution.at(var) << endl;
    }

    for(auto var: xmlParser.getPolicyVariableNames()) {
        // cout << var.substr(0, var.length()-1) << ": " << optimumSolution.at(var) << endl;
        cout << var << ": " << optimumSolution.at(var) << endl;
    }

    cout << setprecision(ss);
    cout << endl;
}

void EconOptimzer::calcActorUtils() {
    parser_t parser;

    expression_t expr;
    expr.register_symbol_table(symbol_table);

    auto equationFunctions = xmlParser.getEquationFunctions();

    cout << "Contribution of different factors: " << endl;
    for(auto func : equationFunctions) {
        parser.compile(func.second, expr);
        double funcValue = expr.value();
        symbol_table.add_constant(func.first, funcValue);
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
