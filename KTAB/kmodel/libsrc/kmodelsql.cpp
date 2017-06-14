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

using std::cout;
using std::endl;
using std::flush;
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
    cout << "A database connection already exists with the name: " << connectionName.toStdString() << endl;
    return;
  }
  QSqlDatabase qdb = QSqlDatabase::addDatabase(dbDriver, connectionName);
  qtDB = new QSqlDatabase(qdb);
  query = QSqlQuery(*qtDB);
}

bool Model::connectDB() {
  return connect(server, port, databaseName, userName, password);
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
  cout << createDBqry.toStdString() << endl;

  if (!query.exec(createDBqry)) {
    cout << query.lastError().text().toStdString() << endl;
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
    std::cerr << "Failed Query: " << qry << std::endl;
    std::cerr << query.lastError().text().toStdString() << endl;
    assert(false);
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
  cout << endl << "Starting basic demo of SQLite in Model class" << endl;

  auto callBack = [](void *data, int numCol, char **stringFields, char **colNames)
  {
    for (int i = 0; i < numCol; i++)
    {
      printf("%s = %s\n", colNames[i], stringFields[i] ? stringFields[i] : "NULL");
    }
    printf("\n");
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
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      exit(0);
    }
    else
    {
      fprintf(stderr, "Opened database successfully (%i)\n", n);
    }
    return;
  };

  auto sExec = [callBack, &db, &zErrMsg](string sql, string msg)
  {
    int rc = sqlite3_exec(db, sql.c_str(), callBack, nullptr, &zErrMsg); // nullptr is the 'data' argument
    if (rc != SQLITE_OK)
    {
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
    }
    else
    {
      fprintf(stdout, msg.c_str());
    }
    return rc;
  };


  // Open database
  cout << endl << flush;
  sOpen(1);
  
  // try some pragmas. these should speed up operations on larger tables,
  // at the expense of making the table vulnerable to corruption if the
  // application abruptly terminates mid-operation.
  sql = "PRAGMA synchronous = off;";
  rc = sExec(sql, "Set synchronous to off \n");
  
  sql = "PRAGMA journal_mode = off;";
  rc = sExec(sql, "Set journal_mode to off \n");
  
  sql = "PRAGMA locking_mode = exclusive;";
  rc = sExec(sql, "Set locking_mode to exclusive \n");

  // Create SQL statement
  sql = "create table if not exists PETS("  \
        "ID INT PRIMARY KEY     NOT NULL," \
        "NAME           text    NOT NULL," \
        "AGE            int     NOT NULL," \
        "BREED         char(50)," \
        "COLOR         char(50) );";

  // Execute SQL statement
  rc = sExec(sql, "Created table successfully \n");
  sqlite3_close(db);


  cout << endl << flush;
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
  \
  "INSERT INTO PETS (ID, NAME, AGE, SPECIES, COLOR) " \
  "VALUES (5, 'Ellie', 8, 'Rhodesian', 'Red' );";

  cout << "NB: This should get one planned SQL error at ID=4" << endl << flush;
  rc = sExec(sql, "Records inserted successfully \n");
  sqlite3_close(db);


  cout << endl << flush;
  sOpen(3);
  sql = "SELECT * from PETS where AGE>5;";
  rc = sExec(sql, "Records selected successfully\n");
  cout << "NB: ID=5 was never inserted due to planned SQL error at ID=4" << endl;
  sqlite3_close(db);


  cout << endl << flush;
  sOpen(4);
  sql = "DROP TABLE PETS;";
  rc = sExec(sql, "Dropped table successfully \n");
  sqlite3_close(db);

  return;
}

