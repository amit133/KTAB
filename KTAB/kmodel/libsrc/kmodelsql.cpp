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

//#include <assert.h>
#include <easylogging++.h>
#include <sstream>
#include <algorithm>

#include "kmodel.h"

#include <QVariant>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDriver>

namespace KBase
{

using std::get;
using std::tuple;

QString Model::dbDriver;
QString Model::server;
int Model::port=5432; // Default port for postgresql
QString Model::databaseName;
QString Model::userName;
QString Model::password;

void Model::initDBDriver(QString connectionName) {
  if (QSqlDatabase::contains(connectionName)) {
    LOG(INFO) << "A database connection already exists with the name: " << connectionName.toStdString();
    return;
  }
  QSqlDatabase qdb = QSqlDatabase::addDatabase(dbDriver, connectionName);
  qtDB = new QSqlDatabase(qdb);
}

bool Model::connectDB() {
  return connect(server, port, databaseName, userName, password);
}

void Model::closeDB()
{
  if(qtDB != nullptr && qtDB->isValid() && qtDB->isOpen()) {
      query.clear();
      qtDB->close();
  }
}

bool Model::connect(const QString& server,
  const int port,
  const QString& databaseName,
  const QString& userName,
  const QString& password)
{
  qtDB->setDatabaseName(databaseName);
  qtDB->setHostName(server);
  qtDB->setPort(port);

  return qtDB->open(userName, password);
}

bool Model::isDB(const QString& databaseName) {
  QString stmt = "select 1 from pg_database where datname = ";
  stmt.append(databaseName);
  query.exec(stmt);
  if (query.size() == 1) {
    // database exists
    return true;
  }
  return false;
}

bool Model::createDB(const QString& dbName) {
  QString createDBqry("CREATE DATABASE \"");
  createDBqry.append(dbName);
  createDBqry.append("\"");
  LOG(INFO) << createDBqry.toStdString();

  if (!query.exec(createDBqry)) {
    LOG(INFO) << query.lastError().text().toStdString();
    return false;
  }

  return true;
}

void Model::configSqlite() const {
  // As we are not dealing with a long-term, mission-critical database,
  // we can shut off some of the journaling stuff intended to protect
  // the DB in case the system crashes in mid-operation.
  // Eliminating these checks can significantly speed operations.
  query.exec("PRAGMA journal_mode = MEMORY");
  query.exec("PRAGMA locking_mode = EXCLUSIVE");
  query.exec("PRAGMA synchronous = OFF");

  // not a performance issue, but necessary for the data layout
  query.exec("PRAGMA foreign_keys = ON");
}

void Model::execQuery(std::string& qry) {
  if (!query.exec(QString::fromStdString(qry))) {
    LOG(INFO) << "Failed Query: " << qry;
    LOG(INFO) << query.lastError().text().toStdString();
    //assert(false);
    throw KException("Model::execQuery:s DB query failed.");
  }
}

void Model::beginDBTransaction() {
  qtDB->transaction();
}

void Model::commitDBTransaction() {
  qtDB->commit();
}

QSqlQuery Model::getQuery()
{
  return query;
}

// JAH 20160728 added KTable class constructor
KTable::KTable(unsigned int ID, const string &name, const string &SQL, unsigned int grpID)
{
  tabID = ID;
  tabName = name;
  tabSQL = SQL;
  tabGrpID = grpID;
}

KTable::~KTable() {};

void Model::demoSQLite()
{
  LOG(INFO) << "Starting basic demo of SQLite in Model class";

  auto callBack = [](void *data, int numCol, char **stringFields, char **colNames)
  {
    for (int i = 0; i < numCol; i++)
    {
      LOG(INFO) << colNames[i] << "=" << (stringFields[i] ? stringFields[i] : "NULL");
    }
    return ((int)0);
  };

  sqlite3 *db = nullptr;
  char* zErrMsg = nullptr;
  int  rc;
  string sql;

  auto sOpen = [&db](unsigned int n)
  {
    int rc = sqlite3_open("test.db", &db);
    if (rc != SQLITE_OK)
    {
      LOG(INFO) << "Can't open database:" << sqlite3_errmsg(db);
      exit(0);
    }
    else
    {
      LOG(INFO) << "Opened database successfully" << n;
    }
    return;
  };

  auto sExec = [callBack, &db, &zErrMsg](string sql, string msg)
  {
    int rc = sqlite3_exec(db, sql.c_str(), callBack, nullptr, &zErrMsg); // nullptr is the 'data' argument
    if (rc != SQLITE_OK)
    {
      LOG(INFO) << "SQL error:" << zErrMsg;
      sqlite3_free(zErrMsg);
    }
    else
    {
      LOG(INFO) << msg;
    }
    return rc;
  };


  // Open database
  sOpen(1);
  
  // try some pragmas. these should speed up operations on larger tables,
  // at the expense of making the table vulnerable to corruption if the
  // application abruptly terminates mid-operation.
  sql = "PRAGMA synchronous = off;";
  rc = sExec(sql, "Set synchronous to off \n");
  assert(SQLITE_OK == rc);
  
  sql = "PRAGMA journal_mode = off;";
  rc = sExec(sql, "Set journal_mode to off \n");
  assert(SQLITE_OK == rc);
  
  sql = "PRAGMA locking_mode = exclusive;";
  rc = sExec(sql, "Set locking_mode to exclusive \n");
  assert(SQLITE_OK == rc);

  // Create SQL statement
  sql = "create table if not exists PETS("  \
        "ID INT PRIMARY KEY     NOT NULL," \
        "NAME           text    NOT NULL," \
        "AGE            int     NOT NULL," \
        "BREED         char(50)," \
        "COLOR         char(50) );";

  // Execute SQL statement
  rc = sExec(sql, "Created table successfully \n");
  assert(SQLITE_OK == rc);
  rc = sqlite3_close(db);
  assert(SQLITE_OK == rc);


  sOpen(2);
  

  // This SQL statement has one deliberate error.
  sql = "INSERT INTO PETS (ID, NAME, AGE, BREED, COLOR) "   \
        "VALUES (1, 'Alice', 6, 'Greyhound', 'Grey' ); "    \
        "INSERT INTO PETS (ID, NAME, AGE, BREED, COLOR) "   \
        "VALUES (2, 'Bob', 4, 'Newfoundland', 'Black' ); "  \
        "INSERT INTO PETS (ID, NAME, AGE, BREED, COLOR) "   \
        "VALUES (3, 'Carol', 7, 'Chihuahua', 'Tan' );"      \
        "INSERT INTO PETS (ID, NAME, AGE, SPECIES, COLOR) " \
        "VALUES (4, 'David', 5, 'Alsation', 'Mixed' );";
  /*
  "INSERT INTO PETS (ID, NAME, AGE, SPECIES, COLOR) " \
  "VALUES (5, 'Ellie', 8, 'Rhodesian', 'Red' );";
  */


  LOG(INFO) << "NB: This should get one planned SQL error at ID=4"; // SPECIES should be BREED
  rc = sExec(sql, "Records inserted successfully \n");
  assert(SQLITE_OK != rc); // check for planned SQL error
  rc = sqlite3_close(db);
  assert(SQLITE_OK == rc); // avoid EFFCPP warning


  sOpen(3);
  sql = "SELECT * from PETS where AGE>5;";
  rc = sExec(sql, "Records selected successfully\n");
  assert(SQLITE_OK == rc);
  LOG(INFO) << "NB: ID=5 was never inserted due to planned SQL error at ID=4";
  sqlite3_close(db);
  assert(SQLITE_OK == rc);


  sOpen(4);
  sql = "DROP TABLE PETS;";
  rc = sExec(sql, "Dropped table successfully \n");
  assert(SQLITE_OK == rc);
  rc = sqlite3_close(db);
  assert(SQLITE_OK == rc);

  return;
}

bool testMultiThreadSQLite (bool tryReset, KBase::ReportingLevel rl) { 
  int mutexP = sqlite3_threadsafe() ;
  bool parP = (0 != mutexP);
  if (ReportingLevel::Silent < rl) {
    if (parP) {
      LOG(INFO) << "This SQLite3 library WAS compiled to be threadsafe.";
    } else {
      LOG(INFO) << "This SQLite3 library was NOT compiled to be threadsafe.";
    }
  }
  if (tryReset && (!parP)) {
    int configRslt = sqlite3_config(SQLITE_CONFIG_SERIALIZED);
    if (configRslt == SQLITE_OK) {
      parP = true;
    }
    else  {
      parP = false;
    }
    if (ReportingLevel::Low < rl){
      LOG(INFO) << "  Note that multi-threading might have been disabled via sqlite3_config.";
      LOG(INFO) << "  Tried to reconfigure to SQLITE_CONFIG_SERIALIZED ... ";
      if (configRslt == SQLITE_OK) {
        LOG(INFO) << "SUCCESS";
      }
      else  {
        LOG(INFO) << "FAILED";
      }
    }
  }
  if (ReportingLevel::Silent < rl) {
    if (parP)  {
      LOG(INFO) << "Possible to continue multi-threaded";
    }
    else {
      LOG(INFO) << "Necessary to continue single-threaded";
    }
  }
  return parP;
}

// note that the function to write to table #k must be kept
// synchronized with the result of createSQL(k) !
// JAH 20160728 modified to return a KTable object instead of the SQL string
KTable * Model::createSQL(unsigned int n)
{

  string sql = "";
  string name = "";
  unsigned int grpID = 0;

  //assert(n < Model::NumTables);
  if (n >= Model::NumTables) {
    throw KException("Model::createSQL: Wrong count of tables for creation");
  }
  switch (n)
  {
  case 0:
    // position-utility table
    // the estimated utility to each actor of each other's position
    sql = "create table if not exists PosUtil ("  \
          "ScenarioId VARCHAR(32) NOT NULL DEFAULT 'None', "\
          "Turn_t     INTEGER     NOT NULL DEFAULT 0, "\
          "Est_h      INTEGER     NOT NULL DEFAULT 0, "\
          "Act_i      INTEGER     NOT NULL DEFAULT 0, "\
          "Pos_j      INTEGER     NOT NULL DEFAULT 0, "\
          "Util       FLOAT       NOT NULL DEFAULT 0.0"\
          ");";
    name = "PosUtil";
    grpID = 4;
    break;

  case 1: // pos-vote table
    // estimated vote of each actor between each pair of positions
    sql = "create table if not exists PosVote ("  \
          "ScenarioId VARCHAR(32) NOT NULL DEFAULT 'None', "\
          "Turn_t     INTEGER     NOT NULL DEFAULT 0, "\
          "Est_h      INTEGER     NOT NULL DEFAULT 0, "\
          "Voter_k    INTEGER     NOT NULL DEFAULT 0, "\
          "Pos_i      INTEGER     NOT NULL DEFAULT 0, "\
          "Pos_j      INTEGER     NOT NULL DEFAULT 0, "\
          "Vote       FLOAT       NOT NULL DEFAULT 0.0"\
          ");";
    name = "PosVote";
    grpID = 1;
    break;

  case 2: // pos-prob table. Note that there may be duplicates, unless we limit it to unique positions
    sql = "create table if not exists PosProb ("  \
          "ScenarioId VARCHAR(32) NOT NULL DEFAULT 'None', "\
          "Turn_t     INTEGER     NOT NULL DEFAULT 0, "\
          "Est_h      INTEGER     NOT NULL DEFAULT 0, "\
          "Pos_i      INTEGER     NOT NULL DEFAULT 0, "\
          "Prob       FLOAT       NOT NULL DEFAULT 0.0"\
          ");";
    name = "PosProb";
    grpID = 1;
    break;

  case 3: // pos-equiv table. E(i)= lowest j s.t. Pos(i) ~ Pos(j). if j < i, it is not unique.
    sql = "create table if not exists PosEquiv ("  \
          "ScenarioId VARCHAR(32) NOT NULL DEFAULT 'None', "\
          "Turn_t     INTEGER     NOT NULL DEFAULT 0, "\
          "Pos_i      INTEGER     NOT NULL DEFAULT 0, "\
          "Eqv_j      INTEGER     NOT NULL DEFAULT 0 "\
          ");";
    name = "PosEquiv";
    grpID = 1;
    break;

  case 4:
    // h's estimate of the utility to k of i challenging j, preceded by intermediate values
    // U{^h}{_k} ( SQ ), current status quo positions
    // U{^h}{_k} ( i >  j ), i defeats j
    // U{^h}{_k} ( i :  j ), contest between i and j
    // U{^h}{_k} ( i => j ), i challenges j
    // Utilities are evaluated so that UtilSQ, UtilVict, UtilChlg, UtilContest,
    // UtilTPVict, UtilTPLoss are comparable, i.e. the differences are meaningful
    sql = "create table if not exists UtilChlg ("  \
          "ScenarioId VARCHAR(32) NOT NULL DEFAULT 'none', "\
          "Turn_t     INTEGER     NOT NULL DEFAULT 0, "\
          "Est_h      INTEGER     NOT NULL DEFAULT 0, "\
          "Aff_k      INTEGER     NOT NULL DEFAULT 0, "\
          "Init_i     INTEGER     NOT NULL DEFAULT 0, "\
          "Rcvr_j     INTEGER     NOT NULL DEFAULT 0, "\
          "Util_SQ    FLOAT       NOT NULL DEFAULT 0, "\
          "Util_Vict  FLOAT       NOT NULL DEFAULT 0, "\
          "Util_Cntst FLOAT       NOT NULL DEFAULT 0, "\
          "Util_Chlg  FLOAT       NOT NULL DEFAULT 0  "\
          ");";
    name = "UtilChlg";
    grpID = 2;
    break;

  case 5:
    // h's estimate that i will defeat j, including all third party contributions
    // P{^h}( i > j )
    sql = "create table if not exists ProbVict ("  \
          "ScenarioId VARCHAR(32) NOT NULL DEFAULT 'None', "\
          "Turn_t     INTEGER     NOT NULL DEFAULT 0, "\
          "Est_h      INTEGER     NOT NULL DEFAULT 0, "\
          "Init_i     INTEGER     NOT NULL DEFAULT 0, "\
          "Rcvr_j     INTEGER     NOT NULL DEFAULT 0, "\
          "Prob       FLOAT       NOT NULL DEFAULT 0"\
          ");";
    name = "PosVict";
    grpID = 2;
    break;

  case 6:
    // h's estimates in a little 3-actor contest with no contribution from others:
    // P{^h}( ik > j )      probability ik defeats j,
    // U{^h}{_k} (ik > j)   utility to k of winning with i, over j
    // U{^h}{_k} (i > jk)   utility to k of losing to i, with j
    sql = "create table if not exists TPProbVictLoss ("  \
          "ScenarioId VARCHAR(32) NOT NULL DEFAULT 'None', "\
          "Turn_t     INTEGER     NOT NULL DEFAULT 0, "\
          "Est_h      INTEGER     NOT NULL DEFAULT 0, "\
          "Init_i     INTEGER     NOT NULL DEFAULT 0, "\
          "ThrdP_k    INTEGER     NOT NULL DEFAULT 0, "\
          "Rcvr_j     INTEGER     NOT NULL DEFAULT 0, "\
          "Prob       FLOAT       NOT NULL DEFAULT 0, "\
          "Util_V     FLOAT       NOT NULL DEFAULT 0, "\
          "Util_L     FLOAT       NOT NULL DEFAULT 0  "\
          ");";
    name = "TPProbVictLoss";
    grpID = 2;
    break;

  case 7:   // short-name and long-description of actors
  {
    char *sqlBuff = newChars(500);
    sprintf(sqlBuff, "create table if not exists ActorDescription ("  \
                     "ScenarioId VARCHAR(32) NOT NULL DEFAULT 'None', "\
                     "Act_i      INTEGER NOT NULL DEFAULT 0, "\
                     "Name       VARCHAR(%u) NOT NULL DEFAULT 'NoName', "\
                     "\"Desc\"   VARCHAR(%u) NOT NULL DEFAULT 'NoName' "\
                     ");", maxActNameLen, maxActDescLen);
    sql = std::string(sqlBuff);
    delete sqlBuff;
    sqlBuff = nullptr;
    name = "ActorDescription";
    grpID = 0;
  }
    break;

  case 8: // Bargain table
    sql = "create table if not exists Bargn ("  \
          "ScenarioId VARCHAR(32) NOT NULL DEFAULT 'None', "\
          "Turn_t INTEGER NOT NULL DEFAULT 0, "\
          "BargnId INTEGER NOT NULL DEFAULT 0, "\
          "Init_Act_i INTEGER NOT NULL DEFAULT 0, "\
          "Recd_Act_j INTEGER NOT NULL DEFAULT 0, "\
          "Value FLOAT NOT NULL DEFAULT 0.0, "\
          "Init_Prob FLOAT NULL DEFAULT 0, "\
          "Init_Seld INTEGER NULL ,"\
          "Recd_Prob FLOAT NULL DEFAULT 0, "\
          "Recd_Seld INTEGER NULL, "\
          "CHECK (Init_Seld in (0,1) AND Recd_Seld in (0,1))"\
          ");";
    name = "Bargn";
    grpID = 4;
    break;
  case 9:  //
    // BargnCoords table
    sql = "create table if not exists BargnCoords ("  \
          "ScenarioId VARCHAR(32) NOT NULL DEFAULT 'None', "\
          "Turn_t INTEGER NOT NULL DEFAULT 0, "\
          "BargnId INTEGER NOT NULL DEFAULT 0, "\
          "Dim_k INTEGER NOT NULL DEFAULT 0, "\
          "Init_Coord FLOAT NULL DEFAULT 0.0,"\
          "Recd_Coord FLOAT NOT NULL DEFAULT 0.0"\
          ");";
    name = "BargnCoords";
    grpID = 3;
    break;

  case 10:  // BargnUtil table creation
    sql = "create table if not exists BargnUtil ("  \
          "ScenarioId VARCHAR(32) NOT NULL DEFAULT 'None', "\
          "Turn_t INTEGER NOT NULL DEFAULT 0, "\
          "BargnId    INTEGER NOT NULL DEFAULT 0, "\
          "Act_i  INTEGER NOT NULL DEFAULT 0, "\
          "Util FLOAT NOT NULL DEFAULT 0.0"\
          ");";
    name = "BargnUtil";
    grpID = 3;
    break;

  case 11:
    // During bargain resolution, this table records the actual vote
    // of each actor between each pair of competing bargains
    sql = "create table if not exists BargnVote ("  \
          "ScenarioId VARCHAR(32) NOT NULL DEFAULT 'None', "\
          "Turn_t INTEGER NOT NULL DEFAULT 0, "\
          "BargnId_i  INTEGER NOT NULL DEFAULT 0, "\
          "BargnId_j INTEGER NOT NULL DEFAULT 0, "\
          "Act_k  INTEGER NOT NULL DEFAULT 0, "\
          "Vote FLOAT NOT NULL DEFAULT 0.0"\
          ");";
    name = "BargnVote";
    grpID = 3;
    break;

  case 12:  //ScenarioDesc creation
    // JAH 20160711 added the RNGSeed field
    sql = "create table if not exists ScenarioDesc ("  \
          "Scenario VARCHAR(512) NOT NULL DEFAULT 'NoName', "\
          "\"Desc\" VARCHAR(512) NOT NULL DEFAULT 'No Description', "\
          "ScenarioId VARCHAR(32) NOT NULL UNIQUE DEFAULT 'None'," \
          "RNGSeed VARCHAR(20) NOT NULL DEFAULT '0'," \
          "VictoryProbModel INTEGER NULL DEFAULT NULL," \
          "ProbCondorcetElection INTEGER NULL DEFAULT NULL," \
          "StateTransition INTEGER NULL DEFAULT NULL," \
          "VotingRule INTEGER NULL DEFAULT NULL," \
          "BigRAdjust INTEGER NULL DEFAULT NULL," \
          "BigRRange INTEGER NULL DEFAULT NULL," \
          "ThirdPartyCommit INTEGER NULL DEFAULT NULL," \
          "InterVecBrgn INTEGER NULL DEFAULT NULL," \
          "BargnModel INTEGER NULL DEFAULT NULL" \
          ");";
    name = "ScenarioDesc";
    grpID = 0;
    break;
  default:
    throw(KException("Model::createSQL unrecognized table number"));
  }

  //assert(grpID < NumSQLLogGrps);
  if (grpID >= NumSQLLogGrps) {
    throw KException("Model::createSQL: group id should be within the allowed range");
  }
  auto tab = new KTable(n,name,sql,grpID);
  return tab;
}


void Model::sqlAUtil(unsigned int t)
{
  //assert(t < history.size());
  if (t >= history.size()) {
    throw KException("Model::sqlAUtil: Specified turn number is beyond the size of history");
  }
  State* st = history[t];
  //assert(nullptr != st);
  if (nullptr == st) {
    throw KException("Model::sqlAUtil: st is a null pointer.");
  }
  //assert(numAct == st->aUtil.size());
  if (numAct != st->aUtil.size()) {
    throw KException("Model::sqlAUtil: Not all actors have utility values.");
  }

  // I don't like passing 'this' into lambda-functions,
  // so I copy the pointer into a local variable I can pass in
  // without exposing everything in 'this'. If this were a big,
  // mission-critical RDBMS, rather than a 1-off record of this run,
  // doing so might be disasterous in case the system crashed before
  // things were cleaned up.
  string sql = "INSERT INTO PosUtil (ScenarioId, Turn_t, Est_h, Act_i, Pos_j, Util) VALUES ('"
    + scenId + "', :turn_t, :est_h, :act_i, :pos_j, :util)";

  query.prepare(QString::fromStdString(sql));

  // Prepared statements cache the execution plan for a query after the query optimizer has
  // found the best plan, so there is no big gain with simple insertions.
  // What makes a huge difference is bundling a few hundred into one atomic "transaction".
  // For this case, runtime droped from 62-65 seconds to 0.5-0.6 (vs. 0.30-0.33 with no SQL at all).
  qtDB->transaction();

  for (unsigned int h = 0; h < numAct; h++)   // estimator is h
  {
    KMatrix uij = st->aUtil[h]; // utility to actor i of the position held by actor j
    for (unsigned int i = 0; i < numAct; i++)
    {
      for (unsigned int j = 0; j < numAct; j++)
      {
        query.bindValue(":turn_t", t);
        query.bindValue(":est_h", h);
        query.bindValue(":act_i", i);
        query.bindValue(":pos_j", j);
        query.bindValue(":util", uij(i, j));
        if (!query.exec()) {
          LOG(INFO) << query.lastError().text().toStdString();
          //assert(false);
          throw KException("Model::sqlAUtil: DB query failed");
        }
      }
    }
  }
  qtDB->commit();
  return;
}

// populates record for table PosEquiv for each step of
// module run
void Model::sqlPosEquiv(unsigned int t)
{
  //assert(t < history.size());
  if (t >= history.size()) {
    throw KException("Model::sqlPosEquiv: Specified turn number is beyond the size of history");
  }
  State* st = history[t];
  //assert(nullptr != st);
  if (nullptr == st) {
    throw KException("Model::sqlPosEquiv: st is a null pointer.");
  }

  string qsql = string("INSERT INTO PosEquiv (ScenarioId, Turn_t, Pos_i, Eqv_j) VALUES ('")
    + scenId + "', :turn_t, :pos_i, :eqv_j)";
  query.prepare(QString::fromStdString(qsql));

  qtDB->transaction();

  // Start inserting record
  for (unsigned int i = 0; i < numAct; i++)
  {
    // calculate the equivalance
    int je = numAct + 1;
    for (unsigned int j = 0; j < numAct && je > numAct; j++)
    {
      if (st->equivNdx(i, j))
      {
        je = j;
      }
    }
    query.bindValue(":turn_t", t);
    query.bindValue(":pos_i", i);
    query.bindValue(":eqv_j", je);
    if (!query.exec()) {
      LOG(INFO) << query.lastError().text().toStdString();
      //assert(false);
      throw KException("Model::sqlPosEquiv: DB query failed");
    }
  }
  // end databse transaction
  qtDB->commit();

  return;
}

void Model::sqlBargainEntries(unsigned int t, int bargainId, int initiator, int receiver, double val)
{
  // prepare the sql statement to insert
  string sql = string("INSERT INTO Bargn (ScenarioId, Turn_t, BargnID, Init_Act_i, Recd_Act_j, Value) VALUES ('")
    + scenId + "', :turn_t, :bargnid, :init_i, :recd_j, :value)";
  query.prepare(QString::fromStdString(sql));

  // start for the transaction
  //qtDB->transaction();

  // Turn_t
  query.bindValue(":turn_t", t);
  //BargnID
  query.bindValue(":bargnid", bargainId);
  //Init_Act_i
  query.bindValue(":init_i", initiator);
  //Recd_Act_j
  query.bindValue(":recd_j", receiver);
  //Value
  query.bindValue(":value", val);
  if (!query.exec()) {
    LOG(INFO) << query.lastError().text().toStdString();
    //assert(false);
    throw KException("Model::sqlBargainEntries: DB query failed");
  }
  //qtDB->commit();
}



void Model::sqlBargainCoords(unsigned int t, int bargnID, const KBase::VctrPstn & initPos, const KBase::VctrPstn & rcvrPos)
{
  int nDim = initPos.numR();
  //assert(nDim == rcvrPos.numR());
  if (nDim != rcvrPos.numR()) {
    throw KException("Model::sqlBargainCoords: dimension mismatch between initiator and receiver actor's positions");
  }

  // prepare the sql statement to insert
  string sql = string("INSERT INTO BargnCoords (ScenarioId, Turn_t, BargnID, Dim_k, Init_Coord, Recd_Coord) VALUES ('")
    + scenId + "', :turn_t, :bargnid, :dim_k, :init_coord, :recd_coord)";
  query.prepare(QString::fromStdString(sql));

  // start for the transaction
  //qtDB->transaction();

  for (int k = 0; k < nDim; k++)
  {

    // Turn_t
    query.bindValue(":turn_t", t);
    //Baragainer
    query.bindValue(":bargnid", bargnID);
    //Dim_K
    query.bindValue(":dim_k", k);

    //Init_Coord
    query.bindValue(":init_coord", initPos(k, 0) * 100.0);
    //Recd_Coord

    query.bindValue(":recd_coord", rcvrPos(k, 0) * 100.0);
    if (!query.exec()) {
      LOG(INFO) << query.lastError().text().toStdString();
      //assert(false);
      throw KException("Model::sqlBargainCoords: DB query failed");
    }
  }

  //qtDB->commit();
}



void Model::sqlBargainUtil(unsigned int t, vector<uint64_t> bargnIds,  KBase::KMatrix Util_mat)
{
  int Util_mat_row = Util_mat.numR();
  int Util_mat_col = Util_mat.numC();


  // prepare the sql statement to insert
  string sql = string("INSERT INTO BargnUtil  (ScenarioId, Turn_t,BargnId, Act_i, Util) VALUES ('")
    + scenId + "', :turn_t, :bgnId, :act_i, :util)";

  query.prepare(QString::fromStdString(sql));
  // start for the transaction
  //qtDB->transaction();
  uint64_t Bargn_i = 0;
  for (unsigned int i = 0; i < Util_mat_row; i++)
  {
    for (unsigned int j = 0; j < Util_mat_col; j++)
    {
      // Turn_t
      query.bindValue(":turn_t", t);
      //Bargn_i
      Bargn_i = bargnIds[j];
      query.bindValue(":bgnId", (qulonglong)Bargn_i);
      //Act_i
      query.bindValue(":act_i", i);
      //Util
      query.bindValue(":util", Util_mat(i, j));
      // finish
      if (!query.exec()) {
        LOG(INFO) << query.lastError().text().toStdString();
        //assert(false);
        throw KException("Model::sqlBargainUtil: DB query failed");
      }
    }
  }

  //qtDB->commit();
}

// JAH 20160731 added this function in replacement to the separate
// populate* functions that separately logged information tables
// this only covers the general "Model" info tables; currently only Actors and Scenarios
void Model::LogInfoTables()
{
  // assert tests for all tables here at the start
  //assert(numAct == actrs.size());
  if (numAct != actrs.size()) {
    throw KException("Model::LogInfoTables: Wrong Actor count");
  }

  // for efficiency sake, we'll do all tables in a single transaction
  // form the insert cmmands
  // prepare the prepared statement statements
  string sql = "INSERT INTO ActorDescription (ScenarioId,Act_i,Name,\"Desc\") VALUES ('"
    + scenId + "', :act_i, :name, :desc)";
  query.prepare(QString::fromStdString(sql));
  qtDB->transaction();
  // Actor Description Table
  // For each actor fill the required information
  for (unsigned int i = 0; i < actrs.size(); i++) {
    Actor * act = actrs.at(i);
    // bind the data
    query.bindValue(":act_i", i);
    query.bindValue(":name", act->name.c_str());
    query.bindValue(":desc", act->desc.c_str());
    // record
    if (!query.exec()) {
      LOG(INFO) << query.lastError().text().toStdString();
      //assert(false);
      throw KException("Model::LogInfoTables: DB query failed");
    }
  }
  qtDB->commit();

  // Scenario Description
  // Turn_t
  // Scen Id
  // rng seed JAH 20160711
  // have to convert to text and store it that way, since sqlite3 doesn't really understand unsigned ints
  char *seedBuff = newChars(50);
  sprintf(seedBuff,"%20llu",rngSeed);
  const char* strSeed = seedBuff;
  sql = string("INSERT INTO ScenarioDesc (Scenario,\"Desc\",ScenarioId,RNGSeed,"
    "VictoryProbModel,ProbCondorcetElection,StateTransition) VALUES ('"
    + scenName + "', '" + scenDesc + "', '" + scenId + "', '" + strSeed + "', "
    + std::to_string(static_cast<int>(vpm)) + ", "
    + std::to_string(static_cast<int>(pcem)) + ", "
    + std::to_string(static_cast<int>(stm))
    + " )");

  execQuery(sql);
  delete [] seedBuff;
  return;
}

void Model::sqlBargainVote(unsigned int t, vector< tuple<uint64_t, uint64_t>> barginidspair_i_j, vector<double> Vote_mat,unsigned int act_k)
{
  int Util_mat_row = Vote_mat.size();

  // prepare the sql statement to insert
  string sql = string("INSERT INTO BargnVote (ScenarioId, Turn_t, BargnId_i, BargnId_j, Act_k, Vote) VALUES ('")
    + scenId + "', :turn_t, :bargnid_i, :bargnid_j, :act_k, :vote)";
  query.prepare(QString::fromStdString(sql));

  // start for the transaction
  //qtDB->transaction();

  for (unsigned int i = 0; i <Util_mat_row ; i++)
  {
    tuple<uint64_t, uint64_t> tijids = barginidspair_i_j[i];
    uint64_t Bargn_i = std::get<0>(tijids);
    uint64_t Bargn_j = std::get<1>(tijids);

    // Turn_t
    query.bindValue(":turn_t", t);
    //Bargn_i
    query.bindValue(":bargnid_i", (qulonglong)Bargn_i);
    //Bargn_j
    query.bindValue(":bargnid_j", (qulonglong)Bargn_j);
    //Act_i
    query.bindValue(":act_k", act_k);
    //Util
    double voteMat = Vote_mat[i];
    query.bindValue(":vote", voteMat);
    // finish
    if (!query.exec()) {
      LOG(INFO) << query.lastError().text().toStdString();
      //assert(false);
      throw KException("Model::sqlBargainVote: DB query failed");
    }
  }
  //qtDB->commit();
}

// populates record for table PosProb for each step of
// module run
void Model::sqlPosProb(unsigned int t)
{
  //assert(t < history.size());
  if (t >= history.size()) {
    throw KException("Model::sqlPosProb: Specified turn number is beyond the size of history");
  }
  State* st = history[t];
  // check module for null
  //assert(nullptr != st);
  if (nullptr == st) {
    throw KException("Model::sqlPosProb: st is a null pointer.");
  }
  // prepare the sql statement to insert
  string sql = string("INSERT INTO PosProb (ScenarioId, Turn_t, Est_h,Pos_i, Prob) VALUES ('")
    + scenId + "', :turn_t, :est_h, :pos_i, :prob)";
  query.prepare(QString::fromStdString(sql));

  // start for the transaction
  qtDB->transaction();
  // collect the information from each estimator,actor
  for (unsigned int h = 0; h < numAct; h++)   // estimator is h
  {
    // calculate the probablity with respect to each estimator
    auto pn = st->pDist(h);
    auto pdt = std::get<0>(pn); // note that these are unique positions
    //assert( fabs(1 - sum(pdt)) < 1e-4);
    if (fabs(1 - sum(pdt)) >= 1e-4) {
      throw KException("Model::sqlPosProb sum of all positions exceeded the limit value.");
    }
    auto unq = std::get<1>(pn);
    // for each actor pupulate the probablity information
    for (unsigned int i = 0; i < numAct; i++)
    {
      // Extract the probabity for each actor
      double prob = st->posProb(i, unq, pdt);
      query.bindValue(":turn_t", t);
      query.bindValue(":est_h", h);
      query.bindValue(":pos_i", i);
      query.bindValue(":prob", prob);
      if (!query.exec()) {
        LOG(INFO) << query.lastError().text().toStdString();
        //assert(false);
        throw KException("Model::sqlPosProb: DB query failed");
      }
    }
  }
  qtDB->commit();
  return;
}
// populates record for table PosProb for each step of
// module run
void Model::sqlPosVote(unsigned int t)
{
  //assert(t < history.size());
  if (t >= history.size()) {
    throw KException("Model::sqlPosVote: Specified turn number is beyond the size of history");
  }
  State* st = history[t];


  // check module for null
  //assert(nullptr != st);
  if (nullptr == st) {
    throw KException("Model::sqlPosVote: st is a null pointer.");
  }
  // prepare the sql statement to insert
  string sql = string("INSERT INTO PosVote (ScenarioId, Turn_t, Est_h, Voter_k, Pos_i, Pos_j, Vote) VALUES ('")
    + scenId + "', :turn_t, :est_h, :voter_k, :pos_i, :pos_j, :vote)";
  query.prepare(QString::fromStdString(sql));

  // start for the transaction
  qtDB->transaction();
  auto vr = VotingRule::Proportional;
  // collect the information from each estimator

  for (unsigned int k = 0; k < numAct; k++)   // voter is k
  {
    auto rd = st->model->actrs[k];
    for (unsigned int i = 0; i < numAct; i++)
    {
      for (unsigned int j = 0; j < numAct; j++)
      {
        for (unsigned int h = 0; h < numAct; h++)   // estimator is h
        {
          if (((h == i) || (h == j)) && (i!=j))
          {
            auto vij = rd->vote(h, i, j, st);
            query.bindValue(":turn_t", t);
            query.bindValue(":est_h", h);
            //voter_k
            query.bindValue(":voter_k", k);
            // position i
            query.bindValue(":pos_i", i);
            //position j
            query.bindValue(":pos_j", j);
            // vote ?
            query.bindValue(":vote", vij);
            if (!query.exec()) {
              LOG(INFO) << query.lastError().text().toStdString();
              //assert(false);
              throw KException("Model::sqlPosVote: DB query failed");
            }
          }
        }
      }
    }
  }
  qtDB->commit();

  return;
}

void Model::createTableIndices() {
    const char * indexUtil = "CREATE INDEX IF NOT EXISTS idx_util ON PosUtil(ScenarioId, Turn_t, Est_h, Act_i, Pos_j)";
    string qry = string(indexUtil);
    execQuery(qry);

    const char *indexActor = "CREATE INDEX IF NOT EXISTS idx_actor ON ActorDescription(ScenarioId)";
    qry = string(indexActor);
    execQuery(qry);
}

void Model::dropTableIndices() {
    const char * indexUtil = "DROP INDEX IF EXISTS idx_util";
    string qry = string(indexUtil);
    execQuery(qry);

    const char * indexActor = "DROP INDEX IF EXISTS idx_actor";
    qry = string(indexActor);
    execQuery(qry);
}

bool Model::loginCredentials(string connString) {
  enum class userParams {
    Driver,
    Server,
    Port,
    Database,
    Uid,
    Pwd
  };

  std::map<std::string, userParams> mapStringToUserParams =
  {
    { "driver", userParams::Driver },
    { "server", userParams::Server },
    { "port", userParams::Port },
    { "database", userParams::Database },
    { "uid", userParams::Uid },
    { "pwd", userParams::Pwd },
  };

  auto trimWhites = [](string &input) {
    // Trim all leading whitespaces
    while (input.size() && std::isspace(input.front())) {
      input.erase(input.begin());
    }

    // Trim all trailing whitespaces
    while (input.size() && std::isspace(input.back())) {
      input.pop_back();
    }
  };

  auto parseKeyValue = [&trimWhites](const string &pair, string &key, string &value) {
    auto it = std::find(pair.begin(), pair.end(), '=');
    if (it == pair.end()) {
      string err = "Missing the separator of key=value pair. The pair should be separated by a '='";
      LOG(INFO) << err;
      lastExceptionMsg = err;
      return false;
    }
    key.assign(pair.begin(), it);
    trimWhites(key);
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);

    value.assign(it + 1, pair.end());
    trimWhites(value);
  };

