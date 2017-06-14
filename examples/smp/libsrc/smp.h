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

#ifndef SMP_LIB_H
#define SMP_LIB_H

#include <iostream>
#include <string>
#include <map>

#include "sqlite3.h"
#include "kutils.h"
#include "prng.h"
#include "kmatrix.h"
#include "gaopt.h"
#include "kmodel.h"

namespace SMPLib {
// namespace to which KBase has no access
using std::function;
using std::map;
using std::ostream;
using std::shared_ptr;
using std::string;
using std::tuple;
using std::vector;
using KBase::newChars;
using KBase::KMatrix;
using KBase::PRNG;
using KBase::Actor;
using KBase::Position;
using KBase::State;
using KBase::Model;
using KBase::VotingRule;
using KBase::ReportingLevel;
using KBase::ThirdPartyCommit;
using KBase::VctrPstn;
using KBase::VUI;
using KBase::BigRAdjust;
using KBase::BigRRange;
using KBase::KTable; // JAH 20160728

class SMPActor;
class SMPState;
class SMPModel;

const string appVersion = "0.1.1";
//const bool testProbPCE = true;

// -------------------------------------------------
// See kmodel.h for explanation of why the enum classes are so repetitive.

enum class SMPBargnModel {
  InitOnlyInterpSMPBM = 0, // Init interpolates, I&R get same one
  InitRcvrInterpSMPBM,   // Init interpolates, so does Rcvr, each both from other
  PWCompInterpSMPBM      // Power-weighted compromise of I-interp and R-interp, I&R both get same one
};
const vector<string> SMPBargnModelNames = {
  "InitOnlyInterp", "InitRcvrInterp", "PWCompInterp" };
ostream& operator<< (ostream& os, const SMPBargnModel& bMod);



// See the documentation in kutils/doc: eS2P2, dS2P2, and eS1P1 were the best, in that order.
// Both estimators involving PMax were the least accurate.
enum class InterVecBrgn {
  S1P1, S2P2, S2PMax
};
const vector<string> InterVecBrgnNames = {
  "S1P1", "S2P2", "S2PMax" };
ostream& operator<< (ostream& os, const InterVecBrgn& ivb);

// -------------------------------------------------
// Plain-Old-Data
struct BargainSMP {
public:
  BargainSMP(const SMPActor* ai, const SMPActor* ar, const VctrPstn & pi, const VctrPstn & pr);
  ~BargainSMP();


  const SMPActor* actInit = nullptr;
  const SMPActor* actRcvr = nullptr;
  VctrPstn posInit = VctrPstn();
  VctrPstn posRcvr = VctrPstn();
  uint64_t getID() const;
protected:
  static uint64_t highestBargainID;
  uint64_t myBargainID = 0;
};

// -------------------------------------------------
// Trivial, SMP-like actor with fixed attributes
// the old smp.cpp file, SpatialState::developTwoPosBargain, for a discussion of
// the interpolative bargaining rule used there. Note that the demoutil
// results indicate that weighting by (P^2 * S^2) better approximates the NBS
// does weighting by (P * S)
class SMPActor : public Actor {

public:

  SMPActor(string n, string d);
  ~SMPActor();

  // interfaces to be provided
  double vote(unsigned int est, unsigned int i, unsigned int j, const State*st) const;
  virtual double vote(const Position * ap1, const Position * ap2, const SMPState* as1) const;
  double posUtil(const Position * ap1, const SMPState* as1) const;


  void randomize(PRNG* rng, unsigned int numD);

  // These actors have a vector position in [0,1]^m
  // They have differing saliences for different dimensions,
  // which are used to determine weighted Euclidean
  // distance from their current ideal position
  // (which is their current position in the State).
  // The distance is curved with risk attitude to get utility.
  // Their vote is determined by applying their scalar capacity
  // to differences in utility
  double sCap = 0;
  //double bigR;
  KMatrix vSal = KMatrix();
  VotingRule vr = VotingRule::Proportional; // reasonable default