bool testMultiThreadSQLite (bool tryReset, KBase::ReportingLevel rl) { 
  int mutexP = sqlite3_threadsafe() ;
  bool parP = (0 != mutexP);
  if (ReportingLevel::Silent < rl) {
    if (parP) {
      cout << "This SQLite3 library WAS compiled to be threadsafe."<<endl << flush;
    } else {
      cout << "This SQLite3 library was NOT compiled to be threadsafe."<<endl << flush;
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
      cout << "  Note that multi-threading might have been disabled via sqlite3_config."<<endl<<flush;
      cout << "  Tried to reconfigure to SQLITE_CONFIG_SERIALIZED ... " << flush;
      if (configRslt == SQLITE_OK) {
        cout << "SUCCESS"<< endl;
      }
      else  {
        cout << "FAILED"<< endl;
      }
    }
  }
  if (ReportingLevel::Silent < rl) {
    if (parP)  {
      cout << "Possible to continue multi-threaded"<< endl << flush;
    }
    else {
      cout << "Necessary to continue single-threaded"<< endl << flush;
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

  assert(n < Model::NumTables);
  switch (n)
  {
  case 0:
    // position-utility table
    // the estimated utility to each actor of each other's position
    sql = "create table if not exists PosUtil ("  \
          "ScenarioId VARCHAR(32) NOT NULL DEFAULT 'None', "\
          "Turn_t	INTEGER NOT NULL DEFAULT 0, "\
          "Est_h	INTEGER NOT NULL DEFAULT 0, "\
          "Act_i	INTEGER NOT NULL DEFAULT 0, "\
          "Pos_j	INTEGER NOT NULL DEFAULT 0, "\
          "Util	FLOAT NOT NULL DEFAULT 0.0"\
          ");";
    name = "PosUtil";
    grpID = 4;
    break;

  case 1: // pos-vote table
    // estimated vote of each actor between each pair of positions
    sql = "create table if not exists PosVote ("  \
          "ScenarioId VARCHAR(32) NOT NULL DEFAULT 'None', "\
          "Turn_t	INTEGER NOT NULL DEFAULT 0, "\
          "Est_h	INTEGER NOT NULL DEFAULT 0, "\
          "Voter_k	INTEGER NOT NULL DEFAULT 0, "\
          "Pos_i	INTEGER NOT NULL DEFAULT 0, "\
          "Pos_j	INTEGER NOT NULL DEFAULT 0, "\
          "Vote	FLOAT NOT NULL DEFAULT 0.0"\
          ");";
    name = "PosVote";
    grpID = 1;
    break;

  case 2: // pos-prob table. Note that there may be duplicates, unless we limit it to unique positions
    sql = "create table if not exists PosProb ("  \
          "ScenarioId VARCHAR(32) NOT NULL DEFAULT 'None', "\
          "Turn_t	INTEGER NOT NULL DEFAULT 0, "\
          "Est_h	INTEGER NOT NULL DEFAULT 0, "\
          "Pos_i	INTEGER NOT NULL DEFAULT 0, "\
          "Prob	FLOAT NOT NULL DEFAULT 0.0"\
          ");";
    name = "PosProb";
    grpID = 1;
    break;

  case 3: // pos-equiv table. E(i)= lowest j s.t. Pos(i) ~ Pos(j). if j < i, it is not unique.
    sql = "create table if not exists PosEquiv ("  \
          "ScenarioId VARCHAR(32) NOT NULL DEFAULT 'None', "\
          "Turn_t	INTEGER NOT NULL DEFAULT 0, "\
          "Pos_i	INTEGER NOT NULL DEFAULT 0, "\
          "Eqv_j	INTEGER NOT NULL DEFAULT 0 "\
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
          "Turn_t	INTEGER NOT NULL DEFAULT 0, "\
          "Est_h	INTEGER NOT NULL DEFAULT 0, "\
          "Aff_k	INTEGER NOT NULL DEFAULT 0, "\
          "Init_i	INTEGER NOT NULL DEFAULT 0, "\
          "Rcvr_j	INTEGER NOT NULL DEFAULT 0, "\
          "Util_SQ	  FLOAT    NOT NULL DEFAULT 0, "\
          "Util_Vict	FLOAT    NOT NULL DEFAULT 0, "\
          "Util_Cntst	FLOAT    NOT NULL DEFAULT 0, "\
          "Util_Chlg	FLOAT    NOT NULL DEFAULT 0  "\
          ");";
    name = "UtilChlg";
    grpID = 2;
    break;

  case 5:
    // h's estimate that i will defeat j, including all third party contributions
    // P{^h}( i > j )
    sql = "create table if not exists ProbVict ("  \
          "ScenarioId VARCHAR(32) NOT NULL DEFAULT 'None', "\
          "Turn_t	INTEGER NOT NULL DEFAULT 0, "\
          "Est_h	INTEGER NOT NULL DEFAULT 0, "\
          "Init_i	INTEGER NOT NULL DEFAULT 0, "\
          "Rcvr_j	INTEGER NOT NULL DEFAULT 0, "\
          "Prob	FLOAT NOT NULL DEFAULT 0"\
          ");";
    name = "PosVict";
    grpID = 2;
    break;

  case 6:
    // h's estimates in a little 3-actor contest with no contribution from others:
    // P{^h}( ik > j )      probability ik defeats j,
    // U{^h}{_k} (ik > j)   utility to k of winning with i, over j
    // U{^h}{_k} (i > jk)   utility to k of losing to i, with j
    sql = "create table if not exists TP_Prob_Vict_Loss ("  \
          "ScenarioId VARCHAR(32) NOT NULL DEFAULT 'None', "\
          "Turn_t	INTEGER NOT NULL DEFAULT 0, "\
          "Est_h	INTEGER NOT NULL DEFAULT 0, "\
          "Init_i	INTEGER NOT NULL DEFAULT 0, "\
          "ThrdP_k	INTEGER NOT NULL DEFAULT 0, "\
          "Rcvr_j	INTEGER NOT NULL DEFAULT 0, "\
          "Prob	FLOAT    NOT NULL DEFAULT 0, "\
          "Util_V	FLOAT    NOT NULL DEFAULT 0, "\
          "Util_L	FLOAT    NOT NULL DEFAULT 0  "\
          ");";
    name = "TP_Prob_Vict_Loss";
    grpID = 2;
    break;

  case 7:   // short-name and long-description of actors
  {
    char *sqlBuff = newChars(500);
    sprintf(sqlBuff, "create table if not exists ActorDescription ("  \
                     "ScenarioId VARCHAR(32) NOT NULL DEFAULT 'None', "\
                     "Act_i	INTEGER NOT NULL DEFAULT 0, "\
                     "Name	VARCHAR(%u) NOT NULL DEFAULT 'NoName', "\
                     "\"Desc\"	VARCHAR(%u) NOT NULL DEFAULT 'NoName' "\
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
          "Turn_t	INTEGER NOT NULL DEFAULT 0, "\
          "BargnId INTEGER NOT NULL DEFAULT 0, "\
          "Init_Act_i INTEGER NOT NULL DEFAULT 0, "\
          "Recd_Act_j INTEGER NOT NULL DEFAULT 0, "\
          "Value FLOAT NOT NULL DEFAULT 0.0, "\
          "Init_Prob FLOAT NULL DEFAULT 0, "\
          "Init_Seld	INTEGER NULL ,"\
          "Recd_Prob FLOAT NULL DEFAULT 0, "\
          "Recd_Seld	INTEGER NULL, "\
          "CHECK (Init_Seld in (0,1) AND Recd_Seld in (0,1))"\
          ");";
    name = "Bargn";
    grpID = 4;
    break;
  case 9:  //
    // BargnCoords table
    sql = "create table if not exists BargnCoords ("  \
          "ScenarioId VARCHAR(32) NOT NULL DEFAULT 'None', "\
          "Turn_t	INTEGER NOT NULL DEFAULT 0, "\
          "BargnId INTEGER NOT NULL DEFAULT 0, "\
          "Dim_k INTEGER NOT NULL DEFAULT 0, "\
          "Init_Coord	FLOAT NULL DEFAULT 0.0,"\
          "Recd_Coord	FLOAT NOT NULL DEFAULT 0.0"\
          ");";
    name = "BargnCoords";
    grpID = 3;
    break;

  case 10:  // BargnUtil table creation
    sql = "create table if not exists BargnUtil ("  \
          "ScenarioId VARCHAR(32) NOT NULL DEFAULT 'None', "\
          "Turn_t	INTEGER NOT NULL DEFAULT 0, "\
          "BargnId    INTEGER NOT NULL	DEFAULT 0, "\
          "Act_i 	INTEGER NOT NULL DEFAULT 0, "\
          "Util	FLOAT NOT NULL DEFAULT 0.0"\
          ");";
    name = "BargnUtil";
    grpID = 3;
    break;

  case 11:
    // During bargain resolution, this table records the actual vote
    // of each actor between each pair of competing bargains
    sql = "create table if not exists BargnVote ("  \
          "ScenarioId VARCHAR(32) NOT NULL DEFAULT 'None', "\
          "Turn_t	INTEGER NOT NULL DEFAULT 0, "\
          "BargnId_i  INTEGER NOT NULL DEFAULT 0, "\
          "BargnId_j INTEGER NOT NULL DEFAULT 0, "\
          "Act_k 	INTEGER NOT NULL DEFAULT 0, "\
          "Vote	FLOAT NOT NULL DEFAULT 0.0"\
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
    throw(KException("Model::createTableSQL unrecognized table number"));
  }

  assert(grpID < NumSQLLogGrps);
  auto tab = new KTable(n,name,sql,grpID);
  return tab;
}


void Model::sqlAUtil(unsigned int t)
{
  assert(nullptr != smpDB);
  assert(t < history.size());
  State* st = history[t];
  assert(nullptr != st);
  assert(numAct == st->aUtil.size());

  // I don't like passing 'this' into lambda-functions,
  // so I copy the pointer into a local variable I can pass in
  // without exposing everything in 'this'. If this were a big,
  // mission-critical RDBMS, rather than a 1-off record of this run,
  // doing so might be disasterous in case the system crashed before
  // things were cleaned up.
  sqlite3 * db = smpDB;
  smpDB = nullptr;

  char* zErrMsg = nullptr;
  auto sqlBuff = newChars(200);
  sprintf(sqlBuff,
          "INSERT INTO PosUtil (ScenarioId, Turn_t, Est_h, Act_i, Pos_j, Util) VALUES ('%s', ?1, ?2, ?3, ?4, ?5)",
          scenId.c_str());

  string sql = "INSERT INTO PosUtil (ScenarioId, Turn_t, Est_h, Act_i, Pos_j, Util) VALUES ('"
    + scenId + "', :turn_t, :est_h, :act_i, :pos_j, :util)";
  assert(nullptr != db);
  const char* insStr = sqlBuff;
  sqlite3_stmt *insStmt;

  sqlite3_prepare_v2(db, insStr, strlen(insStr), &insStmt, NULL);
  query.prepare(QString::fromStdString(sql));

  // Prepared statements cache the execution plan for a query after the query optimizer has
  // found the best plan, so there is no big gain with simple insertions.
  // What makes a huge difference is bundling a few hundred into one atomic "transaction".
  // For this case, runtime droped from 62-65 seconds to 0.5-0.6 (vs. 0.30-0.33 with no SQL at all).
  assert(nullptr != insStmt); // make sure it is ready

  sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);
  qtDB->transaction();

  for (unsigned int h = 0; h < numAct; h++)   // estimator is h
  {
    KMatrix uij = st->aUtil[h]; // utility to actor i of the position held by actor j
    for (unsigned int i = 0; i < numAct; i++)
    {
      for (unsigned int j = 0; j < numAct; j++)
      {
        int rslt = 0;
        rslt = sqlite3_bind_int(insStmt, 1, t);
        query.bindValue(":turn_t", t);
        assert(SQLITE_OK == rslt);
        rslt = sqlite3_bind_int(insStmt, 2, h);
        query.bindValue(":est_h", h);
        assert(SQLITE_OK == rslt);
        rslt = sqlite3_bind_int(insStmt, 3, i);
        query.bindValue(":act_i", i);
        assert(SQLITE_OK == rslt);
        rslt = sqlite3_bind_int(insStmt, 4, j);
        query.bindValue(":pos_j", j);
        assert(SQLITE_OK == rslt);
        rslt = sqlite3_bind_double(insStmt, 5, uij(i, j));
        query.bindValue(":util", uij(i, j));
        assert(SQLITE_OK == rslt);
        rslt = sqlite3_step(insStmt);
        if (!query.exec()) {
          cout << query.lastError().text().toStdString() << endl;
        }
        assert(SQLITE_DONE == rslt);
        sqlite3_clear_bindings(insStmt);
        assert(SQLITE_DONE == rslt);
        rslt = sqlite3_reset(insStmt);
        assert(SQLITE_OK == rslt);
      }
    }
  }
  sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &zErrMsg);
  qtDB->commit();
  sqlite3_finalize(insStmt); // finalize statement to avoid resource leaks
  //printf("Stored SQL for turn %u of all estimators, actors, and positions \n", t);

  delete sqlBuff;
  sqlBuff = nullptr;

  smpDB = db; // give it the new pointer

  return;
}

// populates record for table PosEquiv for each step of
// module run
void Model::sqlPosEquiv(unsigned int t)
{
  // Check database intact
  assert(nullptr != smpDB);
  assert(t < history.size());
  State* st = history[t];
  assert(nullptr != st);
  sqlite3 * db = smpDB;
  smpDB = nullptr;
  // In case of error
  char* zErrMsg = nullptr;
  auto sqlBuff = newChars(200);
  sprintf(sqlBuff,
          "INSERT INTO PosEquiv (ScenarioId, Turn_t, Pos_i, Eqv_j) VALUES ('%s', ?1, ?2, ?3)",
          scenId.c_str());

  assert(nullptr != db);
  const char* insStr = sqlBuff;
  sqlite3_stmt *insStmt;
  sqlite3_prepare_v2(db, insStr, strlen(insStr), &insStmt, NULL);

  string qsql = string("INSERT INTO PosEquiv (ScenarioId, Turn_t, Pos_i, Eqv_j) VALUES ('")
    + scenId + "', :turn_t, :pos_i, :eqv_j)";
  query.prepare(QString::fromStdString(qsql));

  assert(nullptr != insStmt); // make sure it is ready

  qtDB->transaction();

  // Start inserting record
  sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);
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
    int rslt = 0;
    rslt = sqlite3_bind_int(insStmt, 1, t);
    query.bindValue(":turn_t", t);
    assert(SQLITE_OK == rslt);
    rslt = sqlite3_bind_int(insStmt, 2, i);
    query.bindValue(":pos_i", i);
    assert(SQLITE_OK == rslt);
    rslt = sqlite3_bind_int(insStmt, 3, je);
    query.bindValue(":eqv_j", je);
    assert(SQLITE_OK == rslt);
    rslt = sqlite3_step(insStmt);
    if (!query.exec()) {
      cout << query.lastError().text().toStdString() << endl;
    }
    assert(SQLITE_DONE == rslt);
    sqlite3_clear_bindings(insStmt);
    assert(SQLITE_DONE == rslt);
    rslt = sqlite3_reset(insStmt);
    assert(SQLITE_OK == rslt);
  }
  // end databse transaction
  sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &zErrMsg);
  qtDB->commit();

  sqlite3_finalize(insStmt); // finalize statement to avoid resource leaks
  //printf("Stored SQL for turn %u of all estimators, actors, and positions \n", t);
  delete sqlBuff;
  sqlBuff = nullptr;
  smpDB = db;
  return;
}

