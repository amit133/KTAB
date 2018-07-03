#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "optimization.h"
#include "exprtk.hpp"
#include <vector>
#include<cassert>
#include <iomanip>
#include "xml/xmlParser.h"

using namespace std;
using namespace alglib;

typedef exprtk::expression<double>     expression_t;
std::vector<expression_t> expressions;

typedef exprtk::symbol_table<double> symbol_table_t;
symbol_table_t symbol_table;

// An intermediate vector z is required because input parameter x in the callback function of optimization algorithm is a const which causes error in compilation
// Size of this vector needs to be fixed. Keeping it dynamic causes runtime trouble (neither error nor crash) with symbol table of exprtk library.
// I think it is due to change in location of vector in memory if the vector undergoes a change in its size due to push back operation.
vector<double> z; // Rename z to some meaningful name

typedef std::string variableName;
typedef double variableValue;
typedef std::map<variableName, variableValue> valuesOfVars;
valuesOfVars initValuesOfVars;

typedef std::string varName;
std::vector<varName> listOfVars;

typedef exprtk::parser<double> parser_t;

// xml parse
string xmlFile("sampleinput.xml"); 
KXml xmlParser(xmlFile);

// data structure to store the optimized solution
typedef map<string, double> Solution;
Solution optimumSolution;

void setInitValuesOfVars() {
	listOfVars = xmlParser.getVariables();
	auto initValExpressions = xmlParser.getInitComputeExpressions();

    // The map would be populated in a loop when xml reading comes into play
    for(auto var : listOfVars) {
        initValuesOfVars[var] = 0.0;
    }

    symbol_table_t symbol_table_initvals;
    symbol_table_initvals.add_constants();

    //typedef exprtk::details::variable_node<double> exprtk_var_ptr;
 
    auto params = xmlParser.getParameters();
    for(auto param : params) {
        symbol_table_initvals.add_constant(param.first, param.second);
        //exprtk_var_ptr *v = symbol_table_initvals.get_variable(param.first);
        //cout << "Symbol name: " << param.first << ", symbol value: " << v->value() << endl;
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

    for(auto var : listOfVars) {
        //cout << "Variable: " << var << ", Expression: " << initValExpressions[var] << endl;
        parser.compile(initValExpressions[var] , expr);

        // evaluate the expression and store in the map for the values of each variable
        initValuesOfVars[var] = expr.value();

        // Add each constant name with its value in the symbol table
        symbol_table_initvals.add_constant(var, initValuesOfVars.at(var));
        //cout << "Variable: " << var << " , Init Value: " << initValuesOfVars[var] << endl;
    }
}

// TODO: getInitValues() method can be merged with setInitValuesOfVars()
std::string getInitValues() {
    assert(!listOfVars.empty());

    string initVals ("[");
    for(auto var : listOfVars) { // This option of for loop has to rechecked
        try {
            initVals += std::to_string(initValuesOfVars.at(var)) + ",";
        } catch(...) {
            // Note: The key is not present in the map. Do nothing and let the loop continue
        }
    }

    auto policyVarNames = xmlParser.getPolicyVariableNames();
    auto policyVariables = xmlParser.getPolicyVariables();
    for(auto policyName : policyVarNames) {
        initVals += std::to_string(policyVariables.at(policyName)) + ",";
    }

    initVals.back() = ']';

    //cout << "Init Vals: " << initVals << endl;
    return initVals;
}

void setSymbolTable() {
    auto params = xmlParser.getParameters();
    for(auto param : params) {
        //cout << param.first << ": " << param.second << endl;
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
        cout << "z is set to: " << listOfVars[i] << " " << endl;
        symbol_table.add_variable(listOfVars[i].substr(0, listOfVars[i].length() - 1),     z[i]);
    }

    for( ; i < z.size(); ++i) {
        auto policyVarName = policyVarNames[i-listOfVars.size()];
        cout << "z is set to: " << policyVarName << " " << endl;
        symbol_table.add_variable(policyVarName.substr(0, policyVarName.length() - 1),     z[i]);
    }
    
    symbol_table.add_constants();
}

void setMathExpressions() {
    parser_t parser;

    auto systemOfEquations = xmlParser.getOptimizeFunctions();

    size_t equationsCount =  systemOfEquations.size();

    for(int i = 0; i < equationsCount /* Note: count of equations under <optimize> tag of xml*/; ++i) {
        expressions.push_back(expression_t());
        expressions[i].register_symbol_table(symbol_table);
        parser.compile(systemOfEquations[i] , expressions[i]);
    }
}

void  function1_fvec(const real_1d_array &x, real_1d_array &fi, void *ptr)
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
        cout << "fi[" << i <<  "] = " << fi[i] << endl;
    }
    cout << endl;
}

