#include <string>
#include <vector>
#include <map>
#include <tinyxml2.h>

class KXml {
private:
	std:: string xmlFileName_m;
	tinyxml2::XMLDocument doc;
	tinyxml2::XMLElement *rootElement = nullptr;

	typedef std::string variableName;
	typedef std::vector<variableName> listOfVariables;
	listOfVariables listOfVars;

    typedef std::string initComputeExpr;
    typedef std::map<variableName, initComputeExpr> initValExprMap;
    initValExprMap initValExpressions;

	// Policies
	typedef std::string policyName;
	typedef double policyValue;
	typedef std::map<policyName, policyValue> policies;
	policies policyVariables;
	policies policyConstants;
	std::vector<policyName> listOfPolicyVariableNames;
	std::vector<policyName> listOfPolicyConstantNames;
	typedef std::map<std::string, std::tuple<double, double>> policyRange;
	policyRange policyRanges;

	// parameters
	typedef std::string paramName;
	typedef double paramValue;
	typedef std::map<paramName, paramValue> parameters;
	parameters params;

	// System of Optimization Equations
    typedef std::string function;
    typedef std::vector<function> optimizingFunctions;
    optimizingFunctions systemOfFunctions;

	// equations to store values using the optimized values of variables
    typedef std::string functionName;
	typedef std::map<functionName, function> equations;
	equations equationFunctions;

	// Actor utilities functions
	typedef std::string actorName;
	typedef std::string utilFunction;
	typedef std::map<actorName, utilFunction> actorUtilities;
	actorUtilities actorUtils;

private:
	bool LoadXmlFile(std::string & xmlFileName);
	void setActors();
	void setVariables();
	void setPolicies();
	void setParameters();
	void setOptimizeFunctions();
	void setEquations();

public:
	KXml() = delete;
	KXml(const KXml&) = delete;
	KXml& operator=(const KXml&) = delete;
	explicit KXml(std::string & xmlFileName);

	listOfVariables getVariables() const;
	initValExprMap getInitComputeExpressions() const;
	parameters getParameters() const;
	optimizingFunctions getOptimizeFunctions() const;
	policies getPolicyVariables() const;
	policies getPolicyConstants() const;
	std::vector<policyName> getPolicyVariableNames() const;
	std::vector<policyName> getPolicyConstantNames() const;
	policyRange getPolicyRanges() const;
	equations getEquationFunctions() const;
	actorUtilities getActorUtilities() const;
};

