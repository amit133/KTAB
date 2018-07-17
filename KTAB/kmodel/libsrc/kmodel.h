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
// Define the abstract top-level classes of KTAB models.
// -------------------------------------------------
// We try to handle matrices and vectors in such a way
// as to minimize transpositions.
// The KMatrix u_ij is the utility to actor i of the position held by actor j.
// If p_j is a column-vector of the probabilities of each position being
// the actual outcome of this state, then  e = u*p is the column-vector
// of expected value to each actor, e_i.
// With that same u matrix, suppose w_j is the row-vector of
// the effective weights of each actor in this state, then
// z = w * u is the row-vector of weighted sums of utilities.
// Hence, we always treat u_ij, p_i, e_i, w_j, and z_j as
// matrix, c-vec, c-vec, r-vec, and r-vec respectively.
//
// Notice that we declare operators at the very end, outside the namespace.
// -------------------------------------------------
#ifndef KTAB_MODEL_H
#define KTAB_MODEL_H


#include "kutils.h"
#include "kmatrix.h"
#include "prng.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <map>
#include <memory>

namespace KBase {
using std::ostream;
using std::shared_ptr;
using std::string;
using std::tuple;
using std::vector;
using KBase::KMatrix;
using KBase::ReportingLevel;

class KMatrix;
class PRNG;
class State;
class Actor;
class KTable;


// -------------------------------------------------
// The enum classes are quite repetitive.
// It would be nice to eliminate that, but because "enum class"
// is not really a class in C++11, it is non-obvious.
//
// NOTE WELL: Names in each vector<string> must be synchronized with smpSchema.xsd
//
// -------------------------------------------------

// convert back and forth betwwen strings and the enum class items

template <class ET>
ET enumFromName (const string& ets, const vector<string>& etNames) {
  auto et = static_cast<ET>(0);
  unsigned int ns = etNames.size();
  bool found = false;
  for (unsigned int i=0; i<ns; i++) {
    if (etNames[i] == ets) {
      found = true;
      et = static_cast<ET>(i);
    }
  }
  if (!found) {
    throw(KException(string("enumFromName: unrecognized enum-type name: ") + ets));
  }
  return et;
}



template <class ET>
string nameFromEnum (ET et, const vector<string>& etNames) {
  auto n = static_cast<unsigned int>(et);
  bool found = (n < etNames.size());
  if (!found) {
    throw(KException("nameFromEnum: unrecognized enum-type"));
  }

  string ets = etNames[n];
  return ets;
}


// --------------------------------------------

// How much influence to exert (vote) given a difference in [0,1] utility.
// NOTE WELL: ASymProsp is asymmetric in the sense that v(i:k) + v(k:i) != 0.
// The response to positive delta-Util is only 2/3 of the response to negative,
// in line with "prospect theory". It is there largely to ensure that we are
// ready to handle more realistic asymmetric rules, and not always assume symmetry.
enum class VotingRule {
  Binary=0, PropBin, Proportional, PropCbc, Cubic, ASymProsp
};
const vector<string> VotingRuleNames = {
  "Binary", "PropBin", "Prop", "PropCbc", "Cubic", "ASymProsp"};
ostream& operator<< (ostream& os, const VotingRule& vr); 


// Will state transitions be in deterministic or stochastic mode
enum class StateTransMode {
  DeterminsticSTM=0, StochasticSTM
};
const vector<string> StateTransModeNames = {
  "Deterministic", "Stochastic" };
ostream& operator<< (ostream& os, const StateTransMode& stm);


// At this point, the LISP keyword 'defmacro' should leap to mind.
// Take in a list of strings, and template out everything else.

// Will this PCE be Conditional or Markov-Incentive or Markov-Uniform or  ...
enum class PCEModel {
  ConditionalPCM=0, MarkovIPCM, MarkovUPCM
};
const vector<string> PCEModelNames = {
  "Conditional", "MarkovIncentive", "MarkovUniform" };
ostream& operator<< (ostream& os, const PCEModel& pcm);



// whether you consider the probability of a coalition winning to go up linearly
// quadratically, quartically, or even discontinuously with strength ratios
enum class VPModel {
  // Linear law says 2:1 advantage gives pv = 2/3, and 3:1 gives 3/4
  //            and that 11:10 advantage gives 52.4%
  // Square law says 2:1 advantage gives pv = 4/5, and 3:1 gives 9/10
  //            and that 11:10 advantage gives 54.8%
  // Quartic law says 2:1 advantage gives pv = 16/17, and 3:1 gives 81/82
  //            and that 11:10 advantage gives 59.4%
  // Octic law says 2:1 advantage gives pv = 256/257 (99.6%), 3:1 gives 6561/6562 (99.985%)
  //            and that 11:10 advantage gives 68.2%
  // Binary law says that any percentage difference over a small threshold gives
  //            guaranteed success (or loss), with linear interpolation between
  //            to avoid weird round-off effects.

