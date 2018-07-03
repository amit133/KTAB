#include "xmlParser.h"
#include <iostream>
using namespace std;

int main() {
	string xmlFile("sampleinput.xml"); 
	KXml xmlParser(xmlFile);
	auto listOfVars = xmlParser.getVariables();
	auto initValExpressions = xmlParser.getInitComputeExpressions();

	cout << "Expressions for initial values: " << endl;
	for(auto var : listOfVars) {
		cout << var << ": " << initValExpressions.at(var) << endl;
	}

	cout << "Parameters: " << endl;
	auto params = xmlParser.getParameters();
	for(auto param : params) {
		cout << param.first << ": " << param.second << endl;
	}
	cout  << endl << endl;

    auto polVarNames = xmlParser.getPolicyVariableNames();
    cout << "Names of Policy variables: ";
    for(auto policy : polVarNames) {
        cout <<  policy << " ";
    }
    cout << endl;

    auto policyVariables = xmlParser.getPolicyVariables();
    cout << "Policy variable names and initial values: " << endl;
    for(auto policy : policyVariables) {
        cout << policy.first << ": " << policy.second << endl;
    }

    auto policyConstants = xmlParser.getPolicyConstants();
    cout << "Policy constant names and initial values: " << endl;
    for(auto policy : policyConstants) {
        cout << policy.first << ": " << policy.second << endl;
    }

	return 0;
}