void calcActorUtils() {
    // typedef exprtk::details::variable_node<double> exprtk_var_ptr;
    // exprtk_var_ptr *v = symbol_table.get_variable("POSUB");
    // cout << "Symbol name: " << "POSUB" << ", symbol value: " << v->value() << endl;

    parser_t parser;

    expression_t expr;
    expr.register_symbol_table(symbol_table);

    auto equationFunctions = xmlParser.getEquationFunctions();

    for(auto func : equationFunctions) {
        parser.compile(func.second, expr);
        double funcValue = expr.value();
        symbol_table.add_constant(func.first, funcValue);
        cout << func.first << " (" << func.second << "): " << funcValue << endl;
    }

    auto actorUtilFunctions = xmlParser.getActorUtilities();

    for(auto actorUtil : actorUtilFunctions) {
        parser.compile(actorUtil.second, expr);
        double aUtil = expr.value();
        cout << "Util of " << actorUtil.first << " (" << actorUtil.second << "): " << aUtil << endl;
    }
}

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

    setInitValuesOfVars();

    string initValuesOfVars = getInitValues();
    cout << "Init Vals: " << initValuesOfVars << endl;

    setSymbolTable();
    setMathExpressions();

    //OE0,E0,G0,RNW0,PE0,S0,OS0,PS0,N0,KP0,SF0,W0,R0,SH0,C0,RSTAR0,TR0,PO0,B0,O0,KG0,Y0,POSUB0,PG0,PRNW0
    //real_1d_array x = "[0.0305, 0.0156, 0.0140, 0, 1.5625, 0.0389, 0.0673, 1.4932, 1.0000,
    //5.6246, 0.0262, 1.1806, 0.1400, 0.0127, 0.9363, 0.0400, 0.6868, 1.6371, -8.1109, 0.4819, 0, 2.0277, 0.5000, 0.6563, 1.5625]";

    real_1d_array x = initValuesOfVars.c_str();
    double epsx = 0.0000000001;
    ae_int_t maxits = 0;
    minlmstate state;
    minlmreport rep;

    minlmcreatev(25, 25, x, 0.000001, state);
    minlmsetcond(state, epsx, maxits);

    alglib::minlmoptimize(state, function1_fvec);

    minlmresults(state, x, rep);

    size_t x_index = 0;

    for(auto var: listOfVars) {
        optimumSolution[var] = x[x_index];
        symbol_table.remove_variable(var.substr(0, var.length()-1));
        symbol_table.add_constant(var.substr(0, var.length()-1), x[x_index]);
        ++x_index;
    }

    for(auto var: xmlParser.getPolicyVariableNames()) {
        optimumSolution[var] = x[x_index];
        symbol_table.remove_variable(var.substr(0, var.length()-1));
        symbol_table.add_constant(var.substr(0, var.length()-1), x[x_index]);
        ++x_index;
    }

    cout << "Optimized solution set: " << endl;

    cout << std::fixed;
    std::streamsize ss = std::cout.precision();
    cout << std::setprecision(4);
    for(auto var: listOfVars) {
        cout << var << ":       " << optimumSolution.at(var) << endl;
    }

    for(auto var: xmlParser.getPolicyVariableNames()) {
        cout << var << ": " << optimumSolution.at(var) << endl;
    }

    cout << std::setprecision(ss);

    // Calculate actor utilities based on optimized solution
    calcActorUtils();

    cout << endl;
    // for(int index=0; index < x.length(); ++index) {
    //     printf("%.4f\n", x[index]);
    // }

    return 0;
}