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

#include <assert.h>
#include <iostream>

#include <time.h>
#include "kmodel.h"

namespace KBase {

using std::cout;
using std::endl;
using std::flush;
using std::get;
using std::tuple;

using KBase::nameFromEnum;

// --------------------------------------------


// JAH 20160711 added seed 20160730 JAH added sql flags
// BPW 2016-09-28 removed redundant PRNG input variable
Model::Model(string desc, uint64_t sd, vector<bool> f, string Name) {

  history = vector<State*>();
  actrs = vector<Actor*>();
  numAct = 0;
  stop = nullptr;
  rng = nullptr;

  sqlFlags = f; // JAH 20160730 save the vec of SQL flags
  printf("SQL Logging Flags\n");
  for (unsigned int i = 0; i < sqlFlags.size(); i++)
  {
    printf("Grp %u = %u\n", i, sqlFlags[i] ? 1 : 0);
  }

  // Record the UTC time so it can be used as the default scenario name
  std::chrono::time_point<std::chrono::system_clock> st;
  st = std::chrono::system_clock::now();
  std::time_t start_time = std::chrono::system_clock::to_time_t(st);

  const std::chrono::duration<double> tse = st.time_since_epoch();
  std::chrono::seconds::rep microSeconds = std::chrono::duration_cast<std::chrono::microseconds>(tse).count() % 1000000;

  auto utcBuffId = newChars(500);
  auto hshCode = newChars(100);
  auto utcBuff = newChars(200);

  if (0 == desc.length() || 0 == Name.length()) {
    std::strftime(utcBuff, 150, "Scenario-UTC-%Y-%m-%u-%H%M-%S", gmtime(&start_time));
  }

  if (0 == desc.length()) {
    cout << "No scenario description provided to Model::Model, " << endl;
    cout << "Using default description generated from UTC start time." << endl;

    scenDesc = utcBuff;
  }
  else {
    scenDesc = desc;
  }

  if (0 == Name.length()) {
    cout << "No scenario name provided to Model::Model " << endl;
    cout << "Using default description generated from UTC start time." << endl;

    scenName = utcBuff;
  }
  else
  {
    scenName = Name;
  }

  sprintf(utcBuffId, "%s_%u", scenName.c_str(), microSeconds);

  delete utcBuff;
  utcBuff = nullptr;

  //get the hash
  uint64_t scenIdhash = (std::hash < std::string>() (utcBuffId));
  sprintf(hshCode, "%032llX", scenIdhash);
  scenId = hshCode;
  delete hshCode;
  hshCode = nullptr;
  delete utcBuffId;
  utcBuffId = nullptr;

  // BPW 20160717
  // (a) record a reproducible seed even we were given '0'
  // (b) make sure that the given value really is the initial seed
  // (c) at this point, the PRNG* is almost irrelevant, except that someone
  // might, in the future, provide an object which was a new subclass of PRNG*

  rng = new PRNG();
  if (0 == sd) {
    rng->setSeed(0); // random and irreproducible
    sd = rng->uniform(); // random and irreproducible 64-bits
  }

  // JAH 20160711 save the seed for the rng
  setSeed(sd);

  // BPW 20160928 print out the seed actually used (non-zero) instead of the one given (e.g. 0)
  printf("Using PRNG seed: %020llu \n", rngSeed);
  cout << "Scenario Name: -|" << scenName << "|-" << endl << flush;
  cout << "Scenario Description: " << scenDesc << endl;
}

void Model::setSeed(uint64_t seed) {
    rngSeed = seed;
    rng->setSeed(rngSeed);
}

uint64_t Model::getSeed() const {
    return rngSeed;
}

Model::~Model() {
  while (0 < history.size()) {
    State* s = history[history.size() - 1];
    delete s;
    history.pop_back();
  }

  while (0 < actrs.size()) {
    Actor * a = actrs[actrs.size() - 1];
    delete a;
    actrs.pop_back();
  }
  numAct = 0;
  rng = nullptr;

  // JAH 20160802 garbage collect the KTables vector
  while (0 < KTables.size())
  {
    KTable* t = KTables[KTables.size() - 1];
    delete t;
    KTables.pop_back();
  }

  if (nullptr != qtDB && qtDB->isValid()) {
    // Note: It is necessary to free the resources held by query object
    // Else the removeDatabase() method causes segmentation fault
    query.finish();
    query.clear();
    QString connName = qtDB->connectionName();
    qtDB->close();
    delete qtDB;
    QSqlDatabase::removeDatabase(connName);
  }
}


void Model::run() {
  assert(1 == history.size());
  State* s0 = history[0];
  bool done = false;
  unsigned int iter = 0;

  while (!done) {
    assert(nullptr != s0);
    assert(nullptr != s0->step);
    iter++;
    cout << "Starting Model::run iteration " << iter << endl;
    auto s1 = s0->step();
    addState(s1);
    done = stop(iter, s1);
    s0 = s1;
  }
  return;
}

unsigned int Model::addActor(Actor* a) {
  assert(nullptr != a);
  actrs.push_back(a);
  numAct = ((unsigned int)(actrs.size()));
  return numAct;
}


unsigned int Model::addState(State* s) {
  assert(nullptr != s);
  assert(this == s->model);
  s->model = this;
  history.push_back(s);
  auto hs = ((const unsigned int)(history.size()));
  return hs;
}


KMatrix Model::bigRfromProb(const KMatrix & p, BigRRange rr) {
  double pMin = 1.0;
  double pMax = 0.0;
  for (double pi : p) {
    assert(0.0 <= pi);
    pMin = (pi < pMin) ? pi : pMin;
    pMax = (pi > pMax) ? pi : pMax;
  }

  const double pTol = 1E-8;
  assert(fabs(1 - KBase::sum(p)) < pTol);

  function<double(unsigned int, unsigned int)> rfn = nullptr;
  switch (rr) {
  case BigRRange::Min:
    rfn = [pMin, pMax, p](unsigned int i, unsigned int j) {
      return (p(i, j) - pMin) / (pMax - pMin);
    };
    break;
  case BigRRange::Mid:
    rfn = [pMin, pMax, p](unsigned int i, unsigned int j) {
      return (3 * p(i, j) - (pMax + 2 * pMin)) / (2 * (pMax - pMin));
    };
    break;
  case BigRRange::Max:
    rfn = [pMin, pMax, p](unsigned int i, unsigned int j) {
      return (2 * p(i, j) - (pMax + pMin)) / (pMax - pMin);
    };
    break;
  }
  auto rMat = KMatrix::map(rfn, p.numR(), p.numC());
  return rMat;
}


// returns h's estimate of i's risk attitude, using the risk-adjustment-rule
double Model::estNRA(double rh, double  ri, BigRAdjust ra) {
  double rhi = 0.0;
  switch (ra) {
  case BigRAdjust::FullRA:
    rhi = ri;
    break;
  case BigRAdjust::TwoThirdsRA:
    rhi = (rh + (2.0*ri)) / 3.0;
    break;
  case BigRAdjust::HalfRA:
    rhi = (rh + ri) / 2;
    break;
  case BigRAdjust::OneThirdRA:
    rhi = ((2.0*rh) + ri) / 3.0;
    break;
  case BigRAdjust::NoRA:
    rhi = rh;
    break;
  }
  return rhi;
}

int Model::actrNdx(const Actor* a) const {
  int ai = -1;
  for (unsigned int i = 0; i < numAct; i++) {
    if (a == actrs[i]) {
      ai = i;
    }
  }
  return ai; // negative iff this "actor" was not found
}



ostream& operator<< (ostream& os, const VPModel& vpm) {
  string s = nameFromEnum<VPModel>(vpm, KBase::VPModelNames);
  os << s;
  return os;
}


ostream& operator<< (ostream& os, const VotingRule& vr) {
  string s = nameFromEnum<VotingRule>(vr, KBase::VotingRuleNames);
  os << s;
  return os;
}


//   auto et = KBase::enumFromName<VotingRule>(s, KBase::VotingRuleNames);
//   string s2 = KBase::nameFromEnum<VotingRule>(vr, KBase::VotingRuleNames);


ostream& operator<< (ostream& os, const StateTransMode& stm) {
  string s = nameFromEnum<StateTransMode>(stm, KBase::StateTransModeNames);
  os << s;
  return os;
}

ostream& operator<< (ostream& os, const PCEModel& pcm) {
  string s = nameFromEnum<PCEModel>(pcm, KBase::PCEModelNames);
  os << s;
  return os;
}

ostream& operator<< (ostream& os, const ThirdPartyCommit& tpc) {
  string s = nameFromEnum<ThirdPartyCommit>(tpc, KBase::ThirdPartyCommitNames);
  os << s;
  return os;
} 

ostream& operator << (ostream& os, const BigRRange& rRng) {
  string s = nameFromEnum<BigRRange>(rRng, KBase::BigRRangeNames);
  os << s;
  return os;
}

ostream& operator << (ostream& os, const BigRAdjust& rAdj) {
  string s = nameFromEnum<BigRAdjust>(rAdj, KBase::BigRAdjustNames);
  os << s;
  return os;
}



double Model::vote(VotingRule vr, double wi, double uij, double uik) {
  if (wi <= 0.0) { // you can make it really small (10E-10), but never zero or below.
    throw KException("Model::vote - non-positive voting weight");
  }
  double v = 0.0;
  const double du = uij - uik;

  double rBin = 0; // binary response
  const double sTol = 1E-8;
  rBin = du / sTol;
  rBin = (rBin > +1) ? +1 : rBin;
  rBin = (rBin < -1) ? -1 : rBin;

  double rProp = du; // proportional response
  double rCubic = du * du * du; // cubic reponse

  // the following weights determine how much the hybrids deviate from proportional
  const double rbp = 0.2;
  // rbp = 0.2 makes LHS slope and RHS slope of equal size (0.8 each), and twice the center jump (0.4)

  const double rpc = 0.5;

  switch (vr) {
  case VotingRule::Binary:
    v = wi * rBin;
    break;

  case VotingRule::PropBin:
    v = wi * ((1 - rbp)*rProp + rbp*rBin);
    break;

  case VotingRule::Proportional:
    v = wi * rProp;
    break;

  case VotingRule::PropCbc:
    v = wi * ((1 - rpc)*rProp + rpc*rCubic);
    break;

  case VotingRule::Cubic:
    v = wi * rCubic;
    break;

  case VotingRule::ASymProsp:
    if (rProp < 0.0) {
      v = wi * rProp;
    }
    if (0.0 < rProp) {
      v = (2.0 * wi * rProp) / 3.0;
    }
    break;

  default:
    throw KException("Model::vote - Unrecognized VotingRule");
    break;
  }
  return v;
}

tuple<double, double> Model::vProb(VPModel vpm, const double s1, const double s2) {
  const double tol = 1E-8;
  const double minX = 1E-6;
  double x1 = 0;
  double x2 = 0;
  switch (vpm) {
  case VPModel::Linear:
    x1 = s1;
    x2 = s2;
    break;
  case VPModel::Square:
    x1 = KBase::sqr(s1);
    x2 = KBase::sqr(s2);
    break;
  case VPModel::Quartic:
    x1 = KBase::qrtc(s1);
    x2 = KBase::qrtc(s2);
    break;
  case VPModel::Octic:
    x1 = KBase::sqr(KBase::qrtc(s1));
    x2 = KBase::sqr(KBase::qrtc(s2));
    break;
  case VPModel::Binary:
  {
    // this is setup so that 10% or more advantage either way gives a guaranteed
    // result. As with binary voting, it is necessary to have interpolation between
    // to avoid weird round-off effects.
    const double thresh = 1.10;
    if (s1 >= thresh*s2) {
      x1 = 1.0;
      x2 = minX;
    }
    else if (s2 >= thresh*s1) {
      x1 = minX;
      x2 = 1.0;
    }
    else { // less than the threshold difference
      double r12 = s1 / (s1 + s2);
      // We now need a linear rescaling so that
      // when s1/s2 = t, or r12 = t/(t+1), p12 = 1, and
      // when s2/s1 = t, or r12 = 1/(1+t), p12 =0.
      // We can work out (a,b) so that
      // a*(t/(t+1)) + b = 1, and
      // a*(1/(1+t)) + b = 0, then
      // verify that a*(1/(1+1))+b = 1/2.
      //
      double p12 = (r12*(thresh + 1.0) - 1.0) / (thresh - 1.0);
      x1 = p12;
      x2 = 1 - p12;
    }
  }
    break;
  }
  double p1 = x1 / (x1 + x2);
  double p2 = x2 / (x1 + x2);
  assert(0 <= p1);
  assert(0 <= p2);
  assert(fabs(p1 + p2 - 1.0) < tol);

  return tuple<double, double>(p1, p2);
}

// note that while the C_ij can be any arbitrary positive matrix
// with C_kk = 0, the p_ij matrix has the symmetry pij + pji = 1
// (and hence pkk = 1/2).
KMatrix Model::vProb(VPModel vpm, const KMatrix & c) {
  unsigned int numOpt = c.numR();
  assert(numOpt == c.numC());
  auto p = KMatrix(numOpt, numOpt);
  for (unsigned int i = 0; i < numOpt; i++) {
    for (unsigned int j = 0; j < i; j++) {
      double cij = c(i, j);
      assert(0 <= cij);
      double cji = c(j, i);
      assert(0 <= cji);
      assert((0 < cij) || (0 < cji));
      auto ppr = vProb(vpm, cij, cji);
      p(i, j) = get<0>(ppr); // set the lower left  probability: if Linear, cij / (cij + cji)
      p(j, i) = get<1>(ppr); // set the upper right probability: if Linear, cji / (cij + cji)
    }
    p(i, i) = 0.5; // set the diagonal probability
  }
  return p;
}

KMatrix Model::coalitions(function<double(unsigned int ak, unsigned int pi, unsigned int pj)> vfn,
                          unsigned int numAct, unsigned int numOpt) {
  // if several actors occupy the same position, then numAct > numOpt
  const double minC = 1E-8;
  auto c = KMatrix(numOpt, numOpt);
  for (unsigned int i = 0; i < numOpt; i++) {
    for (unsigned int j = 0; j < i; j++) {
      // scan only lower-left
      double cij = minC;
      double cji = minC;
      for (unsigned int k = 0; k < numAct; k++) {
        double vkij = vfn(k, i, j);
		
        if (vkij > 0) {
          cij = cij + vkij;
        }
        if (vkij < 0) {
          cji = cji - vkij;
        }
      }
      c(i, j) = cij;  // set the lower left coalition
      c(j, i) = cji;  // set the upper right coalition

    }
	c(i, i) = minC; // set the diagonal coalition
  }
  return c;
}

// returns a square matrix of prob(OptI > OptJ)
// these are assumed to be unique options.
// w is a [1,actor] row-vector of actor strengths, u is [act,option] utilities.
KMatrix Model::vProb(VotingRule vr, VPModel vpm, const KMatrix & w, const KMatrix & u) {
  // u_ij is utility to actor i of the position advocated by actor j
  unsigned int numAct = u.numR();
  unsigned int numOpt = u.numC();
  // w_j is row-vector of actor weights, for simple voting
  assert(numAct == w.numC()); // require 1-to-1 matching of actors and strengths
  assert(1 == w.numR()); // weights must be a row-vector

  auto vfn = [vr, &w, &u](unsigned int k, unsigned int i, unsigned int j) {
    double vkij = vote(vr, w(0, k), u(k, i), u(k, j));
    return vkij;
  };

  auto c = coalitions(vfn, numAct, numOpt); // c(i,j) = strength of coaltion for i against j
  KMatrix p = vProb(vpm, c);  // p(i,j) = prob Ai defeats Aj
  return p;
}

tuple<KMatrix, KMatrix> Model::probCE2(PCEModel pcm, VPModel vpm, const KMatrix & cltnStrngth) {
  const double pTol = 1E-8;
  unsigned int numOpt = cltnStrngth.numR();
  auto p = KMatrix(numOpt, 1);
  const auto victProb = Model::vProb(vpm, cltnStrngth); // prob of victory, square
  switch (pcm) {
  case PCEModel::ConditionalPCM:
    p = condPCE(victProb);
    break;
  case PCEModel::MarkovIPCM:
    p = markovIncentivePCE(cltnStrngth, vpm);
    break;
  case PCEModel::MarkovUPCM:
    p = markovUniformPCE(victProb);
    break;
  default:
    throw KException("Model::probCE unrecognized PCEModel");
    break;
  }
  assert(numOpt == p.numR());
  assert(1 == p.numC());
  assert(fabs(sum(p) - 1.0) < pTol);
  return tuple<KMatrix, KMatrix>(p, victProb);
}

/*
  // Given square matrix of Prob[i>j] returns a column vector for Prob[i]
  KMatrix Model::probCE(PCEModel pcm, const KMatrix & pv) {
    const double pTol = 1E-8;
    unsigned int numOpt = pv.numR();
    assert(numOpt == pv.numC()); // must be square
    auto test = [&pv, pTol](unsigned int i, unsigned int j) {
      assert(0 <= pv(i, j));
      assert(fabs(pv(i, j) + pv(j, i) - 1.0) < pTol);
      return;
    };
    KMatrix::mapV(test, numOpt, numOpt); // catch gross errors

    auto p = KMatrix();
    switch (pcm) {
    case PCEModel::ConditionalPCM:
      p = condPCE(pv);
      break;
    case PCEModel::MarkovIPCM:
      throw KException("Model::probCE not yet implemented MarkovIPCM");
      break;
    case PCEModel::MarkovUPCM:
      p = markovUniformPCE(pv);
      break;
    default:
      throw KException("Model::probCE unrecognized PCEModel");
      break;
    }
    assert(fabs(sum(p) - 1.0) < pTol);
    return p;
  }
*/

// Given square matrix of strengths, Coalition[i over j] returns a column vector for Prob[i].
// Uses Markov process, not 1-step conditional probability.
// Challenge probabilities are proportional to influence promoting a challenge
KMatrix Model::markovIncentivePCE(const KMatrix & coalitions, VPModel vpm) {
  using KBase::sqr;
  using KBase::qrtc;
  const bool printP = false;
  const double pTol = 1E-8;
  const unsigned int numOpt = coalitions.numR();
  assert(numOpt == coalitions.numC());

  const auto victProbMatrix = vProb(vpm, coalitions);

  // given coalitions, calculate the total incentive for i to challenge j
  // This is n[ i -> j] in the "Markov Voting with Incentives in KTAB" paper
  auto iFn = [victProbMatrix, coalitions](unsigned int i, unsigned int j) {
    const double epsSupport = 1E-10;
    const double sij = coalitions(i, j);
    double inctv = sij * victProbMatrix(i,j);
    if (i == j) {
      inctv = inctv + epsSupport;
    }
    return inctv;
  };

  const auto inctvMatrix = KMatrix::map(iFn, numOpt, numOpt);

  // Using the incentives, calculate the probability of i challenging j,
  // given that j is the current favorite proposal.
  // This is P[ i -> j] in the "Markov Voting with Incentives in KTAB" paper
  // Note that if every actor prefers j to every other option,
  // then all incentive(i,j) will be zero, except incentive(j,j) = eps.
  // Even in this case, we will not get a division by zero error,
  // and it will correctly return that the only "challenger" is j itself,
  // with guaranteed success.
  //
  auto cpFn = [inctvMatrix, numOpt](unsigned int i, unsigned int j) {
    double sum = 0.0;
    for (unsigned int k = 0; k < numOpt; k++) {
      sum = sum + inctvMatrix(k, j);
    }
    const double pij = inctvMatrix(i, j) / sum;
    return pij;
  };

  const auto chlgProbMatrix = KMatrix::map(cpFn, numOpt, numOpt);

  // probability starts as uniform distribution (column vector)
  auto p = KMatrix(numOpt, 1, 1.0) / numOpt;  // all 1/n
  auto q = p;
  unsigned int iMax = 1000;  // 10-30 is typical
  unsigned int iter = 0;
  double change = 1.0;

  // do the markov calculation
  while (pTol < change)  { // && (iter < iMax)
    if (printP) {
      printf("Iteration  %u / %u \n", iter, iMax);
      cout << "pDist:" << endl;
      trans(p).mPrintf(" %.4f");
      cout << endl;
      printf("change: %.4e \n", change);
      cout << endl << flush;
    }
    auto ct = KMatrix(numOpt, numOpt);
    for (unsigned int i = 0; i < numOpt; i++) {
      for (unsigned int j = 0; j < numOpt; j++) {
        // See "Markov Voting with Incentives in KTAB" paper
        ct(i, j) = p(i, 0) * chlgProbMatrix(j, i);
      }
    }
    if (printP) {
      cout << "Ct:" << endl;
      ct.mPrintf("  %.3f");
      cout << endl << flush;
    }
    change = 0.0;
    for (unsigned int i = 0; i < numOpt; i++) {
      double qi = 0.0;
      for (unsigned int j = 0; j < numOpt; j++) {
        double vij = victProbMatrix(i, j);
        double cj = ct(i, j) + ct(j, i);
        qi = qi + vij* cj;
      }
      assert(0 <= qi); // double-check
      q(i, 0) = qi;
      double c = fabs(q(i, 0) - p(i, 0));
      change = (c > change) ? c : change;
    }
    // Newton method improves convergence.
    p = (p+q)/2.0;
    iter++;
    assert(fabs(sum(p) - 1.0) < pTol); // double-check
  }

  assert(iter < iMax); // no way to recover
  return p;
}

/*
  KMatrix Model::markovIncentivePCE(const KMatrix & pv) {
    throw KException("Model::markovIncentivePCE not yet implemented");

    const double pTol = 1E-6;
    unsigned int numOpt = pv.numR();
    auto p = KMatrix(numOpt, 1, 1.0) / numOpt;  // all 1/n
    auto q = p;
    unsigned int iMax = 1000;  // 10-30 is typical
    unsigned int iter = 0;
    double change = 1.0;

    // do the markov calculation

    assert(iter < iMax); // no way to recover
    return p;
  }
  */

// Given square matrix of Prob[i>j] returns a column vector for Prob[i].
// Uses Markov process, not 1-step conditional probability.
// Challenges have uniform probability 1/N
KMatrix Model::markovUniformPCE(const KMatrix & pv) {
  const double pTol = 1E-6;
  unsigned int numOpt = pv.numR();
  auto p = KMatrix(numOpt, 1, 1.0) / numOpt;  // all 1/n
  auto q = p;
  unsigned int iMax = 1000;  // 10-30 is typical
  unsigned int iter = 0;
  double change = 1.0;
  while (pTol < change) {
    change = 0;
    for (unsigned int i = 0; i < numOpt; i++) {
      double pi = 0.0;
      for (unsigned int j = 0; j < numOpt; j++) {
        pi = pi + pv(i, j)*(p(i, 0) + p(j, 0));
      }
      assert(0 <= pi); // double-check
      q(i, 0) = pi / numOpt;
      double c = fabs(q(i, 0) - p(i, 0));
      change = (c > change) ? c : change;
    }
    // Newton method improves convergence.
    p = (p + q) / 2.0;
    iter++;
    assert(fabs(sum(p) - 1.0) < pTol); // double-check
  }
  assert(iter < iMax); // no way to recover
  return p;
}


// Given square matrix of Prob[i>j] returns a column vector for Prob[i].
// Uses 1-step conditional probabilities, not Markov process
KMatrix Model::condPCE(const KMatrix & pv) {
  unsigned int numOpt = pv.numR();
  auto p = KMatrix(numOpt, 1);
  for (unsigned int i = 0; i < numOpt; i++) {
    double pi = 1.0;
    for (unsigned int j = 0; j < numOpt; j++) {
      pi = pi * pv(i, j);
    }
    // double-check
    assert(0 <= pi);
    assert(pi <= 1);
    p(i, 0) = pi; // probability that i beats all alternatives
  }
  double probOne = sum(p); // probability that one option, any option, beats all alternatives
  p = (p / probOne); // conditional probability that i is that one.
  return p;
}

// calculate the [option,1] column vector of option-probabilities.
// w is a [1,actor] row-vector of actor strengths, u is [act,option] utilities.
// This assumes scalar capabilities of actors (w), so that the voting strength
// is a direct function of difference in utilities.Therefore, we can use
// Model::vProb(VotingRule vr, const KMatrix & w, const KMatrix & u)
KMatrix Model::scalarPCE(unsigned int numAct, unsigned int numOpt, const KMatrix & w, const KMatrix & u,
                         VotingRule vr, VPModel vpm, PCEModel pcem, ReportingLevel rl) {

  // auto pv = Model::vProb(vr, vpm, w, u);
  // auto p = Model::probCE(pcem, pv);

  auto vfn = [vr, &w, &u](unsigned int k, unsigned int i, unsigned int j) {
    double vkij = vote(vr, w(0, k), u(k, i), u(k, j));
    return vkij;
  };
  const auto c = coalitions(vfn, numAct, numOpt); // c(i,j) = strength of coaltion for i against j
  const auto pv2 = Model::probCE2(pcem, vpm, c);
  const auto p = get<0>(pv2); //column
  const auto pv = get<1>(pv2); // square
  if (ReportingLevel::Low < rl) {
    printf("Num actors: %i \n", numAct);
    printf("Num options: %i \n", numOpt);


    if ((numAct <= 20) && (numOpt <= 20)) {
      cout << "Actor strengths: " << endl;
      w.mPrintf(" %6.2f ");
      cout << endl << flush;
      cout << "Voting rule: " << vr << endl;
      // printf("         aka %s \n", KBase::vrName(vr).c_str());
      cout << flush;
      cout << "Utility to actors of options: " << endl;
      u.mPrintf(" %+8.3f ");
      cout << endl << flush;

      cout << "Coalition strengths of (i:j): " << endl;
      c.mPrintf(" %8.3f ");
      cout << endl;

      cout << "Probability Opt_i > Opt_j" << endl;
      pv.mPrintf(" %.4f ");
      cout << "Probability Opt_i" << endl;
      p.mPrintf(" %.4f ");
    }
    cout << "Found stable PCE distribution" << endl << flush;
  }
  return p;
}


// -------------------------------------------------
Actor::Actor(string n, string d) {
  name = n;
  desc = d;
}


Actor::~Actor() {
  // empty
}

// this uses the evenly-weighted "Sum of Utilities" model
// Note that the pi and pj numbers already include the contribution of k to the hypothetical little
// i:j conflict with just i,j,k involved
// TODO: implement and offer the "Expected Value of State-Utility" model
// returns the vote v_k(i:j), and the utilities of ik>j and i>jk.
tuple<double, double, double>
Actor::thirdPartyVoteSU(double wk, VotingRule vr, ThirdPartyCommit comm,
                        double pik, double pjk, double uki, double ukj, double ukk) {
  const double pTol = 1E-8;
  assert(0 <= pik);
  assert(0 <= pjk);
  assert(fabs(pik + pjk - 1.0) < pTol);
  double p_ik_j = pik; // estimated probability ik > j
  double p_jk_i = pjk; // estimated probability jk > i
  double p_j_ik = pjk; // estimated probability j  > ik
  double p_i_jk = pik; // estimated probability i  > jk
  double u_ik_def_j = 0;
  double u_j_def_ik = 0;
  double u_i_def_jk = 0;
  double u_jk_def_i = 0;
  // strictly speaking, we should add in all the utilities of unchanged positions, then divide by numAct,
  // thus computing the expected utility for a flat probability distribution,
  // so that the utility of each alternative state is between 0 and 1.
  // However,
  // A: subtraction will make all utilities of unchanged positions cancel here when vk is determined, and
  // B: dividing in P[1>2] = C12 / (C12 + C21) will cancel the factors of 1/4.
  //
  // Therefore, we can drop those terms as long we make sure all comparisons use the sum
  // of three position-utilities, and we obviously do so.
  switch (comm) {
  case ThirdPartyCommit::FullCommit:
    u_ik_def_j = uki + uki + uki;
    u_j_def_ik = ukj + ukj + ukj;
    u_i_def_jk = uki + uki + uki;
    u_jk_def_i = ukj + ukj + ukj;
    break;
  case ThirdPartyCommit::SemiCommit:
    u_ik_def_j = uki + uki + ukk;
    u_j_def_ik = ukj + ukj + ukj;
    u_i_def_jk = uki + uki + uki;
    u_jk_def_i = ukj + ukj + ukk;
    break;
  case ThirdPartyCommit::NoCommit:
    u_ik_def_j = uki + uki + ukk;
    u_j_def_ik = ukj + ukj + ukk;
    u_i_def_jk = uki + uki + ukk;
    u_jk_def_i = ukj + ukj + ukk;
    break;
  }
  double u_ik_j = (p_ik_j * u_ik_def_j) + (p_j_ik * u_j_def_ik);
  double u_i_jk = (p_i_jk * u_i_def_jk) + (p_jk_i * u_jk_def_i);
  double vk = Model::vote(vr, wk, u_ik_j, u_i_jk);
  return tuple<double, double, double>(vk, u_ik_def_j, u_i_def_jk);
}


// Third-party actor n determines his contribution on influence by analyzing the hypothetical
// "little conflict" of just three actors, i.e.  (i:j) with n weighing in on whichever side it favors.
double Actor::vProbLittle(VotingRule vr, double wn, double uni, double unj, double contrib_i_ij, double contrib_j_ij) {

  // the primary actors are assigned at least the minisucle
  // minimum influence contribution, to avoid 0/0 errors.
  if (contrib_i_ij <= 0) {
    assert(contrib_i_ij > 0);
  }
  if (contrib_j_ij <= 0) {
    assert(contrib_j_ij > 0);
  }

  // order-zero estimate depends only on the voting between their two positions
  double contrib_n_ij = Model::vote(vr, wn, uni, unj);

  // add that to whichever side they preferred
  double cni = (contrib_n_ij > 0) ? contrib_i_ij + contrib_n_ij : contrib_i_ij;
  double cnj = (contrib_n_ij < 0) ? contrib_j_ij - contrib_n_ij : contrib_j_ij;

  // assert(0 < cni);
  // assert(0 < cnj);


  double pin = cni / (cni + cnj);
  //double pjn = cnj / (cni + cnj); // just FYI
  return pin;
}

} // end of namespace

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