  Linear=0,  // first power
  Square,    // second power
  Quartic,   // fourth power
  Octic,     // eighth power
  Binary     // threshold, limit of N-th power
};
const vector<string> VPModelNames = {
  "Linear", "Square", "Quartic", "Octic", "Binary" };
ostream& operator<< (ostream& os, const VPModel& vpm);


enum class ThirdPartyCommit {
  NoCommit=0, SemiCommit, FullCommit
};
const vector<string> ThirdPartyCommitNames = {
  "NoCommit", "SemiCommit", "FullCommit" };
ostream& operator<< (ostream& os, const ThirdPartyCommit& tpc);


// estimating risk attitudes from probabilities might have use outside the SMP
enum class BigRRange {
  Min=0, Mid, Max };
const vector<string> BigRRangeNames = {
  "Min", "Mid", "Max" };
ostream& operator << (ostream& os, const BigRRange& rRng);

enum class BigRAdjust {
  NoRA=0, OneThirdRA, HalfRA, TwoThirdsRA, FullRA };
const vector<string> BigRAdjustNames = {
  "None", "OneThird", "Half", "TwoThirds", "Full"};
ostream& operator << (ostream& os, const BigRAdjust& rAdj);

// -------------------------------------------------
// There is not much to say about abstract positions, even
// though the set of possible positions/outcomes is key
// to defining each particular problem.
class Position {
public:
  Position();
  virtual ~Position();

  friend ostream& operator<< (ostream& os, const Position& p) {
    p.print(os);
    return os;
  }

protected:
  virtual void print(ostream& os) const = 0;

private:
};

ostream& operator<< (ostream& os, const Position& p);

// test if this SQLite library was compiled multithreaded.
// If desired, try to reconfigure it
bool testMultiThreadSQLite (bool tryReset, KBase::ReportingLevel rl);

// -------------------------------------------------
// Basic vector position: just a column-vector of numbers.
// They could be interpretted in many ways, as policies
// are often described by a vector of numbers (e.g. tax/subsidy rates).
class VctrPstn : public Position, public KMatrix {
public:
  VctrPstn();
  VctrPstn(unsigned int nr, unsigned int nc);
  explicit VctrPstn(const KMatrix & m); // copy constructor
  virtual ~VctrPstn();
protected:
  virtual void print(ostream& os) const;
private:
};

// -------------------------------------------------
// this is a matching of N items to M categories.
// Note that this is intended to be independent of MtchState, MtchActor, etc.
// Each category is a bucket into which 0, 1, or more items can be put.
// Each item goes in exactly one category. A Matching states, for each of N items,
// which of M categories it goes into. Thus, there are M^N possible matchings.
// Examples are items = pieces of candy, categories = which actor gets them,
// or items = projects to fund, categories = {High, Medium, Low} priority, actors = interest groups
// or items = cabinet seats, categories = parties, actors = interest groups
// or ....
class MtchPstn : public Position {
public:
  MtchPstn();
  virtual ~MtchPstn();
  virtual vector<MtchPstn> neighbors(unsigned int nVar) const;
  // assumes no interaction between items (permutation requires interaction)

  unsigned int numItm = 0;
  unsigned int numCat = 0;
  VUI match = {}; // must be of length numItm

protected:
  virtual void print(ostream& os) const;

private:
};


// bundle up methods relevant to GA over MtchPstn
class MtchGene : public MtchPstn {
public:
  MtchGene();
  ~MtchGene();

  void randomize(PRNG* rng);
  MtchGene * mutate(PRNG * rng) const;
  tuple<MtchGene*, MtchGene*> cross(const MtchGene * g2, PRNG * rng) const;
  //void show() const;
  bool equiv(const MtchGene * g2) const;

  void setState(vector<Actor*> as, vector<MtchPstn*> ps);

protected:
  virtual void print(ostream& os) const;
  void copySelf(MtchGene*) const;
  // links to the State are necessary to evaluate the net support, EU, etc.
  vector<Actor*> actrs = {};
  vector<MtchPstn*> pstns = {};
};


// -------------------------------------------------
class Model {
public:

