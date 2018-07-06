#include "EconOptimizer.h"

using namespace std;

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

    cout << endl;

    return 0;
}