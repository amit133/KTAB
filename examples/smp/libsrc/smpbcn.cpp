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
//
// Demonstrate a very basic, but highly parameterizable, Spatial Model of Politics.
//
// --------------------------------------------

#include "smp.h"
#include <QSqlQuery>
#include <QVariant>
#include <QSqlError>

namespace SMPLib {
using std::cout;
using std::endl;
using std::flush;
using std::function;
using std::get;
using std::string;
using std::map;

using KBase::PRNG;
using KBase::KMatrix;
using KBase::KException;
using KBase::Actor;
using KBase::Model;
using KBase::Position;
using KBase::VctrPstn;
using KBase::BigRAdjust;
using KBase::BigRRange;
using KBase::VPModel;
using KBase::State;
using KBase::StateTransMode;
using KBase::VotingRule;
using KBase::PCEModel;
using KBase::ReportingLevel;
using KBase::nameFromEnum;

// --------------------------------------------
uint64_t BargainSMP::highestBargainID = 1000;

// big enough buffer to build all desired SQLite statements
const unsigned int sqlBuffSize = 250;

// --------------------------------------------
ostream& operator<< (ostream& os, const SMPBargnModel& bMod) {
  string s = nameFromEnum<SMPBargnModel>(bMod, SMPBargnModelNames);
  os << s;
  return os;
}

ostream& operator<< (ostream& os, const InterVecBrgn& ivb) {
  string s = nameFromEnum<InterVecBrgn>(ivb, InterVecBrgnNames);
  os << s;
  return os;
}

// --------------------------------------------

BargainSMP::BargainSMP(const SMPActor* ai, const SMPActor* ar, const VctrPstn & pi, const VctrPstn & pr) {
  assert(nullptr != ai);
  assert(nullptr != ar);
  actInit = ai;
  actRcvr = ar;
  posInit = pi;
  posRcvr = pr;
  myBargainID = BargainSMP::highestBargainID++;
}

BargainSMP::~BargainSMP() {
  actInit = nullptr;
  actRcvr = nullptr;
  posInit = VctrPstn(KMatrix(0, 0));
  posRcvr = VctrPstn(KMatrix(0, 0));
  auto mbid = myBargainID;
  myBargainID = 0;
}


uint64_t BargainSMP::getID() const {
  return myBargainID;
}

// --------------------------------------------
void recordUtility(unsigned int i /* actor id */, const SMPState* obj) {
  obj->calcUtils(i);
}
  
// --------------------------------------------
/*
 * Calculate all the utilities and record in database. utitlity for (i,i,i,j)
 * combination is getting calculated and recorded in a separate method
 */
void SMPState::calcUtils(unsigned int i ) const { // i == actor id 
  const unsigned int na = model->numAct;
  const bool recordTmpSQLP = true;  // Record this in SQLite
  auto pFn = [this, recordTmpSQLP](unsigned int h, unsigned int k, unsigned int i, unsigned int j) {
    probEduChlg(h, k, i, j, recordTmpSQLP); // H's estimate of the effect on K of I->J
  };

  auto getUtils = [this, na, pFn, i](unsigned int j) {
    if( i != j ) {
      for (unsigned int m = 0; m < na; m++) {
        if(m == i) {
          /* Note:
           * pFn(i, i, i, j) = pFn(m, m, i, j) = pFn(m,i,i,j)
           * pFn(i, i, i, j) would be calculated in a different method
           * bestChallengeUtils in the main thread so that the required
           * utilities are ready before the call to bestChallenge()
           */
          pFn(m, j, i, j); // m's estimate of the effect on J of I->J
        } else if (m == j) {
          pFn(m, i, i, j); // m's estimate of the effect on I of I->J
          pFn(m, m, i, j); // pFn(m,j,i,j) would produce a duplicate result
        } else {
          pFn(m, i, i, j); // m's estimate of the effect on I of I->J
          pFn(m, m, i, j); // m's estimate of the effect on I of I->J
          pFn(m, j, i, j); // m's estimate of the effect on J of I->J
        }
      }
    }
  };

  for (unsigned int j = 0; j < bestJ; j++) {
    getUtils(j);
  }

  // Some computes for best J need to be avoided here to prevent duplicate entries in DB
  if( i != bestJ ) {
    for (unsigned int m = 0; m < na; m++) {
      if((m != i) && (m != bestJ)) {
        pFn(m, i, i, bestJ); // m's estimate of the effect on I of I->J
        pFn(m, m, i, bestJ); // m's estimate of the effect on I of I->J
        pFn(m, bestJ, i, bestJ); // m's estimate of the effect on J of I->J
      }
    }
  }

  for (unsigned int j = bestJ+1; j < na; j++) {
    getUtils(j);
  }
}

// --------------------------------------------
void SMPState::bestChallengeUtils(unsigned int i /* actor id */) const {
  const unsigned int na = model->numAct;
  const bool recordTmpSQLP = true;  // Record this in SQLite
  eduChlgsJ eduJ;
  for (unsigned int j = 0; j < na; j++) {
    if( i != j ) {
        eduJ[j] = probEduChlg(i, i, i, j, recordTmpSQLP);
    }
  }
  eduChlgsIJ[i] = eduJ;
}

// --------------------------------------------
vector<double> SMPState::calcVotes(KMatrix w, KMatrix u, int k) const
{
	auto vfn = [&w, &u](VotingRule vr, unsigned int k, unsigned int i, unsigned int j) {
		double vkij = Model::vote(vr, w(0, k), u(k, i), u(k, j));
		return vkij;
	};
	auto li = u.numC();  
	auto lj = w.numC();
	vector<double> votes = {};
	for (unsigned int i = 0; i < li; i++) //li is the number of bargains
	{
		for (unsigned int j = 0; j < i; j++)
		{
			auto vr = ((const SMPActor*)(model->actrs[k]))->vr;
			double vkij = vfn(vr, k, i, j);
			votes.push_back(vkij);
			
		}
	}
	return votes;
}

SMPState* SMPState::doBCN() const {
  const bool recordBargainingP = true;
  auto brgns = vector< vector < BargainSMP* > >();
  const unsigned int na = model->numAct;
  brgns.resize(na);
  for (unsigned int i = 0; i < na; i++) {
    brgns[i] = vector<BargainSMP*>();
  }

  const unsigned int t = myTurn();

  const VPModel vpmBargains = model->vpm;
  const PCEModel pcemBargains = model->pcem;
  const StateTransMode stm = model->stm;

  auto smod = (const SMPModel*)model;
  const VotingRule vrBargains = smod->vrCltn;
  const InterVecBrgn ivb = smod->ivBrgn;
  const SMPBargnModel bMod = smod->brgnMod;

  // TODO: use groupThreads on *this* loop for high-level parallelism
  for (unsigned int i = 0; i < na; i++) {
    auto ai = ((const SMPActor*)(model->actrs[i]));
    auto posI = ((const VctrPstn*)pstns[i]);

    auto sqBrgnI = new BargainSMP(ai, ai, *posI, *posI);
    brgns[i].push_back(sqBrgnI);

    // before we can log this bargain, we need to get the group ID for this table
    // so then we can get the flag to populate the table or not
    // note the implicit assumption that there will never be 43+ groups :-)
    unsigned int grpID = 42;
    for (unsigned int t = 0; t<model->KTables.size(); t++)
    {
        if (model->KTables[t]->tabName == "Bargn")
        {
            grpID = model->KTables[t]->tabGrpID;
            break;
        }
    }
    // be sure that it found this table
    assert(grpID != 42);
    assert(grpID < model->sqlFlags.size());

    if (model->sqlFlags[grpID])
    {
      model->sqlBargainEntries(t, sqBrgnI->getID(), i, i, 0);
    }

    bestChallengeUtils(i);

    auto chlgI = bestChallenge(i);
    const double bestEU = get<2>(chlgI);
    if (0 < bestEU) {
      bestJ = get<0>(chlgI); //
      const double piiJ = get<1>(chlgI); // i's estimate of probability i defeats j
      assert(0 <= bestJ);
      const unsigned int j = bestJ; // for consistency in code below

      const unsigned int t = myTurn(); // need to be on model's history list
      printf("In turn %i actor %u has most advantageous target %u worth %.3f\n", t, i, j, bestEU);

      auto aj = ((const SMPActor*)(model->actrs[j]));
      auto posJ = ((const VctrPstn*)pstns[j]);

      // Look for counter-intuitive cases
      if (piiJ < 0.5) {
        cout << "turn " << t << " , ";
        cout << "i " << i << " , ";
        cout << "j " << j << " , ";
        cout << "bestEU worth " << bestEU << " , ";
        cout << "piiJ " << piiJ << endl;
        cout << endl << flush; // get it printed
        //assert(0.5 <= piiJ);
      }

      std::thread thr(recordUtility, i, this);

      // make the variables local to lexical scope of this block.
      // for testing, calculate and print out a block of data showing each's perspective
      bool recordTmpSQLP = true;  // Record this in SQLite
      auto pFn = [this, recordTmpSQLP](unsigned int h, unsigned int k, unsigned int i, unsigned int j) {
        auto est = probEduChlg(h, k, i, j, recordTmpSQLP); // H's estimate of the effect on K of I->J
        double phij = get<0>(est);
        double edu_hk_ij = get<1>(est);
        printf("Est by %2u of prob %.4f that [%2u>%2u], with expected gain to %2u of %+.4f \n",
               h, phij, i, j, k, edu_hk_ij);
        return est;
      };

      // I's estimate of the effect on I of I->J
      printf("Est by %2u of prob %.4f that [%2u>%2u], with expected gain to %2u of %+.4f \n",
             i, piiJ, i, j, i, get<2>(chlgI));

      pFn(i, j, i, j); // I's estimate of the effect on J of I->J

      auto Vjij = pFn(j, i, i, j); // J's estimate of the effect on I of I->J

      pFn(j, j, i, j); // J's estimate of the effect on J of I->J

      // interpolate a bargain from I's perspective
      BargainSMP* brgnIIJ = SMPActor::interpolateBrgn(ai, aj, posI, posJ, piiJ, 1 - piiJ, ivb);
      const int nai = model->actrNdx(brgnIIJ->actInit);
      const int naj = model->actrNdx(brgnIIJ->actRcvr);
      // verify that identities match up as expected
      assert(nai == i);
      assert(naj == j);

      // interpolate a bargain from targeted J's perspective
      double pjiJ = get<1>(Vjij); // j's estimate of the probability that i defeats j
      BargainSMP* brgnJIJ = SMPActor::interpolateBrgn(ai, aj, posI, posJ, pjiJ, 1 - pjiJ, ivb);

      // calcluate weights as capability times salience
      double sci = brgnIIJ->actInit->sCap;
      double svi = sum(brgnIIJ->actInit->vSal);
      double wi = sci*svi;
      double scj = brgnJIJ->actInit->sCap;
      double svj = sum(brgnIIJ->actRcvr->vSal);
      double wj = scj*svj;

      // create a new bargain whose positions are the weighted averages
      auto bpi = VctrPstn((wi*brgnIIJ->posInit + wj*brgnJIJ->posInit) / (wi + wj));
      auto bpj = VctrPstn((wi*brgnIIJ->posRcvr + wj*brgnJIJ->posRcvr) / (wi + wj));
      BargainSMP *brgnIJ = new  BargainSMP(brgnIIJ->actInit, brgnIIJ->actRcvr, bpi, bpj);

      printf("\n");
      printf("Bargain ");
      showOneBargain(brgnIIJ);
      printf(" from %2u's perspective (brgnIIJ) \n", i);
      printf("  %2u proposes %2u adopt: ", i, i);
      (KBase::trans(brgnIIJ->posInit) * 100.0).mPrintf(" %.3f "); // print on the scale of [0,100]


      printf("  %2u proposes %2u adopt: ", i, j);
      (KBase::trans(brgnIIJ->posRcvr) * 100.0).mPrintf(" %.3f "); // print on the scale of [0,100]
      printf("\n");
      printf("Bargain  ");
      showOneBargain(brgnJIJ);
      printf(" from %2u's perspective (brgnJIJ) \n", j);
      printf("  %2u proposes %2u adopt: ", j, i);

      (KBase::trans(brgnJIJ->posInit) * 100.0).mPrintf(" %.3f "); // print on the scale of [0,100]
      printf("  %2u proposes %2u adopt: ", j, j);
      (KBase::trans(brgnJIJ->posRcvr) * 100.0).mPrintf(" %.3f "); // print on the scale of [0,100]
      //Brgn table base entries


      printf("\n");
      printf("Power-weighted compromise  ");
      showOneBargain(brgnIJ);
      printf(" bargain (brgnIJ) \n");
      printf("  compromise proposes %2u adopt: ", i);
      (KBase::trans(brgnIJ->posInit) * 100.0).mPrintf(" %.3f "); // print on the scale of [0,100]
      printf("  compromise proposes %2u adopt: ", j);
      (KBase::trans(brgnIJ->posRcvr) * 100.0).mPrintf(" %.3f "); // print on the scale of [0,100]
      printf("\n");


      // TODO: make one-perspective an option.
      // For now, emulate it by swapping
      //auto tIJ = brgnIJ;
      //auto tIIJ = brgnIIJ;
      //brgnIJ = tIIJ;
      //brgnIIJ = tIJ;

      cout << "Using "<<bMod<<" to form proposed bargains" << endl;
      switch (bMod) {
      case SMPBargnModel::InitOnlyInterpSMPBM:
        // record the only one used into SQLite JAH 20160802 use the flag
        if(model->sqlFlags[grpID])
        {
          model->sqlBargainEntries(t, brgnIIJ->getID(), i, j, bestEU);
        }
        if(model->sqlFlags[3])
        {          
          model->sqlBargainCoords(t, brgnIIJ->getID(), brgnIIJ->posInit, brgnIIJ->posRcvr);
        }
        // record this one onto BOTH the initiator and receiver queues
        brgns[i].push_back(brgnIIJ); // initiator's copy, delete only it later
        brgns[j].push_back(brgnIIJ); // receiver's copy, just null it out later
        // clean up unused
        delete brgnIJ;
        brgnIJ = nullptr;
        delete brgnJIJ;
        brgnJIJ = nullptr;
        break;


      case SMPBargnModel::InitRcvrInterpSMPBM:
        // record the pair used into SQLite JAH 20160802 use the flag
        if(model->sqlFlags[grpID])
        {
          model->sqlBargainEntries(t, brgnIIJ->getID(), i, j, bestEU);
          model->sqlBargainEntries(t, brgnJIJ->getID(), i, j, bestEU);
        }
        if(model->sqlFlags[3])
        {
          model->sqlBargainCoords(t, brgnIIJ->getID(), brgnIIJ->posInit, brgnIIJ->posRcvr);
          model->sqlBargainCoords(t, brgnJIJ->getID(), brgnJIJ->posInit, brgnJIJ->posRcvr);
        }
        // record these both onto BOTH the initiator and receiver queues
        brgns[i].push_back(brgnIIJ); // initiator's copy, delete only it later
        brgns[i].push_back(brgnJIJ); // initiator's copy, delete only it later
        brgns[j].push_back(brgnIIJ); // receiver's copy, just null it out later
        brgns[j].push_back(brgnJIJ); // receiver's copy, just null it out later
        // clean up unused
        delete brgnIJ;
        brgnIJ = nullptr;
        break;


      case SMPBargnModel::PWCompInterpSMPBM:
        // record the only one used into SQLite JAH 20160802 use the flag
        if(model->sqlFlags[grpID])
        {
          model->sqlBargainEntries(t, brgnIJ->getID(), i, j, bestEU);
        }
        if(model->sqlFlags[3])
        {
          model->sqlBargainCoords(t, brgnIJ->getID(), brgnIJ->posInit, brgnIJ->posRcvr);
        }
        // record this one onto BOTH the initiator and receiver queues
        brgns[i].push_back(brgnIJ); // initiator's copy, delete only it later
        brgns[j].push_back(brgnIJ); // receiver's copy, just null it out later
        // clean up unused
        delete brgnIIJ;
        brgnIIJ = nullptr;
        delete brgnJIJ;
        brgnJIJ = nullptr;
        break;

      default:
        cout << "SMPState::doBCN unrecognized SMPBargnModel" << endl << flush;
        assert(false);
      }

      thr.join();
    }
    else {
      printf("Actor %u has no advantageous targets \n", i);
    }
  }

  if (model->sqlFlags[2]) {
    recordProbEduChlg();
  }

  cout << endl << "Bargains to be resolved" << endl << flush;
  showBargains(brgns);

  auto w = actrCaps();
  cout << "w:" << endl;
  w.mPrintf(" %6.2f ");

  auto ndxMaxProb = [](const KMatrix & cv) {
    const double pTol = 1E-8;
    assert(fabs(KBase::sum(cv) - 1.0) < pTol);
    assert(0 < cv.numR());
    assert(1 == cv.numC());
    auto ndxIJ = ndxMaxAbs(cv);
    unsigned int iMax = get<0>(ndxIJ);
    return iMax;
  };

  // what is the utility to actor nai of the state resulting after
  // the nbj-th bargain of the k-th actor is implemented?
  auto brgnUtil = [this, brgns](unsigned int nk, unsigned int nai, unsigned int nbj) {
    const unsigned int na = model->numAct;
    BargainSMP * b = brgns[nk][nbj];
    assert(nullptr != b);
    double uAvrg = 0.0;

    if (b->actInit == b->actRcvr) { // SQ bargain
      uAvrg = 0.0;
      for (unsigned int n = 0; n < na; n++) {
        // nai's estimate of the utility to nai of position n, i.e. the true value
        uAvrg = uAvrg + aUtil[nai](nai, n);
      }
    }

    else { // all positions unchanged, except Init and Rcvr
      uAvrg = 0.0;
      auto ndxInit = model->actrNdx(b->actInit);
      assert((0 <= ndxInit) && (ndxInit < na)); // must find it
      double uPosInit = ((SMPActor*)(model->actrs[nai]))->posUtil(&(b->posInit), this);
      uAvrg = uAvrg + uPosInit;

      auto ndxRcvr = model->actrNdx(b->actRcvr);
      assert((0 <= ndxRcvr) && (ndxRcvr < na)); // must find it
      double uPosRcvr = ((SMPActor*)(model->actrs[nai]))->posUtil(&(b->posRcvr), this);
      uAvrg = uAvrg + uPosRcvr;

      for (unsigned int n = 0; n < na; n++) {
        if ((ndxInit != n) && (ndxRcvr != n)) {
          // again, nai's estimate of the utility to nai of position n, i.e. the true value
          uAvrg = uAvrg + aUtil[nai](nai, n);
        }
      }
    }

    uAvrg = uAvrg / na;

    assert(0.0 < uAvrg); // none negative, at least own is positive
    assert(uAvrg <= 1.0); // can not all be over 1.0
    return uAvrg;
  };
  // end of λ-fn


  // The key is to build the usual matrix of U_ai (Brgn_m) for all bargains in brgns[k],
  // making sure to divide the sum of the utilities of positions by 1/N
  // so 0 <= Util(state after Brgn_m) <= 1, then do the standard scalarPCE for bargains involving k.

  SMPState* s2 = new SMPState(model);

  map<unsigned int, KBase::KMatrix> actorBargains;
  map<unsigned int, unsigned int> actorMaxBrgNdx;

  // This loop would be another good place for high-level parallelism
  for (unsigned int k = 0; k < na; k++) {
    unsigned int nb = brgns[k].size();
    auto buk = [brgnUtil, k](unsigned int nai, unsigned int nbj) {
      return brgnUtil(k, nai, nbj);
    };
    auto u_im = KMatrix::map(buk, na, nb);

    cout << "u_im: " << endl;
    u_im.mPrintf(" %.5f ");

    cout << "Doing scalarPCE for the " << nb << " bargains of actor " << k << " ... " << flush;
    auto p = Model::scalarPCE(na, nb, w, u_im, vrBargains, vpmBargains, pcemBargains, ReportingLevel::Medium);
    assert(nb == p.numR());
    assert(1 == p.numC());
    actorBargains.insert(map<unsigned int, KBase::KMatrix>::value_type(k, p));

    unsigned int mMax = nb; // indexing actors by i, bargains by m
    switch (stm) {
    case StateTransMode::DeterminsticSTM:
      mMax = ndxMaxProb(p);
      break;
    case StateTransMode::StochasticSTM:
      mMax = model->rng->probSel(p);
      break;
    default:
      throw KException("SMPState::doBCN - unrecognized StateTransMode");
      break;
    }
    // 0 <= mMax assured for uint
    assert(mMax < nb);
    actorMaxBrgNdx.insert(map<unsigned int, unsigned int>::value_type(k, mMax));

    //populate the Bargain Vote & Util tables
    // JAH added sql flag logging control
	if (model->sqlFlags[3])
	{
		auto brgns_k = brgns[k];
		vector< std::tuple<uint64_t, uint64_t>> barginIDsPair_i_j;
		vector<uint64_t> bargnIdsRows = {};
		for (int j = 0; j < nb; j++)
		{
			bargnIdsRows.push_back(brgns[k][j]->getID());
		}
		for (unsigned int brgnFirst = 0; brgnFirst < nb; brgnFirst++)
		{
			for (unsigned int brgnSecond = 0; brgnSecond < brgnFirst; brgnSecond++)
			{
				barginIDsPair_i_j.push_back(tuple<uint64_t, uint64_t>(brgns_k[brgnFirst]->getID(), brgns_k[brgnSecond]->getID()));
			}
		}

		for (unsigned int actor = 0; actor < na; ++actor) {
			auto pv_ij = calcVotes(w, u_im, actor);

			model->sqlBargainVote(t, barginIDsPair_i_j, pv_ij, actor);
		}
		model->sqlBargainUtil(t, bargnIdsRows, u_im);
	}

    // TODO: create a fresh position for k, from the selected bargain mMax.
    VctrPstn * pk = nullptr;
    auto bkm = brgns[k][mMax];
    cout << "Chosen bargain (" << stm << "): " << bkm->getID()
      << " " << mMax + 1 << " out of " << nb << " bargains" << endl;
    auto oldPK = dynamic_cast<VctrPstn *>(pstns[k]);
    if (bkm->actInit == bkm->actRcvr) { // SQ
      pk = new VctrPstn(*oldPK);
    }
    else {
      const unsigned int ndxInit = model->actrNdx(bkm->actInit);
      const unsigned int ndxRcvr = model->actrNdx(bkm->actRcvr);
      if (ndxInit == k) {
        pk = new VctrPstn(bkm->posInit);
      }
      else if (ndxRcvr == k) {
        pk = new VctrPstn(bkm->posRcvr);
      }
      else {
        cout << "SMPState::doBCN: unrecognized actor in bargain" << endl;
        assert(false);
      }

      // If the actor has changed its position, record the bargain id
      for (int dimen = 0; dimen < pk->numR(); dimen++) {
        auto pCoordOld = (*oldPK)(dimen, 0);
        auto pCoord = (*pk)(dimen, 0);
        if (pCoord != pCoordOld) {
          s2->setPosMoverBargain(k, bkm->getID());
        }
      }
    }
    assert(nullptr != pk);

    assert(k == s2->pstns.size());
    s2->pstns.push_back(pk);

    cout << endl << flush;
  }

  if (model->sqlFlags[3])
  {
    // record data so far
    updateBargnTable(brgns, actorBargains, actorMaxBrgNdx);
  }

  // Some bargains are nullptr, and there are two copies of every non-nullptr randomly
  // arranged. If we delete them as we find them, then the second occurance will be corrupted,
  // so the code crashes when it tries to access the memory to see if it matches something
  // already deleted. Hence, we scan them all, building a list of unique bargains,
  // then delete those in order.
  auto uBrgns = vector<BargainSMP*>();

  for (unsigned int i = 0; i < brgns.size(); i++) {
    auto ai = ((const SMPActor*)(model->actrs[i]));
    for (unsigned int j = 0; j < brgns[i].size(); j++) {
      BargainSMP* bij = brgns[i][j];
      if (nullptr != bij) {
        if (ai == bij->actInit) {
          uBrgns.push_back(bij); // this is the initiator's copy, so save it for deletion
        }
      }
      brgns[i][j] = nullptr; // either way, null it out.
    }
  }

  for (auto b : uBrgns) {
    //int aI = model->actrNdx(b->actInit);
    //int aR = model->actrNdx(b->actRcvr);
    //printf("Delete bargain [%2i:%2i] \n", aI, aR);
    delete b;
  }

  // TODO: this really should do all the assessment: ueIndices, rnProb, all U^h_{ij}, raProb
  s2->setUENdx();


  if (0 == accomodate.numC()) { // nothing to copy
    s2->setAccomodate(1.0); // set to identity matrix
  }
  else {
    s2->setAccomodate(accomodate);
  }

  if (0 == ideals.size()) { // nothing to copy
    s2->idealsFromPstns(); // set s2's current ideals to s2's current positions
  }
  else {
    s2->ideals = ideals; // copy s1's old ideals
  }
  s2->newIdeals(); // adjust s2 ideals toward new ones
  double ipDist = s2->posIdealDist(ReportingLevel::Medium);
  printf("rms (pstn, ideal) = %.5f \n", ipDist);
  cout << flush;
  return s2;
}


// h's estimate of the victory probability and expected delta in utility for k from i challenging j,
// compared to status quo.
// Note that the  aUtil vector of KMatrix must be set before starting this.
// TODO: offer a choice the different ways of estimating value-of-a-state: even sum or expected value.
// TODO: we may need to separate euConflict from this at some point
tuple<double, double> SMPState::probEduChlg(unsigned int h, unsigned int k, unsigned int i, unsigned int j, bool sqlP) const {

  // you could make other choices for these two sub-models
  auto sMod = (const SMPModel*)model;
  auto vr = sMod->vrCltn; //VotingRule::Proportional;
  auto tpc = sMod->tpCommit;// KBase::ThirdPartyCommit::SemiCommit;

  double uii = aUtil[h](i, i);
  double uij = aUtil[h](i, j);
  double uji = aUtil[h](j, i);
  double ujj = aUtil[h](j, j);

  // h's estimate of utility to k of status-quo positions of i and j
  const double euSQ = aUtil[h](k, i) + aUtil[h](k, j);
  assert((0.0 <= euSQ) && (euSQ <= 2.0));

  // h's estimate of utility to k of i defeating j, so j adopts i's position
  const double uhkij = aUtil[h](k, i) + aUtil[h](k, i);
  assert((0.0 <= uhkij) && (uhkij <= 2.0));

  // h's estimate of utility to k of j defeating i, so i adopts j's position
  const double uhkji = aUtil[h](k, j) + aUtil[h](k, j);
  assert((0.0 <= uhkji) && (uhkji <= 2.0));

  auto ai = ((const SMPActor*)(model->actrs[i]));
  double si = KBase::sum(ai->vSal);
  double ci = ai->sCap;
  auto aj = ((const SMPActor*)(model->actrs[j]));
  double sj = KBase::sum(aj->vSal);
  assert((0 < sj) && (sj <= 1));
  double cj = aj->sCap;
  const double minCltn = 1E-10;

  // get h's estimate of the principal actors' contribution to their own contest

  // h's estimate of i's unilateral influence contribution to (i:j).
  // When ideals perfectly track positions, this must be positive
  double contrib_i_ij = Model::vote(vr, si*ci, uii, uij);
  if (identAccMat) {
    assert(0 <= contrib_i_ij);
  }
  // If not, you could have the ordering (Idl_i, Pos_j, Pos_i)
  // so that i would prefer j's position over his own.


  // h's estimate of j's unilateral influence contribution to (i:j).
  // When ideals perfectly track positions, this must be negative
  double contrib_j_ij = Model::vote(vr, sj*cj, uji, ujj);
  if (identAccMat) {
    assert(contrib_j_ij <= 0);
  }
  // Similarly, you could have an ordering like (Idl_j, Pos_i, Pos_j)
  // so that j would prefer i's position over his own.

  double chij = minCltn; // strength of complete coalition supporting i over j (initially empty)
  double chji = minCltn; // strength of complete coalition supporting j over i (initially empty)

  // add i's contribution to the appropriate coalition
  if (contrib_i_ij > 0.0) {
    chij = chij + contrib_i_ij;
  }
  assert(0.0 < chij);

  if (contrib_i_ij < 0.0) {
    chji = chji - contrib_i_ij;
  }
  assert(0.0 < chji);

  // add j's contribution to the appropriate coalition
  if (contrib_j_ij > 0.0) {
    chij = chij + contrib_j_ij;
  }
  assert(0.0 < chij);

  if (contrib_j_ij < 0.0) {
    chji = chji - contrib_j_ij;
  }
  assert(0.0 < chji);

  // cache those sums
  contrib_i_ij = chij;
  contrib_j_ij = chji;


  const unsigned int na = model->numAct;

  // we assess the overall coalition strengths by adding up the contribution of
  // individual actors (including i and j, above). We assess the contribution of third
  // parties (n) by looking at little coalitions in the hypothetical (in:j) or (i:nj) contests.
  auto tpvArray = KMatrix(na, 3);
  for (unsigned int n = 0; n < na; n++) {
    if ((n != i) && (n != j)) { // already got their influence-contributions
      auto an = ((const SMPActor*)(model->actrs[n]));

      double cn = an->sCap;
      double sn = KBase::sum(an->vSal);
      double uni = aUtil[h](n, i);
      double unj = aUtil[h](n, j);
      double unn = aUtil[h](n, n);

      // notice that each third party starts afresh,
      // considering only contributions of principals and itself
      double pin = Actor::vProbLittle(vr, sn*cn, uni, unj, contrib_i_ij, contrib_j_ij);

      assert(0.0 <= pin);
      assert(pin <= 1.0);
      double pjn = 1.0 - pin;
      auto vt_uv_ul = Actor::thirdPartyVoteSU(sn*cn, vr, tpc, pin, pjn, uni, unj, unn);
      const double vnij = get<0>(vt_uv_ul);
      chij = (vnij > 0) ? (chij + vnij) : chij;
      assert(0 < chij);
      chji = (vnij < 0) ? (chji - vnij) : chji;
      assert(0 < chji);

      const double utpv = get<1>(vt_uv_ul);
      const double utpl = get<2>(vt_uv_ul);
      // record for SQLite
      tpvArray(n, 0) = pin;
      tpvArray(n, 1) = utpv;
      tpvArray(n, 2) = utpl;
    }
  }

  const double phij = chij / (chij + chji); // ProbVict, for i
  const double phji = chji / (chij + chji);

  const double euVict = uhkij;  // UtilVict
  const double euCntst = phij*uhkij + phji*uhkji; // UtilContest,
  const double euChlg = (1 - sj)*euVict + sj*euCntst; // UtilChlg
  const double duChlg = euChlg - euSQ; //  delta-util of challenge versus status-quo
  auto rslt = tuple<double, double>(phij, duChlg);

  // JAH 20160802 switched to use the model sql flags vector to control logging
  // I keep sqlP and short-circuit & it because sometimes probEduChlg is called to
  // do some temporary calcs which should not be store - this is controlled with sqlP
  if (sqlP && model->sqlFlags[2]) {
    // now that the computation is finished, record everything into SQLite
    //
    // record tpvArray into SQLite turn, est (h), init (i), third party (n), receiver (j), and tpvArray[n]
    // printf ("SMPState::probEduChlg(%2i, %2i, %2i, %i2) = %+6.4f - %+6.4f = %+6.4f\n", h, k, i, j, euCh, euSQ, euChlg);

    unsigned int t = myTurn();

    sqlite3 * db = model->smpDB;
    char* zErrMsg = nullptr; // Error message in case

    auto sqlBuff = newChars(sqlBuffSize);
    // prepare the sql statement to insert. as it does not depend on tpk, keep it outside the loop.

    sprintf(sqlBuff,
            "INSERT INTO TP_Prob_Vict_Loss (ScenarioId, Turn_t, Est_h,Init_i,ThrdP_k,Rcvr_j,Prob,Util_V,Util_L) VALUES ('%s', %u, %u, %u, ?1, %u, ?2, ?3, ?4)",
            model->getScenarioID().c_str(), t, h, i, j);

    // The whole point of a prepared statement is to reuse it.
    // Therefore, we prepare it before the loop, and reuse it inside the loop:
    // just moving it outside loop cut dummyData_3Dim.csv run time from 30 to 10 seconds
    // (with Electric Fence).
    assert(nullptr != db);
    sqlite3_stmt *insStmt;
    sqlite3_prepare_v2(db, sqlBuff, strlen(sqlBuff), &insStmt, NULL);
    assert(nullptr != insStmt); //make sure it is ready

    sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);

    string thij = std::to_string(t)
      + "," + std::to_string(h)
      + "," + std::to_string(i)
      + "," + std::to_string(j);

    for (int tpk = 0; tpk < na; tpk++) {  // third party voter, tpk
      int rslt = 0;

      rslt = sqlite3_bind_int(insStmt, 1, tpk);
      assert(SQLITE_OK == rslt);

      // bind the data
      rslt = sqlite3_bind_double(insStmt, 2, tpvArray(tpk, 0));
      assert(SQLITE_OK == rslt);
      rslt = sqlite3_bind_double(insStmt, 3, tpvArray(tpk, 1));
      assert(SQLITE_OK == rslt);
      rslt = sqlite3_bind_double(insStmt, 4, tpvArray(tpk, 2));
      assert(SQLITE_OK == rslt);

      // actually record it
      rslt = sqlite3_step(insStmt);
      assert(SQLITE_DONE == rslt);
      rslt = sqlite3_clear_bindings(insStmt);
      assert(SQLITE_OK == rslt);
      rslt = sqlite3_reset(insStmt);
      assert(SQLITE_OK == rslt);
    }

    // formatting note: %d means an integer, base 10
    // we will use base 10 by default, and these happen to be unsigned integers, so %i is appropriate

    memset(sqlBuff, '\0', sqlBuffSize);
    sprintf(sqlBuff,
            "INSERT INTO ProbVict (ScenarioId, Turn_t, Est_h,Init_i,Rcvr_j,Prob) VALUES ('%s',%u,%u,%u,%u,%f)",
            model->getScenarioID().c_str(), t, h, i, j, phij);
    sqlite3_exec(db, sqlBuff, NULL, NULL, &zErrMsg);

    string thkij = std::to_string(t)
      + "," + std::to_string(h)
      + "," + std::to_string(k)
      + "," + std::to_string(i)
      + "," + std::to_string(j);

    std::vector<double> eu;
    eu.push_back(euSQ);
    eu.push_back(euVict);
    eu.push_back(euCntst);
    eu.push_back(euChlg);

    // Thread safety lock
    utilDataLock.lock();
    euData.emplace(thkij,eu);
    tpvData.emplace(thij, tpvArray);
    phijData.emplace(thij, phij);
    utilDataLock.unlock();

    // the following four statements could be combined into one table
    memset(sqlBuff, '\0', sqlBuffSize);
    sprintf(sqlBuff,
            "INSERT INTO UtilChlg (ScenarioId, Turn_t, Est_h,Aff_k,Init_i,Rcvr_j,Util_SQ,Util_Vict,Util_Cntst,Util_Chlg) VALUES ('%s',%u,%u,%u,%u,%u,%f,%f,%f,%f)",
            model->getScenarioID().c_str(), t, h, k, i, j, euSQ, euVict, euCntst, euChlg);
    sqlite3_exec(db, sqlBuff, NULL, NULL, &zErrMsg);

    sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &zErrMsg);
    sqlite3_finalize(insStmt); // finalize statement to avoid resource leaks
    //printf("Stored SQL for turn %u of all estimators, actors, and positions \n", t);
    //cout << flush; // so this prints before subsequent assertion failures