  // JAH 20160711 added seed 20160730 JAH added sql flags
  // BPW 2016-09-28 removed redundant PRNG input variable
  //explicit Model(string d, uint64_t s, vector<bool> f);
  explicit Model(string d, uint64_t s, vector<bool> f, string Name = "");
  virtual ~Model();

  // forbid copying of entire model objects
  Model(const Model& that) = delete;

  static const unsigned int minNumActor = 3;
  static const unsigned int maxNumActor = 250; //quite generous, as we expect 10-30.

  static const unsigned int maxScenNameLen = 512; // might be auto-generated in sensitivy analysis
  static const unsigned int maxScenDescLen = 512; // see above
  static const unsigned int maxActNameLen = 25; // quite generous, as we expect 1-5
  static const unsigned int maxActDescLen = 256;

  static const unsigned int sqlBuffSize = 250; // big enough buffer to build all desired SQLite statements


  // default 'victory probability model' for bargains and coalitions
  VPModel vpm = VPModel::Linear;

  // default 'probabilistic Condorcet election' model for bargains and coalitions
  PCEModel pcem = PCEModel::ConditionalPCM;

  // default state transition mode is deterministic, not stochastic
  StateTransMode stm = StateTransMode::DeterminsticSTM;

  // In the abstract, you run a model by stepping it until it is time to stop.
  // In detail, each step is likely to record copious information to
  // a database for analysis, and the stopping criterion is likely to
  // lambda-bind a lot of parameters to determine whether anything
  // significant is likely to happen if the run were to continue.
  void run();

  // simple voting based on the difference in utility.
  static double vote(VotingRule vr, double wi, double uij, double uik);

  // The voting function can index into only occupied states, or a mix of occupied and hypothetical states,
  // or into a set of potential two- or three-actor bargains, etc. etc.
  // The vfn can estimate from any actor's perspective.
  // The actors can do simple or complex voting.

  // calculate strength of coalitions for general actors and options.
  // given a 'voting function'. Returns C(i,j) as the non-negative strength
  // of supporting i over j
  static KMatrix coalitions(function<double(unsigned int ak, unsigned int pi, unsigned int pj)> vfn,
                            unsigned int numAct, unsigned int numOpt);

  // calculate pv[i>j] from coalitions
  // c[i,j] is the strength of coalition supporting OptI over OptJ
  static KMatrix vProb(VPModel vpm, const KMatrix & c);

  // assumes simple voting over those options with that utility matrix,
  // builds coalitions, and returns a square matrix of prob(OptI > OptJ)
  // these are assumed to be unique options.
  // w is a [1,actor] row-vector of actor strengths, u is [act,option] utilities.
  static KMatrix vProb(VotingRule vr, VPModel vpm, const KMatrix & w, const KMatrix & u);

  // calculate column vector P[i] from square matrix pv[i>j]
  //static KMatrix probCE(PCEModel pcm, const KMatrix & pv);

  // from square matrix coalition[i:j], return two matrices:
  // column vector P[i] of outcome probabilities
  // square matrix of P[ i > j] victory probabilities
  static tuple<KMatrix, KMatrix> probCE2(PCEModel pcm, VPModel vpm, const KMatrix & cltnStrngth);

  // calculate the [option,1] column vector of option-probabilities.
  // w is a [1,actor] row-vector of actor strengths, u is [act,option] utilities.
  static KMatrix scalarPCE(unsigned int numAct, unsigned int numOpt, const KMatrix & w,
                           const KMatrix & u, VotingRule vr, VPModel vpm, PCEModel pcem, ReportingLevel rl);


  static KMatrix markovIncentivePCE(const KMatrix & coalitions, VPModel vpm);

  virtual unsigned int addActor(Actor* a); // returns new number of actors, always at least 1
  int actrNdx(const Actor* a) const;

  unsigned int addState(State* s); // returns new number of states, always at least 1

  // you have to provide this λ-fn
  function <bool(unsigned int iter, const State* s)> stop = nullptr;

  // these should probably be less public and more protected
  vector<Actor*> actrs = {};
  unsigned int numAct = 0;
  PRNG * rng = nullptr;
  vector<State*> history = {};

  vector<KTable*> KTables = {}; // JAH added 20160728 this will hold info for all defined tables
  vector<bool> sqlFlags= {};    // JAH added 20160730 this will hold the logging flag for each group of tables

