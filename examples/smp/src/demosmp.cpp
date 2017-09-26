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
// Demonstrate a very basic, but highly parameterized, Spatial Model of Politics.
//
// --------------------------------------------

#include "smp.h"
#include "demosmp.h"
#include <functional>
#include <easylogging++.h>

using KBase::PRNG;
using KBase::KMatrix;
using KBase::Actor;
using KBase::Model;
using KBase::Position;
using KBase::State;
using KBase::VotingRule;
using KBase::VPModel;

void ReplaceStringInPlace(std::string& subject, const std::string& search,
	const std::string& replace) {
	size_t pos = 0;
	while ((pos = subject.find(search, pos)) != std::string::npos) {
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}
}
std::string GenerateDBNameWithTimeStamp()
{
  // Generate a name with following format:
  // smpc-YYYY-MM-DD-HH-MM-SS
  // Ex: smpc-2017-05-24__14-46-29.db
  using namespace std::chrono;
  system_clock::time_point today = system_clock::now();
  time_t tt;
  tt = system_clock::to_time_t(today);
  struct tm * ptm = gmtime(&tt);
  size_t nameLength = 33;
  char *dbname= new char [nameLength];
  sprintf(dbname, "smpc-%d-%02d-%02d__%02d-%02d-%02d_GMT.db",
    ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,
    ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
  std::string name = std::string(dbname);
  delete[] dbname;
  return name;
}

int main(int ac, char **av) {
  using std::string;
  using KBase::dSeed;
  uint64_t seed = -1;
  bool run = true;
  bool euSmpP = false;
  bool randAccP = false;
  bool csvP = false;
  bool xmlP = false;
  bool logMin = false;
  bool saveHist = false;
  string inputCSV = "";
  string inputDBname = "";
  string inputXML = "";
  string connstr;

  auto showHelp = []() {
    printf("\n");
    printf("Usage: specify one or more of these options\n");
    printf("--help           print this message\n");
    printf("--euSMP          exp. util. of spatial model of politics\n");
    printf("--ra             randomize the adjustment of ideal points with euSMP \n");
    printf("--csv <f>        read a scenario from CSV\n");
    printf("--xml <f>        read a scenario from XML\n");
    printf("--logmin         log only scenario information + position histories\n");
    printf("--savehist       export by-dim by-turn position histories (input+'_posLog.csv') and\n");
    printf("                 by-dim actor effective powers (input+'_effPower.csv')\n");
    printf("--seed <n>       set a 64bit seed; default is %020llu; 0 means truly random\n", dSeed);
    printf("--connstr        a comma separated string for database server credentials:\n");
    printf("                 \"Driver=<QPSQL|QSQLITE>;Server=<IP>*;[Port=<port>]*;Database=<DB_name>;\n");
    printf("                 Uid=<user_id>*;Pwd=<password>*\"*for QPSQL only\n");
  };

  if (ac > 1) {
    for (int i = 1; i < ac; i++) {
      if (strcmp(av[i], "--seed") == 0) {
        i++;
        seed = std::stoull(av[i]);
      }
      else if (strcmp(av[i], "--csv") == 0) {
        csvP = true;
        i++;
        if (av[i] != NULL)
        {
                inputCSV = av[i];
        }
        else
        {
                run = false;
                break;
        }
      }
      else if (strcmp(av[i], "--xml") == 0) {
        xmlP = true;
        i++;
        if (av[i] != NULL)
        {
                inputXML = av[i];
        }
        else
        {
                run = false;
                break;
        }
      }
      else if (strcmp(av[i], "--euSMP") == 0) {
        euSmpP = true;
      }
      else if (strcmp(av[i], "--ra") == 0) {
        randAccP = true;
      }
      else if (strcmp(av[i], "--help") == 0) {
        run = false;
      }
      else if (strcmp(av[i], "--logmin") == 0) {
        logMin = true;
      }
      else if (strcmp(av[i], "--savehist") == 0) {
        saveHist = true;
      }
      else if(strcmp(av[i], "--connstr") == 0) {
        i++;
        connstr = av[i];
      }
      else {
        run = false;
        printf("Unrecognized argument %s\n", av[i]);
      }
    }
  }
  else {
    run = false; // no arguments supplied
  }

  // JAH 20160730 vector of SQL logging flags for 5 groups of tables:
  // 0 = Information Tables, 1 = Position Tables, 2 = Challenge Tables,
  // 3 = Bargain Resolution Tables, 4 = VectorPosition table
  // JAH 20161010 added group 4 for only VectorPos so it can be logged alone
  std::vector<bool> sqlFlags = {true,true,true,true,true};
  // JAH 20161010 default is to log all tables, if --logmin is passed, only
  // enable logging groups 0 & 4
  if (logMin)
  {
    sqlFlags = {true,false,false,false,true};
  }

  if (!run) {
    showHelp();
    return 0;
  }

  // Set logging configuration from a file
  //SMPLib::SMPModel::configLogger("./smpc-logger.conf");
  KBase::Model::configLogger("./smpc-logger.conf");

  auto sTime = KBase::displayProgramStart(DemoSMP::appName, DemoSMP::appVersion);
  if (0 == seed) {
    PRNG * rng = new PRNG();
    seed = rng->setSeed(seed); // 0 == get a random number
    delete rng;
    rng = nullptr;
  }

  //printf("Using PRNG seed:  %020llu \n", seed);
  //printf("Same seed in hex:   0x%016llX \n", seed);

  // JAH 20170109 we set the seed first to -1 then change it to dseed
  // here only if input is not xml, so as to ensure that a manually
  // input seed on the cmdline can override the seed in an xml file,
  // but the dseed coming from no seed input can't override it
  if ((seed == -1) && (!xmlP)) {
      seed = KBase::dSeed;
  }

  bool checkCredentials = SMPLib::SMPModel::loginCredentials(connstr);
  if (!checkCredentials) { // Some error with input credentials
    LOG(INFO) << KBase::Model::getLastError();
    return -1;
  }

  // note that we reset the seed every time, so that in case something
  // goes wrong, we need not scroll back too far to find the
  // seed required to reproduce the bug.
  if (euSmpP) {
    try {
      SMPLib::SMPModel::randomSMP(0, 0, randAccP, seed, sqlFlags);
    }
    catch (...) {
      LOG(INFO) << "Exception caught in randomSMP. Check previous messages for error";
    }
  }
  if (csvP) {
    string scenid = SMPLib::SMPModel::runModel(sqlFlags, inputCSV, seed, saveHist);
    if (scenid.empty()) {
      LOG(INFO) << "Error: " << KBase::Model::getLastError();
    }
    SMPLib::SMPModel::destroyModel();
  }
  if (xmlP) {
    string scenid = SMPLib::SMPModel::runModel(sqlFlags, inputXML, seed, saveHist);
    if (scenid.empty()) {
      LOG(INFO) << "Error: " << KBase::Model::getLastError();
    }
    SMPLib::SMPModel::destroyModel();
  }

  KBase::displayProgramEnd(sTime);
  return 0;
}




// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
