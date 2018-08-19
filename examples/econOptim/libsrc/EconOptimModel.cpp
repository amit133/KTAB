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
// Demonstrate some very basic functionality in
// the DemoLeon namespace
// --------------------------------------------


#include "EconOptimModel.h"


using KBase::PRNG;
using KBase::KMatrix;
using KBase::Actor;
using KBase::Model;
using KBase::Position;
using KBase::State;
using KBase::KException;

using KBase::PCEModel;
using KBase::VotingRule;
using KBase::VPModel;

namespace DemoEconOptim {
const double TolIFD = 1E-6;

EconoptimActor::EconoptimActor(string n, string d, EconoptimModel* em, unsigned int id) : Actor(n, d) {
  if (nullptr == em) {
    throw KException("EconoptimActor::EconoptimActor: em is null pointer");
  }
  vr = VotingRule::Proportional;
  eMod = ((const EconoptimModel*)em);
  idNum = id;
  minS = 0;
  refS = 0.5;
  refU = 0.5;
  maxS = 1;
}


EconoptimActor::~EconoptimActor() {}


double EconoptimActor::vote(unsigned int est, unsigned int i, unsigned int j, const State* st) const {
  unsigned int k = st->model->actrNdx(this);
  auto uk = st->aUtil[est];
  double uhki = uk(k, i);
  double uhkj = uk(k, j);
  const double sCap = sum(vCap);
  const double vij = Model::vote(vr, sCap, uhki, uhkj);
  // as mentioned below, I calculate the vote the easy way.
  // I could compile the matrix of votes and re-use it.
  return vij;
}


double EconoptimActor::vote(const Position * ap1, const Position * ap2) const {
  auto p1 = ((const VctrPstn*)ap1);
  auto p2 = ((const VctrPstn*)ap2);

  // it rather inefficient to re-run the entire economic
  // model (yielding L+N utilities) separately for each actor.
  // Correct first, fast later (unless architecture prevents speed-up!)
  if ((0 >= refU) || (refU >= 1)) {
    throw KException("EconoptimActor::vote: refU must be in the range (0, 1)");
  }
  if ((minS >= refS) || (refS >= maxS)) {
    throw KException(string("EconoptimActor::vote: refS must be in the range (") + std::to_string(minS) + "," + std::to_string(maxS) + ")");
  }
  const double u1 = posUtil(p1);
  const double u2 = posUtil(p2);
  const double sCap = sum(vCap);
  if (0 >= sCap) {
    throw KException("EconoptimActor::vote: sCap must be positive");
  }
  const double v12 = Model::vote(vr, sCap, u1, u2);
  // for now, I do it the easy way.

  // More plausible, but tedious, would be to do a little sensitivity analysis
  // on each individual tax rate to see whether it, in context, is beneficial.
  // For example, suppose we want v(A:B) between two tax policies.
  // For each component i, define A'_i = A, but with i-th component replaced by B_i.
  // So da_i = u(A) - u(A'_i) is one assessment of the benefit of having A_i rather than B_i.
  // If the u(X)= dot(g,X), this would be just g_i * (A_i - B_i).
  //
  // Similarly, B'_i = B, but with i-th component replace by A_i.
  // Now db_i = u(B'_i) - u(B) is a second assessment of the benefit of having A_i rather than B_i.
  // Again, this would be  g_i * (A_i - B_i).
  //
  // di = (da_i + db_i)/2 = combined assessment of benefit of A_i over B_i
  //
  // v_i(A:B) = Model::vote(vr, vCap(i), +di/2, -di/2)
  //
  // Finally, set v(A:B) = sum_i of v_i(A:B)
  //
  // Perhaps an even better approach would be for other actors to look at the
  // capability to influence what the other actors care about. If Ai can influence X but not Y,
  // and Aj cares about Y but not X, then there is little which Ai can hold at risk to influence
  // Aj's actions on dimension Y. If they both care about and influence X, while Aj alone influences Y,
  // then the Nash Bargain may have Aj giving up something of Y to Ai in order to get more of X.
  // These cross-issue trades probably have to be addressed via a full multi-dimensional Nash Bargaining
  // solution; see the 'kutils' demo for Nash bargaining with 4D positions that takes into account per-component
  // salience and overall win-probability. This would seem to involve assessing each dimension as separately
  // as a 1D issue with salience and capability for each dimension -- then take the vector of outcomes
  // as the reference outcome from which to start cross-issue trades in Nash bargaining.
  //
  // This works best if one can assess utility separately along each dimension, as per the SMP, even
  // though the individual differences get combined into one weighted Euclidean distance in the SMP.
  // It is not so clear in this Leontief system, because of economic linkages, though the little
  // sensitivity analysis suggested above might suffice.

  return v12;
}



double EconoptimActor::posUtil(const Position * ap) const {
  auto tax = ((const VctrPstn*)ap);
  auto shares = eMod->vaShares(*tax, false);
  double si = shares(0, idNum);
  double u = shareToUtil(si);
  return u;
}

double EconoptimActor::shareToUtil(double gdpShare) const {
  double u = refU;
  if (gdpShare < refS)
    u = KBase::rescale(gdpShare, minS, refS, 0, refU);
  else
    u = KBase::rescale(gdpShare, refS, maxS, refU, 1);
  return u;
}


void EconoptimActor::randomize(PRNG* rng) {
  unsigned int numD = eMod->N;
  double sc = rng->uniform(20, 200);
  vCap = KMatrix::uniform(rng, numD, 1, 1.0, 5.0);
  vCap = (sc / sum(vCap)) * vCap;
  // utility scale is not random, but must be set from a run-matrix.
  return;
}

void EconoptimActor::setShareUtilScale(const KMatrix & runs) {
  // each row gives shares in [factor | sector] order, so it needs to know its column number
  refU = 0.5556;
  // induce risk-aversion:
  // worst plausible changes from the base-case could cost refU,
  // while best plausible imporovements can only gain 1-refU.
  // ratio of slopes = refU/(1-refU), e.g. 0.6/0.4 = 3/2 or (5/9)/(1-(5/9)) = 1.25
  //
  refS = runs(0, idNum); // by construction, row-0 is the zero-tax reference case
  minS = refS;
  maxS = refS;
  const unsigned int nr = runs.numR();
  for (unsigned int i = 1; i < nr; i++) {
    const double si = runs(i, idNum);
    minS = (si < minS) ? si : minS;
    maxS = (si > maxS) ? si : maxS;
  }

  if ((0 >= minS) || (minS >= maxS)) {
    throw KException("EconoptimActor::setShareUtilScale: minS must be in the range [0,maxS]");
  }
  const double sf = 1.001;
  minS = minS / sf;
  maxS = maxS*sf;
  if ((minS >= refS) || (refS >= maxS)) {
    throw KException("EconoptimActor::setShareUtilScale: refS must be in the range [minS,maxS]");
  }

  double slopeRatio = 2.0;//1.5;
  // we want to have the slope steeper on the loss side than on the gain side,
  // sr > 1, compared to the reference share from zero-tax.
  // (refU - 0)/(refS-minS) = sr * (1 - refU)/(maxS - minS)
  // so ...
  refU = (slopeRatio*(refS - minS)) / ((maxS - refS) + (slopeRatio*(refS - minS)));
  return;
}


EconoptimState::EconoptimState(EconoptimModel * em) : State(em) {
  eMod = ((const EconoptimModel *)em); // avoids many type conversions later
}

EconoptimState::~EconoptimState() {
}


// TODO: see SMPState::pDist or RPState::pDist for a model which must be adapted to use vector-capabilities
tuple <KMatrix, VUI> EconoptimState::pDist(int persp) const {
  KMatrix pd;
  auto un = VUI();
  throw KException("EconoptimActor::setShareUtilScale: dummy function");
  return tuple <KMatrix, VUI>(pd, un);
}


bool EconoptimState::equivNdx(unsigned int i, unsigned int j) const {
  /// Compare two actual positions in the current state
  auto vpi = ((const VctrPstn *)(pstns[i]));
  auto vpj = ((const VctrPstn *)(pstns[j]));
  if (vpi == nullptr) {
    throw KException("EconoptimState::equivNdx: vpi is null pointer");
  }
  if (vpj == nullptr) {
    throw KException("EconoptimState::equivNdx: vpj is null pointer");
  }
  double diff = norm((*vpi) - (*vpj));
  auto lm = ((const EconoptimModel *)model);
  bool rslt = (diff < lm->posTol);
  return rslt;
}


EconoptimState* EconoptimState::stepSUSN() {
  setAUtil(-1, ReportingLevel::Medium);
  auto s2 = doSUSN(ReportingLevel::Silent);
  s2->step = [s2]() {
    return s2->stepSUSN();
  };
  return s2;
}


// i's estimate of the GDP from j's policy.
// This computes more than it needs to, so it is inefficient, but
// the time-cost is negligable compared to the rest of the processing.
double EconoptimState::estGDP(unsigned int i, unsigned int j) {
  auto lMod = (EconoptimModel*)model;
  if (i >= lMod->L + lMod->N) {
    throw KException("EconoptimState::estGDP: i out of range");
  }
  if (j >= lMod->L + lMod->N) {
    throw KException("EconoptimState::estGDP: j out of range");
  }

  auto delta = [](double x, double y) {
    const double d = 1E-10;
    return (2.0*fabs(x - y)) / (d + fabs(x) + fabs(y));
  };

  auto vpj = ((const VctrPstn *)(pstns[j]));
  KMatrix pj = KMatrix(*vpj);
  auto vs = lMod->vaShares(pj, false);


  double factorVA = 0.0;
  for (unsigned int k = 0; k < lMod->L; k++) { // sum VA over factors
    factorVA = factorVA + vs(0, k);
  }

  double sectorVA = 0.0;
  for (unsigned int k = 0; k < lMod->N; k++) { // sum VA over sectors
    sectorVA = sectorVA + vs(0, lMod->L + k);
  }


  double gdpij = 0.0;
  if (i < lMod->L) {
    gdpij = factorVA;
  }
  else {
    gdpij = sectorVA;
  }

  return gdpij;
}

EconoptimState* EconoptimState::doSUSN(ReportingLevel rl) const {
  using std::get;
  EconoptimState* s2 = nullptr;
  // TODO: filter out essentially-duplicate positions

  auto assertSimilar = [](const KMatrix & x, const KMatrix & y) {
    if (KBase::maxAbs(x - y) >= 1E-10) {
      throw KException("EconoptimState::doSUSN: x and y are not at same values");
    }
    return;
  };


  const auto u = aUtil[0]; // all have same beliefs in this demo


  const unsigned int numA = model->numAct;
  if (numA != eMod->actrs.size()) {
    throw KException("EconoptimState::doSUSN: inaccurate number of actors");
  }
  auto vpm = VPModel::Linear;
  const unsigned int numP = pstns.size();
  if (0 >= numP) {
    throw KException("EconoptimState::doSUSN: numP must be positive");
  }
  auto euMat = [rl, numA, numP, vpm, this](const KMatrix & uMat) {

    // again, I could do a complex vote, but I'll do the easy one.
    // BTW, be sure to lambda-bind uh *after* it is modified.
    auto vkij = [this, uMat](unsigned int k, unsigned int i, unsigned int j) {  // vote_k ( i : j )
      auto ak = (EconoptimActor*)(eMod->actrs[k]);
      auto ck = KBase::sum(ak->vCap);
      auto v_kij = Model::vote(ak->vr, ck, uMat(k, i), uMat(k, j));
      return v_kij;
    }; // end of vkij

    const auto c = Model::coalitions(vkij, numA, numP);
    const auto pv2 = Model::probCE2(model->pcem, model->vpm, c);
    const auto p = get<0>(pv2);
    const auto pv = get<1>(pv2);
    const auto eu = uMat*p;

    if (ReportingLevel::Low < rl) {
      LOG(INFO) << "Assessing EU from util matrix:";
      uMat.mPrintf(" %.6f ");

      LOG(INFO) << "Coalition strength matrix:";
      c.mPrintf(" %12.6f ");

      LOG(INFO) << "Probability Opt_i > Opt_j:";
      pv.mPrintf(" %.6f ");

      LOG(INFO) << "Probability Opt_i:";
      p.mPrintf(" %.6f ");

      LOG(INFO) << "Expected utility to actors:";
      eu.mPrintf(" %.6f ");
    }

    return eu;
  }; // end of euMat

  if (ReportingLevel::Low < rl) {
    LOG(INFO) << "Assessing utility of actual state to all actors";
    for (unsigned int h = 0; h < numA; h++) {
      auto aPos = ((VctrPstn*)(pstns[h]));
      LOG(INFO) << "Actual vector-position (possibly non-neutral) of actor" << h << ":";
      trans(*aPos).mPrintf(" %+.6f ");
    }
  }
  const auto eu0 = euMat(u);

  // end of setup?

  auto assessEU = [rl, this, u, assertSimilar, euMat](unsigned int h, const KMatrix & hPos) {
    // build the hypothetical utility matrix by modifying the h-column
    // of h's matrix (his expectation of the util to everyone else of changing his own position).
    const auto uh0 = aUtil[h];
    assertSimilar(u, uh0);  // all have same beliefs in this demo
    auto uh = uh0;
    bool normP = false;
    const double ifd = eMod->infsDegree(hPos);
    if (ifd >= TolIFD) {
      throw KException("EconoptimState::doSUSN: ifd must be less than TolIFD");
    }
    auto fTax = eMod->makeFTax(hPos);
    auto shrs = eMod->vaShares(fTax, normP);  // row-vector (was using hPos, not fTax)
    for (unsigned int i = 0; i < eMod->numAct; i++) {
      auto ai = (EconoptimActor*)(eMod->actrs[i]);
      auto ui = ai->shareToUtil(shrs(0, i));
      uh(i, h) = ui; // utility to actor i of this hypothetical position by h
    }


    if (ReportingLevel::Low < rl) {
      LOG(INFO) << "Assessing utility to" << h << "of hypo-pos:";
      trans(hPos).mPrintf(" %+.6f ");

      LOG(INFO) << "Hypo-util minus base util:";
      (uh - uh0).mPrintf(" %+.4E ");
    }

    const auto eu = euMat(uh);
    return eu(h, 0);
  }; // end of assessEU


  s2 = new EconoptimState((EconoptimModel *)model);
  if (model->numAct != s2->pstns.size()) {
    throw KException("EconoptimState::doSUSN: Number of positions should be equal to number of actors");
  }
  for (unsigned int h = 0; h < numA; h++) {
    auto vhc = new KBase::VHCSearch();
    vhc->eval = [this, h, assessEU](const KMatrix & m1) {
      auto m2 = eMod->makeFTax(m1); // make it feasible
      return assessEU(h, m2);
    };
    vhc->nghbrs = KBase::VHCSearch::vn2;

    auto aPos = ((VctrPstn*)(pstns[h]));
    // JAH 20160811 changed to display actor names and not just id
    LOG(INFO) <<"Search for best next-position of actor " << eMod->actrs[h]->name;
    //printf("Search for best next-position of actor %2i starting from ", h);
    //trans(*aPos).printf(" %+.6f ");
    auto rslt = vhc->run(*aPos,                       // p0
                         1000, 10, 1E-4,              // iterMax, stableMax, sTol
                         0.01, 0.618, 1.25, 1e-6,     // step0, shrink, stretch, minStep
                         ReportingLevel::Silent);
    // note that typical improvements in utility in the first round are on the order of 1E-1 or 1E-2.
    // Therefore, any improvement of less than 1/100th of that (below sTol = 1E-4) is considered "stable"

    double vBest = get<0>(rslt);
    KMatrix pBest = get<1>(rslt);
    unsigned int in = get<2>(rslt);
    unsigned int sn = get<3>(rslt);

    //cout << "pBest :    ";
    //trans(pBest).printf(" %+.6f ");


    delete vhc;
    vhc = nullptr;
    LOG(INFO) << "Iter:" << in << "Stable:" << sn;
    LOG(INFO) << KBase::getFormattedString("Best value for %2u: %+.6f", h, vBest);
    LOG(INFO) << "Best point:";
    trans(pBest).mPrintf(" %+.6f ");
    KMatrix rBest = eMod->makeFTax(pBest);
    LOG(INFO) << "Best rates for" << h << ":";
    trans(rBest).mPrintf(" %+.6f ");

    VctrPstn * posBest = new VctrPstn(rBest);
    //s2->pstns.push_back(posBest);
    if (nullptr != s2->pstns[h]) { // make sure it was empty
      throw KException("EconoptimState::doSUSN: position pointer is null");
    }
    s2->pstns[h] = posBest;

    const double du = vBest - eu0(h, 0);
    LOG(INFO) << KBase::getFormattedString("EU improvement for %2u of %+.4E", h, du);
    //printf("  vBest = %+.6f \n", vBest);
    //printf("  eu0(%i, 0) for %i = %+.6f \n", h, h, eu0(h,0));
    //cout << endl << flush;
    // Logically, du should always be non-negative, as VHC never returns a worse value than the starting point.
    //const double eps = 0; // 1E-5 ;
    if (0 > du) {
      throw KException("EconoptimState::doSUSN: du must be non-negative");
    }
  }

  auto p0 = ((VctrPstn*)(pstns[0]));
  KMatrix meanP = KMatrix(p0->numR(), p0->numC());
  for (unsigned int i = 0; i < numA; i++) {
    auto iPos = ((VctrPstn*)(pstns[i]));
    auto y = *iPos;
    meanP = meanP + y;
  }
  meanP = meanP / numA;

  auto acFn = [this](unsigned int i, unsigned int j) {
    const double nTol = 1E-8;
    const auto iPos = ((VctrPstn*)(pstns[i]));
    const auto y = *iPos;
    const auto jPos = ((VctrPstn*)(pstns[j]));
    const auto x = *jPos;
    double acij = lCorr(y, x);
    // avoid NAN by assigning zero in those cases
    if ((KBase::norm(x) < nTol) || (KBase::norm(y) < nTol)) {
      acij = 0.0;
    }
    return acij;
  };

  auto rcFn = [this, meanP](unsigned int i, unsigned int j) {
    const auto iPos = ((VctrPstn*)(pstns[i]));
    const auto y = *iPos;
    const auto jPos = ((VctrPstn*)(pstns[j]));
    const auto x = *jPos;
    double acij = lCorr(y - meanP, x - meanP);
    return acij;
  };

  KMatrix acMat = KMatrix::map(acFn, numA, numA);
  LOG(INFO) << "Absolute correlation of policies";
  acMat.mPrintf(" %+0.4f ");

  LOG(INFO) << "Mean policy";
  trans(meanP).mPrintf(" %+0.4f ");

  LOG(INFO) << "Euclidean distance to mean policy:";
  for (unsigned int i = 0; i < numA; i++) {
    const auto iPos = ((VctrPstn*)(pstns[i]));
    const auto y = *iPos;
    LOG(INFO) << KBase::getFormattedString("  %2u  %0.4f", i, KBase::norm(y - meanP));
  }

  KMatrix rcMat = KMatrix::map(rcFn, numA, numA);
  LOG(INFO) << "Correlation of policies relative to mean policy";
  rcMat.mPrintf(" %+0.4f ");



  if (nullptr == s2) {
    throw KException("EconoptimState::doSUSN: s2 is null pointer");
  }
  if (numP != s2->pstns.size()) {
    throw KException("EconoptimState::doSUSN: inaccurate number of positions");
  }
  if (numA != s2->model->numAct) {
    throw KException("EconoptimState::doSUSN: inaccurate numer of actors");
  }
  return s2;
}
// end of doSUSN

void EconoptimState::setAllAUtil(ReportingLevel rl) {
  unsigned int numA = model->numAct;
  auto eMod0 = (EconoptimModel*)model;
  auto uFn1 = [eMod0, this](unsigned int i, unsigned int j) {
    auto ai = ((EconoptimActor*)(eMod0->actrs[i]));
    double uij = ai->posUtil(pstns[j]);
    return uij;
  };
  auto u = KMatrix::map(uFn1, numA, numA);
  if (KBase::ReportingLevel::Low < rl) {
    LOG(INFO) << "Raw actor-pos util matrix";
    u.mPrintf(" %.4f ");
  }

  // for the purposes of this demo, I consider each actor to know exactly what the others value.
  // They usually disagree on the likely consequences of a policy, as factors and sectors
  // use different economic models to estimate the consequences of a policy.
  // They usually value the consequences differently, as each factor and sector values only the GDP-share
  // they expect to get.
  // But they know what consequences the others expect, and how they will value those consequences,
  // even if they disagree on both facts and values.
  LOG(INFO) << "aUtil size:" << aUtil.size();

  if (0 != aUtil.size()) {
    throw KException("EconoptimState::setAllAUtil: aUtil should have zero member values");
  }
  for (unsigned int i = 0; i < numA; i++) {
    aUtil.push_back(u);
  }
  return;
}

// -------------------------------------------------

EconoptimModel::EconoptimModel(string d, uint64_t s, vector<bool> f) : Model(d, s, f) {
  // some arbitrary yet plausible default values
  maxSub = 0.50;
  maxTax = 1.00;
  posTol = 0.00001;
}

EconoptimModel::~EconoptimModel() {
  // nothing
}


double EconoptimModel::stateDist(const EconoptimState* s1, const EconoptimState* s2) {
  unsigned int n = s1->pstns.size();
  if(n != s2->pstns.size()) {
    throw KException("EconoptimModel::stateDist: inaccurate size of position in s2");
  }
  double dSum = 0;
  for (unsigned int i = 0; i < n; i++) {
    auto vp1i = ((const VctrPstn*)(s1->pstns[i]));
    auto vp2i = ((const VctrPstn*)(s2->pstns[i]));
    dSum = dSum + KBase::norm((*vp1i) - (*vp2i));
  }
  return dSum;
}




// Go back through the state history and print every actor's estimate of GDP from every other actor's policy-position
void EconoptimModel::printEstGDP() {
  for (unsigned int t = 0; t < history.size(); t++) {
    auto ls = (EconoptimState*)(history[t]);
    auto fn = [this, ls](unsigned int i, unsigned int j) {
      return ls->estGDP(i, j);
    };
    auto gdp = KMatrix::map(fn, numAct, numAct);
    LOG(INFO) << "For turn" << t << ", estimated GDP by actor (row) of positions (policy):";
    gdp.mPrintf("%8.2f ");
  }

  return;
}

// L factors, M consumption groups, N sectors
tuple<KMatrix, KMatrix, KMatrix, KMatrix> EconoptimModel::makeBaseYear(unsigned int numF, unsigned int numCG, unsigned int numS, PRNG* rng) {

  using KBase::inv;
  using KBase::iMat;
  using KBase::norm;

  L = numF;
  M = numCG;
  N = numS;


  LOG(INFO) << "Build random but consistent base year data for I/O model.";
  LOG(INFO) << "We follow the standard I/O layout";
  LOG(INFO) << "For 2 factors, 1 cons. group, and 4 sectors, it would be as follows:";
  LOG(INFO) << " T T T T C X";
  LOG(INFO) << " T T T T C X";
  LOG(INFO) << " T T T T C X";
  LOG(INFO) << " T T T T C X";
  LOG(INFO) << " V V V V";
  LOG(INFO) << " V V V V";
  LOG(INFO) << "    "; // force blank line
  LOG(INFO) << "Synthetic data has"
    << L << "factors," << M << "consumption groups," << N << "industrial sectors"
    << "and one export sector (with constant elasticity demand)";

  if(nullptr == rng ) {
    throw KException("EconoptimModel::printEstGDP: rng is null pointer");
  }
  if(L <= 0) {
    throw KException("EconoptimModel::printEstGDP: L must be positive");
  }
  if(M <= 0) {
    throw KException("EconoptimModel::printEstGDP: M must be positive");
  }
  if(N <= 0) {
    throw KException("EconoptimModel::printEstGDP: N must be positive");
  }

  // These data are simple enough that they are clearly
  // correct-by-construction, so I do not check them
  // here. Both data and model get checked in makeIOModel.

  // Build a random transaction matrix.
  // Ensure that the value-added in each column is 1/2 to 2/3
  // of the total value in that column.
  auto trns = KMatrix::uniform(rng, N, N, 100.0, 999.0);
  auto rev = KMatrix::uniform(rng, L, N, 0.1, 1.0);
  for (unsigned int n = 0; n < N; n++) { // n-th column
    double cSum = 0.0;
    for (unsigned int j = 0; j < N; j++) { // j-th row of transactions
      cSum = cSum + trns(j, n);
    }
    double rnSum = 0.0;
    for (unsigned int l = 0; l < L; l++) { // el-th row of value-added
      rnSum = rnSum + rev(l, n);
    }
    double rnTrgt = cSum*rng->uniform(1.0, 2.0);
    for (unsigned int l = 0; l < L; l++) { // el-th row of value-added
      double rln = (rev(l, n) * rnTrgt) / rnSum;
      rev(l, n) = rln;
    }
  }

  // now we have to allocate all that value-added to each row of consumption and export.
  // get full column-sums, so we can see what slack exists on each row.
  auto sumClmT = vector<double>(N);
  auto sumClmR = vector<double>(N);
  for (unsigned int n = 0; n < N; n++) {
    double cs = 0.0;
    for (unsigned int i = 0; i < N; i++) {
      cs = cs + trns(i, n);
    }
    sumClmT[n] = cs;
    cs = 0.0;
    for (unsigned int l = 0; l < L; l++) {
      cs = cs + rev(l, n);
    }
    sumClmR[n] = cs;
  }
  auto sumRowT = vector<double>(N);
  for (unsigned int i = 0; i < N; i++) {
    double rs = 0.0;
    for (unsigned int j = 0; j < N; j++) {
      rs = rs + trns(i, j);
    }
    sumRowT[i] = rs;
  }

  // now, (sumClmT[i] + sumClmR[i]) - sumRowT[i] is amount to be allocated,
  // so it had better be positive. As value-added is 1/2 to 2/3 of the total
  // in every column, we'd expect this usually to be satisfied, but there
  // is always the chance that some unlucky row will come up short. Thus, we require
  // (sumClmT[i] + sumClmR[i]) - sumRowT[i] > 0.10 * sumRowT[i] > 0, and we will
  // achieve that by raising value added (i.e. sumClmR[i]) if necessary until
  // sumClmR[i] > 1.1*sumRowT[i] - sumClmT[i]
  auto f = vector<double>(N);
  for (unsigned int i = 0; i < N; i++) {
    f[i] = 1.0;
  }
  for (unsigned int i = 0; i < N; i++) {
    while (f[i] * sumClmR[i] < 1.1*sumRowT[i] - sumClmT[i]) {
      f[i] = 1.15 * f[i];
      LOG(INFO) << KBase::getFormattedString("Raised f[%u] to %.3f", i, f[i]);
    }
    for (unsigned int l = 0; l < L; l++) {
      rev(l, i) = f[i] * rev(l, i);
    }
    sumClmR[i] = f[i] * sumClmR[i];
  }

  LOG(INFO) << "Transactions:";
  trns.mPrintf(" %7.1f ");

  LOG(INFO) << "Value-added revenue:";
  rev.mPrintf(" %7.1f ");

  auto xprt = KMatrix::uniform(rng, N, 1, 1.0, 100.0);
  auto cons = KMatrix(N, M);

  for (unsigned int i = 0; i < N; i++) {
    double rDef = sumClmT[i] + sumClmR[i] - sumRowT[i];
    double v = xprt(i, 0);
    double rSum = v;
    for (unsigned int m = 0; m < M; m++) {
      v = rng->uniform(1, 100);
      cons(i, m) = v;
      rSum = rSum + v;
    }
    xprt(i, 0) = xprt(i, 0)*(rDef / rSum);
    for (unsigned m = 0; m < M; m++) {
      cons(i, m) = cons(i, m)*(rDef / rSum);
    }
  }

  LOG(INFO) << "Cons:";
  cons.mPrintf(" %7.1f ");

  LOG(INFO) << "Exports:";
  xprt.mPrintf(" %7.1f ");

  auto rslt = tuple<KMatrix, KMatrix, KMatrix, KMatrix>(trns, rev, xprt, cons);
  return rslt;
}

// L factors, M consumption groups, N sectors
void EconoptimModel::makeIOModel(const KMatrix & trns, const KMatrix & rev, const KMatrix & xprt, const KMatrix & cons, PRNG* rng) {

  using KBase::inv;
  using KBase::iMat;
  using KBase::norm;

  LOG(INFO) << " "; // force blank line
  LOG(INFO) << "Build I/O model from base-year data";

  x0 = xprt;

  if(N != trns.numR()) {
    throw KException("EconoptimModel::makeIOModel: trns must have N number of rows");
  }

  if(N != trns.numC()) {
    throw KException("EconoptimModel::makeIOModel: trns must have N number of columns");
  }

  if(L != rev.numR()) {
    throw KException("EconoptimModel::makeIOModel: rev must have L number of rows");
  }
  if(N != rev.numC()) {
    throw KException("EconoptimModel::makeIOModel: rev must have N number of columns");
  }
  if(M != cons.numC()) {
    throw KException("EconoptimModel::makeIOModel: cons must have M number of columns");
  }
  if(N != cons.numR()) {
    throw KException("EconoptimModel::makeIOModel: cons must have N number of rows");
  }

  if(N != xprt.numR()) {
    throw KException("EconoptimModel::makeIOModel: xprt must have N number of rows");
  }
  if(1 != xprt.numC()) {
    throw KException("EconoptimModel::makeIOModel: xprt must be a column vector");
  }
  if(L <= 0) {
    throw KException("EconoptimModel::makeIOModel: L must be positive");
  }
  if(M <= 0) {
    throw KException("EconoptimModel::makeIOModel: M must be positive");
  }
  if(N <= 0) {
    throw KException("EconoptimModel::makeIOModel: N must be positive");
  }

  auto delta = [](double x, double y) {
    return (2 * fabs(x - y)) / (fabs(x) + fabs(y));
  };

  auto mDelta = [](const KMatrix & x, const KMatrix & y) {
    return (2 * norm(x - y)) / (norm(x) + norm(y));
  };


  // we assume each consumption group is driven by a Cobb-Douglas utility function,
  // which is easily inferred for each column. The budget constraints just add up
  // to the total value added, so we construct a random matrix to do that.
  double sxc = sum(xprt);
  auto zeta0 = KMatrix(N, M); // the CD coefficients
  auto sumConsClm = KMatrix(M, 1);
  double sCons = 0.0;
  for (unsigned int m = 0; m < M; m++) {
    double scc = 0.0;
    for (unsigned int i = 0; i < N; i++) {
      scc = scc + cons(i, m);
    }
    for (unsigned int i = 0; i < N; i++) {
      zeta0(i, m) = cons(i, m) / scc;
    }
    sumConsClm(m, 0) = scc;
    sCons = sCons + scc;
  }
  double sVA = 0.0;
  auto sumVARows = KMatrix(L, 1);
  for (unsigned int l = 0; l < L; l++) {
    double svr = 0.0;
    for (unsigned int j = 0; j < N; j++) {
      svr = svr + rev(l, j);
    }
    sumVARows(l, 0) = svr;
    sVA = sVA + svr;
  }


  auto sumVAClms = KMatrix(1, N);
  for (unsigned int j = 0; j < N; j++) {
    double svc = 0.0;
    for (unsigned int l = 0; l < L; l++) {
      svc = svc + rev(l, j);
    }
    sumVAClms(0, j) = svc;
  }

  LOG(INFO) << " check (export + cons = value added)";
  if (fabs(sxc + sCons - sVA) >= 0.001) {
    throw KException("EconoptimModel::makeIOModel: (sxc + sCons) and sVA have mismatched values");
  }
  LOG(INFO) << "ok";

  // Now, we have budgets of each consumption group as an Mx1 vector, B_i
  // and the revenue of each VA group an Lx1 vector, R_j, and we
  // need MxL expenditure matrix, E_ij, so that B_i = sum_j [ E_ij * R_j ]
  //
  // Note that because VA comes from exports as well, the total consumer
  // budget will add up to less than revenue.
  //
  // We can set random e_ij~U, and scale to get E_ij:
  // B_i = sum_j [ ( f_i * e_ij ) * R_j ]
  // = f_i * ( sum_j [ e_ij * R_j ] )
  // so
  // f_i = B_i / ( sum_j [ e_ij * R_j ] )
  auto eij = KMatrix::uniform(rng, M, L, 10.0, 100.0);

  auto expnd = KMatrix(M, L);
  for (unsigned int i = 0; i < M; i++) {
    double si = 0.0;
    for (unsigned int j = 0; j < L; j++) {
      si = si + eij(i, j)*sumVARows(j, 0);
    }
    double fi = sumConsClm(i, 0) / si;
    for (unsigned int j = 0; j < L; j++) {
      expnd(i, j) = fi * eij(i, j);
    }
  }

  LOG(INFO) << "sumVARows";
  sumVARows.mPrintf(" %.1f ");

  LOG(INFO) << "sumConsClm";
  sumConsClm.mPrintf(" %.1f ");

  LOG(INFO) << "expenditure matrix";
  expnd.mPrintf(" %.2f ");

  LOG(INFO) << "check (sumConsClm = expnd * sumVARows) ... ";
  if (mDelta(sumConsClm, expnd*sumVARows) >= 1e-6) {
    throw KException("EconoptimModel::makeIOModel: sumConsClm and expnd*sumVARows have mismatched values");
  }

  LOG(INFO) << "ok";

  double dpr = rng->uniform(0.08, 0.12);
  LOG(INFO) << KBase::getFormattedString("Depreciation: %.3f", dpr);
  double grw = rng->uniform(0.02, 0.05);
  LOG(INFO) << KBase::getFormattedString("Growth: %.3f", grw);

  eps = KMatrix::uniform(rng, N, 1, 2.0, 3.0);
  LOG(INFO) << "export elasticities";
  eps.mPrintf(" %.2f ");

  auto capReq = KMatrix(N, N);
  for (unsigned int i = 0; i < N; i++) {
    for (unsigned int j = 0; j < N; j++) {
      double cr = rng->uniform(5.0, 10.0);
      cr = (cr*cr) / 5000.0; // usually tiny, sometimes quite significant
      capReq(i, j) = cr;
    }
  }

  LOG(INFO) << "Capital Requirements, B";
  capReq.mPrintf(" %.4f ");

  // ------------------------------------------------------------
  // start checking that the expected identities hold,
  // i.e. that I got everything correct.
  // column matrix of row-sums
  auto qClm = KMatrix(N, 1);
  for (unsigned int i = 0; i < N; i++) {
    double qi = xprt(i, 0);
    for (unsigned int j = 0; j < N; j++) {
      double tij = trns(i, j);
      qi = qi + tij;
    }
    for (unsigned int k = 0; k < M; k++) {
      double cik = cons(i, k);
      qi = qi + cik;
    }
    qClm(i, 0) = qi;
  }

  LOG(INFO) << "Column vector of total outputs (export+trans+cons)";
  qClm.mPrintf(" %.1f ");

  // row matrix of column-sums
  auto qRow = KMatrix(1, N);
  for (unsigned int j = 0; j < N; j++) {
    double qj = 0;
    for (unsigned int i = 0; i < N; i++) {
      double tij = trns(i, j);
      qj = qj + tij;
    }
    for (unsigned int h = 0; h < L; h++) {
      double rhj = rev(h, j);
      qj = qj + rhj;
    }
    qRow(0, j) = qj;
  }


  LOG(INFO) << "check row-sums == clm-sums ... ";
  double tol = 0.001; // tolerance in matching row and column sums
  for (unsigned int n = 0; n < N; n++) {
    if (delta(qClm(n, 0), qRow(0, n)) >= tol) {
      throw KException("EconoptimModel::makeIOModel: qClm and qRow have mismatched values");
    }
  }
  LOG(INFO) << "ok";

  auto A = KMatrix(N, N);
  for (unsigned int j = 0; j < N; j++) {
    double qj = qRow(0, j);
    for (unsigned int i = 0; i < N; i++) {
      double aij = trns(i, j) / qj;
      A(i, j) = aij;
    }
  }
  LOG(INFO) << "A matrix:";
  A.mPrintf(" %.4f  ");


  rho = KMatrix(L, N);
  for (unsigned int j = 0; j < N; j++) {
    double qj = qRow(0, j);
    for (unsigned int h = 0; h < L; h++) {
      double rhj = rev(h, j) / qj;
      rho(h, j) = rhj;
    }
  }

  vas = KMatrix(1, N);
  for (unsigned int j = 0; j < N; j++) {
    double vj = 0.0;
    for (unsigned int h = 0; h < L; h++) {
      vj = vj + rho(h, j);
    }
    vas(0, j) = vj;
  }

  // ------------------------------------------
  LOG(INFO) << "Shares of GDP to VA factors (labor groups)";
  LOG(INFO) << " check budgetL == rho x qClm:";
  auto budgetL = rho * qClm;
  budgetL.mPrintf(" %.2f "); //  these are the VA to factors
  if (mDelta(sumVARows, budgetL) >= tol) {
    throw KException("EconoptimModel::makeIOModel: sumVARows and budgetL have mismatched values");
  }
  LOG(INFO) << "ok";

  auto budgetS = KMatrix(1, N);
  for (unsigned int j = 0; j < N; j++) {
    double vs = qClm(j, 0) * vas(0, j);
    budgetS(0, j) = vs;
  }
  LOG(INFO) << "Shares of GDP to industry sectors (using Alpha, not Beta)";
  LOG(INFO) << "budgetS:";
  budgetS.mPrintf(" %.2f ");
  if (mDelta(sumVAClms, budgetS) >= tol) {
    throw KException("EconoptimModel::makeIOModel: sumVAClms and budgetS have mismatched values");
  }
  LOG(INFO) << "ok";

  LOG(INFO) << "GDP:";
  (vas*qClm).mPrintf(" %.2f ");


  auto budgetC = KMatrix(M, 1); // column-vector budget of each consumption category
  for (unsigned int k = 0; k < M; k++) {
    double bk = 0;
    for (unsigned int i = 0; i < N; i++) {
      double cik = cons(i, k);
      bk = bk + cik;
    }
    budgetC(k, 0) = bk;
  }
  LOG(INFO) << "budgetC";
  budgetC.mPrintf(" %.4f ");
  if (mDelta(budgetC, sumConsClm) >= tol) {
    throw KException("EconoptimModel::makeIOModel: budgetC and sumConsClm have mismatched values");
  }
  LOG(INFO) << "ok";

  // because the initial prices are all 1, zeta_ik = theta_ik/P_i is
  // just theta_ik, which is C_ik/BC_k
  auto zeta = KMatrix(N, M);
  for (unsigned int k = 0; k < M; k++) {
    double bck = budgetC(k, 0);
    for (unsigned int i = 0; i < N; i++) {
      zeta(i, k) = cons(i, k) / bck;
    }
  }
  LOG(INFO) << "zeta";
  zeta.mPrintf(" %.4f "); // OK

  LOG(INFO) << "cons";
  (zeta * budgetC).mPrintf("%.4f  "); // OK

  LOG(INFO) << "expnd";
  expnd.mPrintf(" %.4f ");

  LOG(INFO) << "budgetL";
  budgetL.mPrintf(" %.4f ");

  LOG(INFO) << "check budgetC == expnd x budgetL";
  (expnd*budgetL).mPrintf("%.4f  ");
  if (mDelta(budgetC, expnd*budgetL) >= tol) {
    throw KException("EconoptimModel::makeIOModel: budgetC, expnd and budgetL have mismatched values");
  }
  LOG(INFO) << "ok";

  auto alpha = A + (zeta * expnd * rho);
  LOG(INFO) << "alpha ";
  alpha.mPrintf(" %.4f ");
  for (auto a : alpha) {
    if (0.0 >= a) {
      throw KException("EconoptimModel::makeIOModel: a must be positive");
    }
  }

  auto id = iMat(N);
  aL = inv(id - alpha);

  LOG(INFO) << "check aL * X == qClm";
  (aL*xprt).mPrintf(" %.4f ");
  if (mDelta(aL*xprt, qClm) >= tol) {
    throw KException("EconoptimModel::makeIOModel: aL, xprt and qClm have mismatched values");
  }
  for (auto x : aL) {
    if (0.0 >= x) {
      throw KException("EconoptimModel::makeIOModel: x must be positive");
    }
  }
  LOG(INFO) << "ok";

  auto beta = alpha + (dpr + grw)*capReq;
  LOG(INFO) << "beta:";
  beta.mPrintf(" %.4f ");

  bL = inv(id - beta);
  auto betaQX = bL*xprt;
  LOG(INFO) << "check bL * X == betaQX";
  betaQX.mPrintf(" %.4f ");
  for (auto x : bL) {
    if (0.0 >= x) {
      throw KException("EconoptimModel::makeIOModel: x must be positive");
    }
  }
  LOG(INFO) << "ok";

  auto budgetBS = KMatrix(1, N);
  for (unsigned int j = 0; j < N; j++) {
    double vbs = betaQX(j, 0) * vas(0, j);
    budgetBS(0, j) = vbs;;
  }
  LOG(INFO) << "Shares of GDP to industry sectors (using Beta, not Alpha)";
  LOG(INFO) << "budgetBS:";
  budgetBS.mPrintf(" %.2f ");

  // A more numerically stable expression for aL: I + a + a^2 + a^3 + ...
  // To give everything time to percolate through the entire economy,
  // up to a power of 5N seems to be enough usually to get
  // delta(pow,inv) below 0.005 on these matrices, but sometimes
  // 11N is necessary. I won't bother with it here.
  //
  // Note that if S(n) = I + a + a^2 + ... + a^n then
  // (I-a)^(-1) = S(n) + O(a^(n+1)) and we can
  // estimate the size of the remaining errors from
  // mean(a^(n+1)) compared to mean(S(n)).

  return;
}

// prepModel takes in a set of "real" data and sets up the IO model, and performs
// several checks to verify all the data is internally consistent; this replicates
// some of the functionality in makeBaseYear & makeIOModel
// numFac = L, numCon = M, numSec = N
void EconoptimModel::prepModel(unsigned int numFac, unsigned int numCon, unsigned int numSec,
  KMatrix & xprt, KMatrix & cons, KMatrix & elast, KMatrix & trns, KMatrix & rev, KMatrix & expnd, KMatrix & Bmat)
{

  using KBase::inv;
  using KBase::iMat;
  using KBase::norm;

  // check stuff first - compiler doesn't like the asserts for nullptr inequality(?), so comment out
  if(numFac <= 0) {
    throw KException("EconoptimModel::prepModel: numFac must be positive");
  }
  if(numCon <= 0) {
    throw KException("EconoptimModel::prepModel: numCon must be positive");
  }
  if(numSec <= 0) {
    throw KException("EconoptimModel::prepModel: numSec must be positive");
  }
  if(xprt.numR() != numSec ) {
    throw KException("EconoptimModel::prepModel: inaccurate number of rows in xprt");
  }
  if(xprt.numC() != 1) {
    throw KException("EconoptimModel::prepModel: inaccurate number of columns in xprt");
  }
  if(cons.numR() != numSec) {
    throw KException("EconoptimModel::prepModel: inaccurate number of rows in cons");
  }

  if(cons.numC() != numCon) {
    throw KException("EconoptimModel::prepModel: inaccurate number of columns in cons");
  }
  if(elast.numR() != numSec) {
    throw KException("EconoptimModel::prepModel: inaccurate number of rows in elast");
  }
  if(elast.numC() != 1) {
    throw KException("EconoptimModel::prepModel: elast must be a column vector");
  }
  if(trns.numR() != numSec) {
    throw KException("EconoptimModel::prepModel: inaccurate number of rows in trns");
  }

  if(trns.numC() != numSec) {
    throw KException("EconoptimModel::prepModel: inaccurate number of columns in trns");
  }
  if(rev.numR() != numFac) {
    throw KException("EconoptimModel::prepModel: inaccurate number of rows in rev");
  }
  if(rev.numC() != numSec) {
    throw KException("EconoptimModel::prepModel: inaccurate number of columns in rev");
  }

  if(expnd.numR() != numCon) {
    throw KException("EconoptimModel::prepModel: inaccurate number of rows in expnd");
  }
  if(expnd.numC() != numFac) {
    throw KException("EconoptimModel::prepModel: inaccurate number of columns in expnd");
  }
  if(Bmat.numR() != numSec) {
    throw KException("EconoptimModel::prepModel: inaccurate number of rows in Bmat");
  }
  if(Bmat.numC() != numSec) {
    throw KException("EconoptimModel::prepModel: inaccurate number of columns in Bmat");
  }
  auto delta = [](double x, double y) {
    return (2 * fabs(x - y)) / (fabs(x) + fabs(y));
  };

  auto mDelta = [](const KMatrix & x, const KMatrix & y) {
    return (2 * norm(x - y)) / (norm(x) + norm(y));
  };

  // insert items into the model
  N = numSec;
  L = numFac;
  M = numCon;

  x0 = xprt;
  LOG(INFO) << "Base Year Export Demand:";
  x0.mPrintf(" %.4f ");

  LOG(INFO) << "Base Year Domestic Demand:";
  cons.mPrintf(" %.4f ");

  eps = elast;
  LOG(INFO) << "Export Elasticities:";
  eps.mPrintf(" %.4f ");

  LOG(INFO) << "Base Year Intra-sectoral Transactions:";
  trns.mPrintf(" %8.4f ");

  LOG(INFO) << "Factor Value-Added Revenue:";
  rev.mPrintf(" %8.4f ");

  // some calculations which we need for building and validating the IO model
  // much of this copied almost verbatim (vercodim?) from makeIOModel
  double sxc = sum(xprt);
  auto zeta0 = KMatrix(N, M); // the CD coefficients
  auto sumConsClms = KMatrix(M, 1);
  double sCons = 0.0;
  for (unsigned int m = 0; m < M; m++) {
    double scc = 0.0;
    for (unsigned int i = 0; i < N; i++) {
      scc = scc + cons(i, m);
    }
    for (unsigned int i = 0; i < N; i++) {
      zeta0(i, m) = cons(i, m) / scc;
    }
    sumConsClms(m, 0) = scc;
    sCons = sCons + scc;
  }

  double sVA = 0.0;
  auto sumVARows = KMatrix(L, 1);
  for (unsigned int l = 0; l < L; l++) {
    double svr = 0.0;
    for (unsigned int j = 0; j < N; j++) {
      svr = svr + rev(l, j);
    }
    sumVARows(l, 0) = svr;
    sVA = sVA + svr;
  }

  auto sumVAClms = KMatrix(1, N);
  for (unsigned int j = 0; j < N; j++) {
    double svc = 0.0;
    for (unsigned int l = 0; l < L; l++) {
      svc = svc + rev(l, j);
    }
    sumVAClms(0, j) = svc;
  }

  auto sumTRNSClms = KMatrix(1, N);
  for (unsigned int j = 0; j < N; j++) {
    double stc = 0.0;
    for (unsigned int l = 0; l < N; l++) {
      stc = stc + trns(l, j);
    }
    sumTRNSClms(0, j) = stc;
  }

  // now compute the rho & vas matrices
  auto qFromTransVA = sumVAClms + sumTRNSClms;
  rho = KMatrix(L,N);
  for (unsigned int l = 0; l < L; l++)
  {
    for (unsigned int j = 0; j < N; j++)
    {
        rho(l,j) = rev(l,j)/qFromTransVA(0,j);
    }
  }
  vas = KMatrix(1, N);
  for (unsigned int j = 0; j < N; j++) {
    double vj = 0.0;
    for (unsigned int h = 0; h < L; h++) {
      vj = vj + rho(h, j);
    }
    vas(0, j) = vj;
  }
  LOG(INFO) << "Factor Value-Add Shares:";
  rho.mPrintf(" %0.4f ");
  LOG(INFO) << "Total Value-Add Shares:";
  vas.mPrintf(" %0.4f ");

  LOG(INFO) << " check (export + cons = value added) ... ";
  if(fabs(sxc + sCons - sVA) >= 0.001) {
    throw KException("EconoptimModel::prepModel: diff between values not correct");
  }

  LOG(INFO) << "ok";

  LOG(INFO) << "sumVARows:";
  sumVARows.mPrintf(" %.4f ");

  LOG(INFO) << "sumConsClms:";
  sumConsClms.mPrintf(" %.4f ");

  LOG(INFO) << "Expenditure Matrix:";
  expnd.mPrintf(" %.4f ");

  LOG(INFO) << "check (sumConsClm = expnd * sumVARows) ... ";
  auto expTSumVA = expnd*sumVARows;
  expTSumVA.mPrintf(" %0.4f ");
  if(mDelta(sumConsClms, expTSumVA) >= 1e-6) {
    throw KException("EconoptimModel::prepModel: inaccurate values");
  }

  LOG(INFO) << "ok";

  // just display here the B matrix
  LOG(INFO) << "Scaled Complete Capital Requirements, B:";
  Bmat.mPrintf(" %.4f ");

  auto qClm = KMatrix(N, 1);
  for (unsigned int i = 0; i < N; i++) {
    double qi = xprt(i, 0);
    for (unsigned int j = 0; j < N; j++) {
      double tij = trns(i, j);
      qi = qi + tij;
    }
    for (unsigned int k = 0; k < M; k++) {
      double cik = cons(i, k);
      qi = qi + cik;
    }
    qClm(i, 0) = qi;
  }
  LOG(INFO) << "Column vector of total outputs (export+trans+cons):";
  qClm.mPrintf(" %.4f ");

  // row matrix of column-sums
  auto qRow = KMatrix(1, N);
  for (unsigned int j = 0; j < N; j++) {
    double qj = 0;
    for (unsigned int i = 0; i < N; i++) {
      double tij = trns(i, j);
      qj = qj + tij;
    }
    for (unsigned int h = 0; h < L; h++) {
      double rhj = rev(h, j);
      qj = qj + rhj;
    }
    qRow(0, j) = qj;
  }
  LOG(INFO) << "check row-sums == clm-sums ... ";
  double tol = 0.001; // tolerance in matching row and column sums
  for (unsigned int n = 0; n < N; n++) {
    if(delta(qClm(n, 0), qRow(0, n)) >= tol) {
      throw KException("EconoptimModel::prepModel: inaccurate row and col values");
    }
  }
  LOG(INFO) << "ok";

  auto A = KMatrix(N, N);
  for (unsigned int j = 0; j < N; j++) {
    double qj = qRow(0, j);
    for (unsigned int i = 0; i < N; i++) {
      double aij = trns(i, j) / qj;
      A(i, j) = aij;
    }
  }
  LOG(INFO) << "A matrix:";
  A.mPrintf(" %.4f ");

  // ------------------------------------------
  LOG(INFO) << "Shares of GDP to VA factors (labor groups)";
  LOG(INFO) << " check budgetL == rho x qClm:";
  auto budgetL = rho * qClm;
  budgetL.mPrintf(" %.4f "); //  these are the VA to factors
  if (mDelta(sumVARows, budgetL) >= tol) {
    throw KException("EconoptimModel::prepModel: inaccurate budgetL");
  }
  LOG(INFO) << "ok";

  auto budgetS = KMatrix(1, N);
  for (unsigned int j = 0; j < N; j++) {
    double vs = qClm(j, 0) * vas(0, j);
    budgetS(0, j) = vs;
  }
  LOG(INFO) << "Shares of GDP to industry sectors (using Alpha, not Beta)";
  LOG(INFO) << "budgetS:";
  budgetS.mPrintf(" %.4f ");
  if(mDelta(sumVAClms, budgetS) >= tol) {
    throw KException("EconoptimModel::prepModel: inaccurate budgetS");
  }
  LOG(INFO) << "ok";

  LOG(INFO) << "GDP:";
  (vas*qClm).mPrintf(" %.4f ");

  auto budgetC = KMatrix(M, 1); // column-vector budget of each consumption category
  for (unsigned int k = 0; k < M; k++) {
    double bk = 0;
    for (unsigned int i = 0; i < N; i++) {
      double cik = cons(i, k);
      bk = bk + cik;
    }
    budgetC(k, 0) = bk;
  }
  LOG(INFO) << "budgetC:";
  budgetC.mPrintf(" %.4f ");
  if(mDelta(budgetC, sumConsClms) >= tol) {
    throw KException("EconoptimModel::prepModel: inaccurate budget numbers for cons columns");
  }
  LOG(INFO) << "ok";

  // because the initial prices are all 1, zeta_ik = theta_ik/P_i is
  // just theta_ik, which is C_ik/BC_k
  auto zeta = KMatrix(N, M);
  for (unsigned int k = 0; k < M; k++) {
    double bck = budgetC(k, 0);
    for (unsigned int i = 0; i < N; i++) {
      zeta(i, k) = cons(i, k) / bck;
    }
  }
  LOG(INFO) << "zeta:";
  zeta.mPrintf(" %.4f "); // OK

  LOG(INFO) << "Domestic Demand computed from zeta*budgetC:";
  (zeta * budgetC).mPrintf(" %.4f "); // OK

  LOG(INFO) << "check budgetC == expnd x budgetL";
  (expnd*budgetL).mPrintf(" %.4f ");
  if(mDelta(budgetC, expnd*budgetL) >= tol) {
    throw KException("EconoptimModel::prepModel: inaccurate budget numbers for budgetL");
  }
  LOG(INFO) << "ok";

  // compute & validate alpha, then invert & validate I-alpha
  auto alpha = A + (zeta * expnd * rho);
  LOG(INFO) << "alpha:";
  alpha.mPrintf(" %.4f ");
  for (auto a : alpha) {
    if(0.0 >= a) {
      throw KException("EconoptimModel::prepModel: a must be positive");
    }
  }

  auto id = iMat(N);
  aL = inv(id - alpha);
  LOG(INFO) << "check aL * X == qClm";
  (aL*xprt).mPrintf(" %.4f ");
  if(mDelta(aL*xprt, qClm) >= tol) {
    throw KException("EconoptimModel::prepModel: inaccurate value");
  }
  for (auto x : aL) {
    if(0.0 >= x) {
      throw KException("EconoptimModel::prepModel: x must be positive");
    }
  }
  LOG(INFO) << "ok";

  // compute beta, then invert & validate I-beta
  auto beta = alpha + Bmat;
  LOG(INFO) << "beta:";
  beta.mPrintf(" %.4f ");

  bL = inv(id - beta);
  auto betaQX = bL*xprt;
  LOG(INFO) << "check bL * X";
  betaQX.mPrintf(" %.4f ");
  for (auto x : bL) {
    if(0.0 >= x) {
      throw KException("EconoptimModel::prepModel: x must be positive");
    }
  }
  LOG(INFO) << "ok";

  auto budgetBS = KMatrix(1, N);
  for (unsigned int j = 0; j < N; j++) {
    double vbs = betaQX(j, 0) * vas(0, j);
    budgetBS(0, j) = vbs;;
  }
  LOG(INFO) << "Shares of GDP to industry sectors (using Beta, not Alpha)";
  LOG(INFO) << "budgetBS:";
  budgetBS.mPrintf(" %.4f ");


  return;
}


// the usual CES demand function:  (q1/q0) = (p0/p1)^eps ,
// applied to each component where p0 = 1 and p1 = p0+tau
KMatrix EconoptimModel::xprtDemand(const KMatrix & tau) const {
  assert(sameShape(x0, tau));
  if (!sameShape(x0, tau)) {
    throw KException("EconoptimModel::xprtDemand: x0 and tau are not similar shaped");
  }
  auto x1 = KMatrix(N, 1);
  double p0 = 1;
  for (unsigned int i = 0; i < N; i++) {
    double pi = p0 + tau(i, 0);
    if(0.0 >= pi) {
      throw KException("EconoptimModel::xprtDemand: pi can not be negative");
    }
    x1(i, 0) = pow(p0 / pi, eps(i, 0)) * x0(i, 0);
  }
  return x1;
}

KMatrix EconoptimModel::randomFTax(PRNG* rng) {
  using KBase::dot;
  KMatrix ftax = KMatrix(N, 1);

  auto makeRand = [this, rng]() {
    auto tax = KMatrix(N, 1);
    for (unsigned int i = 0; i < N; i++) {
      double ti = rng->uniform(-1, +1);
      if (ti < 0) {
        ti = ti*maxSub;
      }
      if (0 < ti) {
        ti = ti*maxTax;
      }
      tax(i, 0) = ti;
    }
    return tax;
  };

  auto clip = [this](const KMatrix & tm1) {
    auto tm2 = tm1;
    for (unsigned int i = 0; i < tm1.numR(); i++) {
      for (unsigned int j = 0; j < tm1.numC(); j++) {
        double t = tm1(i, j);
        t = (t < -maxSub) ? -maxSub : t;
        t = (maxTax < t) ? maxTax : t;
        tm2(i, j) = t;
      }
    }
    return tm2;
  };

  bool retry = true;
  while (retry) {
    retry = false;
    try {
      auto t1 = makeRand();
      // now tax is a bunch of random numbers, within [-maxSub, +maxTax] limits.
      // first, adjust to revenue neutrality assuming no demand-effects,
      // then, clip that back within allowable limits (esp make sure -1 < ti)
      auto t2 = clip(KBase::makePerp(t1, x0));
      // of course, that might ruin the revenu-neutrality, so repeat:
      auto t3 = clip(KBase::makePerp(t2, x0));
      // finally, adjust that including demand effects:
      ftax = makeFTax(t3);
    }
    catch (KBase::KException&) {
      retry = true;
    }
  }
  return ftax;
}

// this occaisonally fails to terminate, so we throw an exception
// in those cases. This generally indicates an unreasonably revenue-non-neutral
// initial tax, but search algorithms try crazy stuff: be prepared.
// One way to avoid trouble is to make sure the initial tax is
// within bounds and revenue neutral for the base case: dot(x0,tax) = 0.
// It usually takes 10-15 iterations, so 100 is generous.
KMatrix EconoptimModel::makeFTax(const KMatrix & tax) const {
  using KBase::ReportingLevel;
  using KBase::makePerp;
  auto srl = ReportingLevel::Silent;

  if (ReportingLevel::Silent < srl) {
    LOG(INFO) << "Raw Tax:";
    trans(tax).mPrintf(" %+0.6f ");
  }


  unsigned int N = x0.numR();
  if(1 != x0.numC()) {
    throw KException("EconoptimModel::makeFTax: x0 must be a column vector");
  }
  assert(sameShape(tax, x0));

  string keNeg = "EconoptimModel::makeFTax: subsidies at or over 100%";
  for (unsigned int i = 0; i < N; i++) {
    double pi = 1.0 + tax(i, 0);
    if (pi <= 0) {
      if (ReportingLevel::Silent < srl) {
        LOG(INFO) << keNeg;
      }
      throw KBase::KException(keNeg);
    }
  }


  auto x1 = x0;
  string keItr = "EconoptimModel::makeFTax: iteration limit exceeded";
  const double tol = TolIFD / 10;
  const unsigned int iterMax = 100;
  unsigned int iter = 0;
  auto tau = tax;
  auto t2 = tax;
  double d = infsDegree(tau);
  //printf("First d in makeFTax: %.4E \n", d);
  //printf("Initial tax vector: ");
  //trans(tax).printf(" %+.6f ");
  const double shrink = 0.9;

  while (d > tol) {
    x1 = xprtDemand(tau);
    t2 = makePerp(tau, x1);
    tau = (t2 + tau) / 2;
    if (iter > (iterMax / 4)) {
      tau = shrink * tau;
    }
    d = infsDegree(tau);
    iter = iter + 1;
    if (ReportingLevel::Low < srl) {
      LOG(INFO) << KBase::getFormattedString("%3u/%3u: %.3E", iter, iterMax, d);
    }
    if (iter > iterMax) {
      if (ReportingLevel::Silent < srl) {
        LOG(INFO) << keItr;
      }
      throw KBase::KException(keItr);
    }
  }
  for (unsigned int i = 0; i < N; i++) {
    double pi = 1.0 + tau(i, 0);
    if(0.0 >= pi) { // no negative prices
      throw KException("EconoptimModel::makeFTax: pi can not be negative");
    }
  }

  const double ifd = infsDegree(tau);
  if(ifd > tol) { // one last check that it is A-OK
    throw KException("EconoptimModel::makeFTax: ifd must not be greater than tol");
  }
  return tau;
}

double EconoptimModel::infsDegree(const KMatrix & tax) const {
  double mAbs = 0.0;
  bool OK = true;
  for (unsigned int i = 0; i < N; i++) {
    double ti = tax(i, 0);
    if ((ti < 0) && (ti < -maxSub)) {
      mAbs = mAbs - (ti + maxSub); // e.g. ti = -0.7, maxSub = 0.5
      OK = false;
    }
    if(0 > mAbs) {
      throw KException("EconoptimModel::infsDegree: mAbs must be non-negative");
    }
    if ((0 < ti) && (maxTax < ti)) {
      mAbs = mAbs + (ti - maxTax);
      OK = false;
    }
    if(0 > mAbs) {
      throw KException("EconoptimModel::infsDegree: mAbs must be non-negative");
    }

  }
  if (OK) {
    auto xt = xprtDemand(tax);
    auto t2 = KBase::makePerp(tax, xt);
    mAbs = mAbs + KBase::maxAbs(t2 - tax);
  }
  return mAbs;
}

// Return row-vector of expected factor shares (e.g. L of them, e.g. labor), then expected sector shares (N of them).
// As they are estimated by different economic models, sum of factor VA will usually NOT match sum of sector VA
KMatrix  EconoptimModel::vaShares(const KMatrix & tax, bool normalizeSharesP) const {

  using KBase::sum;

  auto xt = xprtDemand(tax);

  if(infsDegree(tax) >= TolIFD) { // make sure it is a feasible tax
    throw KException("EconoptimModel::infsDegree: It is not a feasible tax");
  }

  auto qA = aL * xt; // N-by-1 column vector
  auto budgetL = rho * qA;

  auto qB = bL * xt; // N-by-1 column vector
  auto budgetS = KMatrix(1, N);
  for (unsigned int j = 0; j < N; j++) {
    double vs = qB(j, 0) * vas(0, j);
    budgetS(0, j) = vs;
  }

  // note that the sums of factor and of sector VA's will
  // NOT be equal, as they are assessed by different models.
  auto fShares = KMatrix(1, L);
  auto sShares = KMatrix(1, N);

  for (unsigned int j = 0; j < L; j++) {
    double s = budgetL(j, 0);
    if(0 >= s) {
      throw KException("EconoptimModel::infsDegree: s must be positive within L");
    }
    fShares(0, j) = s;
  }
  for (unsigned int j = 0; j < N; j++) {
    double s = budgetS(0, j);
    if(0 >= s) {
      throw KException("EconoptimModel::infsDegree: s must be positive within N");
    }
    sShares(0, j) = s;
  }

  if (normalizeSharesP) {
    fShares = fShares / sum(fShares);
    sShares = sShares / sum(sShares);

  }

  return KBase::joinH(fShares, sShares); //  [factor | sector]  as promised
}


KMatrix EconoptimModel::monteCarloShares(unsigned int nRuns, PRNG* rng) {
  auto rl = KBase::ReportingLevel::Low;
  // each run is a row of unnormalized [factor | sector] shares
  // the first row is the base case of zero taxes (row 0 <--> tax 0)
  const bool normP = false;
  if((0 > maxSub) || (maxSub >= 1)) {
    throw KException("EconoptimModel::monteCarloShares: maxSub is not in range [0,1)");
  }
  if(0 > maxTax) {
    throw KException("EconoptimModel::monteCarloShares: maxTax must be non-negative");
  }

  auto runs = KMatrix(nRuns, L + N);
  auto tau = KMatrix(N, 1); // zero taxes
  auto shr = vaShares(tau, normP);
  for (unsigned int j = 0; j < L + N; j++) {
    runs(0, j) = shr(0, j);
  }

  for (unsigned int i = 1; i < nRuns; i++) {
    tau = randomFTax(rng); // this occaisonally takes a long time
    tau = makeFTax(tau);
    double ifd = infsDegree(tau);
    if(ifd >= TolIFD) {
      throw KException("EconoptimModel::monteCarloShares: ifd must be less than TolIFD");
    }
    shr = vaShares(tau, normP);

    for (unsigned int j = 0; j < L + N; j++) {
      runs(i, j) = shr(0, j);
    }

    if (KBase::ReportingLevel::Medium <= rl) {
      LOG(INFO) <<"MC tax policy %4u:" << i;
      tau.mPrintf(" %+.4f ");
      LOG(INFO) <<"MC shares %4u:" << i;
      shr.mPrintf(" %+.4f ");
      for (unsigned int j = 0; j < L + N; j++) {
        if (L <= j) {
          // as they are in [factor |sector] order,
          // we have L factors to skip then N sectors to show
          LOG(INFO) << KBase::getFormattedString("for MC tax policy %4u, actor %2u taxed %+.4f has share %+.4f",
                 i, j, tau(j - L, 0), shr(0, j));
        }
      }
    }
  }

  return runs;
}
// -------------------------------------------------

EconoptimModel* demoSetup(unsigned int numFctr, unsigned int numCGrp, unsigned int numSect, uint64_t s, PRNG* rng) {
  using std::get;

  LOG(INFO) << KBase::getFormattedString("Setting up with PRNG seed:  %020llu", s);
  rng->setSeed(s);

  // because votes, and hence coalition strengths, cannot be computed simply as a function
  // of difference in utility (though influence is correlated with the difference), we
  // have to build the coalition strength matrix directly from votes.
  // Model::vProb(const KMatrix & c)

  //const unsigned int numFctr = 2;
  //const unsigned int numCGrp = 2;
  //const unsigned int numSect = 5;

  // if there are many sectors, it gets a bit unrealistic to assume that the impact on each is negotiated
  // separately. So we might reduce the dimensionality by assuming that there are K << N
  // attributes being taxed, which appear in the goods in a random mix.
  // That is, the policy is to tax/subsidize those attributes (e.g. embodied CO2), and
  // each good is affected differently by the policy.
  // Thus, generate K basis tax vectors, of N components each, and take the K policy
  // parameters as the weighting vectors (+/-) on those basis taxes.


  // JAH 20160830 changed to pass in the seed
  auto eMod0 = new EconoptimModel("", s);
  eMod0->stop = nullptr; // no stop-run method, for now

  const unsigned int maxIter = 50; // TODO: realistic limit
  double qf = 1000.0;



  eMod0->stop = [maxIter, qf](unsigned int iter, const State * s) {
    bool tooLong = (maxIter <= iter);
    bool quiet = false;
    if (1 < iter) {
      auto sf = [](unsigned int i1, unsigned int i2, double d12) {
        LOG(INFO) << KBase::getFormattedString("sDist [%2i,%2i] = %.2E   ", i1, i2, d12);
        return;
      };

      auto s0 = ((const EconoptimState*)(s->model->history[0]));
      auto s1 = ((const EconoptimState*)(s->model->history[1]));
      auto d01 = EconoptimModel::stateDist(s0, s1);
      sf(0, 1, d01);

      auto sx = ((const EconoptimState*)(s->model->history[iter - 0]));
      auto sy = ((const EconoptimState*)(s->model->history[iter - 1]));
      auto dxy = EconoptimModel::stateDist(sx, sy);
      sf(iter - 0, iter - 1, dxy);

      quiet = (dxy < d01 / qf);
      if (quiet)
        LOG(INFO) <<"Quiet";
      else
        LOG(INFO) <<"Not Quiet";
    }
    return (tooLong || quiet);
  };


  auto eSt0 = new EconoptimState(eMod0);
  eMod0->addState(eSt0);

  eSt0->step = nullptr; // no step method, for now
  //eSt0->step = [eSt0]() {return eSt0->stepSUSN(); };

  auto trxc = eMod0->makeBaseYear(numFctr, numCGrp, numSect, rng);

  auto trns = get<0>(trxc);
  auto rev = get<1>(trxc);
  auto xprt = get<2>(trxc);
  auto cons = get<3>(trxc);

  eMod0->makeIOModel(trns, rev, xprt, cons, rng);

  // determine the reference level, as well as upper and lower
  // bounds on economic gain/loss for these actors in this economy.
  unsigned int nRuns = 2500;
  LOG(INFO) << "Calibrate utilities via Monte Carlo of " << nRuns << " runs ... ";
  const auto runs = eMod0->monteCarloShares(nRuns, rng); // row 0 is always 0 tax
  LOG(INFO) << "done";

  LOG(INFO) << "EU State for Econ actors with vector capabilities";
  const unsigned int numA = numSect + numFctr;
  const unsigned int eDim = numSect;
  LOG(INFO) <<"Number of actors" << numA;
  LOG(INFO) <<"Number of econ policy factors" << eDim;
  // Note that if eDim < numA, then they do not have enough degrees of freedom to
  // precisely target benefits. If eDim > numA, then they do.


  {   // this block makes local all the temp vars used to build the state.

    // for this demo, assign consistent voting rule to the actors.
    // Note that because they have vector capabilities, not scalar,
    // we cannot do a simple scalar election, and the only way to
    // build the 'coalitions' matrix is by voting of the actors themselves.
    auto overallVR = VotingRule::Proportional;

    // now create those actors and display them
    auto es = vector<Actor*>();
    for (unsigned int i = 0; i < numA; i++) {
      string ni = "EconoptimActor-";
      ni.append(std::to_string(i));
      string di = "Random econ actor";
      auto ai = new EconoptimActor(ni, di, eMod0, i);
      ai->randomize(rng);;
      ai->vr = overallVR;
      ai->setShareUtilScale(runs);
      es.push_back(ai);
    }
    if(numA != es.size()) {
      throw KException("EconoptimModel::monteCarloShares: size of es must be equal to actor count");
    }

    // now we give them positions that are slightly better for themselves
    // than the base case. this is just a random sampling.
    auto ps = vector<VctrPstn*>();
    for (unsigned int i = 0; i < numA; i++) {
      double maxAbs = 0.0; // with 0.0, all have zero-tax SQ as their initial position
      auto ai = (const EconoptimActor*)es[i];
      auto t0 = KMatrix::uniform(rng, eDim, 1, -maxAbs, +maxAbs);
      auto t1 = eMod0->makeFTax(t0);
      VctrPstn* ep = nullptr;
      const double minU = 0.0; // could be ai->refU, or with default, 0.6
      const double maxU = 1.0; // could be (1+ai->refU)/2, or with default, 0.8
      if(0 >= ai->refU) {
        throw KException("EconoptimModel::monteCarloShares: refU must be positive");
      }
      double ui = -1;
      while ((ui < minU) || (maxU < ui)) {
        if (nullptr != ep) {
          delete ep;
        }
        t0 = KMatrix::uniform(rng, eDim, 1, -maxAbs, +maxAbs);
        t1 = eMod0->makeFTax(t0);
        ep = new VctrPstn(t1);
        ui = ai->posUtil(ep);
      }
      ps.push_back(ep);
    }
    if(numA != ps.size()) {
      throw KException("EconoptimModel::monteCarloShares: size of ps must be equal to actor count");
    }

    for (unsigned int i = 0; i < numA; i++) {
      eMod0->addActor(es[i]);
      eSt0->pushPstn(ps[i]);
    }
  }
  // end of local-var block

  for (unsigned int i = 0; i < numA; i++) {
    auto ai = (const EconoptimActor*)eMod0->actrs[i];
    auto pi = (const VctrPstn*)eSt0->pstns[i];
    LOG(INFO) << i << ":" << ai->name << "," << ai->desc;
    LOG(INFO) << "voting rule:" << ai->vr;
    LOG(INFO) << "Pos vector:";
    trans(*pi).mPrintf(" %+7.3f ");
    LOG(INFO) << "Cap vector:";
    trans(ai->vCap).mPrintf(" %7.3f ");
    LOG(INFO) << KBase::getFormattedString("minS: %.3f", ai->minS);
    LOG(INFO) << KBase::getFormattedString("refS: %.3f", ai->refS);
    LOG(INFO) << KBase::getFormattedString("maxS: %.3f", ai->maxS);
  }

  eSt0->setAUtil(-1, KBase::ReportingLevel::Low);
  auto u = eSt0->aUtil[0];
  if(numA != eSt0->model->numAct) {
    throw KException("EconoptimModel::monteCarloShares: inaccurate number of actors in eSt0 model");
  }

  auto vfn = [eMod0, eSt0](unsigned int k, unsigned int i, unsigned int j) {
    if(j == i) {
      throw KException("EconoptimModel::monteCarloShares: j and i must not be same");
    }

    auto ak = ((EconoptimActor*)(eMod0->actrs[k]));
    double vk = ak->vote(i, i, j, eSt0);
    return vk;
  };

  KMatrix c = Model::coalitions(vfn, eMod0->actrs.size(), eSt0->pstns.size());
  LOG(INFO) << "Coalition strength matrix:";
  c.mPrintf(" %9.3f ");

  auto vpm = VPModel::Linear;
  auto pcem = PCEModel::ConditionalPCM;

  const auto pv2 = Model::probCE2(pcem, vpm, c);
  const auto p = get<0>(pv2); // column
  const auto pv = get<1>(pv2); // square
  LOG(INFO) << "Probability Opt_i > Opt_j:";
  pv.mPrintf(" %.4f ");
  LOG(INFO) << "Probability Opt_i:";
  p.mPrintf(" %.4f ");
  auto eu0 = u*p;
  LOG(INFO) << "Expected utility to actors:";
  eu0.mPrintf(" %.4f ");
  return eMod0;
} // end of demoSetup


// JAH 20160809 create a version of the Leon App that can
// run with real-ish data; this could eventually be extended
// to take in the setup data from csv
// note that this bypasses entirely demoSetup, makeBaseYear, and makeIOModel
void demoRealEcon(bool OSPonly, uint64_t s, PRNG* rng)
{
  using std::vector;
  using std::get;

  // setup Leon model - this is all copied from demoSetup
  auto eMod0 = new EconoptimModel("", s);
  eMod0->stop = nullptr; // no stop-run method, for now

  const unsigned int maxIter = 50; // TODO: realistic limit
  double qf = 1000.0;

  eMod0->stop = [maxIter, qf](unsigned int iter, const State * s) {
    bool tooLong = (maxIter <= iter);
    bool quiet = false;
    if (1 < iter) {
      auto sf = [](unsigned int i1, unsigned int i2, double d12) {
        LOG(INFO) << KBase::getFormattedString("sDist [%2i,%2i] = %.2E   ", i1, i2, d12);
        return;
      };

      auto s0 = ((const EconoptimState*)(s->model->history[0]));
      auto s1 = ((const EconoptimState*)(s->model->history[1]));
      auto d01 = EconoptimModel::stateDist(s0, s1);
      sf(0, 1, d01);

      auto sx = ((const EconoptimState*)(s->model->history[iter - 0]));
      auto sy = ((const EconoptimState*)(s->model->history[iter - 1]));
      auto dxy = EconoptimModel::stateDist(sx, sy);
      sf(iter - 0, iter - 1, dxy);

      quiet = (dxy < d01 / qf);
      if (quiet)
        LOG(INFO) << "Quiet";
      else
        LOG(INFO) << "Not Quiet";

    }
    return (tooLong || quiet);
  };

  auto eSt0 = new EconoptimState(eMod0);
  eMod0->addState(eSt0);
  eSt0->step = nullptr; // no step method, for now

  // SETUP THE DATA NOW - THIS IS THE NEW STUFF
  const unsigned int N = 9;
  const unsigned int M = 1;
  const unsigned int L = 3;

  // base year export by sector
  // this is from '[IO-USA-1981.xlsx]C+E'!F6:F14
  vector<double> xInput = {7.2451, 1.3015, 0.0228, 76.4894, 94.6275, 20.3296,
    23.5439, 27.6867, 19.9283};
  auto xprt = KMatrix::vecInit(xInput, N, 1);

  // base year domestic demand by sector
  // this is from '[IO-USA-1981.xlsx]C+E'!E6:E14
  vector<double> cInput = {15.5082, 9.5952, 303.6608, 295.1940, 365.1954, 122.5666,
    424.4459, 662.0114, 741.6478};
  auto cons = KMatrix::vecInit(cInput, N, M);

  // export demand price elasticities - different scenarios
  unsigned int epsscen = 2;
  KMatrix eps = KMatrix();  // must declare it here even if capscen == 0 because of scope
  switch (epsscen)
  {
    case 0: // simulated
      eps = KMatrix::uniform(rng, N, 1, 2.0, 3.0);
      break;
    case 1: // equal
      eps = KMatrix(N,1,2.5);
      break;
    case 2: // negatively correlated with VA - most value-add linked with most elastic
    {
      vector<double> eInput = {3.000, 2.750, 2.625, 2.250, 2.500, 2.875, 2.375, 2.125, 2.000};
      eps = KMatrix::vecInit(eInput,N,1);
      break;
    }
    case 3: // positively correlated with VA - most value-add linked with least elastic
    {
      vector<double> eInput = {2.000, 2.250, 2.375, 2.750, 2.500, 2.125, 2.625, 2.875, 3.000};
      eps = KMatrix::vecInit(eInput,N,1);
      break;
    }
  }
  // scenarios 2 & 3 calcs are from '[IO-USA-1981.xlsx]Trans-O'!V29:W37
  LOG(INFO) << "Export Price Elasticities Generated with Scenario" << epsscen;

  // base year transactions between all sectors
  // this is from '[IO-USA-1981.xlsx]Trans-L'!C2:K10
  vector<double> tInput = {41.7728, 0.0000, 0.4084, 4.6623, 95.9681, 0.0000,
    1.3071, 3.0326, 7.0988, 0.1770, 13.3697, 2.4507, 13.9868, 180.7771, 43.3691,
    0.0000, 0.0000, 2.3663, 1.9470, 9.8936, 0.8169, 7.4596, 7.8114, 15.7706,
    5.2285, 33.3587, 22.4796, 3.1861, 10.4283, 119.2672, 327.2914, 37.9409, 12.8136,
    5.2285, 2.0217, 42.5929, 28.8516, 6.1500, 26.1408, 59.6771, 330.3087, 51.7472,
    26.1427, 11.1196, 115.9473, 5.1331, 6.4174, 12.2535, 45.6902, 64.7227, 87.2310,
    41.1748, 22.2391, 65.0724, 7.2571, 3.2087, 32.2675, 48.4876, 50.2158, 10.3494,
    13.0714, 4.0435, 36.6772, 11.5052, 12.8349, 5.7183, 14.9193, 13.3909, 11.3351,
    41.1748, 147.5870, 62.7062, 3.8941, 4.8131, 38.8027, 39.1631, 49.0999, 24.6415,
    87.5781, 57.6196, 115.9473};
  auto trns = KMatrix::vecInit(tInput,N,N);

  // base year factor value-added
  // this is from '[IO-USA-1981.xlsx]Trans-L'!C11:K13
  vector<double> vInput = {32.7324, 100.9019, 69.5304, 127.6188, 98.2364, 105.2256,
    254.0997, 401.0373, 391.3663, 16.0422, 44.2612, 19.9704, 56.0749, 43.1645,
    51.5711, 145.4282, 209.2748, 204.2281, 24.5049, 55.1145, 80.8226, 187.4229,
    144.2715, 78.7763, 33.1342, 119.5358, 116.6532};
  auto rev = KMatrix::vecInit(vInput,L,N);


  // vector of expenditures by factor into consumption group(s)
  // this is from '[IO-USA-1981.xlsx]JAH'!C11:K13
  vector<double> eInput = {0.951327802789937, 0.875269217592669, 0.886106910793789};
  auto expnd = KMatrix::vecInit(eInput,M,L);

  // scaled (d+g)B matrix
  // this is from '[IO-USA-1981.xlsx]BCK'!C4:K12
  double regul = 0.02457; // this is from '[IO-USA-1981.xlsx]JAH'!H99
  vector<double> bInput = {0.0230, 0.0000, 0.0000, 0.0000, 0.0004, 0.0000, 0.0000,
    0.0000, 0.0000, 0.0003, 0.0022, 0.0001, 0.0003, 0.0008, 0.0005, 0.0001, 0.0001,
    0.0001, 0.0416, 0.0472, 0.0051, 0.0064, 0.0059, 0.0514, 0.0133, 0.2974, 0.0058,
    0.0808, 0.0419, 0.0165, 0.0482, 0.0301, 0.1242, 0.0284, 0.0299, 0.0209, 0.0009,
    0.0005, 0.0002, 0.0011, 0.0049, 0.0014, 0.0003, 0.0003, 0.0002, 0.0035, 0.0018,
    0.0007, 0.0022, 0.0016, 0.0084, 0.0012, 0.0013, 0.0009, 0.0145, 0.0075, 0.0030,
    0.0076, 0.0056, 0.0224, 0.0301, 0.0054, 0.0038, 0.0009, 0.0005, 0.0002, 0.0006,
    0.0004, 0.0014, 0.0003, 0.0003, 0.0002, 0.0000, 0.0000, 0.0000, 0.0005, 0.0002,
    0.0000, 0.0000, 0.0000, 0.0005};
  auto Bmat = regul*KMatrix::vecInit(bInput,N,N);

  // set up for scenarios of capacities
  // 0 = random, 1 = equal, 2 = self-weighted, 3 = input-weighted
  unsigned int capscen = 3;
  KMatrix caps = KMatrix();  // must declare it here even if capscen == 0 because of scope
  switch (capscen) {
  case 0:
    // just keep the randomizer code below
    break;
  case 1:
  {
    // every actor has the same capability in each dim (which are all summed anyway)
    caps = KMatrix(N + L, N, 1.0);
    LOG(INFO) << "Capabilities Matrix (Scen 1):";
    caps.mPrintf(" %0.3f ");
    break;
  }
  case 2:
  {
    // set up with cap = 1 for each sector in his own dim, and 1/N everywhere else
    // factors all have 1/N capability - not that this automatically underweights them
    // relative to the sectors
    auto eye = KMatrix(N, N, 1.0 / N);
    for (unsigned int r = 0; r < N; r++)
    {
      for (unsigned int c = 0; c < N; c++)
      {
        if (r == c)
        {
          eye(r, c) = 1.0;
        }
      }
    }
    caps = KBase::joinV(KMatrix(L, N, 1.0 / N), eye);
    LOG(INFO) << "Capabilities Matrix (Scen 2):";
    caps.mPrintf(" %0.3f ");
    break;
    // P.S. JAH 20160830 looking through the existing code, I determined that this
    // is wasted effort, as the vector of weights for each actor is simply summed
    // to a scalar number :-(; keeping this scenario for possible future use
  }
  case 3:
  {
    // JAH 20161003 weight each sector according to it's total VA
    // from '[IO-USA-1981.xlsx]Trans-O'!C26:N26
    vector<double> compVA = { 1537.2745, 750.0555, 923.6700, 62.8064, 205.5044,
      249.9203, 458.6496, 275.9301, 199.5496, 458.0236, 571.5965, 729.0196 };
    caps = KMatrix::vecInit(compVA, N + L, 1);
    LOG(INFO) << "Capabilities Matrix (Scen 3):";
    caps.mPrintf(" %0.3f ");
    break;
  }
  }

  // now prep it all
  eMod0->prepModel(L, M, N, xprt, cons, eps, trns, rev, expnd, Bmat);

  // factors and sectors - JAH 20161003 changed order of actors
  vector<string> actNames = { "HH Pay", "Other Pay", "Imports",
    "Agr", "Ming", "Const", "DurGood", "NDurGood", "Trans,C,U","W&R Trade", "Fin,Ins,RE", "Other Srv" };
  vector<string> actDescs = { "HH Pay", "Other Pay", "Imports", "Agriculture",
    "Mining", "Construction", "Durable goods manufacturing", "Nondurable goods manufacturing",
    "Transportation, communication and utilities", "Wholesale and retail trade",
    "Finance, insurance and real estate", "Other services" };

  // NOW BACK TO STUFF COPIED FROM demoSetup (with obvious edits)
  // determine the reference level, as well as upper and lower
  // bounds on economic gain/loss for these actors in this economy.
  unsigned int nRuns = 2500;
  LOG(INFO) << "Calibrate utilities via Monte Carlo of" << nRuns << "runs ... ";
  const auto runs = eMod0->monteCarloShares(nRuns, rng); // row 0 is always 0 tax

  LOG(INFO) << "EU State for Econ actors with vector capabilities";
  const unsigned int numA = N + L;
  const unsigned int eDim = N;
  LOG(INFO) << "Number of actors" << numA;
  LOG(INFO) << "Number of econ policy factors" << eDim;
  // Note that if eDim < numA, then they do not have enough degrees of freedom to
  // precisely target benefits. If eDim > numA, then they do.

  {   // this block makes local all the temp vars used to build the state.

    // for this demo, assign consistent voting rule to the actors.
    // Note that because they have vector capabilities, not scalar,
    // we cannot do a simple scalar election, and the only way to
    // build the 'coalitions' matrix is by voting of the actors themselves.
    auto overallVR = VotingRule::Proportional;

    // now create those actors and display them
    // JAH edited to use actor name & descs from the realish data
    auto es = vector<Actor*>();
    for (unsigned int i = 0; i < numA; i++) {
      auto ai = new EconoptimActor(actNames[i], actDescs[i], eMod0, i);
      // JAH 20160814 changed to allow scenarios
      switch (capscen) {
      case 0:
        ai->randomize(rng);
        break;
      default:
        // get a row from the scenario capabilities matrix and transpose to the column used by Actor
        //ai->vCap = KBase::trans(caps.getRow(i));
        ai->vCap = KBase::trans(KBase::hSlice(caps, i));
        break;
      }
      ai->vr = overallVR;
      ai->setShareUtilScale(runs);
      es.push_back(ai);
    }
    if(numA != es.size()) {
      throw KException("EconoptimModel::monteCarloShares: size of es must be equal to actor count");
    }

    // now we give them positions that are slightly better for themselves
    // than the base case. this is just a random sampling.
    auto ps = vector<VctrPstn*>();
    for (unsigned int i = 0; i < numA; i++) {
      double maxAbs = 0.0; // with 0.0, all have zero-tax SQ as their initial position
      auto ai = (const EconoptimActor*)es[i];
      auto t0 = KMatrix::uniform(rng, eDim, 1, -maxAbs, +maxAbs);
      auto t1 = eMod0->makeFTax(t0);
      VctrPstn* ep = nullptr;
      const double minU = 0.0; // could be ai->refU, or with default, 0.6
      const double maxU = 1.0; // could be (1+ai->refU)/2, or with default, 0.8
      if(0 >= ai->refU) {
        throw KException("EconoptimModel::monteCarloShares: refU must be positive");
      }
      double ui = -1;
      while ((ui < minU) || (maxU < ui)) {
        if (nullptr != ep) {
          delete ep;
        }
        t0 = KMatrix::uniform(rng, eDim, 1, -maxAbs, +maxAbs);
        t1 = eMod0->makeFTax(t0);
        ep = new VctrPstn(t1);
        ui = ai->posUtil(ep);
      }
      ps.push_back(ep);
    }
    if(numA != ps.size()) {
      throw KException("EconoptimModel::monteCarloShares: size of ps should match with actor count");
    }

    for (unsigned int i = 0; i < numA; i++) {
      eMod0->addActor(es[i]);
      eSt0->pushPstn(ps[i]);
    }
  }
  // end of local-var block

  for (unsigned int i = 0; i < numA; i++) {
    auto ai = (const EconoptimActor*)eMod0->actrs[i];
    auto pi = (const VctrPstn*)eSt0->pstns[i];
    LOG(INFO) << i << ":" << ai->name << "," << ai->desc;
    LOG(INFO) << "voting rule:" << ai->vr;
    LOG(INFO) << "Pos vector:";
    trans(*pi).mPrintf(" %+7.3f ");
    LOG(INFO) << "Cap vector:";
    trans(ai->vCap).mPrintf(" %7.3f ");
    LOG(INFO) << KBase::getFormattedString("minS: %.3f", ai->minS);
    LOG(INFO) << KBase::getFormattedString("refS: %.3f", ai->refS);
    LOG(INFO) << KBase::getFormattedString("maxS: %.3f", ai->maxS);
  }

  eSt0->setAUtil(-1, KBase::ReportingLevel::Low);
  auto u = eSt0->aUtil[0];
  if(numA != eSt0->model->numAct) {
    throw KException("EconoptimModel::monteCarloShares: inaccurate number of actors");
  }

  auto vfn = [eMod0, eSt0](unsigned int k, unsigned int i, unsigned int j) {
    if(j == i) {
      throw KException("EconoptimModel::monteCarloShares: j and i must not be same");
    }
    auto ak = ((EconoptimActor*)(eMod0->actrs[k]));
    double vk = ak->vote(i, i, j, eSt0);
    return vk;
  };

  KMatrix c = Model::coalitions(vfn, eMod0->actrs.size(), eSt0->pstns.size());
  LOG(INFO) << "Coalition strength matrix:";
  c.mPrintf(" %9.3f ");

  const auto vpm = VPModel::Linear;
  const auto pcem = PCEModel::ConditionalPCM;

  const auto pv2 = Model::probCE2(pcem, vpm, c);
  const auto p = get<0>(pv2);
  const auto pv = get<1>(pv2);

  LOG(INFO) << "Probability Opt_i > Opt_j:";
  pv.mPrintf(" %.4f ");

  LOG(INFO) << "Probability Opt_i:";
  p.mPrintf(" %.4f ");
  auto eu0 = u*p;
  LOG(INFO) << "Expected utility to actors:";
  eu0.mPrintf(" %.4f ");

  // choose which model to run: only OSPs bargaining to find the CP, or
  // UMAs bargaining with SUSN and PCE over the proposals from OSPs
  if (OSPonly) {
    // begin content copied from demoMaxEcon
    eSt0->aUtil = vector<KMatrix>(); // dropping any old ones
    eSt0->step = nullptr;

    auto sCap = KMatrix(eMod0->numAct, 1);
    for (unsigned int i = 0; i < eMod0->numAct; i++) {
      auto ai = (EconoptimActor*)(eMod0->actrs[i]);
      double si = sum(ai->vCap);
      sCap(i, 0) = si;
    }

    auto omegaFn = [eMod0, sCap](const KMatrix & m1) {
      auto m2 = eMod0->makeFTax(m1); // make it feasible
      auto shares = eMod0->vaShares(m2, false);
      double omega = 0;
      for (unsigned int i = 0; i < eMod0->numAct; i++) {
        auto ai = (EconoptimActor*)(eMod0->actrs[i]);
        double si = shares(0, i);
        double ui = ai->shareToUtil(si);
        omega = omega + ui*sCap(i, 0);
      }
      return omega;
    };

    auto reportFn = [eMod0](const KMatrix & m) {
      KMatrix r = eMod0->makeFTax(m);
      if(eMod0->infsDegree(r) >= TolIFD) { // make sure it is a feasible tax
        throw KException("EconoptimModel::monteCarloShares: Not a feasible tax");
      }
      LOG(INFO) << "Rates:";
      trans(r).mPrintf(" %+.6f ");
      return;
    };

    // TODO: do a VHCSearch here
    auto vhc = new KBase::VHCSearch();
    vhc->eval = omegaFn; // [] (const KMatrix & m1) { return 0.0;};
    vhc->nghbrs = KBase::VHCSearch::vn2;
    vhc->report = reportFn;

    auto rslt = vhc->run(KMatrix(N, 1),            // p0
                         1000, 10, 1E-4,              // iterMax, stableMax, sTol
                         0.01, 0.618, 1.25, 1e-6,     // step0, shrink, stretch, minStep
                         ReportingLevel::Medium);
    // note that typical improvements in utility in the first round are on the order of 1E-1 or 1E-2.
    // Therefore, any improvement of less than 1/100th of that (below sTol = 1E-4) is considered "stable"

    double vBest = get<0>(rslt);
    KMatrix pBest = get<1>(rslt);
    unsigned int in = get<2>(rslt);
    unsigned int sn = get<3>(rslt);

    delete vhc;
    vhc = nullptr;
    LOG(INFO) << "Iter:" << in << "Stable:" << sn;
    LOG(INFO) << KBase::getFormattedString("Best value : %+.6f", vBest);
    LOG(INFO) << "Best point:";
    trans(pBest).mPrintf(" %+.6f ");
    KMatrix rBest = eMod0->makeFTax(pBest);
    LOG(INFO) << "Best rates:";
    trans(rBest).mPrintf(" %+.6f ");

    delete vhc;
    vhc = nullptr;
    // end content copied from demoMaxEcon*/
  }
  else {
    // begin content copied from demoEUEcon
    eSt0->aUtil = vector<KMatrix>(); // dropping any old ones
    eSt0->step = [eSt0]() {
      return eSt0->stepSUSN();
    };

    eMod0->run();
    // end content copied from demoEUEcon
  }

  // JAH 2060814 want to display a matrix of final policies, as well as the final mean policy
  // this is predominantly for automatic extraction of results
  auto stfinal = eMod0->history[eMod0->history.size() - 1];
  LOG(INFO) << eMod0->history.size() - 1 << "Iterations Completed";
  LOG(INFO) << "FINAL POLICIES:";

  // compute the mean - I tried to get this as a method of the EconoptimModel and EconoptimState, but failed :-(
  auto p0 = ((VctrPstn*)(stfinal->pstns[0]));
  // want to display as row vectors, so might as well start of transposing
  KMatrix meanP = KMatrix(p0->numC(), p0->numR(), 0.0);
  for (unsigned int i = 0; i < eMod0->numAct; i++)
  {
    auto iPos = ((VctrPstn*)(stfinal->pstns[i]));
    auto y = KBase::trans(*iPos);
    meanP = meanP + y;
    y.mPrintf(" %+0.4f ");
  }
  meanP = meanP / numA;
  // talk
  LOG(INFO) << "FINAL MEAN POLICY:";
  meanP.mPrintf(" %+0.4f ");

  LOG(INFO) << "Estimates of GDP over time";
  eMod0->printEstGDP();

  delete eMod0; // and all the actors in it
  eMod0 = nullptr;

  return;
}

} // namespace