void Model::sqlBargainEntries(unsigned int t, int bargainId, int initiator, int receiver, double val)
{
  // initiate the database
  sqlite3 * db = smpDB;

  // Error message in case
  char* zErrMsg = nullptr;
  auto sqlBuff = newChars(200);
  // prepare the sql statement to insert
  sprintf(sqlBuff,
          "INSERT INTO Bargn (ScenarioId, Turn_t, BargnID, Init_Act_i, Recd_Act_j,Value) VALUES ('%s',?1, ?2, ?3, ?4,?5)",
          scenId.c_str());

  assert(nullptr != db);
  const char* insStr = sqlBuff;
  sqlite3_stmt *insStmt;
  sqlite3_prepare_v2(db, insStr, strlen(insStr), &insStmt, NULL);
  assert(nullptr != insStmt); // make sure it is ready

  string sql = string("INSERT INTO Bargn (ScenarioId, Turn_t, BargnID, Init_Act_i, Recd_Act_j, Value) VALUES ('")
    + scenId + "', :turn_t, :bargnid, :init_i, :recd_j, :value)";
  query.prepare(QString::fromStdString(sql));

  // start for the transaction
  sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);
  qtDB->transaction();

  int rslt = 0;
  // Turn_t
  rslt = sqlite3_bind_int(insStmt, 1, t);
  query.bindValue(":turn_t", t);
  assert(SQLITE_OK == rslt);
  //BargnID
  rslt = sqlite3_bind_int(insStmt, 2, bargainId);
  query.bindValue(":bargnid", bargainId);
  assert(SQLITE_OK == rslt);
  //Init_Act_i
  rslt = sqlite3_bind_int(insStmt, 3, initiator);
  query.bindValue(":init_i", initiator);
  assert(SQLITE_OK == rslt);
  //Recd_Act_j
  rslt = sqlite3_bind_int(insStmt, 4, receiver);
  query.bindValue(":recd_j", receiver);
  assert(SQLITE_OK == rslt);
  //Value
  rslt = sqlite3_bind_double(insStmt, 5, val);
  query.bindValue(":value", val);
  assert(SQLITE_OK == rslt);
  rslt = sqlite3_step(insStmt);
  if (!query.exec()) {
    cout << query.lastError().text().toStdString() << endl;
  }
  assert(SQLITE_DONE == rslt);
  sqlite3_clear_bindings(insStmt);
  assert(SQLITE_DONE == rslt);
  rslt = sqlite3_reset(insStmt);
  assert(SQLITE_OK == rslt);

  sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &zErrMsg);
  qtDB->commit();
  //printf("Stored SQL for turn %u of all estimators, actors, and positions \n", t);
  sqlite3_finalize(insStmt); // finalize statement to avoid resource leaks
  delete sqlBuff;
  sqlBuff = nullptr;

  smpDB = db;
}



