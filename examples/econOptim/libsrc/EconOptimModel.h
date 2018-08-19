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

#ifndef ECONOPTIM_MODEL_H
#define ECONOPTIM_MODEL_H

#include <assert.h>
#include <chrono>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <tuple>
#include <vector>

#include "kutils.h"
#include "prng.h"
#include "kmatrix.h"
#include "gaopt.h"
#include "hcsearch.h"
#include "kmodel.h"

namespace DemoEconOptim {
// namespace to which KBase has no access

using std::string;
using std::tuple;
using std::vector;

using KBase::KMatrix;
using KBase::PRNG;
using KBase::ReportingLevel;
using KBase::VUI;

using KBase::Actor;
using KBase::Position;
using KBase::State;
using KBase::Model;
using KBase::VotingRule;

using KBase::VctrPstn;

class EconoptimModel;
class EconoptimState;
class EconoptimActor;

// -------------------------------------------------

//EconoptimModel* demoSetup(unsigned int numFctr, unsigned int numCGrp, unsigned int numSect, uint64_t s, PRNG* rng);
//void demoEUEcon(uint64_t s, PRNG* rng);
//void demoMaxEcon(uint64_t s, PRNG* rng);
// -------------------------------------------------
// trivial economic actor, with fixed attributes and a linear utility function
class EconoptimActor : public Actor {
public:
  EconoptimActor(string n, string d, EconoptimModel* em, unsigned int id);
  ~EconoptimActor();

  // interfaces to be provided
  double vote(unsigned int est,unsigned int p1, unsigned int p2, const State* st) const;
  virtual double vote(const Position * ap1, const Position * ap2) const;
  double posUtil(const Position * ap1) const;

  const EconoptimModel * eMod = nullptr;
  unsigned int idNum = 0;
  // econ model returns results as a row-vector of [factor | sector],
  // so it needs to know its ID to specify which column.


  // These actors have an idea of how good each policy is
  // for them, as determined by their gradient vector, but
  // they have no ideal position: utility = dot(grad, position),
  // so they want as much of the goods as they can get - and less of the bads.
  //
  // They have different capabilities to lobby for things they
  // care about, as expressed in their capability vector.
  //
  // With scalar cap, their proportional vote for P1 over P2 would be the following:
  //
  // vk(P1:p2) = c * ( dot(g,P1) - dot(g,P2))
  //           = c * sum_k (gk*p1k - gk*p2k)
  //
  // This would be quite reasonable behavior, but it would not exerise the SW interfaces.
  //
  // We use vector capabilities, so that they can only react to each component
  // within their capability limit. This requires estimating separately
  // the value of each provision, in the context of the whole proposal.
  // They then exert influence on each provision separately, which sums
  // to their total "vote".
  //
  // Note that this voting rule can not be represented as a function of the
  // difference in overall utilities. If two policies differ only on a dimension for
  // which they lack capability, they exert zero influence (vote=0) even though
  // there is a big utility difference. If two other policies differ by just as
  // much utility on a dimension for which they have ample capability, they may
  // exert strong influence. Same (delta-Util) input, but different (influence)
  // outputs: no function can do that.
  //
  // Note well: this voting scheme can do stupid things. If u(ep1)>u(ep2) overall,
  // but the only dimension on which the actor happens to have influence is one
  // where ep2 is better, then the actor will oppose ep1 overall.
  // Maybe this would be a good model of actors who just do what they can
  // on each aspect separately. Such actors might well oppose (on narrow grounds)
  // a policy which would in fact benefit them (on broad grounds).
  // Hence, I flip the sign in those cases. Either way, it's just a demo.
  //
  // The point is NOT to demonstrate a realistic model of either economic
  // bargaining or vector-capabilities, because this one certainly is not,
  // but to show that we are not locked into either SMP or scalar-capabilities.
  //
  KMatrix vCap = KMatrix();
  VotingRule vr = VotingRule::Proportional; // plausible default


  // with the Cubic voting rule, you can get very skewed results
  // unless all actors have their utilities in a standard [0,1]
  // interval. By construction, the minimum possible (and achievable)
  // is zero, but we still need to work out the scale factor
  // for this actor to convert "raw" utilities to [0,1];
  double minS = 0;
  double refS = 0.5;
  double refU = 0.5;
  double maxS = 1;

  void randomize(PRNG* rng);
  void setShareUtilScale(const KMatrix & runs);
  double shareToUtil(double gdpShare) const;
};


class EconoptimState : public State {
public:
  explicit EconoptimState(EconoptimModel * em);
  ~EconoptimState();
  const EconoptimModel * eMod = nullptr; // saves a lot of type-casting


  // use the parameters of your state to compute the relative probability of each actor's position
  virtual tuple <KMatrix, VUI>  pDist(int persp) const ;

  EconoptimState* stepSUSN();

