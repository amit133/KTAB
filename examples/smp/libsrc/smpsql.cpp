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
// Demonstrate a very basic SQLite database for the Spatial Model of Politics.
//
// --------------------------------------------

#include "smp.h"
#include "sqlite3.h"
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

using KBase::PRNG;
using KBase::KMatrix;
using KBase::KException;
using KBase::Actor;
using KBase::Model;
using KBase::Position;
using KBase::State;
using KBase::VotingRule;
using KBase::ReportingLevel;


// --------------------------------------------

void bindExecuteBargnTableUpdate(sqlite3_stmt *updateStmt, size_t turn, int bargnID,
                                 int initActor, double initProb, bool isInitSelected,
                                 int recvActor, double recvProb, bool isRecvSelected);

// --------------------------------------------
// JAH 20160728 modified to return a KTable object instead of the SQL string
KTable * SMPModel::createSQL(unsigned int n)  {
  string sql = "";
  string name = "";
  unsigned int grpID = 0;
  // check total number of table exceeds
  assert(n < Model::NumTables + NumTables);
  if (n < Model::NumTables) {
    return Model::createSQL(n);
  }
  else
  {
    switch (n-Model::NumTables) {
    case  0: // coordinates of each actor's position
		sql = "create table if not exists VectorPosition ("  \
			"ScenarioId VARCHAR(32) NOT NULL DEFAULT 'None', "\
			"Turn_t		INTEGER NOT NULL DEFAULT 0, "\
			"Act_i		INTEGER NOT NULL DEFAULT 0, "\
			"Dim_k		INTEGER NOT NULL DEFAULT 0, "\
			"Pos_Coord		FLOAT NOT NULL DEFAULT 0,"\
			"Idl_Coord		FLOAT NOT NULL DEFAULT 0, " \
			"Mover_BargnId	INTEGER NULL DEFAULT 0" \
      ");";
      name = "VectorPosition";
      grpID = 4;// JAH 20161010 put in group 4 all by itself
      break;

    case 1: // salience to each actor of each dimension
      sql = "create table if not exists SpatialSalience ("  \
            "ScenarioId VARCHAR(32) NOT NULL DEFAULT 'None', "\
            "Turn_t		INTEGER NOT NULL DEFAULT 0, "\
            "Act_i		INTEGER NOT NULL DEFAULT 0, "\
            "Dim_k		INTEGER NOT NULL DEFAULT 0, "\
            "Sal		FLOAT NOT NULL DEFAULT 0.0"\
            ");";
      name = "SpatialSalience";
      grpID = 0;
      break;

    case 2: // scalar capability of each actor
      sql = "create table if not exists SpatialCapability ("  \
            "ScenarioId VARCHAR(32) NOT NULL DEFAULT 'None', "\
            "Turn_t		INTEGER NOT NULL DEFAULT 0, "\
            "Act_i		INTEGER NOT NULL DEFAULT 0, "\
            "Cap		FLOAT NOT NULL DEFAULT 0.0"\
            ");";
      name = "SpatialCapability";
      grpID = 0;
      break;


    case 3: // names of dimensions, so the GUI can use it JAH 20160727
    {
      char *sqlBuff = newChars(500);
      sprintf(sqlBuff, "create table if not exists DimensionDescription ("  \
                       "ScenarioId VARCHAR(32) NOT NULL DEFAULT 'None', "\
                       "Dim_k	INTEGER NOT NULL DEFAULT 0, "\
                       "\"Desc\" VARCHAR(%u) NOT NULL DEFAULT 'NoDesc' "\
                       ");", maxDimDescLen);
      sql = std::string(sqlBuff);
      delete sqlBuff;
      sqlBuff = nullptr;

      name = "DimensionDescription";
      grpID = 0;
    }
      break;
    case 4: // affinities of actors, so the GUI can use it
    {
        sql = "create table if not exists Accommodation ("  \
            "ScenarioId VARCHAR(32) NOT NULL DEFAULT 'None', "\
            "Act_i      INTEGER NOT NULL DEFAULT 0, "\
            "Act_j      INTEGER NOT NULL DEFAULT 0, "\
            "Affinity   FLOAT NOT NULL DEFAULT 0.0"\
            ");";
        name = "Accommodation";
        grpID = 0;
        break;
    }
    default:
      throw(KException("SMPModel::createSQL unrecognized table number"));
    }
  }

  assert(grpID<(Model::NumSQLLogGrps+NumSQLLogGrps));
  auto tab = new KTable(n,name,sql,grpID);
  return tab;
}