void Model::sqlBargainCoords(unsigned int t, int bargnID, const KBase::VctrPstn & initPos, const KBase::VctrPstn & rcvrPos)
{
  int nDim = initPos.numR();
  assert(nDim == rcvrPos.numR());

  // initiate the database
  sqlite3 * db = smpDB;
  // Error message in case
  char* zErrMsg = nullptr;
  auto sqlBuff = newChars(200);

  // prepare the sql statement to insert
  sprintf(sqlBuff,
          "INSERT INTO BargnCoords (ScenarioId, Turn_t,BargnId, Dim_k, Init_Coord,Recd_Coord) VALUES ('%s',?1, ?2, ?3, ?4,?5)",
          scenId.c_str());

  assert(nullptr != db);
  const char* insStr = sqlBuff;
  sqlite3_stmt *insStmt;
  sqlite3_prepare_v2(db, insStr, strlen(insStr), &insStmt, NULL);
  assert(nullptr != insStmt); // make sure it is ready

  string sql = string("INSERT INTO BargnCoords (ScenarioId, Turn_t, BargnID, Dim_k, Init_Coord, Recd_Coord) VALUES ('")
    + scenId + "', :turn_t, :bargnid, :dim_k, :init_coord, :recd_coord)";
  query.prepare(QString::fromStdString(sql));

  // start for the transaction
  sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);
  qtDB->transaction();

  for (int k = 0; k < nDim; k++)
  {

    int rslt = 0;
    // Turn_t
    rslt = sqlite3_bind_int(insStmt, 1, t);
    query.bindValue(":turn_t", t);
    assert(SQLITE_OK == rslt);
    //Baragainer
    rslt = sqlite3_bind_int(insStmt, 2, bargnID);
    query.bindValue(":bargnid", bargnID);
    assert(SQLITE_OK == rslt);
    //Dim_K
    rslt = sqlite3_bind_int(insStmt, 3, k);
    query.bindValue(":dim_k", k);
    assert(SQLITE_OK == rslt);

    //Init_Coord
    rslt = sqlite3_bind_double(insStmt, 4, initPos(k,0) * 100.0); // Log at the scale of [0,100]
    query.bindValue(":init_coord", initPos(k, 0) * 100.0);
    assert(SQLITE_OK == rslt);
    //Recd_Coord

    rslt = sqlite3_bind_double(insStmt, 5, rcvrPos(k, 0) * 100.0); // Log at the scale of [0,100]
    query.bindValue(":recd_coord", rcvrPos(k, 0) * 100.0);
    assert(SQLITE_OK == rslt);
    rslt = sqlite3_step(insStmt);
    if (!query.exec()) {
      cout << query.lastError().text().toStdString() << endl;
    }
    assert(SQLITE_DONE == rslt);
    sqlite3_clear_bindings(insStmt);
    assert(SQLITE_DONE == rslt);
    rslt = sqlite3_reset(insStmt);
    assert(SQLITE_OK == rslt);

  }

  sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &zErrMsg);
  qtDB->commit();
  sqlite3_finalize(insStmt); // finalize statement to avoid resource leaks
  delete sqlBuff;
  sqlBuff = nullptr;
  smpDB = db;
}