    delete sqlBuff;
    sqlBuff = nullptr;
  }
  return rslt;
}


tuple<int, double, double> SMPState::bestChallenge(unsigned int i) const {
  int bestJ = -1;
  double pIJ = 0;
  double bestEU = -1.00;

  // for SMP, positive expected gains on the first turn are typically in the 0.5 to 0.01 range
  // I take a fraction of the minimum.
  const double minSigEDU = 1e-5; // TODO: 1/20 of the minimum, or 0.0005

  for(const auto& eduJ : eduChlgsIJ[i]) {
    double pij = get<0>(eduJ.second);
    double edu = get<1>(eduJ.second);
    if ((minSigEDU < edu) && (bestEU < edu)) {
      bestJ = eduJ.first;
      pIJ = pij;
      bestEU = edu;
    }
  }
  if (0 <= bestJ) {
    assert(minSigEDU < bestEU);
  }
  auto rslt = tuple<int, double, double>(bestJ, pIJ, bestEU);
  return rslt;
}

uint64_t SMPState::getPosMoverBargain(unsigned int actor) const {
  return positionMovers.at(actor);
}

void SMPState::setPosMoverBargain(unsigned int actor, uint64_t bargainID) {
  positionMovers.insert(moverBargains::value_type(actor, bargainID));
}

}; // end of namespace

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