  // the attributes used in this method are not generally part of
  // other actors, and not all positions can be represented as a list of doubles.
  static BargainSMP* interpolateBrgn(const SMPActor* ai, const SMPActor* aj,
                                     const VctrPstn* posI, const VctrPstn * posJ,
                                     double prbI, double prbJ, InterVecBrgn ivb);


protected:
  static void interpBrgnSnPm(unsigned int n, unsigned int m,
                             double tik, double sik, double prbI,
                             double tjk, double sjk, double prbJ,
                             double & bik, double & bjk);
  static void interpBrgnS2PMax(double tik, double sik, double prbI,
                               double tjk, double sjk, double prbJ,
                               double & bik, double & bjk);


};

class SMPState : public State {

public:
  explicit SMPState(Model * m);
  virtual ~SMPState();

  // Calculate the weighted-Euclidean distance matrix, compared to the given positions.
  //
  // If none are provided, it will compare ideals to the postions in this state:
  // vDiff(i,j) = distance from i's ideal to j's position, using i's salience-weights.
  //
  // If vPos provided, it will compare those to the positions in this state:
  // vDiff(i,j) = distance from vPos[i] to j's position, using i's salience-weights.
  virtual void setVDiff(const vector<VctrPstn> & vPos = {});

  // returns h's estimate of i's risk attitude, using the risk-adjustment-rule
  double estNRA(unsigned int h, unsigned int i, BigRAdjust ra) const;

  // returns row-vector of actor's capabilities
  KMatrix actrCaps() const;


  SMPState* stepBCN();

  // The key steps of BCN are to identify a target (and perhaps other target-relevant info)
  // and then to develop a Bargain (possibly nullptr if no bargain is mutually preferable
  // to conflict)
  // you have to provide λ-fn for both of the following
  function<shared_ptr<void>(const Actor* ai, const State* s)> bestTarget = nullptr;
  function<shared_ptr<void>(const Actor* aInit, const Actor* aRcvr, shared_ptr<void> btData, const State* s)> bargain = nullptr;

  virtual void addPstn(Position* p);

  // use the parameters of your state to compute the relative probability of each actor's position
  virtual tuple< KMatrix, VUI> pDist(int persp) const;
  void showBargains(const vector < vector < BargainSMP* > > & brgns) const;
  void showOneBargain(const BargainSMP* b) const;

  virtual bool equivNdx(unsigned int i, unsigned int j) const;

  void setNRA(); // TODO: this just sets risk neutral, for now
  // return actor's normalized risk attitude (if set)
  double aNRA(unsigned int i) const;

  void setAccomodate(double adjRate = 1.0);
  // set ideal-accomodation matrix to scaled identity matrix, for current number of actors

  void setAccomodate(const KMatrix & aMat);
  // set ideal-accomodation matrix to given matrix

  // get the ideal-accommodation matrix
  KMatrix getAccomodate();

  // initialize the actors' ideals from the given list of VctrPstn.
  // If the list is omitted or empty, it uses their current positions
  void idealsFromPstns(const vector<VctrPstn> &  ps = {});
  VctrPstn getIdeal(unsigned int n) const;

  uint64_t getPosMoverBargain(unsigned int actor) const;

  void setPosMoverBargain(unsigned int actor, uint64_t bargainID);

  void calcUtils(unsigned int i) const;  // i == actor id 

protected:

private:

  mutable std::mutex utilDataLock;
  mutable std::multimap<string, KMatrix> tpvData;
  mutable std::multimap<string, double>phijData;
  mutable std::multimap<string, vector<double>> euData;
  void recordProbEduChlg() const;

  // this sets the values in all the AUtil matrices
  virtual void setAllAUtil(ReportingLevel rl);

  virtual void setOneAUtil(unsigned int perspH, ReportingLevel rl);

