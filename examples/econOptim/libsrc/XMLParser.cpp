#include <iostream>
#include <cassert>
#include "XMLParser.h"

using namespace std;
using namespace tinyxml2;

KXml::KXml(string & xmlFileName) {
	xmlFileName_m = xmlFileName;
	bool fileOpened = LoadXmlFile(xmlFileName);
	if(!fileOpened) {
		cerr << "XML File load failure" << endl;
		return;
	}

	setActors();
	setVariables();
	setPolicies();
	setParameters();
	setOptimizeFunctions();
	setEquations();
}

bool KXml::LoadXmlFile(string & xmlFileName) {
	XMLError eResult = doc.LoadFile(xmlFileName_m.c_str());
	if(eResult != XML_SUCCESS) {
		cerr << "Could not open xml file: " << xmlFileName_m << endl; 
		return false;
	}
	rootElement = doc.RootElement();

	return true;
}

void KXml::setActors() {
	auto getChildText = [](XMLElement *parentElement, const char * childElementName) {
		auto childElement = parentElement->FirstChildElement(childElementName);
		assert(childElement != 0);
		auto childText = childElement->GetText();
		return childText;
	};

	auto getSiblingText = [](XMLElement *currentElement, const char * siblingElementName){
		auto siblingElement = currentElement->NextSiblingElement(siblingElementName);
		assert(siblingElement != 0);
		auto text = siblingElement->GetText();
		assert(text != 0);
		return text;
	};

	XMLElement *actorsList = rootElement->FirstChildElement("actors");
	XMLElement *actor = actorsList->FirstChildElement("actor");
	while(actor) {
		auto nameElement = actor->FirstChildElement("name");
		auto actorName = nameElement->GetText();

		// auto actorName = getChildText(actor, "name");

		auto descriptionElement = nameElement->NextSiblingElement("description");
		auto description = descriptionElement->GetText();

		auto influenceElement = descriptionElement->NextSiblingElement("influence");
		auto influence = influenceElement->GetText();

		auto utilityElement = influenceElement->NextSiblingElement("utility");
		auto utility = utilityElement->GetText();

		actorUtils[actorName] = utility;

		actor = actor->NextSiblingElement("actor");
	}
}

KXml::actorUtilities KXml::getActorUtilities() const {
	return actorUtils;
}

void KXml::setVariables() {
	XMLElement *variables = rootElement->FirstChildElement("variables");
	auto varElem = variables->FirstChildElement("variable");
	while(varElem) {
		auto varNameElem = varElem->FirstChildElement("name");
		assert (varNameElem != nullptr);

		auto varName = varNameElem->GetText();
		assert(varName != 0);
		listOfVars.push_back(string(varName));

		auto varInitValElem = varElem->FirstChildElement("initial");
		auto varInitVal = varInitValElem->GetText();

		// If an initial value is not there, the formula to compute the initial value should be present
		if(varInitVal == 0) {
			auto varInitValComputeElem = varElem->FirstChildElement("compute");
			assert(varInitValComputeElem != 0);

			auto varInitValComputeExpr = varInitValComputeElem->GetText();
			//cout << varName << ": " << varInitValComputeExpr << endl;

			initValExpressions[listOfVars.back()] = string(varInitValComputeExpr);
		} else {
			//cout << varName << ": " << varInitVal << endl;
			initValExpressions[listOfVars.back()] = string(varInitVal);
		}

		// Go to the next variable
		varElem = varElem->NextSiblingElement();
	}

	// cout << "Expressions for init values: ";
	// for(auto var : listOfVars) {
	// 	cout << var << ": " << initValExpressions.at(var) << endl;
	// }
	// cout  << endl << endl;
}