void Model::sqlBargainUtil(unsigned int t, vector<uint64_t> bargnIds,  KBase::KMatrix Util_mat)
{
  // initiate the database
  sqlite3 * db = smpDB;

  // Error message in case
  char* zErrMsg = nullptr;
  auto sqlBuff = newChars(200);

  int Util_mat_row = Util_mat.numR();
  int Util_mat_col = Util_mat.numC();


  // prepare the sql statement to insert
  sprintf(sqlBuff,
          "INSERT INTO BargnUtil  (ScenarioId, Turn_t,BargnId, Act_i, Util) VALUES ('%s',?1, ?2, ?3, ?4)",
          scenId.c_str());

  string sql = string("INSERT INTO BargnUtil  (ScenarioId, Turn_t,BargnId, Act_i, Util) VALUES ('")
    + scenId + "', :turn_t, :bgnId, :act_i, :util)";

  assert(nullptr != db);
  const char* insStr = sqlBuff;
  sqlite3_stmt *insStmt;

  sqlite3_prepare_v2(db, insStr, strlen(insStr), &insStmt, NULL);
  query.prepare(QString::fromStdString(sql));
  assert(nullptr != insStmt); // make sure it is ready
	// start for the transaction
	sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);
  qtDB->transaction();
  uint64_t Bargn_i = 0;
    for (unsigned int i = 0; i < Util_mat_row; i++)
    {
		for (unsigned int j = 0; j < Util_mat_col; j++)
		{
			
			int rslt = 0;
			// Turn_t
			rslt = sqlite3_bind_int(insStmt, 1, t);
      query.bindValue(":turn_t", t);
      assert(SQLITE_OK == rslt);
			//Bargn_i
			Bargn_i = bargnIds[j];
			rslt = sqlite3_bind_int(insStmt, 2, Bargn_i);
      query.bindValue(":bgnId", (qulonglong)Bargn_i);
      assert(SQLITE_OK == rslt);
			//Act_i
			rslt = sqlite3_bind_int(insStmt, 3, i);
      query.bindValue(":act_i", i);
			assert(SQLITE_OK == rslt);
			//Util
			rslt = sqlite3_bind_double(insStmt, 4, Util_mat(i, j));
      query.bindValue(":util", Util_mat(i, j));
      assert(SQLITE_OK == rslt);
			// finish  
			assert(SQLITE_OK == rslt);
			rslt = sqlite3_step(insStmt);
      if (!query.exec()) {
        cout << query.lastError().text().toStdString() << endl;
      }
      assert(SQLITE_DONE == rslt);
			sqlite3_clear_bindings(insStmt);
			assert(SQLITE_DONE == rslt);
			rslt = sqlite3_reset(insStmt);
			assert(SQLITE_OK == rslt);
		}
	}

  sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &zErrMsg);
  qtDB->commit();
  sqlite3_finalize(insStmt); // finalize statement to avoid resource leaks

  delete sqlBuff;
  sqlBuff = nullptr;

  smpDB = db;
}