  // output an existing actor util table, for the given turn, to SQLite
  void sqlAUtil(unsigned int t);
  // output an existing PosEquiv table, for the given turn, to SQLite
  void sqlPosEquiv(unsigned int t);
  // output an existing PosEquiv table, for the given turn, to SQLite
    void sqlPosProb(unsigned int t);
    void sqlPosVote(unsigned int t);
    void sqlBargainEntries(unsigned int t, int bargainId, int initiator, int receiver, double val);
    void sqlBargainCoords(unsigned int t, int bargnID,  const KBase::VctrPstn & initPos, const KBase::VctrPstn & rcvrPos);
    //void sqlBargainUtil(unsigned int t, vector<uint64_t> bargnIds,  KBase::KMatrix Util_mat);
	void sqlBargainUtil(unsigned int t, vector<uint64_t> bargnIds, KBase::KMatrix Util_mat);
	

	void sqlBargainVote(unsigned int t, vector< std::tuple<uint64_t, uint64_t>> barginidspair_i_j, vector<double> Vote_mat, unsigned int act_k);

  void LogInfoTables(); // JAH 20160731

  void createTableIndices();

  void dropTableIndices();

  static void demoSQLite();

  static KMatrix bigRfromProb(const KMatrix & p, BigRRange rr);

  // returns h's estimate of i's risk attitude, using the risk-adjustment-rule
  static double estNRA(double rh, double  ri, BigRAdjust ra) ;

  string getScenarioID() const {
    return scenId;
  }
  string getScenarioName() const {
    return scenName;
  };

  void setSeed(uint64_t seed);

  uint64_t getSeed() const;

  static KTable * createSQL(unsigned int n);

  void initDBDriver(QString connectionName);
  bool connectDB();
  void closeDB();
  static bool loginCredentials(string connString);
  void beginDBTransaction();
  void commitDBTransaction();
  QSqlQuery getQuery();

  static void configLogger(string logFile);
  static string getLastError();

protected:
  //static string createTableSQL(unsigned int tn);
  static const int NumTables = 13; //TODO: constant need to be redefined when new table is added
  static const int NumSQLLogGrps = 5; // TODO : Add one to this num when new logging group is added
  // note that the function to write to table #k must be kept
  // synchronized with the result of createSQL(k) !



  // Note that, with composite models, there many be dozens interacting.
  string scenName = "Scen"; // default is set from UTC time
  string scenDesc = ""; // default is set from UTC time
  string scenId = "none";
  uint64_t rngSeed = 0; // JAH 20160711 rng seed

  // this is the basic model of victory dependent on strength-ratio
  static tuple<double, double> vProb(VPModel vpm, const double s1, const double s2);

  static QString dbDriver;
  static QString server;
  static int port;
  static QString databaseName;
  static QString userName;
  static QString password;
  QSqlDatabase *qtDB = nullptr;
  mutable QSqlQuery query;
  void configSqlite() const;
  void execQuery(std::string& qry);
  bool createDB(const QString& dbName);
  bool connect(const QString& server,
    const int port,
    const QString& databaseName,
    const QString& userName,
    const QString& password);
  bool isDB(const QString& databaseName);

  static string lastExceptionMsg;
private:
  static KMatrix markovUniformPCE(const KMatrix & pv);
  //static KMatrix markovIncentivePCE(const KMatrix & pv);
  static KMatrix condPCE(const KMatrix & pv);
};


// -------------------------------------------------
class State {
public:
  explicit State(Model* mod);
  virtual ~State();


  void randomizeUtils(double minU, double maxU, double uNoise); //testing only

  void clear();

  // Notice that there are two different ways to add positions.
  // Because the state constructor pre-allocates the pstns array to match
  // the number of actors, it sometimes makes an array of non-trivial size,
  // and sometimes it makes an array of size zero.
  // If you are in the former case, like when a model is doing bargaining from 
  // the current state to the next, then you add positions by putting them
  // in the right, pre-allocated space.
  // However, if you are in the latter case, like when a new model is having
  // its first round of actors created and their positions recorded, then
  // you need to push actors into the Model and push their positions into the
  // State. This function is for the latter case.
  virtual void pushPstn(Position* p);

  // use the parameters of your state to compute the relative probability of each unique position.
  // persp = -1 means use everyone's separate perspectives (i.e. get actual probabilities, not one actor's beliefs).
  // Because the voting mechanisms may differ, so v_k(i:j) could differ widely from sub-class to sub-class,
  // it is tricky to make a single function to do this.
  virtual tuple< KMatrix, VUI> pDist(int persp) const = 0;

  Model * model = nullptr;
  function <State* ()> step = nullptr; // you have to provide this λ-fn
  vector<Position*> pstns = {};