void SMPModel::sqlTest() {
  initDBDriver(QString("smpDB"));

  if (0 == dbDriver.compare("QPSQL")) {
    if (!connectDB()) {
      // connect with the default postgres db (the user should have admin privilege)
      if(!connect(server, port, "postgres", userName, password)) {
        cout << "Please check the login credentials, ip address or port number" << endl;
        assert(false);
      }

      // Check if the database exists
      if (!isDB(databaseName)) {
        // if doesn't exist create one
        if (createDB(databaseName)) {
          // close the connection to the postgres db
          qtDB->close();
          // connect to the newly created database
          if (connectDB()) {
            cout << "Connected to newly created database." << endl;
            query = QSqlQuery(*qtDB);
          }
          else {
            cout << "Not connected to new db" << endl;
            cout << qtDB->lastError().text().toStdString() << endl;
            assert(false);
          }
        }
        else {
          assert(false);
        }
      }
      else {
        cout << "Database " << databaseName.toStdString()
          << " exists but not able to connect to it." << endl;
        assert(false);
      }
    }
    else {
      query = QSqlQuery(*qtDB);
    }
  }
  else if (0 == dbDriver.compare("QSQLITE")) {
    qtDB->setDatabaseName(databaseName);
    qtDB->open();
    query = QSqlQuery(*qtDB);
    configSqlite();
  }

  // Create & execute SQL statements
  // JAH 20160728 rewritten to complete the vector of KTables before creating the table
  for (unsigned int i = 0; i < SMPModel::NumTables + Model::NumTables; i++) {
    // get the table and add to the vector
    auto thistable = SMPModel::createSQL(i);
    assert(nullptr != thistable);
    KTables.push_back(thistable);
    // create the table
    execQuery(thistable->tabSQL);
  }

  // just a test to get linkages correct

  auto callBack = [](void *NotUsed, int argc, char **argv, char **azColName) {
    for (int i = 0; i < argc; i++) {
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return ((int)0);
  };

  smpDB = nullptr;
  sqlite3 * db; // I don't like passing 'this' into lambda-functions
  char* zErrMsg = nullptr;
  string sql;

  auto sOpen = [&db](unsigned int n, string dbPath) {
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
      fprintf(stdout, "Can't open database: %s\n", sqlite3_errmsg(db));
      exit(0);
    }
    else {
      fprintf(stdout, "Successfully opened database #%i\n", n);
    }
    cout << endl << flush;
    return;
  };

  auto sExec = [&db, callBack, &zErrMsg](string sql, string msg) {
    int rc = sqlite3_exec(db, sql.c_str(), callBack, nullptr, &zErrMsg);
    if (rc != SQLITE_OK) {
      fprintf(stdout, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
    }
    else {
      fprintf(stdout, msg.c_str());
    }
    return rc;
  };

  // Open database
  cout << endl << flush;
  sOpen(1,dbPath); //passing DB FileName

  // As we are not dealing with a long-term, mission-critical database,
  // we can shut off some of the journaling stuff intended to protect
  // the DB in case the system crashes in mid-operation.
  // Eliminating these checks can significantly speed operations.
  sqlite3_exec(db, "PRAGMA journal_mode = MEMORY", NULL, NULL, &zErrMsg);
  sqlite3_exec(db, "PRAGMA locking_mode = EXCLUSIVE", NULL, NULL, &zErrMsg);
  sqlite3_exec(db, "PRAGMA synchronous = OFF", NULL, NULL, &zErrMsg);
  
  // not a performance issue, but necessary for the data layout
  sqlite3_exec(db, "PRAGMA foreign_keys = ON", NULL, NULL, &zErrMsg);

  // Create & execute SQL statements
  // JAH 20160728 rewritten to complete the vector of KTables before creating the table
  for (unsigned int i = 0; i < SMPModel::NumTables +Model::NumTables; i++) {
    // get the table and add to the vector
    auto thistable = SMPModel::createSQL(i);
    assert(nullptr != thistable);
    KTables.push_back(thistable);
    // create the table
    auto buff = newChars(100);
    sExec(thistable->tabSQL, buff); // ignore return-code
    // talk a little now
    sprintf(buff, "Created SMPModel table %s(%u) successfully \n", thistable->tabName.c_str(),i);
    delete buff;
    buff = nullptr;
    cout << flush;
  }
  cout << endl << flush;

  smpDB = db;
  return;
}

// JAH 20160731 added this function in replacement to the separate
// populate* functions that separately logged information tables
// this calls the kmodel version for Actors and Scenarios and then handles
// salience, capability, and dimensions tables
void SMPModel::LogInfoTables()
{
  // first call the KModel version to do the actors and scenarios tables
  Model::LogInfoTables();

  // now do mine
  int rslt = 0;
  // check the database availability
  assert(nullptr != smpDB);
  char* zErrMsg = nullptr;

  // assert tests for all tables here at the start
  assert(numDim == dimName.size());

  // for efficiency sake, we'll do all tables in a single transaction
  // form insert commands
  string sqlD = string("INSERT INTO DimensionDescription (ScenarioId,Dim_k,Desc) VALUES ('")
      + scenId + "', ?1, ?2)";
  string sqlD1 = string("INSERT INTO DimensionDescription (ScenarioId,Dim_k,\"Desc\") VALUES ('")
    + scenId + "', :dim_k, :desc)";
  char *sqlBuffD = const_cast<char *>(sqlD.c_str());

  string sqlC = string("INSERT INTO SpatialCapability (ScenarioId, Turn_t, Act_i, Cap) VALUES ('")
      + scenId + "', ?1, ?2, ?3)";
  string sqlC1 = string("INSERT INTO SpatialCapability (ScenarioId, Turn_t, Act_i, Cap) VALUES ('")
    + scenId + "', :turn_t, :act_i, :cap)";
  char *sqlBuffC = const_cast<char *>(sqlC.c_str());

  string sqlS = string("INSERT INTO SpatialSalience (ScenarioId, Turn_t, Act_i, Dim_k,Sal) VALUES ('")
      + scenId + "', ?1, ?2, ?3, ?4)";
  string sqlS1 = string("INSERT INTO SpatialSalience (ScenarioId, Turn_t, Act_i, Dim_k,Sal) VALUES ('")
    + scenId + "', :turn_t, :act_i, :dim_k, :sal)";
  char *sqlBuffS = const_cast<char *>(sqlS.c_str());

  string sqlSc = string("UPDATE ScenarioDesc SET VotingRule = ?1, BigRAdjust = ?2, "
      "BigRRange = ?3, ThirdPartyCommit = ?4, InterVecBrgn = ?5, BargnModel = ?6 "
      " WHERE ScenarioId = '")
      + scenId + "'";
  string sqlSc1 = string("UPDATE ScenarioDesc SET VotingRule = :vr, BigRAdjust = :br, "
    "BigRRange = :brr, ThirdPartyCommit = :tpc, InterVecBrgn = :ivb, BargnModel = :bm "
    " WHERE ScenarioId = '")
    + scenId + "'";
  char *sqlBuffScene = const_cast<char *>(sqlSc.c_str());

  string sqlAcc = string("INSERT INTO Accommodation (ScenarioId, Act_i, Act_j, Affinity) VALUES ('")
      + scenId
      + "', ?1, ?2, ?3)";
  //TODO Change sqlAcc1 to sqlAcc and delete previous line for sqlAcc. Move all new sql strings
  // before corresponding for loops
  string sqlAcc1 = string("INSERT INTO Accommodation (ScenarioId, Act_i, Act_j, Affinity) VALUES ('")
    + scenId
    + "', :act_i, :act_j, :affinity)";
  char *sqlAccomod = const_cast<char *>(sqlAcc.c_str());

  // prepare the prepared statement statements
  sqlite3_stmt *insStmtD;
  sqlite3_prepare_v2(smpDB, sqlBuffD, strlen(sqlBuffD), &insStmtD, NULL);
  assert(nullptr != insStmtD);
  sqlite3_stmt *insStmtC;
  sqlite3_prepare_v2(smpDB, sqlBuffC, strlen(sqlBuffC), &insStmtC, NULL);
  assert(nullptr != insStmtC);
  sqlite3_stmt *insStmtS;
  sqlite3_prepare_v2(smpDB, sqlBuffS, strlen(sqlBuffS), &insStmtS, NULL);
  assert(nullptr != insStmtS);

  sqlite3_stmt *insStmtScene=nullptr;
  sqlite3_prepare_v2(smpDB, sqlBuffScene, strlen(sqlBuffScene), &insStmtScene, NULL);
  assert(nullptr != insStmtScene);

  sqlite3_stmt *insStmtAccomod = nullptr;
  sqlite3_prepare_v2(smpDB, sqlAccomod, strlen(sqlAccomod), &insStmtAccomod, NULL);
  assert(nullptr != insStmtAccomod);

  sqlite3_exec(smpDB, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);
  qtDB->transaction();

  // Retrieve accommodation matrix
  auto st = dynamic_cast<SMPState *>(history.back());
  assert(st != nullptr);
  auto accM = st->getAccomodate();

  // Accomodation table to record affinities
  query.prepare(QString::fromStdString(sqlAcc1));
  assert((accM.numR() == numAct) && (accM.numC() == numAct));
  for (unsigned int Act_i = 0; Act_i < numAct; ++Act_i) {
      for (unsigned int Act_j = 0; Act_j < numAct; ++Act_j) {
          //bind the data
          rslt = sqlite3_bind_int(insStmtAccomod, 1, Act_i);
          query.bindValue(":act_i", Act_i);
          assert(SQLITE_OK == rslt);
          rslt = sqlite3_bind_int(insStmtAccomod, 2, Act_j);
          query.bindValue(":act_j", Act_j);
          assert(SQLITE_OK == rslt);
          rslt = sqlite3_bind_double(insStmtAccomod, 3, accM(Act_i, Act_j));
          query.bindValue(":affinity", accM(Act_i, Act_j));
          assert(SQLITE_OK == rslt);
          // record
          rslt = sqlite3_step(insStmtAccomod);
          if (!query.exec()) {
            cout << query.lastError().text().toStdString() << endl;
          }
          assert(SQLITE_DONE == rslt);
          sqlite3_clear_bindings(insStmtAccomod);
          assert(SQLITE_DONE == rslt);
          rslt = sqlite3_reset(insStmtAccomod);
          assert(SQLITE_OK == rslt);
      }
  }

  // Dimension Description Table
  query.prepare(QString::fromStdString(sqlD1));
  for (unsigned int k = 0; k < dimName.size(); k++)
  {
    // bind the data
    rslt = sqlite3_bind_int(insStmtD, 1, k);
    query.bindValue(":dim_k", k);
    assert(SQLITE_OK == rslt);
    rslt = sqlite3_bind_text(insStmtD, 2, dimName[k].c_str(), -1, SQLITE_TRANSIENT);
    query.bindValue(":desc", dimName[k].c_str());
    assert(SQLITE_OK == rslt);
    // record
    rslt = sqlite3_step(insStmtD);
    if (!query.exec()) {
      cout << query.lastError().text().toStdString() << endl;
    }
    assert(SQLITE_DONE == rslt);
    sqlite3_clear_bindings(insStmtD);
    assert(SQLITE_DONE == rslt);
    rslt = sqlite3_reset(insStmtD);
    assert(SQLITE_OK == rslt);
  }

  // Spatial Capability
  query.prepare(QString::fromStdString(sqlC1));
  // for each turn extract the information
  for (unsigned int t = 0; t < history.size(); t++) {
    auto st = history[t];
    // get each actors capability value for each turn
    auto cp = (const SMPState*)history[t];
    auto caps = cp->actrCaps();
    for (unsigned int i = 0; i < numAct; i++) {
      // bind data
      rslt = sqlite3_bind_int(insStmtC, 1, t);
      query.bindValue(":turn_t", t);
      assert(SQLITE_OK == rslt);
      rslt = sqlite3_bind_int(insStmtC, 2, i);
      query.bindValue(":act_i", i);
      assert(SQLITE_OK == rslt);
      rslt = sqlite3_bind_double(insStmtC, 3, caps(0, i));
      query.bindValue(":cap", caps(0, i));
      assert(SQLITE_OK == rslt);
      // record
      rslt = sqlite3_step(insStmtC);
      if (!query.exec()) {
        cout << query.lastError().text().toStdString() << endl;
      }
      assert(SQLITE_DONE == rslt);
      sqlite3_clear_bindings(insStmtC);
      assert(SQLITE_DONE == rslt);
      rslt = sqlite3_reset(insStmtC);
      assert(SQLITE_OK == rslt);
    }
  }

  // Spatial Salience
  query.prepare(QString::fromStdString(sqlS1));
  // for each turn extract the information
  for (unsigned int t = 0; t < history.size(); t++) {
    // Get the individual turn
    auto st = history[t];
    // get the SMPState for turn
    auto cp = (const SMPState*)history[t];
    // Extract information for each actor and dimension
    for (unsigned int i = 0; i < numAct; i++) {
      for (unsigned int k = 0; k < numDim; k++) {
        // Populate the actor
        auto ai = ((const SMPActor*)actrs[i]);
        // Get the Salience Value for each actor
        //                double sal = ai->vSal(k, 0);
        //bind the data
        rslt = sqlite3_bind_int(insStmtS, 1, t);
        query.bindValue(":turn_t", t);
        assert(SQLITE_OK == rslt);
        rslt = sqlite3_bind_int(insStmtS, 2, i);
        query.bindValue(":act_i", i);
        assert(SQLITE_OK == rslt);
        rslt = sqlite3_bind_int(insStmtS, 3, k);
        query.bindValue(":dim_k", k);
        assert(SQLITE_OK == rslt);
        rslt = sqlite3_bind_double(insStmtS, 4, ai->vSal(k, 0)); // was sal
        query.bindValue(":sal", ai->vSal(k, 0));
        assert(SQLITE_OK == rslt);
        // record
        rslt = sqlite3_step(insStmtS);
        if (!query.exec()) {
          cout << query.lastError().text().toStdString() << endl;
        }
        assert(SQLITE_DONE == rslt);
        sqlite3_clear_bindings(insStmtS);
        assert(SQLITE_DONE == rslt);
        rslt = sqlite3_reset(insStmtS);
        assert(SQLITE_OK == rslt);
      }
    }
  }

  query.prepare(QString::fromStdString(sqlSc1));
  //ScenarioDesc table
  rslt = sqlite3_bind_int(insStmtScene, 1, static_cast<int>(vrCltn));
  query.bindValue(":vr", static_cast<int>(vrCltn));
  assert(SQLITE_OK == rslt);
  rslt = sqlite3_bind_int(insStmtScene, 2, static_cast<int>(bigRAdj));
  query.bindValue(":br", static_cast<int>(bigRAdj));
  assert(SQLITE_OK == rslt);
  rslt = sqlite3_bind_int(insStmtScene, 3, static_cast<int>(bigRRng));
  query.bindValue(":brr", static_cast<int>(bigRRng));
  assert(SQLITE_OK == rslt);
  rslt = sqlite3_bind_int(insStmtScene, 4, static_cast<int>(tpCommit));
  query.bindValue(":tpc", static_cast<int>(tpCommit));
  assert(SQLITE_OK == rslt);
  rslt = sqlite3_bind_int(insStmtScene, 5, static_cast<int>(ivBrgn));
  query.bindValue(":ivb", static_cast<int>(ivBrgn));
  assert(SQLITE_OK == rslt);
  rslt = sqlite3_bind_int(insStmtScene, 6, static_cast<int>(brgnMod));
  query.bindValue(":bm", static_cast<int>(brgnMod));
  assert(SQLITE_OK == rslt);

  rslt = sqlite3_step(insStmtScene);
  if (!query.exec()) {
    cout << query.lastError().text().toStdString() << endl;
  }
  assert(SQLITE_DONE == rslt);
  sqlite3_clear_bindings(insStmtScene);
  assert(SQLITE_DONE == rslt);
  rslt = sqlite3_reset(insStmtScene);
  assert(SQLITE_OK == rslt);

  // finish
  sqlite3_exec(smpDB, "END TRANSACTION", NULL, NULL, &zErrMsg);
  qtDB->commit();
  // finalize statement to avoid resource leaks
  sqlite3_finalize(insStmtD);
  sqlite3_finalize(insStmtC);
  sqlite3_finalize(insStmtS);
  sqlite3_finalize(insStmtScene);
  sqlite3_finalize(insStmtAccomod);

  return;
}


// --------------------------------------------


void bindExecuteBargnTableUpdate(sqlite3_stmt *updateStmt, size_t turn, int bargnID,
                                 int initActor, double initProb, bool isInitSelected,
                                 int recvActor, double recvProb, bool isRecvSelected)  {

  int rslt = 0;

  rslt = sqlite3_bind_double(updateStmt, 1, initProb);
  assert(SQLITE_OK == rslt);

  //Init_Seld
  rslt = isInitSelected ? sqlite3_bind_int(updateStmt, 2, 1) : sqlite3_bind_int(updateStmt, 2, 0);
  assert(SQLITE_OK == rslt);

  // For SQ cases, there would be no receiver
  if (initActor != recvActor) {
    rslt = sqlite3_bind_double(updateStmt, 3, recvProb);
    assert(SQLITE_OK == rslt);

    //Recd_Seld
    rslt = isRecvSelected ? sqlite3_bind_int(updateStmt, 4, 1) : sqlite3_bind_int(updateStmt, 4, 0);
    assert(SQLITE_OK == rslt);
  }

  rslt = sqlite3_bind_int(updateStmt, 5, turn);
  assert(SQLITE_OK == rslt);

  rslt = sqlite3_bind_int(updateStmt, 6, bargnID);
  assert(SQLITE_OK == rslt);

  rslt = sqlite3_bind_int(updateStmt, 7, initActor);
  assert(SQLITE_OK == rslt);

  rslt = sqlite3_bind_int(updateStmt, 8, recvActor);
  assert(SQLITE_OK == rslt);

  rslt = sqlite3_step(updateStmt);
  assert(SQLITE_DONE == rslt);
  sqlite3_clear_bindings(updateStmt);
  assert(SQLITE_DONE == rslt);
  rslt = sqlite3_reset(updateStmt);
  assert(SQLITE_OK == rslt);

  return;
}



void SMPState::updateBargnTable(const vector<vector<BargainSMP*>> & brgns,
                                map<unsigned int, KBase::KMatrix>  actorBargains,
                                map<unsigned int, unsigned int>   actorMaxBrgNdx) const {

  const unsigned int t = myTurn();

  string sql = string("UPDATE Bargn SET Init_Prob = :init_prob, Init_Seld = :init_seld, "
    "Recd_Prob = :recd_prob, Recd_Seld = :recd_seld "
    "WHERE ('" + model->getScenarioID() + "' = ScenarioId) "
    "and (:turn_t = Turn_t) and (:bgnId = BargnId) "
    "and (:init_act_i = Init_Act_i) and (:recd_act_j = Recd_Act_j)");

  QSqlQuery query = model->getQuery();

  query.prepare(QString::fromStdString(sql));

  auto updateBargn = [&query, t](int bargnID,
    int initActor, double initProb, int isInitSelected,
    int recvActor, double recvProb, int isRecvSelected) {

    query.bindValue(":init_prob", initProb);

    query.bindValue(":init_seld", isInitSelected);

    // For SQ cases, there would be no receiver
    if (initActor != recvActor) {
      query.bindValue(":recd_prob", recvProb);

      query.bindValue(":recd_seld", isRecvSelected);
    }
    else {
      // Pass NULL values for SQ cases
      query.bindValue(":recd_prob", QVariant::Double);

      query.bindValue(":recd_seld", QVariant::Int);
    }

    query.bindValue(":turn_t", t);

    query.bindValue(":bgnId", bargnID);

    query.bindValue(":init_act_i", initActor);

    query.bindValue(":recd_act_j", recvActor);

    if (!query.exec()) {
      cout << query.lastError().text().toStdString() << endl;
    }

    return;
  };

  sqlite3 *db = model->smpDB;
  auto sqlBuff = newChars(300);

  // JAH 20160822 fixed the oversight of not conditioning on the scenario
  sprintf(sqlBuff, "UPDATE Bargn SET Init_Prob = ?1, Init_Seld = ?2, Recd_Prob = ?3, Recd_Seld = ?4 \
          WHERE (\"%s\" = ScenarioId) and (?5 = Turn_t) and (?6 = BargnId) and (?7 = Init_Act_i) and (?8 = Recd_Act_j)",
                  model->getScenarioID().c_str());

  const char* updateStr = sqlBuff;
  sqlite3_stmt *updateStmt = nullptr;

  // prepare the sql statement to update the db
  sqlite3_prepare_v2(model->smpDB, updateStr, strlen(updateStr), &updateStmt, NULL);

  assert(nullptr != updateStmt); // make sure it is ready

  // Error message in case
  char* zErrMsg = nullptr;

  // start for the transaction
  sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);
  model->beginDBTransaction();

  // Update the bargain table for the bargain values for init actor and recd actor
  // along with the info whether a bargain got selected or not in the respective actor's queue
  for (unsigned int i = 0; i < brgns.size(); i++) {
    auto ai = ((const SMPActor*)(model->actrs[i]));
    double initProb = -0.1;
    int initSelected = -1;
    auto bargains_i = brgns[i];
    int initBgnNdx = 0;
    auto initActr = -1;
    auto rcvrActr = -1;
    //uint64_t bgID = 0; // tag uninitialized value
    int countDown = 2; // Stop iterating if cases for i:i and i:j processed
    for (auto bg : bargains_i) {
      assert(nullptr != bg);
      if (bg->actInit == bg->actRcvr) { // For SQ case
        initActr = model->actrNdx(bg->actInit);
        rcvrActr = initActr;
        initProb = (actorBargains[initActr])(initBgnNdx, 0);
        initSelected = initBgnNdx == actorMaxBrgNdx[initActr] ? 1 : 0;

        bindExecuteBargnTableUpdate(updateStmt, t, bg->getID(),
                                    initActr, initProb, initSelected,
                                    rcvrActr, -1.0, 0);
        updateBargn(bg->getID(),
          initActr, initProb, initSelected,
          rcvrActr, -1.0, 0);

        if (0 == --countDown) {
          break;
        }
      }
      else {
        if (ai == bg->actInit) { // this bargain is initiated by current actor
          initActr = model->actrNdx(bg->actInit);
          initProb = (actorBargains[initActr])(initBgnNdx, 0);
          initSelected = initBgnNdx == actorMaxBrgNdx[initActr] ? 1 : 0;
          rcvrActr = model->actrNdx(bg->actRcvr);

          // Get the bargains of receiver actor
          auto brgnRcvr = brgns[rcvrActr];
          int rcvrBgNdx = 0;
          double rcvrProb = -1.0;
          int rcvrSelected = -1;
          for (auto bgRcv : brgnRcvr) {
            assert(nullptr != bgRcv);
            if (ai == bgRcv->actInit) {
              rcvrProb = (actorBargains[rcvrActr])(rcvrBgNdx, 0);

              // Check if it is the selected bargain for receiver actor
              rcvrSelected = actorMaxBrgNdx[rcvrActr] == rcvrBgNdx ? 1 : 0;

              --countDown;
              bindExecuteBargnTableUpdate(updateStmt, t, bg->getID(),
                                          initActr, initProb, initSelected,
                                          rcvrActr, rcvrProb, rcvrSelected);
              updateBargn(bg->getID(),
                initActr, initProb, initSelected,
                rcvrActr, rcvrProb, rcvrSelected);
              break;
            }
            ++rcvrBgNdx;
          }

          if (0 == countDown) {
            break;
          }
        }
      }

      ++initBgnNdx;
    }
  }

  sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &zErrMsg);
  model->commitDBTransaction();

  sqlite3_finalize(updateStmt); // finalize statement to avoid resource leaks

  delete sqlBuff;
  sqlBuff = nullptr;
  model->smpDB = db;
  return;
}