// JAH 20160731 added this function in replacement to the separate
// populate* functions that separately logged information tables
// this only covers the general "Model" info tables; currently only Actors and Scenarios
void Model::LogInfoTables()
{
  int rslt = 0;
  // check the database availability
  sqlite3 * db = smpDB;
  assert(nullptr != smpDB);
  char* zErrMsg = nullptr;

  // assert tests for all tables here at the start
  assert(numAct == actrs.size());

  // for efficiency sake, we'll do all tables in a single transaction
  // form the insert cmmands
  auto sqlBuffA = newChars(sqlBuffSize);
  sprintf(sqlBuffA,"INSERT INTO ActorDescription (ScenarioId,Act_i,Name,Desc) VALUES ('%s', ?1, ?2, ?3)",
          scenId.c_str());
  auto sqlBuffS = newChars(sqlBuffSize);
  sprintf(sqlBuffS,"INSERT INTO ScenarioDesc (Scenario,Desc,ScenarioId,RNGSeed,"
      "VictoryProbModel,ProbCondorcetElection,StateTransition) VALUES ('%s',?1,?2,?3,?4,?5,?6)",
          scenName.c_str());
  // prepare the prepared statement statements
  sqlite3_stmt *insStmtA;

  string sql = "INSERT INTO ActorDescription (ScenarioId,Act_i,Name,\"Desc\") VALUES ('"
    + scenId + "', :act_i, :name, :desc)";
  sqlite3_prepare_v2(smpDB, sqlBuffA, strlen(sqlBuffA), &insStmtA, NULL);
  query.prepare(QString::fromStdString(sql));
  assert(nullptr != insStmtA);
  sqlite3_stmt *insStmtS;
  sqlite3_prepare_v2(smpDB, sqlBuffS, strlen(sqlBuffS), &insStmtS, NULL);
  assert(nullptr != insStmtS);

  sqlite3_exec(smpDB, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);
  qtDB->transaction();
  // Actor Description Table
  // For each actor fill the required information
  for (unsigned int i = 0; i < actrs.size(); i++) {
    Actor * act = actrs.at(i);
    // bind the data
    rslt = sqlite3_bind_int(insStmtA, 1, i);
    query.bindValue(":act_i", i);
    assert(SQLITE_OK == rslt);
    rslt = sqlite3_bind_text(insStmtA, 2, act->name.c_str(), -1, SQLITE_TRANSIENT);
    query.bindValue(":name", act->name.c_str());
    assert(SQLITE_OK == rslt);
    rslt = sqlite3_bind_text(insStmtA, 3, act->desc.c_str(), -1, SQLITE_TRANSIENT);
    query.bindValue(":desc", act->desc.c_str());
    assert(SQLITE_OK == rslt);
    // record
    rslt = sqlite3_step(insStmtA);
    if (!query.exec()) {
      cout << query.lastError().text().toStdString() << endl;
    }
    assert(SQLITE_DONE == rslt);
    sqlite3_clear_bindings(insStmtA);
    assert(SQLITE_DONE == rslt);
    rslt = sqlite3_reset(insStmtA);
    assert(SQLITE_OK == rslt);
  }
  qtDB->commit();

  // Scenario Description
  // Turn_t
  rslt = sqlite3_bind_text(insStmtS, 1, scenDesc.c_str(), -1, SQLITE_TRANSIENT);
  assert(SQLITE_OK == rslt);
  // Scen Id
  rslt = sqlite3_bind_text(insStmtS, 2, scenId.c_str(), -1, SQLITE_TRANSIENT);
  assert(SQLITE_OK == rslt);
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
  rslt = sqlite3_bind_text(insStmtS, 3, strSeed, -1, SQLITE_TRANSIENT);
  assert(SQLITE_OK == rslt);
  rslt = sqlite3_bind_int(insStmtS, 4, static_cast<int>(vpm));
  assert(SQLITE_OK == rslt);
  rslt = sqlite3_bind_int(insStmtS, 5, static_cast<int>(pcem));
  assert(SQLITE_OK == rslt);
  rslt = sqlite3_bind_int(insStmtS, 6, static_cast<int>(stm));
  assert(SQLITE_OK == rslt);
  // record
  assert(SQLITE_OK == rslt);
  rslt = sqlite3_step(insStmtS);
  assert(SQLITE_DONE == rslt);
  sqlite3_clear_bindings(insStmtS);
  assert(SQLITE_DONE == rslt);
  rslt = sqlite3_reset(insStmtS);
  assert(SQLITE_OK == rslt);

  // finish
  sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &zErrMsg);
  // finalize statement to avoid resource leaks
  sqlite3_finalize(insStmtA);
  sqlite3_finalize(insStmtS);
  delete sqlBuffA;
  sqlBuffA = nullptr;
  delete sqlBuffS;
  sqlBuffS = nullptr;
  return;
}

void Model::sqlBargainVote(unsigned int t, vector< tuple<uint64_t, uint64_t>> barginidspair_i_j, vector<double> Vote_mat,unsigned int act_k)
{
	// initiate the database
	sqlite3 * db = smpDB;

	// Error message in case
	char* zErrMsg = nullptr;
	auto sqlBuff = newChars(200);

	int Util_mat_row = Vote_mat.size();
//	int Util_mat_col = Vote_mat[0].size();

	// prepare the sql statement to insert
	sprintf(sqlBuff,
		"INSERT INTO BargnVote  (ScenarioId, Turn_t,BargnId_i,  BargnId_j, Act_k, Vote) VALUES ('%s', ?1, ?2, ?3,?4,?5)",scenId.c_str());
	
    assert(nullptr != db);
	const char* insStr = sqlBuff;
	sqlite3_stmt *insStmt;
	sqlite3_prepare_v2(db, insStr, strlen(insStr), &insStmt, NULL);
	assert(nullptr != insStmt); //make sure it is ready

  string sql = string("INSERT INTO BargnVote (ScenarioId, Turn_t, BargnId_i, BargnId_j, Act_k, Vote) VALUES ('")
    + scenId + "', :turn_t, :bargnid_i, :bargnid_j, :act_k, :vote)";
  query.prepare(QString::fromStdString(sql));

    // start for the transaction
	sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);
  qtDB->transaction();

	for (unsigned int i = 0; i <Util_mat_row ; i++)
	{
		tuple<uint64_t, uint64_t> tijids = barginidspair_i_j[i];
		uint64_t Bargn_i = std::get<0>(tijids);
		uint64_t Bargn_j = std::get<1>(tijids);
		
			int rslt = 0;
			// Turn_t
			rslt = sqlite3_bind_int(insStmt, 1, t);
    query.bindValue(":turn_t", t);
      assert(SQLITE_OK == rslt);
			//Bargn_i
			rslt = sqlite3_bind_int(insStmt, 2, Bargn_i);
    query.bindValue(":bargnid_i", (qulonglong)Bargn_i);
      assert(SQLITE_OK == rslt);
			//Bargn_j
			rslt = sqlite3_bind_int(insStmt, 3, Bargn_j);
    query.bindValue(":bargnid_j", (qulonglong)Bargn_j);
      assert(SQLITE_OK == rslt);
			//Act_i
			rslt = sqlite3_bind_int(insStmt, 4, act_k);
    query.bindValue(":act_k", act_k);
      assert(SQLITE_OK == rslt);
			//Util
			rslt = sqlite3_bind_double(insStmt, 5, Vote_mat[i]);
    double voteMat = Vote_mat[i];
    query.bindValue(":vote", voteMat);
      assert(SQLITE_OK == rslt);
			// finish  
			assert(SQLITE_OK == rslt);
			rslt = sqlite3_step(insStmt);
    if (!query.exec()) {
      cout << query.lastError().text().toStdString() << endl;
    }
      assert(SQLITE_DONE == rslt);
			sqlite3_clear_bindings(insStmt);
			assert(SQLITE_DONE == rslt);
			rslt = sqlite3_reset(insStmt);
			assert(SQLITE_OK == rslt);
	}
	sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &zErrMsg);
  qtDB->commit();
  sqlite3_finalize(insStmt); // finalize statement to avoid resource leaks

	delete sqlBuff;
	sqlBuff = nullptr;

	smpDB = db;
}