  KMatrix vDiff = KMatrix(); // vDiff(i,j) = difference between idl[i] and pos[j], using actor i's saliences as weights
  KMatrix rnProb = KMatrix(); // probability of each Unique state, when actors are treated as risk-neutral

  // risk-aware probabilities are uProb

  KMatrix nra = KMatrix();

  SMPState* doBCN() const;

  // returns estimated probability k wins (given likely coaltiions), and expected delta-util of that challenge.
  // If desired, record in SQLite.
  tuple<double, double> probEduChlg(unsigned int h, unsigned int k, unsigned int i, unsigned int j, bool sqlP) const;

  // return best j, p[i>j], edu[i->j]
  tuple<int, double, double> bestChallenge(unsigned int i) const;

  // the actor's ideal, against which they judge others' positions
  vector<VctrPstn> ideals = {};

  // The matrix of rates at which they adjust their ideals toward positions.
  // Change it ONLY via setAccomodate, so as to keep identAccMat in synch
  KMatrix accomodate = KMatrix();
  bool identAccMat = true;

  // rest the new ideal points, based on other's positions and one's old ideal point
  void newIdeals();

  // return RMS distance between ideals and positions
  double posIdealDist(ReportingLevel rl = ReportingLevel::Silent) const;

  void updateBargnTable(const vector<vector<BargainSMP*>> & brgns,
                        map<unsigned int, KBase::KMatrix>  actorBargains,
                        map<unsigned int, unsigned int>  actorMaxBrgNdx) const;

  /**
   * Calculate all challenge utilities (i, i, i, j) which would be used to find the best challenge
   */
  void bestChallengeUtils(unsigned int i /* actor id */) const;

  // Record the bargain id that caused an actor to move in each turn
  using moverBargains = std::map<
    unsigned int, // actor id
    uint64_t // bargain id
    >;
  mutable moverBargains positionMovers;

  using eduChlgsJ = std::map<unsigned int /*j*/, tuple<double, double> >;
  
  mutable std::map<unsigned int /*i*/, eduChlgsJ> eduChlgsIJ;
  
  mutable int bestJ;

	private:
		vector<double> calcVotes(KMatrix w, KMatrix u, int actor) const;
};

class SMPModel : public Model {
  friend class SMPState;
public:
  explicit SMPModel( string desc = "", uint64_t s=KBase::dSeed, vector<bool> f={}, string sceName = ""); // JAH 20160711 added rng seed
  virtual ~SMPModel();

  static string dbPath; //to store db file name from SMPQ GUI, default is testsmp.db
  static void setDBPath(std::string dbName); // to initialize DB Name to dbPath variable

  static const unsigned int maxDimDescLen = 256; // JAH 20160727 added

  static double bsUtil(double sd, double R);
  static double bvDiff(const KMatrix & vd, const  KMatrix & vs);
  static double bvUtil(const KMatrix & vd, const  KMatrix & vs, double R);

  static std::string runModel(std::vector<bool> sqlFlags, std::string dbFilePath,
      std::string inputDataFile, uint64_t seed, bool saveHist, std::vector<int> modelParams = std::vector<int>());

  // this sets up a standard configuration and runs it
  static void configExec(SMPModel * md0);

  // read, configure, and run from CSV
  static string csvReadExec(uint64_t seed, string inputCSV, vector<bool> f, string dbFilePath,
                          vector<int> par=vector<int>());

  // read, configure, and run from XML
  static string xmlReadExec(string inputXML, vector<bool> f, string dbFilePath);

  static void randomSMP(unsigned int numA, unsigned int sDim, bool accP, uint64_t s, vector<bool> f, string inputDBname);

  static SMPModel * csvRead(string fName, uint64_t s, vector<bool> f);
  static SMPModel * xmlRead(string fName,vector<bool> f);