  string parsedParam;
  std::stringstream  inputCredential(const_cast<char*>(connString.c_str()));
  while (getline(inputCredential, parsedParam, ';'))
  {
    //auto it = std::find(parsedParam.begin(), parsedParam.end(), '=');
    string key, value;
    //key.assign(parsedParam.begin(), it);
    //std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    //value.assign(it + 1, parsedParam.end());
    parseKeyValue(parsedParam, key, value);
    if(key.empty() || value.empty()) {
      string err = "Invalid key-pair passed: ";
      err += parsedParam;
      LOG(INFO) << err;
      lastExceptionMsg = err;
      return false;
    }
    userParams userParam;
    try {
      userParam = mapStringToUserParams.at(key);
    }
    catch (const std::out_of_range& oor) {
      LOG(INFO) << "Error: Wrong connection string provided for DB!";
      LOG(INFO) << "Error:" << oor.what();
      //assert(false);
      //throw KException("Model::loginCredentials: Wrong connection string for DB");
      string err = "Error in connection string for key: ";
      err += key;
      lastExceptionMsg = err;
      return false;
    }

    switch (mapStringToUserParams[key]) {
    case userParams::Driver:
      dbDriver = QString::fromStdString(value);
      break;
    case userParams::Server:
      server = QString::fromStdString(value);
      break;
    case userParams::Port:
      port = std::stoi(value);
      break;
    case userParams::Database:
      databaseName = QString::fromStdString(value);
      break;
    case userParams::Uid:
      userName = QString::fromStdString(value);
      break;
    case userParams::Pwd:
      password = QString::fromStdString(value);
      break;
    default:
      lastExceptionMsg = "Error in input credentials format";
      LOG(INFO) << lastExceptionMsg;
      return false;
    }
  }