// populates record for table PosProb for each step of
// module run
void Model::sqlPosProb(unsigned int t)
{
  assert(nullptr != smpDB);
  assert(t < history.size());
  State* st = history[t];
  // check module for null
  assert(nullptr != st);
  // initiate the database
  sqlite3 * db = smpDB;
  smpDB = nullptr;
  // Error message in case
  char* zErrMsg = nullptr;
  auto sqlBuff = newChars(200);
  // prepare the sql statement to insert
  sprintf(sqlBuff,
          "INSERT INTO PosProb (ScenarioId, Turn_t, Est_h,Pos_i, Prob) VALUES ('%s', ?1, ?2, ?3, ?4)",
          scenId.c_str());
  const char* insStr = sqlBuff;

  assert(nullptr != db);
  sqlite3_stmt *insStmt;
  sqlite3_prepare_v2(db, insStr, strlen(insStr), &insStmt, NULL);
  assert(nullptr != insStmt); //make sure it is ready

  string sql = string("INSERT INTO PosProb (ScenarioId, Turn_t, Est_h,Pos_i, Prob) VALUES ('")
    + scenId + "', :turn_t, :est_h, :pos_i, :prob)";
  query.prepare(QString::fromStdString(sql));

  // start for the transaction
  sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);
  qtDB->transaction();
  // collect the information from each estimator,actor
  for (unsigned int h = 0; h < numAct; h++)   // estimator is h
  {
    // calculate the probablity with respect to each estimator
    auto pn = st->pDist(h);
    auto pdt = std::get<0>(pn); // note that these are unique positions
    assert( fabs(1 - sum(pdt)) < 1e-4);
    auto unq = std::get<1>(pn);
    // for each actor pupulate the probablity information
    for (unsigned int i = 0; i < numAct; i++)
    {
      int rslt = 0;
      // Extract the probabity for each actor
      double prob = st->posProb(i, unq, pdt);
      rslt = sqlite3_bind_int(insStmt, 1, t);
      query.bindValue(":turn_t", t);
      assert(SQLITE_OK == rslt);
      rslt = sqlite3_bind_int(insStmt, 2, h);
      query.bindValue(":est_h", h);
      assert(SQLITE_OK == rslt);
      rslt = sqlite3_bind_int(insStmt, 3, i);
      query.bindValue(":pos_i", i);
      assert(SQLITE_OK == rslt);
      rslt = sqlite3_bind_double(insStmt, 4, prob);
      query.bindValue(":prob", prob);
      assert(SQLITE_OK == rslt);
      rslt = sqlite3_step(insStmt);
      if (!query.exec()) {
        cout << query.lastError().text().toStdString() << endl;
      }
      assert(SQLITE_DONE == rslt);
      sqlite3_clear_bindings(insStmt);
      assert(SQLITE_DONE == rslt);
      rslt = sqlite3_reset(insStmt);
      assert(SQLITE_OK == rslt);
    }
  }
  sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &zErrMsg);
  qtDB->commit();
  sqlite3_finalize(insStmt); // finalize statement to avoid resource leaks
  //printf("Stored SQL for turn %u of all estimators, actors, and positions \n", t);

  delete sqlBuff;
  sqlBuff = nullptr;

  smpDB = db; // give it the new pointer

  return;
}
// populates record for table PosProb for each step of
// module run
void Model::sqlPosVote(unsigned int t)
{
  assert(nullptr != smpDB);
  assert(t < history.size());
  State* st = history[t];


  // check module for null
  assert(nullptr != st);
  // initiate the database
  sqlite3 * db = smpDB;
  smpDB = nullptr;
  // Error message in case
  char* zErrMsg = nullptr;
  auto sqlBuff = newChars(200);
  // prepare the sql statement to insert
  sprintf(sqlBuff,
          "INSERT INTO PosVote (ScenarioId, Turn_t, Est_h, Voter_k,Pos_i, Pos_j,Vote) VALUES ('%s', ?1, ?2, ?3, ?4,?5,?6)",
          scenId.c_str());

  assert(nullptr != db);
  const char* insStr = sqlBuff;
  sqlite3_stmt *insStmt;
  sqlite3_prepare_v2(db, insStr, strlen(insStr), &insStmt, NULL);
  assert(nullptr != insStmt); //make sure it is ready

  string sql = string("INSERT INTO PosVote (ScenarioId, Turn_t, Est_h, Voter_k, Pos_i, Pos_j, Vote) VALUES ('")
    + scenId + "', :turn_t, :est_h, :voter_k, :pos_i, :pos_j, :vote)";
  query.prepare(QString::fromStdString(sql));

  // start for the transaction
  sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &zErrMsg);
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
            int rslt = 0;
            rslt = sqlite3_bind_int(insStmt, 1, t);
            query.bindValue(":turn_t", t);
            assert(SQLITE_OK == rslt);
            rslt = sqlite3_bind_int(insStmt, 2, h);
            query.bindValue(":est_h", h);
            assert(SQLITE_OK == rslt);
            //voter_k
            rslt = sqlite3_bind_int(insStmt, 3, k);
            query.bindValue(":voter_k", k);
            assert(SQLITE_OK == rslt);
            // position i
            rslt = sqlite3_bind_int(insStmt, 4, i);
            query.bindValue(":pos_i", i);
            assert(SQLITE_OK == rslt);
            //position j
            rslt = sqlite3_bind_int(insStmt, 5, j);
            query.bindValue(":pos_j", j);
            assert(SQLITE_OK == rslt);
            // vote ?
            rslt = sqlite3_bind_double(insStmt, 6, vij);
            query.bindValue(":vote", vij);
            assert(SQLITE_OK == rslt);
            rslt = sqlite3_step(insStmt);
            if (!query.exec()) {
              cout << query.lastError().text().toStdString() << endl;
            }
            assert(SQLITE_DONE == rslt);
            sqlite3_clear_bindings(insStmt);
            assert(SQLITE_DONE == rslt);
            rslt = sqlite3_reset(insStmt);
            assert(SQLITE_OK == rslt);
          }
        }
      }
    }
  }
  sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &zErrMsg);
  qtDB->commit();

  sqlite3_finalize(insStmt); // finalize statement to avoid resource leaks
  //printf("Stored SQL for turn %u of all estimators, actors, and positions \n", t);

  delete sqlBuff;
  sqlBuff = nullptr;

  smpDB = db; // give it the new pointer

  return;
}