//int main(int ac, char **av) {
//  el::Configurations confFromFile("./econoptim-logger.conf");
//  el::Loggers::reconfigureAllLoggers(confFromFile);
//  using KBase::dSeed;
//
//  auto sTime = KBase::displayProgramStart();
//  uint64_t seed = dSeed;
//  bool run = true;
//  bool euEconP = false;
//  bool maxEconP = false;
//  bool rlEconP = false;
//  bool rlOSP = false; // true = run the model with only OSPs, false = UMAs & OSPs
//
//  auto showHelp = []() {
//    printf("\n");
//    printf("Usage: specify one or more of these options\n");
//    printf("--help              print this message\n");
//    printf("--euEcon            exp. util. of IO econ model\n");
//    printf("--maxEcon           max support of IO econ model\n");
//    printf("--rlEcon (OSP|UMA)  use synthesized data, with either all OSPs (maxEcon), or UMAs & OSPs (euEcon) \n");
//    printf("--seed <n>          set a 64bit seed, in decimal\n");
//    printf("                    0 means truly random\n");
//    printf("                    default: %020llu \n", dSeed);
//  };
//
//  // tmp args
//  //seed = 0;
//  //maxEconP = true;
//
//
//  if (ac > 1) {
//    for (int i = 1; i < ac; i++) {
//      if (strcmp(av[i], "--seed") == 0) {
//        i++;
//        seed = std::stoull(av[i]);
//      }
//      else if (strcmp(av[i], "--euEcon") == 0) {
//        euEconP = true;
//      }
//      else if (strcmp(av[i], "--maxEcon") == 0) {
//        maxEconP = true;
//      }
//      else if (strcmp(av[i], "--rlEcon") == 0) {
//        rlEconP = true;
//        // this is the demo with real synthesized data, now, which model?
//        i++;
//        if (strcmp(av[i],"OSP")==0) {
//          rlOSP = true;
//        }
//      }
//      else if (strcmp(av[i], "--help") == 0) {
//        run = false;
//      }
//      else {
//        run = false;
//        LOG(INFO) << "Unrecognized argument " << av[i];
//      }
//    }
//  }
//
//  if (!run) {
//    showHelp();
//    return 0;
//  }
//
//  LOG(INFO) << KBase::getFormattedString("Given PRNG seed:  %020llu", seed);
//  PRNG * rng = new PRNG();
//  seed = rng->setSeed(seed); // 0 == get a random number
//
//  LOG(INFO) << KBase::getFormattedString("Using PRNG seed:  %020llu", seed);
//  LOG(INFO) << KBase::getFormattedString("Same seed in hex:   0x%016llX", seed);
//
//  // don't understand why these were set to L = M = 5 and N = 10; the article had L = 3, M = 2, N = 5
//  const unsigned int numF = 3; // 5; // 2;
//  const unsigned int numG = 2; //5; // 2;
//  const unsigned int numS = 5; //10; //5;
//
//  // note that we reset the seed every time, so that in case something
//  // goes wrong, we need not scroll back too far to find the
//  // seed required to reproduce the bug.
//
//  if (rlEconP)
//  {
//    LOG(INFO) << "R----------------------------------";
//    try {
//		DemoEconOptim::demoRealEcon(rlOSP, seed, rng);
//    }
//    catch (KException &ke) {
//      LOG(INFO) << ke.msg;
//    }
//    catch (...) {
//      LOG(INFO) << "Unknown exception from DemoEconOptim::demoRealEcon";
//    }
//  }
//  LOG(INFO) << "-----------------------------------";
//
//  delete rng;
//  KBase::displayProgramEnd(sTime);
//  return 0;
//}


// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
