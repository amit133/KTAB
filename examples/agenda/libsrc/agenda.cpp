// ------------------------------------------
// Copyright KAPSARC. Open Source MIT License
// ------------------------------------------
#include "agenda.h"


using std::cout;
using std::endl;
using std::flush;

namespace AgendaControl {

  uint64_t fact(unsigned int n) {
    uint64_t f = 0;
    switch (n) {
    case 0:
    case 1:
      f = 1;
      break;
    case 2:
      f = 2;
      break;
    default:
      f = n * fact(n - 1);
      break;
    }
    return f;
  }


  uint64_t numSets(unsigned int n, unsigned int m) {
    assert(0 < n);
    assert(0 < m);
    uint64_t ns = fact(n) / (fact(m)*fact(n - m));
    return ns;
  }

  uint64_t numAgenda(unsigned int n) {
    // how many distinct agendas are there for n items?
    // Crucially, [x:y] == [y:x]
    assert(0 < n);
    uint64_t cn = 0;
    switch (n) {
    case 1:
      cn = 1;
      // only [a]
      break;
    case 2:
      cn = 1;
      // only [a:b]
      break;

    case 3:
      cn = 3;
      // [a:[b:c]]
      // [b:[a:c]]
      // [c:[a:b]]
      break;

    default: {
      // An agenda over n>3 items can only be a choice between sub-agendas.
      // Suppose the left has i items and the right has j=n-i items.
      // There are choose(n,i) ways to choose a unique, unordered set for the LHS,
      // and for each such choice there is only one possible RHS (all not in the LHS).
      // Because the LHS and RHS are distinct, any agendas addressing them are distinct,
      // so there are C(i) distinct ways to address the LHS, C(j) distinct ways to address the RHS,
      // and C(i)*C(j) distinct combinations.
      // We then sum those up for 0 < i < n. This does double count, as each (i,n-i)
      // division is matched by a (n-i, i) division, so the sum gets divided by 2.
      uint64_t cm = 0;
      for (unsigned int i = 1; i < n; i++) {
        unsigned int j = n - i;
        uint64_t splits = numSets(n, i);
        uint64_t lC = numAgenda(i);
        uint64_t rC = numAgenda(j);
        cm = cm + splits*lC*rC;
      }
      cn = cm / 2;
      assert(cm == (2 * cn));
    }
             break;
    }
    return cn;
  }


  // returns the list of all ways to choose an unordered set of m items from a set of n.
  vector< vector <unsigned int> > chooseSet(const unsigned int n, const unsigned int m) {
    assert(0 < n);
    assert(0 < m);
    auto csnm = vector< vector <unsigned int> >();
    if (1 == m) {
      for (unsigned int i = 0; i < n; i++) {
        vector<unsigned int> ci = { i };
        csnm.push_back(ci);
      }
    }
    else {
      auto cLess = chooseSet(n, m - 1);
      for (auto lst : cLess) {
        unsigned int len = lst.size();
        unsigned int maxEl = lst[len - 1];
        for (unsigned int j = maxEl + 1; j < n; j++) {
          vector<unsigned int> ls = lst;
          ls.push_back(j);
          csnm.push_back(ls);
        }
      }
    }
    return csnm;
  }


  tuple<vector<unsigned int>, vector<unsigned int>> indexedSet(const vector<unsigned int> xs,
    const vector<unsigned int> is) {
    vector<unsigned int> rslt = {};
    for (auto i : is) {
      rslt.push_back(xs[i]);
    }
    vector<unsigned int> comp = {};
    for (unsigned int i = 0; i < xs.size() - rslt.size(); i++) {
      comp.push_back(0);
    }
    std::set_difference(xs.begin(), xs.end(),
      rslt.begin(), rslt.end(),
      comp.begin()
      );
    auto pr = tuple<vector<unsigned int>, vector<unsigned int>>(rslt, comp);
    return pr;
  }