  vector<KMatrix> aUtil = {}; // aUtil[h](i,j) is h's estimate of the utility to A_i of Pos_j

  // This sets the actor/position utility matrix as estimated by H.
  // If H == -1, then set them all.
  void setAUtil(int perspH = -1, ReportingLevel rl = ReportingLevel::Silent);

  void setUENdx();

  // determine if the i-th position in this state is equivalent to the j-th position
  virtual bool equivNdx(unsigned int i, unsigned int j) const = 0;

  double posProb(unsigned int i, const VUI & unq, const KMatrix & pdt) const;

  // return the turn-number of this state.
  // 0 == initial state, and error if not in the model's history
  unsigned int myTurn() const;

protected:
  VUI uIndices = {}; // which positions occupied postions are unique, generated by KBase::ueIndices
  VUI eIndices = {}; // to which unique position each occupied postions matches, generated by KBase::ueIndices

  KMatrix uProb = KMatrix(); // probability of each Unique state

  virtual void setAllAUtil(ReportingLevel rl) = 0;

  virtual void setOneAUtil(unsigned int perspH, ReportingLevel rl); // TODO: make this non-dummy

private:
};


// In the abstract, we can not say much about actors.
// Different utility models might assume different parameters
// in the utility function - and the associated actor
// would have to have their own value for those parameters.
// For example, their capability may change over time,
// so an actor like that would have to have a history of capabilities
// into  which the State could look.
// An actor in multi-dimensional issues might have different
// capabilities on different issues.
// They do not have fixed positions, but change over time.
// And so on.
class Actor {
public:
  Actor(string n, string d);
  virtual ~Actor();

  // returns the vote v_k(i:j), and the utilities of ik>j and i>jk.
  static tuple<double, double, double>
  thirdPartyVoteSU(double wk, VotingRule vr, ThirdPartyCommit comm,
                   double pik, double pjk, double uki, double ukj, double ukk);


  // Third-party actor n determines his contribution on influence by analyzing the hypothetical
  // "little conflict" of just three actors, i.e.  (i:j) with n weighing in on whichever side it favors.
  static double vProbLittle(VotingRule vr, double wn, double uni, double unj, double contrib_i_ij, double contrib_j_ij);

  string name = "GA"; // a short name, usually 2-5 characters
  string desc = "Generic Actor"; // short description, like a line or two.


  // the most common kinds of votes for actors are the following:

  // Vote between positions occupied by two different actors in the same
  // state, just looking up stored information.
  // Note well: it cannot be assumed that the vote between two
  // options can be determined simply by looking at the difference
  // in stored utilities.
  virtual double vote(unsigned int est,unsigned int p1, unsigned int p2, const State* st) const = 0;

  // Bear in mind that some Domain Specific Utility Models, like
  // full-scale computable general equilibrium (CGE) models, may
  // be very expensive to evaluate. Evaluating one policy position yields
  // a predicted state to which all the actors react with their differing utility fns.
  // Thus, we can get a whole column of the U_ij matrix from one CGE evaluation;
  // it would be very slow to re-evaluate the same model (Pj) once for each actor (Ai).
  // Of course, if every actor has a completely different model to predict the
  // states that will result from a policy, then you have no choice but to
  // run each model separately for each actor - so it's best to keep them simple.
  // It may turn out that in policy debates, actors actually use very simple
  // heuristics that are *informed* by complex off-line considerations, so you'd
  // be back to using very simple models.
  // Most actors will have functions somewhat like the following, but
  // the number and type of inputs will vary. Use lambda-fns to
  // construct the 'vfn' for Models::coalition, binding whatever extra
  // parameters your actor needs (e.g. a State* is necessary for strategic voting).
  // double posUtil(const Position * ap1) const = 0;
  // double vote(const Position * ap1, const Position * ap2) const = 0;


protected:
};

// JAH 20160728 KTAB logging table object
// this holds the SQL code needed to generate the table, the name of the table
// (to make it easy to find a table in a vector of KTables), the id number used
// for createSQL, as well as a group ID number, which is used to toggle logging
// for logical groups of tables
class KTable {
public:
  KTable(unsigned int ID, const string &name, const string &SQL, unsigned int grpID);
  virtual ~KTable();

  unsigned int tabID;
  string tabName;
  string tabSQL;
  unsigned int tabGrpID;
  // be sure you use protection on your privates!
protected:

private:
};

}; // end of namespace


bool operator==(const KBase::MtchPstn& mp1, const KBase::MtchPstn& mp2);


// -------------------------------------------------
#endif
// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