  static  SMPModel * initModel(vector<string> aName, vector<string> aDesc, vector<string> dName,
	  const KMatrix & cap, // one row per actor
	  const KMatrix & pos, // one row per actor, one column per dimension
	  const KMatrix & sal, // one row per actor, one column per dimension
	  const KMatrix & accM,
	  uint64_t s, vector<bool> f, string scenName, string scenDesc);

  // print history of each actor in CSV (might want to generalize to arbitrary VctrPstn)
  void showVPHistory() const;

  void LogInfoTables(); // JAH 20160731

  // output the two files needed to draw Sankey diagrams
  void sankeyOutput(string inputCSV) const;

  // output the two files needed to draw Sankey diagram for Database
  static void sankeyOutput(string outputFile, string dbName, std::string scenarioId) ;

  // number of spatial dimensions in this SMP
  void addDim(string dn);
  unsigned int numDim = 0;
  vector<string> dimName = {};
  double posTol = 1E-3; // on a scale of 0 to 100, this is a difference of just 0.1

  static double stateDist(const SMPState* s1, const SMPState* s2);

  static KTable * createSQL(unsigned int n) ;

  // this does not set AUtil, just output it to SQLite
  //virtual void sqlAUtil(unsigned int t);

  //Model Parameters
  static void updateModelParameters(SMPModel *md0, vector<int> parameters);
  static void displayModelParams(SMPModel *md0);

  //default parameters for SMPQ
  static vector<int> getDefaultModelParameters();

  static void destroyModel();

  /**
   * This version of getQuadMapPoint is meant to be used after a model run is finished
   * but the model objest still exists so that the history could be used
   */
  static double getQuadMapPoint(size_t t, size_t est_h, size_t aff_k, size_t init_i, size_t rcvr_j);

  /**
  * This version of getQuadMapPoint is meant to be used on a db file which contains the results
  * of at least one model run
  */
  static double getQuadMapPoint(string dbname, string scenarioID, size_t turn, size_t est_h,
      size_t aff_k, size_t init_i, size_t rcvr_j);

protected:
  //sqlite3 *smpDB = nullptr; // keep this protected, to ease multi-threading
  //string scenName = "Scen";
  static const int NumTables = 5; // TODO : Add one to this num when new table is added

  static const int NumSQLLogGrps = 0; // TODO : Add one to this num when new logging group is added

  // note that the function to write to table #k must be kept
  // synchronized with the result of createTableSQL(k) !
  void sqlTest();

  // voting rule for actors when forming coalitions over positions or bargains
  VotingRule vrCltn = VotingRule::Proportional;

  ThirdPartyCommit tpCommit = ThirdPartyCommit::SemiCommit;

  // anchoring and adjustment of perceived risk attitudes
  BigRAdjust bigRAdj = BigRAdjust::OneThirdRA;

  // range of big-R values
  BigRRange bigRRng = BigRRange::Mid; // Mid == use [-0.5, +1.0] scale

  // how actors interpolate their bargains
  InterVecBrgn ivBrgn = InterVecBrgn::S2P2;

  // different ways to combine bargaining perspectives
  SMPBargnModel brgnMod = SMPBargnModel::InitOnlyInterpSMPBM;
  // PWCompInterSMPBM, InitOnlyInterpSMPBM or InitRcvrInterpSMPBM;

private:
  void releaseDB();

  
  static tuple<double, double> calcContribs(VotingRule vrCltn, double wi, double wj, tuple<double, double, double, double>(utils));

  // Method used in sqlite execution for callback functionality
  static int callBack(void *data, int numCol, char **stringFields, char **colNames);

  // fieldVals is used to store the result of select sql queries
  static std::vector<string> fieldVals;

  // Method used in sqlite execution for callback functionality for sankeyOoutput
  static int sankeyCallBack(void *data, int numCol, char **stringFields, char **colNames);

  // fieldVals is used to store the result of select sql queries
  static std::vector<string> dbFieldVals;
 };


extern SMPModel * md0 ;
};// end of namespace

// --------------------------------------------
#endif
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