  vector<Agenda*> agendaSet(const vector<unsigned int> xs) {
    unsigned int n = xs.size();
    assert(0 < n);
    auto showA = [](const vector<unsigned int> &as) {
      printf("[");
      for (auto a : as) {
        printf(" %u ", a);
      }
      printf("]");
      return;
    };

    vector<Agenda*> as = {};

    switch (n) {
    case 1: {
      Agenda* a = new Terminal(xs[0]);
      as.push_back(a);
    }
            break;
    case 2: {
      Agenda* lsa = new Terminal(xs[0]);
      Agenda* rsa = new Terminal(xs[1]);
      Agenda* a = new Choice(lsa, rsa);
      as.push_back(a);
    }
            break;

    default: // n>2, odd or even
      for (unsigned int k = 1; k <= (n / 2); k++) {
        vector<vector<unsigned int>> leftIndices = {};

        leftIndices = chooseSet(n, k);
        // if n is odd, then when k= (n/2), we have k < n-k, so the
        // left-hand agenda is smaller than and distinct from the right-hand agenda.
        // However, when n is even, the latter half of the list must be discarded
        // because of symmetry.
        // Suppose n=4 for (a,b,c,d) and we choose 2.
        // ((a,b), (a,c), (a,d), (b,c), (b,d), (c,d)) is the usual set of 6 unique subsets of size 2.
        // However, if those are the left-hand agendas, then the right-hand are their complements:
        // ((c,d), (b,d), (b,c), (a,d), (a,c), (a,b)).
        // Thus, the complements of the second half are exactly the first half, so by symmetry
        // we discard the second half.
        if (n == (2 * k)) {
          unsigned int m = leftIndices.size();
          assert(m == 2 * (m / 2));
          vector<vector<unsigned int>> shortIndices = {};
          for (unsigned int i = 0; i < (m / 2); i++){
            shortIndices.push_back(leftIndices[i]);
          }
          leftIndices = shortIndices;
        }

        for (auto lhi : leftIndices) {
          //showA(lhi);
          //cout << " -- ";
          auto pr = indexedSet(xs, lhi);
          vector<unsigned int> lhs = std::get<0>(pr);
          vector<unsigned int> rhs = std::get<1>(pr);

          auto lAgendas = agendaSet(lhs);
          auto rAgendas = agendaSet(rhs);
          for (auto la : lAgendas) {
            for (auto ra : rAgendas) {
              Agenda* a = new Choice(la, ra);
              as.push_back(a);
            }
          }

          // showA(lhs);
          // cout << " : ";
          // showA(rhs);
          // cout << endl;
        }
      }
      break;
    }

    // printf("Found %i agendas \n", as.size());
    // for (auto a : as) {
    //   cout << *a << endl;
    // }
    // cout << endl << flush;

    assert(numAgenda(n) == as.size());

    return as;
  }

  // ------------------------------------------
  Agenda* Agenda::makeRandom(unsigned int n, PartitionRule pr, PRNG* rng) {
    auto xs = vector<int>();
    for (unsigned int i = 0; i < n; i++) {
      xs.push_back(i);
    }
    for (unsigned int i = 0; i < n; i++) {
      unsigned int j = rng->uniform() % n;
      auto xi = xs[i];
      auto xj = xs[j];
      xs[i] = xj;
      xs[j] = xi;
    }
    Agenda* ap = makeAgenda(xs, pr, rng);
    return ap;
  }


  vector<Agenda*> Agenda::enumerateAgendas(unsigned int n, PartitionRule pr) {
    vector<unsigned int> items = {};
    for (unsigned int i = 0; i < n; i++) {
      items.push_back(i);
    }
    vector<Agenda*> as1 = agendaSet(items);
    vector<Agenda*> as2 = {};
    for (auto a : as1){
      if (a->balanced(pr)) {
        as2.push_back(a);
      }
    }
    return as2;
  }


  unsigned int Agenda::minAgendaSize(PartitionRule pr, unsigned int n) {
    assert(0 < n); // could be 1, but not 0
    unsigned int minM = 0;
    switch (pr) {
    case PartitionRule::FullBalancedPR:
      minM = n / 2;
      break;
    case PartitionRule::ModBalancedPR:
      if (n <= 5) {
        minM = n / 2;
      }
      else { // 6 or more
        minM = n / 3;
      }
      break;

    case PartitionRule::FreePR:
      if (1 == n) {
        minM = 0;
      }
      else {
        minM = 1;
      }
      break;
    }

    return minM;
  }

