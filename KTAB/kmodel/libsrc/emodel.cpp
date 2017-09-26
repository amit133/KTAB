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

#include "hcsearch.h"
#include "emodel.h"
#include <thread>
#include <easylogging++.h>


namespace KBase {

using std::get;
using std::tuple;
using std::thread;

// --------------------------------------------
template <class PT>
// JAH 20160711 added rng seed
EModel<PT>::EModel(string desc, uint64_t s, vector <bool> f) : Model(desc, s, f) {
  // nothing yet
}

template <class PT>
void EModel<PT>::setOptions() {
  //assert(nullptr != enumOptions);
  if (nullptr == enumOptions) {
    throw KException("EModel<PT>::setOptions: enumOptions is a null pointer");
  }
  //assert(0 == theta.size());
  if (0 != theta.size()) {
    throw KException("EModel<PT>::setOptions: theta size is must be zero");
  }
  theta = enumOptions();
  //assert (minNumOptions <= theta.size());
  if (minNumOptions > theta.size()) {
    throw KException("EModel<PT>::setOptions: invalid number of options");
  }
  return;
}



template <class PT>
EModel<PT>::~EModel() {
  theta = {};
}


template <class PT>
unsigned int EModel<PT>::numOptions() const {
  return theta.size();
}


template <class PT>
PT EModel<PT>::nthOption(unsigned int i) const {
  //assert(i < theta.size());
  if (i >= theta.size()) {
    throw KException("EModel<PT>::nthOption: no member in theta at index i");
  }
  return theta[i];
}

template <class PT>
bool EModel<PT>::equivStates(const EState<PT>* es1, const EState<PT>*  es2) const {
  const unsigned int np = es1->pstns.size();
  bool eqvP = (np == es2->pstns.size()); // most basic test
  eqvP = eqvP && (es1->eMod == es2->eMod); //
  for (unsigned int i=0; i<np; i++) {
    eqvP = eqvP && (es1->posNdx(i) == es2->posNdx(i));
  }
  return eqvP;
}


template <class PT>
KMatrix EModel<PT>::actorWeights() const {
  auto w = KMatrix(1, numAct);
  for (unsigned int i=0; i<numAct; i++) {
    auto ai = (EActor<PT>*) (actrs[i]);
    w(0,i) = ai->sCap;
  }
  return w;
}

// --------------------------------------------

template <class PT>
EActor<PT>::EActor(EModel<PT>* m, string n, string d) : Actor(n,d) {
  eMod = m;
}


template <class PT>
EActor<PT>::~EActor() {
  // nothing yet
}


template <class PT>
double EActor<PT>::vote(unsigned int est,
                        unsigned int p1, unsigned int p2,
                        const State* st) const {
  /// vote between the current positions to actors at
  /// positions p1 and p2 of this state

  // get the two corresponding indices into Theta
  auto es = (const EState<PT>*) st;
  auto n1 = es->posNdx(p1);
  auto n2 = es->posNdx(p2);

  // TODO: lookup real utilities of Theta[n]
  //assert (false);
  throw KException("EActor<PT>::vote: dummy method");

  double u1 = 0.0; // some fn of ep1
  double u2 = 0.0; // some fn of ep2

  const double v12 = Model::vote(vr, sCap, u1, u2);
  return v12;
}


// --------------------------------------------
template <class PT>
EPosition<PT>::EPosition(EModel<PT>* m, int n) : Position() {
  //assert(nullptr != m);
  if (nullptr == m) {
    throw KException("EPosition<PT>::EPosition: m is a null pointer");
  }
  eMod = m;
  //assert(0 <= n);
  if (0 > n) {
    throw KException("EPosition<PT>::EPosition: n must be non-negative");
  }
  const unsigned int numOpt = eMod->numOptions();
  //assert(n < numOpt);
  if (n >= numOpt) {
    throw KException("EPosition<PT>::EPosition: n must be less than number of options");
  }
  ndx = n;
}

template <class PT>
EPosition<PT>::~EPosition() {
  eMod = nullptr;
  ndx = -1;
}



template <class PT>
void EPosition<PT>::print(ostream& os) const {
  os << "[EPosition " << ndx <<"]";
  return;
}

// --------------------------------------------
template <class PT>
EState<PT>::EState( EModel<PT>* mod) : State(mod) {
  step = nullptr;
  eMod = (EModel<PT>*) model;
}

template <class PT>
EState<PT>::~EState() {
  step = nullptr;
}


template <class PT>
void EState<PT>::show() const {
  //cout << "EState<PT>::show()  not yet implemented" << endl << flush;
  unsigned int na = pstns.size();
  string log = "[EState";
  for (unsigned int i=0; i<na; i++) {
    log += " " + std::to_string(posNdx(i));
  }
  log += "]";
  LOG(INFO) << log;
  return;
}


template <class PT>
bool EState<PT>::equivNdx(unsigned int i, unsigned int j) const {
  //assert (i < pstns.size());
  //assert (j < pstns.size());
  if (i >= pstns.size()) {
    throw KException("EState<PT>::equivNdx: Invalid index of Initiator actor");
  }

  if (j >= pstns.size()) {
    throw KException("EState<PT>::equivNdx: Invalid index of Receiver actor");
  }

  auto pi = (const EPosition<PT>*) (pstns[i]);
  //assert(nullptr != pi);
  if (nullptr == pi) {
    throw KException("EState<PT>::equivNdx: pi is a null pointer");
  }

  auto pj = (const EPosition<PT>*) (pstns[j]);
  //assert(nullptr != pj);
  if (nullptr == pj) {
    throw KException("EState<PT>::equivNdx: pj is a null pointer");
  }
  bool eqv = (pi->getIndex() == pj->getIndex());
  return eqv;
}


template <class PT>
unsigned int EState<PT>::posNdx(const unsigned int i) const {
  //assert (i < pstns.size());
  if (i >= pstns.size()) {
    throw KException("EState<PT>::posNdx: index should be within pstns's size");
  }
  auto pi = (const EPosition<PT>*)(pstns[i]);
  //assert (nullptr != pi);
  if (nullptr == pi) {
    throw KException("EState<PT>::posNdx: pi is a null pointer");
  }
  int ni = pi->getIndex();
  //assert (0 <= ni);
  if (0 > ni) {
    throw KException("EState<PT>::posNdx: ni must be non-negative");
  }
  auto ndx = (unsigned int) ni;
  return ndx;
}

template <class PT>
EState<PT>* EState<PT>::stepSUSN() {
  LOG(INFO) << "State number " << model->history.size() - 1;
  show();
  if ((0 == uIndices.size()) || (0 == eIndices.size())) {
    setUENdx();
  }
  LOG(INFO) << "Setting all utilities from objective perspective";
  setAUtil(-1, ReportingLevel::Low);
  auto s2 = doSUSN(ReportingLevel::Low);
  //assert(nullptr != s2);
  if (nullptr == s2) {
    throw KException("EState<PT>::stepSUSN: s2 is a null pointer");
  }
  s2->step = [s2]() {
    return s2->stepSUSN();
  };
  return s2;
}



template <class PT>
KMatrix EState<PT>::uMatH(int h) const {
  const unsigned int numA = eMod->numAct;
  const unsigned int na2 = eMod->actrs.size();
  //assert(numA == na2);
  if (numA != na2) {
    throw KException("EState<PT>::uMatH: count of actors not as per expectation");
  }
  const unsigned int numU = uIndices.size();
  //assert((0 < numU) && (numU <= numA));
  if ((0 >= numU) || (numU > numA)) {
    throw KException("EState<PT>::uMatH: numU is not in valid range");
  }
  //assert(numA == eIndices.size());
  if (numA != eIndices.size()) {
    throw KException("EState<PT>::uMatH: eIndices' size should be equal to number of actors");
  }

  // These are each actor's beliefs about the utilities
  // to other actors of the currently occupied positions
  const unsigned int aus = aUtil.size();
  //assert (KBase::Model::minNumActor <= aus);
  if (KBase::Model::minNumActor > aus) {
    throw KException("EState<PT>::uMatH: each actor doesn't have utility value");
  }

  KMatrix u = aUtil[0];
  // Utilities to actors of the currently occupied positions.
  if (0 <= h) {
    u = aUtil[h];
  }
  else {
    for (unsigned int i = 0; i < numA; i++) { // i's est of i's utility
      for (unsigned int j = 0; j < numA; j++) { // j's position
        u(i, j) = aUtil[i](i, j);
      }
    }
  }
  const unsigned int numP = pstns.size();

  // In the state, one-to-one matching of actors and actually
  // occupied positions (probably including duplicates)
  //assert(numA == numP);
  if (numA != numP) {
    throw KException("EState<PT>::uMatH: actors and corrsponding postitions don't match one-to-one");
  }

  auto uufn = [u, this](unsigned int i, unsigned int j1) {
    return u(i, uIndices[j1]);
  };
  auto uUnique = KMatrix::map(uufn, numA, numU);
  return uUnique;
}

template <class PT>
EState<PT>* EState<PT>::doSUSN(ReportingLevel rl) const {
  const unsigned int numA = eMod->numAct;
  const unsigned int na2 = eMod->actrs.size();
  //assert(numA == na2);
  if (numA != na2) {
    throw KException("EState<PT>::doSUSN: count of actors not as per expectation");
  }
  const unsigned int numU = uIndices.size();
  //assert((0 < numU) && (numU <= numA));
  if ((0 >= numU) || (numU > numA)) {
    throw KException("EState<PT>::doSUSN: numU is not in valid range");
  }
  //assert(numA == eIndices.size());
  if (numA != eIndices.size()) {
    throw KException("EState<PT>::doSUSN: eIndices' size should be equal to number of actors");
  }

  // Utilities to actors of the currently occupied positions.
  // All have same beliefs in this demo
  const auto u = aUtil[0];

  const auto vpm = eMod->vpm; // get the 'victory probability model'
  const unsigned int numP = pstns.size();
  auto uUnique = uMatH(0);

  // Get expected-utility vector, one entry for each actor, in the current state.
  const auto eu0 = expUtilMat(rl, numA, numP, vpm, uUnique); //  without duplicates

  if (ReportingLevel::Low < rl) {
    LOG(INFO) << "--------------------------------------- ";
    LOG(INFO) << "Actor expected utilities in actual state: ";
    for (unsigned int h = 0; h < numA; h++)
    {
      LOG(INFO) << KBase::getFormattedString("%3u , %.5f \n", h, eu0(h, 0));
    }
    LOG(INFO) << "Positions in this state: ";
    show();

    LOG(INFO) << KBase::getFormattedString("Out of %u positions, %u were unique, with these indices: ", numA, numU);
    for (auto i : uIndices)
    {
      LOG(INFO) << KBase::getFormattedString("%2i ", i);
    }
  }

  // Note that EState::makeNewEState is a pure virtual method,
  // so what gets called here is the method from derived classes,
  // which can provide the extra structure of a derived class.
  auto s2 = makeNewEState();

  //assert (0 == s2->pstns.size());
  if (0 != s2->pstns.size()) {
    throw KException("EState<PT>::doSUSN: s2 shouldn't have any positions yet");
  }
  for (unsigned int h = 0; h < numA; h++) {
    s2->pstns.push_back(nullptr);
  }
  // TODO: clean up the nesting of lambda-functions: ~200 lines is too long
  // perhaps create a hypothetical state and run setOneAUtil(h,Silent) on it

  // Evaluate h's estimate of the expected utility, to h, of
  // advocating position mp. To do this, build a hypothetical utility matrix representing
  // h's estimates of the direct utilities to all other actors of h adopting this
  // Position. Do that by modifying the h-column of h's matrix.
  // Then compute the expected probability distribution, over h's hypothetical position
  // and everyone else's actual position. Finally, compute the expected utility to
  // each actor, given that distribution, and pick out the value for h's expected utility.
  // That is the expected value to h of adopting the position.
  // ehFN :: (int, EPosition<PT>) ==> double
  auto ehFN = [this, rl, u](unsigned int h, const EPosition<PT> & eph)  {
    // This correctly handles duplicated/unique options
    // We modify the given utility matrix so that the h-column
    // corresponds to the given mph, but we need to prune duplicates as well.
    // This entails some type-juggling.

    const int eNdx = eph.getIndex();
    //assert (0 <= eNdx);
    if (0 > eNdx) {
      throw KException("EState<PT>::doSUSN: index must be non-negative");
    }
    auto uVec = actorUtilVectFn(h, eNdx);

    // all have same beliefs in this demo: verify
    const KMatrix uh0 = aUtil[h]; // constant
    //assert(KBase::maxAbs(u - uh0) < 1E-10);
    if (KBase::maxAbs(u - uh0) >= 1E-10) {
      throw KException("EState<PT>::doSUSN: all actors dont have beliefs");
    }
    auto uh = uh0; // to be modified

    for (unsigned int i = 0; i < eMod->numAct; i++) {
      // lookup h's prior evaluation of utils over the enumerated space
      // of utility to actor i of this hypothetical position by h
      double uih = uVec[i];
      uh(i, h) = uih;
    }


    // 'uh' now has the correct h-column. Now we need to see how many options
    // are unique in the hypothetical state, and keep only those columns.
    // This entails juggling back and forth between the all current positions
    // and the one hypothetical position (mph at h).
    // Thus, the next call to expUtilMat will consider only unique options.

    // Compare indices to see if the positions are equivalent.
    // For h, use the hypothetical 'eph', and for all others use
    // the stored pointers.
    // Cannot use State::equivNdx because the comparisons for index 'h'
    // use the hypothetical mph, not pstns[h]
    auto equivHNdx = [this, h, eph](const unsigned int i, const unsigned int j) {
      int ni = (i == h) ? eph.getIndex() : posNdx(i);
      int nj = (j == h) ? eph.getIndex() : posNdx(j);
      return (ni == nj);
    };

    auto ns = KBase::uiSeq(0, model->numAct - 1);
    const VUI uNdx = get<0>(KBase::ueIndices<unsigned int>(ns, equivHNdx));
    const unsigned int numU = uNdx.size();
    auto hypUtil = KMatrix(eMod->numAct, numU);
    // we need now to go through 'uh', copying column J the first time
    // the J-th position is determined to be equivalent to something in the unique list
    for (unsigned int i = 0; i < eMod->numAct; i++) {
      for (unsigned int j1 = 0; j1 < numU; j1++) {
        unsigned int j2 = uNdx[j1];
        hypUtil(i, j1) = uh(i, j2); // hypothetical utility in column h
      }
    }

    if (false) {
      LOG(INFO) << "constructed hypUtil matrix:";
      hypUtil.mPrintf(" %8.2f ");
    }


    if (ReportingLevel::Low < rl) {
      LOG(INFO) << "--------------------------------------- ";
      LOG(INFO) << KBase::getFormattedString("Assessing utility to %2i of hypo-pos: ", h);
      LOG(INFO) << eph;
      LOG(INFO) << "Hypo-util minus base util: ";
      (uh - uh0).mPrintf(" %+.4E ");
    }

    const KMatrix eu = expUtilMat(rl,
                                  eMod->numAct, pstns.size(), eMod->vpm,
                                  hypUtil); // uh or hypUtil
    const double euh = eu(h, 0);
    //assert(0.0 <= euh);
    if (0.0 > euh) {
      throw KException("EState<PT>::doSUSN: euh must be non-negative");
    }
    return euh;
  };
  // end of ehFN



  // 'Returns' void in order to have the right type-signature for threading.
  // Does a generic hill climb to find the best next position for actor h,
  // and stores it in s2.
  // To do that, it uses three functions for evaluation, neighbors, and show:
  // efn, nghbrPerms, and sfn.
  auto newPosFn = [this, ehFN, rl,  u, eu0, s2](const unsigned int h)  {
    if (ReportingLevel::Low < rl) {
      LOG(INFO) << "Started newPosFn for "<<h;
    }
    s2->pstns[h] = nullptr;
    auto ph = ((const EPosition<PT>*)(pstns[h]));
    auto ghc = new KBase::GHCSearch<EPosition<PT>>();

    ghc->eval = [ehFN, h](const EPosition<PT>  eph)  {
      return ehFN(h, eph);
    };
    const unsigned int numOpt = eMod->numOptions();

    // The 'neighbors' in general are ALL the enumerated positions,
    // so it does not even use the actor's current position
    auto nfn = [this, rl, numOpt](const EPosition<PT> & ) {
      vector<EPosition<PT>> ns = {};
      //ns.resize(numOpt); // compiler tries to fill with EPosition(), and fails.
      for (unsigned int i=0; i<numOpt; i++) {
        auto ep = EPosition<PT>(eMod, i);
        ns.push_back(ep);
      }
      if (ReportingLevel::Low < rl) {
        LOG(INFO) << "Found "<<ns.size()<<" neighbors";
      }
      //assert (0 < ns.size());
      if (0 >= ns.size()) {
        throw KException("EState<PT>::doSUSN: ns must be positive");
      }
      return ns;
    };

    ghc->nghbrs = nfn;

    // show some representation of this position on cout
    ghc->show = [](const EPosition<PT> & ep) {
      LOG(INFO) << ep ;
      return;
    };

    if (ReportingLevel::Low < rl) {
      LOG(INFO) << "---------------------------------------- ";
      LOG(INFO) << KBase::getFormattedString("Search for best next-position of actor %2i ", h);
    }
    auto rslt = ghc->run(*ph, // start from h's current positions
                         ReportingLevel::Silent,
                         100, // iter max
                         3, 0.001); // stable-max, stable-tol
    double vBest = get<0>(rslt);
    EPosition<PT> pBest = get<1>(rslt);
    unsigned int iterN = get<2>(rslt);
    unsigned int stblN = get<3>(rslt);
    delete ghc;
    ghc = nullptr;

    if (ReportingLevel::Medium < rl) {
      LOG(INFO) << KBase::getFormattedString("Iter: %u  Stable: %u \n", iterN, stblN);
      LOG(INFO) << KBase::getFormattedString("Best value for %2i: %+.6f \n", h, vBest);
      LOG(INFO) << "Best position:    " << pBest;
    }

    auto posBest = new EPosition<PT>(pBest);

    // Actually record the new position
    s2->pstns[h] = posBest;
    // no need for mutex, as s2->pstns is the only shared var,
    // and each h is different.

    double du = vBest - eu0(h, 0); // (hypothetical, future) - (actual, current)
    if (ReportingLevel::Low < rl) {
      LOG(INFO) << KBase::getFormattedString("Expected EU improvement for %2i of %+.4E \n", h, du);
      if (ReportingLevel::Medium < rl) {
        LOG(INFO) << KBase::getFormattedString("  vBest = %+.6f \n", vBest);
        LOG(INFO) << KBase::getFormattedString("  eu0(%i, 0) for %i = %+.6f \n", h, h, eu0(h,0));
        LOG(INFO) << KBase::getFormattedString("  du = %+.6f \n", du);
      }
    }
    // Logically, du should always be non-negative, as GHC never returns a worse value than the starting point.
    // However, actors plan on the assumption that all others do not change - yet they do.
    const double eps = 0.0001; //  enough to avoid problems with round-off error
    //assert(-eps <= du);
    if (-eps > du) {
      throw KException("EState<PT>::doSUSN: round off error in du making du negative");
    }
    return;
  };
  // end of newPosFn

  bool parP = KBase::testMultiThreadSQLite(false, rl);

  // concurrent execution works, but mixes up the printed log.
  // So we disable it if reporting is desired.
  parP = parP && (rl <= ReportingLevel::Low);
  if (ReportingLevel::Silent < rl) {
    if (parP) {
      LOG(INFO) << "Will continue with multi-threaded execution";
    }
    else {
      LOG(INFO) << "Will continue with single-threaded execution";
    }
  }

  // Each actor, h, finds the position which maximizes their EU in this situation.
  if (parP) {
    KBase::groupThreads(newPosFn, 0, numA - 1);
  }
  else {
    for (unsigned int h = 0; h < numA; h++) {
      newPosFn(h);
    }
  }
  //assert(nullptr != s2);
  if (nullptr == s2) {
    throw KException("EState<PT>::doSUSN: s2 is a null pointer");
  }
  //assert(numP == s2->pstns.size());
  if (numP != s2->pstns.size()) {
    throw KException("EState<PT>::doSUSN: Number of positions is not matching with s2's positions");
  }
  //assert(numA == s2->model->numAct);
  if (numA != s2->model->numAct) {
    throw KException("EState<PT>::doSUSN: actor count mismatched");
  }

  //for (auto p : s2->pstns)
  //{
  //  assert(nullptr != p);
  //}
  s2->setUENdx();
  return s2;
}
// end of doSUSN




template <class PT>
EState<PT>* EState<PT>::stepBCN() {
  LOG(INFO) << "State number " << model->history.size() - 1;
  if ((0 == uIndices.size()) || (0 == eIndices.size())) {
    setUENdx();
  }
  setAUtil(-1, ReportingLevel::Medium);
  show();

  auto s2 = doBCN(ReportingLevel::Low);

  s2->step = [s2]() {
    return s2->stepBCN();
  };
  return s2;
}

template <class PT>
EState<PT>* EState<PT>::doBCN(ReportingLevel rl) const {
  EState<PT>* s2 = nullptr;

  LOG(INFO) << "EState<PT>::doBCN not yet implemented";
  // do something

  //assert (s2 != nullptr);
  if (nullptr == s2) {
    throw KException("EState<PT>::doBCN: s2 is a null pointer");
  }
  return s2;
}
// end of doBCN




template <class PT>
EState<PT>* EState<PT>::stepMCN() {
  LOG(INFO) << "State number " << model->history.size() - 1;
  if ((0 == uIndices.size()) || (0 == eIndices.size())) {
    setUENdx();
  }
  setAUtil(-1, ReportingLevel::Low);
  show();

  auto s2 = doMCN(ReportingLevel::Low);

  s2->step = [s2]() {
    return s2->stepMCN();
  };
  return s2;
}

template <class PT>
EState<PT>* EState<PT>::doMCN(ReportingLevel rl) const {
  const unsigned int numA = eMod->numAct;
  const unsigned int na2 = eMod->actrs.size();
  //assert(numA == na2);
  if (numA != na2) {
    throw KException("EState<PT>::doMCN: actor count in error");
  }
  const unsigned int numU = uIndices.size();
  //assert((0 < numU) && (numU <= numA));
  if ((0 >= numU) || (numU > numA)) {
    throw KException("EState<PT>::doMCN: numU is not within range");
  }
  //assert(numA == eIndices.size());
  if (numA != eIndices.size()) {
    throw KException("EState<PT>::doMCN: eIndices should have size equal to count of actors");
  }

  const auto vpm = eMod->vpm; // get the 'victory probability model'
  const unsigned int numP = pstns.size();

  // get matrix of utilities for unique positions
  auto uUnique = uMatH(0);

  // Get expected-utility col-vector, one entry for each actor,
  // in the current state, with only unique postion utilities
  const auto eu0 = expUtilMat(rl, numA, numP, vpm, uUnique);

  if (ReportingLevel::Low < rl) {
    LOG(INFO) << "--------------------------------------- ";
    LOG(INFO) << "Assessing utility of actual state to all actors ";
    for (unsigned int h = 0; h < numA; h++)
    {
      LOG(INFO) << KBase::getFormattedString("%3u , %.5f \n", h, eu0(h, 0));
    }
    LOG(INFO) << "Positions in this state: ";
    show();

    LOG(INFO) << KBase::getFormattedString("Out of %u positions, %u were unique, with these indices: ", numA, numU);
    for (auto i : uIndices)
    {
      LOG(INFO) << KBase::getFormattedString("%2i ", i);
    }
  }

  auto clearPstns = [numA] (EState<PT>* est) {
    // Amit changed the constructor of ALL Models to (sometimes) pre-allocate data.
    // Sometimes it pre-allocates to the right size, filled with nullptr.
    // Sometimes it pre-allocates to size zero.
    //assert((0 == est->pstns.size()) || (numA == est->pstns.size()));
    if ((0 != est->pstns.size()) && (numA != est->pstns.size())) {
      throw KException("EState<PT>::doMCN: there must be either no positions or each actor should have positions");
    }
    if (0 == est->pstns.size()) {
      est->pstns.resize(numA);
      for (unsigned int i = 0; i < numA; i++) {
        est->pstns[i] = nullptr;
      }
    }
    if (numA == est->pstns.size()) {
      for (unsigned int i=0; i<numA; i++) {
        if (nullptr != est->pstns[i]) {
          delete est->pstns[i];
          est->pstns[i] = nullptr;
        }
      }
    }
    return;
  };

  // The following code is quite similar to doSUSN.
  // However, it is essentially doing a general hill climbing
  // search, ala GHCSearch<>, except with better control
  // of the logging.

  VUI currNdcs = {};
  currNdcs.resize(numP);
  for (unsigned int i = 0; i < numA; i++) {
    auto ppi = (EPosition<PT>*) (pstns[i]);
    currNdcs[i] = ppi->getIndex();
  }

  vector<VUI> neighbors = {};
  // the 0-neighbor, so we never take something less than the current
  neighbors.push_back(currNdcs);

 // const unsigned int numOpt = eMod->numOptions();
  const unsigned int numSim = eMod->nSim;

  // add 1-neighbors
  for (unsigned int ai = 0; ai < numA; ai++) {
    const VUI simI = similarPol(currNdcs[ai], numSim);
    for (auto hpi : simI) {
      if (hpi != currNdcs[ai]) {
        VUI newN = currNdcs;
        newN[ai] = hpi;
        neighbors.push_back(newN);
      }
    }
  }

  // add 2-neighbors, avoiding equivalent permutations
  for (unsigned int ai = 0; ai < numA; ai++) {
    const VUI simI = similarPol(currNdcs[ai], numSim);
    for (auto hpi : simI) {
      for (unsigned int aj = 0; aj < numA; aj++) {
        const VUI simJ = similarPol(currNdcs[aj], numSim);
        for (auto hpj : simJ) {
          if ((ai < aj) && (hpi != currNdcs[ai]) && (hpj != currNdcs[aj])) {
            VUI newN = currNdcs;
            newN[ai] = hpi;
            newN[aj] = hpj;
            neighbors.push_back(newN);
          }
        }
      }
    }
  }

  // three-neighbors are possible, but prohibitively expensive and not very helpful.

  if (ReportingLevel::Silent < rl) {
    LOG(INFO) << "Creating and ranking "<<neighbors.size() << " neighboring states";
  }
  // create and rank all (!) the neighboring states
  const auto w = eMod->actorWeights(); // a row vector
  const unsigned int numNghbrs = neighbors.size();

  // these two variables collect the results of a parallel search
  // for the neighbor with highest Zeta
  double bestZeta = 0.0; // all real zetas are positive
  unsigned int bestNghbr = 0;
  std::mutex nsEvalMutex;

  // somewhat more explicit than "auto"
  function < EState<PT>* (const VUI& , bool)>
      stateFromVUI = [this, numA, clearPstns] (const VUI& ni, bool autilP) {
    // Note that EState::makeNewEState is a pure virtual method,
    // so what gets called here is the method from derived classes,
    // which can provide the extra structure of a derived class.
    auto ns = makeNewEState();
    clearPstns(ns);
    for (unsigned int j=0; j<numA; j++) {
      auto pij = new EPosition<PT>(eMod, ni[j]);
      //assert (nullptr == ns->pstns[j]);
      if (nullptr != ns->pstns[j]) {
        throw KException("EState<PT>::doMCN: postition of j is a null pointer");
      }
      ns->pstns[j] = pij;
    }
    ns->setUENdx();
    if (autilP) {
      ns->setAUtil(-1, ReportingLevel::Medium);
    }
    return ns;
  };


  function<void(unsigned int)> testNghbr =
      [&neighbors, &w, &nsEvalMutex, &bestZeta, &bestNghbr, rl, numA, numP, this, stateFromVUI]
      (unsigned int i) {
    const auto rlNewEU = ReportingLevel::Silent;
    const VUI ni = neighbors[i];
    auto ns = stateFromVUI(ni, true);
    const unsigned int numUi = ns->uIndices.size();

    auto uMati = ns->uMatH(0);
    //assert(numA == uMati.numR());
    if (numA != uMati.numR()) {
      throw KException("EState<PT>::doMCN: uMati matrix doesn't have rows for each actor");
    }
    //assert (numUi == uMati.numC());uMati
    if (numUi != uMati.numC()) {
      throw KException("EState<PT>::doMCN: uMati matrix has different count for columns");
    }
    auto eui = ns->expUtilMat(rlNewEU, numA, numP, eMod->vpm, uMati); // col-vec
    double zi = dot(trans(w), eui);

    // fast, critical section
    nsEvalMutex.lock();
    //assert (0.0 <= bestZeta);
    if (0.0 > bestZeta) {
      throw KException("EState<PT>::doMCN: bestZeta must be non-negative");
    }
    //assert (0.0 < zi);
    if (0.0 >= zi) {
      throw KException("EState<PT>::doMCN: zi must be positive");
    }
    double delta = (zi - bestZeta)/(zi + bestZeta);
    // Empirically, delta seems to be either at least E-4, or at most 1E-13.
    // So I put the cut-off two orders below "significant".
    const double sigDelta = 1E-6;
    if ((bestZeta < zi) && (delta > sigDelta)) {
      bestZeta = zi;
      bestNghbr = i;
      if (ReportingLevel::Low < rl) {
        LOG(INFO) << KBase::getFormattedString("New best neighbor is %u with z=%.4f (delta=%.2E)\n",
               i, zi, delta);
        KBase::printVUI(ni);
      }
    }
    nsEvalMutex.unlock();
    delete ns;
    ns = nullptr;
    return;
  };
  bool parP = KBase::testMultiThreadSQLite(false, rl);

  // concurrent execution works, but mixes up the printed log.
  // So we disable it if reporting is desired.
  //parP = parP && (rl <= ReportingLevel::Low);

  if (ReportingLevel::Silent < rl) {
    if (parP) {
      LOG(INFO) << "Will continue with multi-threaded execution";
    }
    else {
      LOG(INFO) << "Will continue with single-threaded execution";
    }
  }

  if (parP) {
    KBase::groupThreads(testNghbr, 0, numNghbrs-1);
  }
  else {
    for (unsigned int i=0; i<numNghbrs; i++) {
      testNghbr(i);
    }
  }

  VUI nghbr = neighbors[bestNghbr];
  if (ReportingLevel::Silent < rl) {
    LOG(INFO) << KBase::getFormattedString("Highest zeta is %.5f for state %u: \n", bestZeta, bestNghbr);
    printVUI(nghbr);
  }

  auto s2 = stateFromVUI(nghbr, false);

  return s2;
}
// end of doMCN

template<class PT>
KMatrix EState<PT>::hypExpUtilMat () const {
  KMatrix eu;

  // TODO: fix or delete hypExpUtilMat?
  //assert(false);
  throw KException("EState<PT>::hypExpUtilMat: dummy method");

  return eu;
}

template<class PT>
VUI EState<PT>::powerWeightedSimilarity(const KMatrix& uMat, unsigned int ti, unsigned int nSim) const
{
  const unsigned int numPos = eMod->numOptions();
  const unsigned int numAct = eMod->numAct;

  const auto colI = KBase::vSlice(uMat, ti);
  vector<TDI> vdk = {};
  vdk.resize(numPos);
  for (unsigned int k = 0; k < numPos; k++) {
    double dk = 0.0;
    //const auto colK = KBase::vSlice(uMat, k);
    //assert (numAct == colK.numR());
    for (unsigned int j=0; j<numAct; j++) {
      auto ej = (const KBase::EActor<unsigned int>*)(eMod->actrs[j]);
      double sj = ej->sCap;
      //auto duj = colI(j,0)-colK(j,0);
      const double duj = uMat(j,ti) - uMat(j, k);
      dk = dk + (sj*duj*duj);
    }
    //LOG(INFO) << KBase::getFormattedString("%2u PW %.4f \n", k, dk);
    vdk[k] = TDI(dk, k);
  }

  auto tupleLess = [](TDI t1, TDI t2) {
    const double d1 = get<0>(t1);
    const double d2 = get<0>(t2);
    return (d1 < d2);
  };
  std::sort(vdk.begin(), vdk.end(), tupleLess);

  const unsigned int num = (nSim < numPos) ? nSim : numPos;
  VUI sdk = {};
  sdk.resize(num);
  for (unsigned int i = 0; i < num; i++) {
    unsigned int ki = get<1>(vdk[i]);
    sdk[i] = ki;
  }
  return sdk;
}


// TODO: use pDist instead of the near-duplicate code in expUtilMat
/// Calculate the probability distribution over states from this perspective
template<class PT>
tuple <KMatrix, VUI> EState<PT>::pDist(int persp) const {
  const unsigned int numA = eMod->numAct;
  // for this demo, the number of positions is exactly the number of actors
  const unsigned int numP = numA;

  // get unique indices and their probability
  //assert(0 < uIndices.size()); // should have been set with setUENdx();
  if (0 >= uIndices.size()) {
    throw KException("EState<PT>::pDist: size of uIndices must be positive");
  }
  //auto uNdx2 = uniqueNdx(); // get the indices to unique positions

  const unsigned int numU = uIndices.size();
  //assert(numU <= numP); // might have dropped some duplicates
  if (numU > numP) {
    throw KException("EState<PT>::pDist: numU should not be greater than numP");
  }

  LOG(INFO) << "Number of aUtils: " << aUtil.size();

  /*
    const auto u = aUtil[0]; // all have same beliefs in this demo
    assert ((-1 == persp) || (0 == persp));

    auto uufn = [u, this](unsigned int i, unsigned int j1) {
      return u(i, uIndices[j1]);
    };

    const auto um2 = KMatrix::map(uufn, numA, numU);
    */

  const auto uMat = uMatH(persp);
  //assert(uMat.numR() == numA); // must include all actors
  if (uMat.numR() != numA) { // must include all actors
    throw KException("EState<PT>::pDist: uMat must include all actors");
  }
  //assert(uMat.numC() == numU);
  if (uMat.numC() != numU) {
    throw KException("EState<PT>::pDist: uMat should have numU number of columns");
  }

  // assert (norm(uMat - um2) < 1E-6);
  // cout << "uMatH passed" << endl << flush;

  // vote_k ( i : j )
  auto vkij = [this, uMat](unsigned int k, unsigned int i, unsigned int j) {
    auto ak = (EActor<PT>*)(eMod->actrs[k]);
    auto v_kij = Model::vote(ak->vr, ak->sCap, uMat(k, i), uMat(k, j));
    return v_kij;
  };

  // the following uses exactly the values in the given expected
  // utility matrix, which is usually NOT square
  const auto c = Model::coalitions(vkij, uMat.numR(), uMat.numC());
  const auto ppv = Model::probCE2(eMod->pcem, eMod->vpm, c);
  const auto p = get<0>(ppv); // column
  //const auto pv = get<1>(ppv); // square
  const auto eu = uMat*p; // column
  //assert(numA == eu.numR());
  if (numA != eu.numR()) {
    throw KException("EState<PT>::pDist: size of eu should be equal to the count of actors");
  }
  //assert(1 == eu.numC());
  if (1 != eu.numC()) {
    throw KException("EState<PT>::pDist: eu should be a column vector");
  }
  return tuple <KMatrix, VUI>(p, uIndices);
}


// Given the utility matrix, uMat, calculate the expected utility to each actor,
// as a column-vector. Again, this is from the perspective of whoever developed uMat.
// TODO: remove redunant numP parameter
template <class PT>
KMatrix EState<PT>::expUtilMat  (KBase::ReportingLevel rl,
                                 unsigned int numA,
                                 unsigned int numP,
                                 KBase::VPModel vpm,
                                 const KMatrix & uMat) const
{
  //assert (numA == numP); // one-to-one matching of actors and their positions
  if (numA != numP) { // one-to-one matching of actors and their positions
    throw KException("EState<PT>::expUtilMat: actors and their positions should match one-to-one");
  }

  // BTW, be sure to lambda-bind uMat *after* it is modified.
  //assert(uMat.numR() == numA); // must include all actors
  if (uMat.numR() != numA) {
    throw KException("EState<PT>::expUtilMat: row count of uMat must be equal to count of actors");
  }
  //assert(uMat.numC() <= numP); // might have dropped some duplicates
  if (uMat.numC() > numP) { // might have dropped some duplicates
    throw KException("EState<PT>::expUtilMat: column count of uMat must not be greater than numP");
  }

  auto assertRange = [](const KMatrix& m, unsigned int i, unsigned int j) {
    // due to round-off error, we must have a tolerance factor
    const double tol = 1E-10;
    const double mij = m(i, j);
    const bool okLower = (0.0 <= mij + tol);
    const bool okUpper = (mij <= 1.0 + tol);
    if (!okLower || !okUpper) {
      LOG(INFO) << KBase::getFormattedString("%f  %i  %i  \n", mij, i, j);
    }
    //assert(okLower);
    if (!okLower) {
      throw KException("EState<PT>::expUtilMat: lower limit crossed");
    }
    //assert(okUpper);
    if (!okUpper) {
      throw KException("EState<PT>::expUtilMat: upper limit crossed");
    }
    return;
  };

  auto uRng = [assertRange, uMat](unsigned int i, unsigned int j) {
    assertRange(uMat, i, j);
    return;
  };
  KMatrix::mapV(uRng, uMat.numR(), uMat.numC());

  // vote_k(i:j)
  auto vkij = [this, uMat](unsigned int k, unsigned int i, unsigned int j) {
    auto ak = (const EActor<PT>*)(eMod->actrs[k]);
    auto v_kij = Model::vote(ak->vr, ak->sCap, uMat(k, i), uMat(k, j));
    return v_kij;
  };

  // the following uses exactly the values in the given expected
  // utility matrix, which is usually NOT square
  const auto c = Model::coalitions(vkij, uMat.numR(), uMat.numC());
  // use whatever 'vpm' was supplied
  const auto ppv = Model::probCE2(eMod->pcem, vpm, c);
  const auto p = get<0>(ppv); // column
  const auto pv = get<1>(ppv); // square
  const auto eu = uMat*p; // column
  //assert(numA == eu.numR());
  if (numA != eu.numR()) {
    throw KException("EState<PT>::expUtilMat: row count of eu must be equal to count of actors");
  }
  //assert(1 == eu.numC());
  if (1 != eu.numC()) {
    throw KException("EState<PT>::expUtilMat: eu must be a column vector");
  }
  auto euRng = [assertRange, eu](unsigned int i, unsigned int j) {
    assertRange(eu, i, j);
    return;
  };
  KMatrix::mapV(euRng, eu.numR(), eu.numC());

  if (ReportingLevel::Low < rl) {
    LOG(INFO) << KBase::getFormattedString("Util matrix is %u x %u \n", uMat.numR(), uMat.numC());
    LOG(INFO) << "Assessing EU from util matrix: ";
    uMat.mPrintf(" %.6f ");

    LOG(INFO) << "Coalition strength matrix";
    c.mPrintf(" %12.6f ");

    LOG(INFO) << "Probability Opt_i > Opt_j";
    pv.mPrintf(" %.6f ");

    LOG(INFO) << "Probability Opt_i";
    p.mPrintf(" %.6f ");

    LOG(INFO) << "Expected utility to actors: ";
    eu.mPrintf(" %.6f ");
  }

  return eu;
};
// end of expUtilMat





} // end of namespace

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