void SMPState::recordProbEduChlg() const {
  size_t basePos = -1;
  size_t digCount = 0;
  size_t nextCommaPos = string::npos;

  auto nextActor = [&basePos, &nextCommaPos](string &actors) {
    basePos = nextCommaPos + 1;
    nextCommaPos = actors.find_first_of(",", basePos);
    size_t digCount = nextCommaPos - basePos;
    return actors.substr(basePos, digCount);
  };

  QSqlQuery query = model->getQuery();
  string qsql;
  qsql = string("INSERT INTO TP_Prob_Vict_Loss "
    "(ScenarioId, Turn_t, Est_h, Init_i, ThrdP_k, Rcvr_j, Prob, Util_V, Util_L) "
    "VALUES ("
    "'") + model->getScenarioID() + "',"
    " :t, :h, :i, :thrdp_k, :j, :prob, :util_v, :util_l )";

  query.prepare(QString::fromStdString(qsql));

  model->beginDBTransaction();
  for (auto &tpv : tpvData) {
    auto thij = tpv.first;
    auto tpvArray = tpv.second;

    basePos = -1;
    digCount = 0;
    nextCommaPos = string::npos;

    auto t = std::stoi(nextActor(thij));
    auto h = std::stoi(nextActor(thij));
    auto i = std::stoi(nextActor(thij));
    auto j = std::stoi(nextActor(thij));

    query.bindValue(":t", t);
    query.bindValue(":h", h);
    query.bindValue(":i", i);
    query.bindValue(":j", j);

    const unsigned int na = model->numAct;

    for (int tpk = 0; tpk < na; tpk++) {  // third party voter, tpk
      query.bindValue(":thrdp_k", tpk);

      // bind the data
      query.bindValue(":prob", tpvArray(tpk, 0));
      query.bindValue(":util_v", tpvArray(tpk, 1));
      query.bindValue(":util_l", tpvArray(tpk, 2));

      // actually record it
      if (!query.exec()) {
        cout << query.lastError().text().toStdString() << endl;
      }
    }
  }

  qsql = string("INSERT INTO ProbVict "
    "(ScenarioId, Turn_t, Est_h,Init_i,Rcvr_j,Prob) VALUES ('")
    + model->getScenarioID() + "', :t, :h, :i, :j, :phij)";

  query.prepare(QString::fromStdString(qsql));

  for (auto &phijVal : phijData) {
    auto thij = phijVal.first;
    auto phij = phijVal.second;

    basePos = -1;
    digCount = 0;
    nextCommaPos = string::npos;

    auto t = std::stoi(nextActor(thij));
    auto h = std::stoi(nextActor(thij));
    auto i = std::stoi(nextActor(thij));
    auto j = std::stoi(nextActor(thij));

    query.bindValue(":t", t);
    query.bindValue(":h", h);
    query.bindValue(":i", i);
    query.bindValue(":j", j);
    query.bindValue(":phij", phij);

    // actually record it
    if (!query.exec()) {
      cout << query.lastError().text().toStdString() << endl;
    }
  }

  qsql = string("INSERT INTO UtilChlg "
    "(ScenarioId, Turn_t, Est_h,Aff_k,Init_i,Rcvr_j,Util_SQ,Util_Vict,Util_Cntst,Util_Chlg) VALUES ('")
    + model->getScenarioID() + "', :t, :h, :k, :i, :j, :euSQ, :euVict, :euCntst, :euChlg)";

  query.prepare(QString::fromStdString(qsql));

  for (auto &euVal : euData) {
    auto thkij = euVal.first;
    basePos = -1;
    digCount = 0;
    nextCommaPos = string::npos;

    auto t = std::stoi(nextActor(thkij));
    auto h = std::stoi(nextActor(thkij));
    auto k = std::stoi(nextActor(thkij));
    auto i = std::stoi(nextActor(thkij));
    auto j = std::stoi(nextActor(thkij));

    auto eu = euVal.second;
    auto euSQ = eu[0];
    auto euVict = eu[1];
    auto euCntst = eu[2];
    auto euChlg = eu[3];

    query.bindValue(":t", t);
    query.bindValue(":h", h);
    query.bindValue(":k", k);
    query.bindValue(":i", i);
    query.bindValue(":j", j);
    query.bindValue(":euSQ", euSQ);
    query.bindValue(":euVict", euVict);
    query.bindValue(":euCntst", euCntst);
    query.bindValue(":euChlg", euChlg);

    // actually record it
    if (!query.exec()) {
      cout << query.lastError().text().toStdString() << endl;
    }
  }

  model->commitDBTransaction();
  return;
}

};
// end of namespace

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