  Agenda* Agenda::makeAgenda(vector<int> xs, PartitionRule pr, PRNG* rng) {
    const int n = xs.size();
    const int minM = minAgendaSize(pr, n);


    Agenda* ap = nullptr;
    if (1 == n) {
      ap = new Terminal(xs[0]);
    }
    else {
      int m = 0;
      while ((m < minM) || (n - m < minM)) {
        m = 1 + rng->uniform() % (n - 1);
      }
      auto ys = vector<int>();
      auto zs = vector<int>();
      for (unsigned int i = 0; i < m; i++) {
        ys.push_back(xs[i]);
      }
      for (unsigned int i = m; i < n; i++) {
        zs.push_back(xs[i]);
      }
      assert(minM <= ys.size());
      assert(minM <= zs.size());
      auto a1 = makeAgenda(ys, pr, rng);
      auto a2 = makeAgenda(zs, pr, rng);
      ap = new Choice(a1, a2);
    }
    return ap;
  }

  // ------------------------------------------

  double Choice::eval(const KMatrix& val, const KMatrix& cap, VotingRule vr, unsigned int i) {
    setProbs(val, cap, vr);
    double valL = lhs->eval(val, cap, vr, i);
    double valR = rhs->eval(val, cap, vr, i);
    double ev = (lProb*valL) + (rProb*valR);
    //cout << "Eval " << i << " of " << *this << " using " << vrName(vr) << " = " << ev << endl << flush;
    return ev;
  };

  void Choice::clearProbs()  {
    lProb = -1.0;
    rProb = -1.0;
    return;
  };

  void Choice::assertProbs()  {
    assert(0.0 <= lProb);
    assert(0.0 <= rProb);
    assert(fabs(lProb + rProb - 1.0) < 1E-8);
    return;
  };

  void Choice::setProbs(const KMatrix& val, const KMatrix& cap, VotingRule vr) {
    if ((lProb < 0) || (rProb < 0)) {
      const unsigned int numA = val.numR();
      double sL = 1E-10;
      double sR = 1E-10;
      for (unsigned int k = 0; k < numA; k++) {
        double lv = lhs->eval(val, cap, vr, k);
        double rv = rhs->eval(val, cap, vr, k);
        double voteLR = Model::vote(vr, cap(k, 0), lv, rv);
        sL = (voteLR > 0) ? (sL + voteLR) : sL;
        sR = (voteLR < 0) ? (sR - voteLR) : sR;
      }
      assert(0 < sL);
      assert(0 < sR);
      auto ppr = Model::vProb(Model::VPModel::Quartic, sL, sR);
      lProb = std::get<0>(ppr); // if Linear, sL / (sL + sR);
      rProb = std::get<1>(ppr); // if Linear, sR / (sL + sR); 
    }
    return;
  }

  void Choice::showProbs(double pArrive) {
    assertProbs();
    //cout << pArrive << endl;
    double pl = pArrive * lProb;
    double pr = pArrive * rProb;
    cout << "[";
    lhs->showProbs(pl);
    cout << ":";
    rhs->showProbs(pr);
    cout << "]";
    return;
  }

  bool Choice::balanced(PartitionRule pr) const {
    const unsigned int n = length();
    // every choice is between two items, and the shortest possible sub-agenda
    // is a Terminal of length 1, so this must be at least 2 long
    assert(2 <= n);
    unsigned int lN = lhs->length();
    unsigned int rN = rhs->length();
    unsigned int minN = n;

    switch (pr) {
    case PartitionRule::FreePR:
      minN = 1;
      break;

    case PartitionRule::ModBalancedPR:
      if (n <= 5) {
        minN = n / 2;
      }
      else {
        minN = n / 3;
      }
      break;

    case PartitionRule::FullBalancedPR:
      minN = n / 2;
      break;
    };
    bool ok = (minN <= lN) && (minN <= rN) && (lhs->balanced(pr)) && (rhs->balanced(pr));
    return ok;
  }
  // ------------------------------------------
  double Terminal::eval(const KMatrix& val, const KMatrix&, VotingRule, unsigned int i) {
    double ev = val(i, item);
    //cout << "Eval " << i << " of " << *this << " = " << ev << endl << flush;
    return ev;
  };


  void Terminal::showProbs(double pArrive) {
    printf("%.4f", pArrive);
    return;
  }

}; // end of namespace

// ------------------------------------------
// Copyright KAPSARC. Open Source MIT License
// ------------------------------------------