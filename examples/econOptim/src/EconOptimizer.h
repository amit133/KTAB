#include "xml/xmlParser.h"
#include <string>
#include <map>
#include <vector>
#include <optimization.h>
#include "exprtk/exprtk.hpp"

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

    typedef exprtk::parser<double> parser_t;

    // The string would contain a comma separated list of initial values of all equation variables and policy variables
    std::string initValues;

    // data structure to store the optimized solution
    typedef std::map<std::string, double> Solution;
    Solution optimumSolution;

    // We need a static member function as it would serve as the callback function for the optimzer algorithm
    static void function1_fvec(const alglib::real_1d_array &x, alglib::real_1d_array &fi, void *ptr);

public:
    EconOptimzer(KXml &xmlParser): xmlParser(xmlParser) {
    }
    void setInitValuesOfVars();
    std::string getInitVals() const;
    void setSymbolTable();
    void setMathExpressions();
    void optimize();
    void calcActorUtils();
};