void KXml::setPolicies() {
	XMLElement *policies = rootElement->FirstChildElement("policy");
	auto policyElem = policies->FirstChildElement();
	while(policyElem) {
		auto policyNameElem = policyElem->FirstChildElement("name");
		assert (policyNameElem != nullptr);

		auto policyName = policyNameElem->GetText();

		// Each policy would have a set of base values called as base policy
		auto policyBaseElem = policyElem->FirstChildElement("base");
		auto policyBase = policyBaseElem->DoubleText();

		std::map<std::string, std::tuple<double, double>> policyRanges;

		auto policyLowLimitElem = policyElem->FirstChildElement("lowlimit");
		auto policyLowerLimit = policyLowLimitElem->DoubleText();

		auto policyHighLimitElem = policyElem->FirstChildElement("highlimit");
		auto policyHigherLimit = policyHighLimitElem->DoubleText();

		policyRanges[policyName] = tuple<double, double>(policyLowerLimit, policyHigherLimit);

		// There could two categories of policies. One which would be optimized (variable) and the other which would be used as a constant value (constant)
		string policyType = policyElem->Name();
		//cout << "policy Name " << policyElem->Name() << endl;
		if(policyType == "variable") {
			//cout << "policy variable, " << policyName << ": " << policyBase << endl;
			listOfPolicyVariableNames.push_back(policyName);
			policyVariables[policyName] = policyBase;
		} else if(policyType == "constant") {
			//cout << "policy constant, " << policyName << ": " << policyBase << endl;
			listOfPolicyConstantNames.push_back(policyName);
			policyConstants[policyName] = policyBase;
		} else {
			cerr << "Invalid xml tag under policy" << endl;
		}

		// Go to the next policy
		policyElem = policyElem->NextSiblingElement();
	}
}

KXml::policies KXml::getPolicyVariables() const {
	return policyVariables;
}

KXml::policies KXml::getPolicyConstants() const {
	return policyConstants;
}

std::vector<KXml::policyName> KXml::getPolicyVariableNames() const {
	return listOfPolicyVariableNames;
}

std::vector<KXml::policyName> KXml::getPolicyConstantNames() const {
	return listOfPolicyConstantNames;
}

void KXml::setParameters() {
	XMLElement *parameters = rootElement->FirstChildElement("parameters");
	XMLElement *paramElement = parameters->FirstChildElement("parameter");
	while(paramElement) {
		auto paramNameElem = paramElement->FirstChildElement("name");
		auto paramName = paramNameElem->GetText();

		auto paramValueElem = paramElement->FirstChildElement("value");
		auto paramValue = paramValueElem->DoubleText();

		params[paramName] = paramValue;

		//cout << paramName << ": " << paramValue << endl;

		paramElement = paramElement->NextSiblingElement("parameter");
	}
}

KXml::parameters KXml::getParameters() const {
	return params;
}

void KXml::setOptimizeFunctions() {
	XMLElement *optimizeFunctions = rootElement->FirstChildElement("optimize");
	XMLElement *optimizeEquation = optimizeFunctions->FirstChildElement("equation");
	while(optimizeEquation) {
		auto optimizeEquationElem = optimizeEquation->FirstChildElement("function");
		auto optimizeFunction = optimizeEquationElem->GetText();
		//cout << "optimize function: " << optimizeFunction << endl;
		systemOfFunctions.push_back(optimizeFunction);

		// Go to next function
		optimizeEquation = optimizeEquation->NextSiblingElement();
	}
}

KXml::optimizingFunctions KXml::getOptimizeFunctions() const {
	return systemOfFunctions;
}

void KXml::setEquations() {
	XMLElement *equations = rootElement->FirstChildElement("equations");
	XMLElement *equationElement = equations->FirstChildElement("equation");
	while(equationElement) {
		auto equationNameElem = equationElement->FirstChildElement("name");
		auto equationName = equationNameElem->GetText();

		auto equationFunctionElem = equationElement->FirstChildElement("function");
		auto equationFunction = equationFunctionElem->GetText();

		//cout << equationName << ": " << equationFunction << endl;
		equationFunctions[equationName] = equationFunction;
		equationElement = equationElement->NextSiblingElement();
	}
}

KXml::equations KXml::getEquationFunctions() const {
	return equationFunctions;
}

KXml::listOfVariables KXml::getVariables() const {
	return listOfVars;
}

KXml::initValExprMap KXml::getInitComputeExpressions() const {
	return initValExpressions;
}