void Model::createTableIndices() {
    char * zErrMsg = nullptr;
    const char * indexUtil = "CREATE INDEX IF NOT EXISTS idx_util ON PosUtil(ScenarioId, Turn_t, Est_h, Act_i, Pos_j)";
    int rc = sqlite3_exec(smpDB, indexUtil, nullptr, nullptr, &zErrMsg);
    assert(rc == SQLITE_OK);
    string qry = string(indexUtil);
    execQuery(qry);

    const char *indexActor = "CREATE INDEX IF NOT EXISTS idx_actor ON ActorDescription(ScenarioId)";
    rc = sqlite3_exec(smpDB, indexActor, nullptr, nullptr, &zErrMsg);
    assert(rc == SQLITE_OK);
    qry = string(indexActor);
    execQuery(qry);
}

void Model::dropTableIndices() {
    char * zErrMsg = nullptr;
    const char * indexUtil = "DROP INDEX IF EXISTS idx_util";
    int rc = sqlite3_exec(smpDB, indexUtil, nullptr, nullptr, &zErrMsg);
    string qry = string(indexUtil);
    execQuery(qry);

    assert(rc == SQLITE_OK);

    const char * indexActor = "DROP INDEX IF EXISTS idx_actor";
    rc = sqlite3_exec(smpDB, indexActor, nullptr, nullptr, &zErrMsg);
    assert(rc == SQLITE_OK);
    qry = string(indexActor);
    execQuery(qry);
}

void Model::loginCredentials(string connString) {
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
    { "Driver", userParams::Driver },
    { "Server", userParams::Server },
    { "Port", userParams::Port },
    { "Database", userParams::Database },
    { "Uid", userParams::Uid },
    { "Pwd", userParams::Pwd },
  };

  string parsedParam;
  std::stringstream  inputCredential(const_cast<char*>(connString.c_str()));
  while (getline(inputCredential, parsedParam, ';'))
  {
    auto it = std::find(parsedParam.begin(), parsedParam.end(), '=');
    string key, value;
    key.assign(parsedParam.begin(), it);
    value.assign(it + 1, parsedParam.end());
    userParams userParam;
    try {
      userParam = mapStringToUserParams.at(key);
    }
    catch (const std::out_of_range& oor) {
      std::cerr << "Error: Wrong connection string provided for DB!"
        << endl << oor.what() << endl;
      assert(false);
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
      if (value.empty()) {
        cout << "Enter the password to connect with database: ";
        std::cin >> value;
        cout << endl;
      }
      password = QString::fromStdString(value);
      break;
    default:
      cout << "Error in input credentials format." << endl;
    }
  }

  if (dbDriver.isEmpty() || databaseName.isEmpty()) {
    cout << "Error! Database type or database name can not be left blank." << endl;
    assert(false);
  }

  // We use either Postgresql or SQLITE
  if (dbDriver.compare("QPSQL") && dbDriver.compare("QSQLITE")) {
    cout << "Error! Wrong driver name. Supported Drivers: postgres(QPSQL), sqlite3(QSQLITE)" << endl;
    assert(false);
  }

  // for a non-sqlite db
  if (!dbDriver.compare("QPSQL")) {
    if (server.isEmpty()) {
      cout << "Error! Please provide address for postgres server" << endl;
      assert(false);
    }

    // Prompt for password if the password has not been provided
    if (password.isEmpty()) {
      cout << "Enter the password to connect with database: ";
      string value;
      std::cin >> value;
      cout << endl;
      password = QString::fromStdString(value);
    }
  }

  if (!dbDriver.compare("QSQLITE")) {
    databaseName.append(".db");
  }
}

} // end of namespace

// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