  // i's estimate of the GDP
  double estGDP(unsigned int i, unsigned int j);

protected:
  EconoptimState * doSUSN(ReportingLevel rl) const;
  virtual bool equivNdx(unsigned int i, unsigned int j) const;

  void setAllAUtil(ReportingLevel rl);

private:
};

// Each actor's  utility function assigns a utility of 0 to the lowest GDP
// share they could plausibly get, and 1 to the highest they could plausibly get.
// This can be done simply by generating a thousand revenue-neutral but otherwise random
// taxe vectors, and observing the highest and lowest GDP shares for each actor.
// It is not a complete sample, so you will sometimes see utilities slightly outside the
// [0,1] range. This is not a problem, as all actors are basically on the same scale.
//
// Of course, the zero-tax vector is feasible, and it is taken as the reference / base case
// for every actor. I'd like to assign a utility of 0.6 for each actor to the GDP share they
// get in the base case, with linear interpolation/extrapolation on either side of it.
// This makes all the actors somewhat risk-averse.
class EconoptimModel : public Model {
  friend class EconoptimActor;
  friend class EconoptimState;
public:
  // JAH 20160711 added rng seed 20160730 JAH added sql flags
  explicit EconoptimModel( string d="", uint64_t s=KBase::dSeed, vector<bool> f= {});
  virtual ~EconoptimModel();

  // synthesize random, but not-ridiculous, data for a base year
  tuple<KMatrix, KMatrix, KMatrix, KMatrix> makeBaseYear(unsigned int numF, unsigned int numCG, unsigned int numS, PRNG* rng);

  // given base year data (real or synthetic), construct a plausible I/O model from that data.
  void makeIOModel(const KMatrix & trns, const KMatrix & rev, const KMatrix & xprt, const KMatrix & cons, PRNG* rng);

  // JAH 20160809 added
  void prepModel(unsigned int numFac, unsigned int numCon, unsigned int numSec, KMatrix & xprt, KMatrix & cons,
    KMatrix & elast, KMatrix & trns, KMatrix & rev, KBase::KMatrix &expnd, KBase::KMatrix &Bmat);

  // Estimate vector of export demands, given a vector tau of tax/subsidy rates.
  // assuming base year prices of 1.0
  KMatrix xprtDemand(const KBase::KMatrix& tau) const;

  // make a revenue-neutral but otherwise random tax vector
  KMatrix randomFTax(PRNG* rng);

  // Given an arbitrary tax/subsidy vector, search for the nearest which is revenue-neutral.
  // It may flip the signs of some components. It may throw KException, so be prepared.
  KMatrix makeFTax(const KBase::KMatrix& tax) const;

  double infsDegree(const KMatrix & tax) const;

  // Return row-vector of expected factor shares (e.g. L of them, e.g. labor), then expected sector shares (N of them).
  // As they are estimated by different economic models, sum of factor VA will usually NOT match sum of sector VA
  KMatrix vaShares(const KMatrix & tax, bool normalizeSharesP) const;

  KMatrix monteCarloShares(unsigned int nRuns, KBase::PRNG* rng);

  // considering all the positions as vectors, return the distance between states.
  static double stateDist (const EconoptimState* s1 , const EconoptimState* s2 );

  /// how close together positions must be to be considered equivalent
  double posTol = 1E-5;

  // Go back through the state history and print every actor's estimate of GDP from every other actor's policy-position
  void printEstGDP();

protected:
  unsigned int L = 0; // factors of production
  unsigned int M = 0; // consumption groups
  unsigned int N = 0; // economic sectors
  // a subsidy of 0.5 cuts price in half, a subsidy of 0.90 cuts it by a factor 10, and a subsidy of 1 makes it free.
  // Hence 0 <= maxSub < 1
  double maxSub = 0.5;

  // a tax of 0.5 raises the price 50%, a tax of 9 raises by a factor of 10.
  // Hence 0 <= maxTax
  double maxTax = 0.5;

  // x0: column-vecttor of base year (zero tax) exports
  KMatrix  x0 = KMatrix();

  // eps: column-vector of price-elasticities of export
  KMatrix  eps = KMatrix();

  // aL: Leontief matrix, inv(I-A), taking no account of future growth and investment
  //     aL * X = qClm
  //     used by factors to estimate impact.
  KMatrix  aL = KMatrix();

  // bL: Leontief matrix, taking into account future growth and investment
  //     bL * X == betaQX
  //     used by sectors to estimate impact
  KMatrix  bL = KMatrix();

  // rho: matrix mapping output to factor VA/budgets: budgetL == rho x qClm:
  //      with dimensions [L, 1] = [L,N] * [N,1]
  KMatrix  rho = KMatrix();

  // vas: row-vector of fractional value-added shares per sector (i.e. rho added over factors)
  KMatrix  vas = KMatrix();

private:

};


};
// -------------------------------------------------
#endif

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