  if (dbDriver.isEmpty()) {
    lastExceptionMsg = "Error! Database driver name can not be left blank.";
    LOG(INFO) << lastExceptionMsg;
    //assert(false);
    //throw KException("Model::loginCredentials: Database type or name is blank");
    return false;
  }

  if (databaseName.isEmpty()) {
    lastExceptionMsg = "Error! Database name can not be left blank.";
    LOG(INFO) << lastExceptionMsg;
    //assert(false);
    //throw KException("Model::loginCredentials: Database type or name is blank");
    return false;
  }

  // We use either Postgresql or SQLITE
  if (dbDriver.compare("QPSQL") && dbDriver.compare("QSQLITE")) {
    lastExceptionMsg = "Error! Wrong driver name. Supported Drivers: postgres(QPSQL), sqlite3(QSQLITE)";
    LOG(INFO) << lastExceptionMsg;
    //assert(false);
    //throw KException("Model::loginCredentials: Unsupported DB driver");
    return false;
  }

  // for a non-sqlite db
  if (!dbDriver.compare("QPSQL")) {
    if (server.isEmpty()) {
      lastExceptionMsg = "Error! No ip address provided for postgres server";
      LOG(INFO) << lastExceptionMsg;
      //assert(false);
      //throw KException("Model::loginCredentials: No ip address provided for postgresql server");
      return false;
    }
  }

  if (!dbDriver.compare("QSQLITE")) {
    databaseName.append(".db");
  }

  return true;
}

} // end of namespace

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
