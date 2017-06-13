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
// -------------------------------------------------

#include "database.h"

Database::Database()
{
    // Database Initiaization
    QStringList driverList;
    driverList = QSqlDatabase::drivers();

    if (!driverList.contains("QSQLITE", Qt::CaseInsensitive))
    {
        emit Message("Database Error", "No QSQLITE support! Check all needed dll-files!");
    }

    if (!driverList.contains("QPSQL", Qt::CaseInsensitive))
    {
        emit Message("Database Error", "No QPSQL support! Check all needed dll-files!");
    }

}

Database::~Database()
{
    releaseDB();
}

void Database::openDB(QString dbPath, QString dbType, QString connectionString, bool run)
{
    QSqlDatabase qdb = QSqlDatabase::addDatabase(dbType, "guiDb");
    db = new QSqlDatabase(qdb);
    if(dbType == "QSQLITE")
    {
        db->setDatabaseName(dbPath);
        dbName=dbPath;

        if(!db->open())
        {
            emit Message("Database Error", db->lastError().text());
        }
        qry = new QSqlQuery(*db);

        if(db->isOpen())
        {
            // Scenarios list in db
            getScenarioList(run);

            getActorsDescriptionDB();

            // to update numActors in db
            getNumActors();

            // number of states/turns in db
            getNumStates();

            //Affinty
            getAffinityDB();

            readVectorPositionTable(0,scenarioM,0);//turn
        }
    }
    else if (dbType == "QPSQL")
    {
        db->setDatabaseName(dbPath);
        dbName=dbPath;
        //qry = new QSqlQuery(*db);

        QStringList connectionParameters = connectionString.split(";");

        QStringList connectionValues;
        if(connectionParameters.length()>=5)
        {
            QString server = connectionParameters.at(4); //server
            connectionValues = server.split("=");

            server = connectionValues.at(1);
            db->setHostName(server);

            if(connectionString.contains("Port"))
            {
                QString port = connectionParameters.at(5);
                connectionValues = port.split("=");

                port  = connectionValues.at(1);
                db->setPort(port.toInt());
            }
            else
            {
                db->setPort(5432);
            }

            QString uId = connectionParameters.at(2); // Uid
            connectionValues = uId.split("=");
            uId  = connectionValues.at(1);

            QString pwd = connectionParameters.at(3); //Pwd
            connectionValues = pwd.split("=");
            pwd  = connectionValues.at(1);

            if(!db->open(uId,pwd))
            {
                emit Message("Database Error", db->lastError().text());
            }
            else
            {
                qry = new QSqlQuery(*db);

                // Scenarios list in db
                getScenarioList(run);

                getActorsDescriptionDB();

                // to update numActors in db
                getNumActors();

                // number of states/turns in db
                getNumStates();

                //Affinty
                getAffinityDB();

                readVectorPositionTable(0,scenarioM,0);//turn
            }
        }
        else
        {
            emit Message("Database", "Less parameters");
        }
    }

}

void Database::checkPostgresDB(QString connectionString, bool imp)
{
    if(connectionString.contains("QPSQL"))
    {
        dbPostgres = QSqlDatabase::addDatabase("QPSQL","checkDB");

        QStringList connectionParameters = connectionString.split(";");
        QStringList connectionValues;

        QString dbNm = connectionParameters.at(1); // DB
        connectionValues = dbNm.split("=");
        dbNm  = connectionValues.at(1);
        dbPostgres.setDatabaseName(dbNm);
        qDebug()<<dbNm << "database";

        if(connectionParameters.length()>=5)
        {
            QString server = connectionParameters.at(4); //server
            connectionValues = server.split("=");

            server  = connectionValues.at(1);
            dbPostgres.setHostName(server);

            if(connectionString.contains("Port"))
            {
                QString port = connectionParameters.at(5);
                connectionValues = port.split("=");

                port  = connectionValues.at(1);
                dbPostgres.setPort(port.toInt());
            }
            else
            {
                dbPostgres.setPort(5432);
            }

            QString uId = connectionParameters.at(2); // Uid
            connectionValues = uId.split("=");
            uId  = connectionValues.at(1);

            QString pwd = connectionParameters.at(3); //Pwd
            connectionValues = pwd.split("=");
            pwd  = connectionValues.at(1);

            if(!dbPostgres.open(uId,pwd))
            {
                emit Message("Database Error", dbPostgres.lastError().text());
            }
            else
            {
                qry = new QSqlQuery(dbPostgres);
                getDatabaseList(imp);

                if(dbPostgres.isOpen())
                {
                    dbPostgres.close();
                    QSqlDatabase::removeDatabase("checkDB");
                }
            }
        }
    }
}

void Database::getScenarioData(int turn, QString scenario,int dim)
{
    scenarioM=scenario;
    //model parameters for current scenario
    getModelParameters();

    readVectorPositionTable(turn,scenarioM,dim);//turn
}

void Database::getScenarioDataEdit(QString scenario)
{
    scenarioM=scenario;
}

void Database::getStateCount()
{
    getNumStates();
}

void Database::getDimensionCount()
{
    dimensionsList = new QStringList;
    QString query= QString("select Dim_k, 'Desc' from DimensionDescription where ScenarioId = '%1'").arg(scenarioM);
    qry->exec(query);

    while(qry->next())
    {
        numDimension = qry->value(0).toInt();
        dimensionsList->append(qry->value(1).toString());
    }

    emit dimensionsCount(numDimension,dimensionsList);
}

void Database::getActorsDescriptionDB()
{
    actorNameList.clear();
    actorDescList.clear();

    QString query= QString("select Name,'DESC' from ActorDescription  where ScenarioId = '%1' ").arg(scenarioM);

    qry->exec(query);

    while(qry->next())
    {
        actorNameList.append(qry->value(0).toString());
        actorDescList.append(qry->value(1).toString());
    }

    emit actorsNameDesc(actorNameList , actorDescList);
}

void Database::getInfluenceDB(int turn)
{
    actorInfluence.clear();

    //qDebug()<<scenario_m << "turn" << turn ;
    QString query= QString(" select SpatialCapability.Cap from SpatialCapability,ActorDescription where "
                           " ActorDescription.Act_i = SpatialCapability.Act_i "
                           " and SpatialCapability.ScenarioId='%1' "
                           " and ActorDescription.ScenarioId='%1'"
                           " and SpatialCapability.Turn_t='%2' "
                           ).arg(scenarioM).arg(turn);

    qry->exec(query);

    while(qry->next())
    {
        actorInfluence.append(qry->value(0).toString());
        //qDebug()<<actorInfluence << "Influence" <<turn;
    }

    emit actorsInflu(actorInfluence);
}

void Database::getPositionDB(int dim, int turn)
{
    actorPosition.clear();
    //qDebug()<<scenario_m;

    QString query= QString(" select VectorPosition.Pos_Coord from VectorPosition,ActorDescription where"
                           " ActorDescription.Act_i = VectorPosition.Act_i"
                           " and VectorPosition.ScenarioId='%2' "
                           " and ActorDescription.ScenarioId='%2'"
                           " and VectorPosition.Turn_t='%3'"
                           " and VectorPosition.dim_k='%1'")
            .arg(dim).arg(scenarioM).arg(turn);

    qry->exec(query);

    while(qry->next())
    {
        actorPosition.append(qry->value(0).toString());
    }
    if(actorPosition.length()>0)
        emit actorsPostn(actorPosition,dim);

}

void Database::getSalienceDB(int dim, int turn)
{
    actorSalience.clear();
    //qDebug()<<scenario_m;

    QString query= QString(" select SpatialSalience.Sal from SpatialSalience,ActorDescription where"
                           " ActorDescription.Act_i = SpatialSalience.Act_i"
                           " and SpatialSalience.ScenarioId='%2' "
                           " and ActorDescription.ScenarioId='%2' "
                           " and SpatialSalience.Turn_t='%3'"
                           " and SpatialSalience.dim_k='%1'")
            .arg(dim).arg(scenarioM).arg(turn);

    qry->exec(query);

    while(qry->next())
    {
        actorSalience.append(qry->value(0).toString());
    }
    emit actorsSalnce(actorSalience,dim);
}

void Database::getAffinityDB()
{
    actorI.clear();
    actorJ.clear();
    actorAffinity.clear();

    QString query= QString(" select Act_i, Act_j, Affinity from Accommodation where ScenarioId='%1' ").arg(scenarioM);

    qry->exec(query);

    while(qry->next())
    {
        actorI.append(qry->value(0).toInt());
        actorJ.append(qry->value(1).toInt());
        actorAffinity.append(qry->value(2).toString());
    }
    //      qDebug()<<actorAffinity.length() << actorI.length() << actorJ.length();
    emit actorsAffinity(actorAffinity,actorI,actorJ);
}

void Database::getActorsInRangeFromDB(double lowerRng, double higherRng, int dim, int turn)
{
    double lwr = lowerRng;
    double upr = higherRng;
    actorIdsList.clear();
    actorSalienceList.clear();
    actorCapabilityList.clear();

    QString query= QString(" select Act_i from VectorPosition where"
                           " Pos_Coord >= '%1'  AND Pos_Coord < '%2' AND "
                           " Dim_k='%3' AND ScenarioId='%4' "
                           "AND Turn_t='%5'")
            .arg(lwr).arg(upr).arg(dim).arg(scenarioM).arg(turn);

    qry->exec(query);

    while(qry->next())
    {
        actorIdsList.append(qry->value(0).toInt());
    }
    // qDebug()<<actorIdsList << "ACat tat"  << query;

    for(int actInd =0; actInd < actorIdsList.length() ; actInd++)
    {
        QString query= QString(" select Sal from SpatialSalience where"
                               " Act_i = '%1' AND ScenarioId='%2' AND Turn_t='%3'"
                               " AND Dim_k='%4'")
                .arg(actorIdsList.at(actInd)).arg(scenarioM).arg(turn).arg(dim);

        qry->exec(query);

        while(qry->next())
        {
            actorSalienceList.append(qry->value(0).toDouble());
        }
    }
    for(int actInd =0; actInd < actorIdsList.length() ; actInd++)
    {
        QString query1= QString(" select Cap from SpatialCapability where"
                                " Act_i = '%1' AND ScenarioId='%2' AND Turn_t='%3'")
                .arg(actorIdsList.at(actInd)).arg(scenarioM).arg(turn);

        qry->exec(query1);

        while(qry->next())
        {
            actorCapabilityList.append(qry->value(0).toDouble());
        }
    }
    emit listActorsSalienceCapability(actorIdsList,actorSalienceList,actorCapabilityList,lwr,upr);
}

void Database::getDims()
{
    dimList = new QStringList;
    QString query= QString("select 'Desc' from DimensionDescription where ScenarioId = '%1'").arg(scenarioM);
    qry->exec(query);

    while(qry->next())
    {
        dimList->append(qry->value(0).toString());
    }
    emit dimensList(dimList);
}

void Database::getUtilChlgAndSQvalues(QVector<int> VHAxisValues)
{
    if(10==VHAxisValues.length())
    {
        QString query = QString(" select H.diff as Hori_Coord, V.diff as Vert_Coord from "
                                "(select Turn_t,Est_h,Aff_k,Init_i,Rcvr_j,Util_Chlg - Util_SQ as diff from UtilChlg "
                                " where ScenarioId = '%1' and  Turn_t = '%2' and Init_i = '%3' and Rcvr_j = '%4' "
                                " and aff_k = '%5' and est_h = '%6') as V "
                                " inner join "
                                "(select Turn_t,Est_h,Aff_k,Init_i,Rcvr_j,Util_Chlg - Util_SQ as diff from UtilChlg "
                                " where ScenarioId = '%1' and Turn_t = '%2' and Init_i = '%3' and Rcvr_j ='%4' "
                                " and aff_k = '%7' and est_h = '%8') as H "
                                " on (V.Turn_t = H.Turn_t) and (V.Init_i = H.Init_i) and (V.Rcvr_j = H.Rcvr_j)")
                .arg(scenarioM).arg(VHAxisValues[0]).arg(VHAxisValues[3]).arg(VHAxisValues[4])
                .arg(VHAxisValues[2]).arg(VHAxisValues[1]).arg(VHAxisValues[7]).arg(VHAxisValues[6]);

        qry->exec(query);

        while(qry->next())
        {
            hCord=qry->value(0).toDouble();
            vCord=qry->value(1).toDouble();
        }
        emit utilChlngAndSQ(VHAxisValues[0],hCord,vCord,VHAxisValues[4]);
    }
}

void Database::releaseDB()
{
    QString connectionName = db->connectionName();;
    //    qDebug()<<connectionName;
    qry->finish();
    qry->clear();
    delete qry;
    if(nullptr != db) {
        delete db;
    }
    QSqlDatabase::removeDatabase(connectionName);
}

void Database::getActorMovedDataDB(QString scenario)
{
    actorMovedModel = new QStandardItemModel(this);

    int colIndex =0;
    int rowIndex =0;

    QString query;

    query= QString("select M.Movd_Turn, M.Act_i as Movd_ActorID, M.Dim_k, M.PrevPos, M.CurrPos, M.Diff, "
                   "M.Mover_BargnID, MI.Name as Initiator, MR.Name as Receiver, B.Init_Act_i, B.Recd_Act_j "
                   "from (select L0.Act_i, L0.Dim_k, L0.Turn_t as Movd_Turn, L0.Mover_BargnId, L0.Pos_Coord "
                   "as CurrPos, L1.Pos_Coord as PrevPos, L0.Pos_Coord - L1.Pos_Coord as Diff "
                   "from (select * from VectorPosition where Turn_t <> 0 and ScenarioID = '%1') "
                   "as L0 inner join (select * from VectorPosition where ScenarioID = '%1') "
                   "as L1 on L0.Turn_t = (L1.Turn_t+1) and L0.Act_i = L1.Act_i and L0.Dim_k = L1.Dim_k "
                   "where L0.Pos_Coord <> L1.Pos_Coord ) as M inner join (select * from Bargn where "
                   "ScenarioID = '%1') as B on M.Mover_BargnId = B.BargnId inner join "
                   "ActorDescription as MI on B.Init_Act_i = MI.Act_i and B.ScenarioID = MI.ScenarioID inner join "
                   "ActorDescription as MR on B.Recd_Act_j = MR.Act_i and B.ScenarioID = MR.ScenarioID  ").arg(scenario);

    qry->exec(query);
    while(qry->next())
    {
        actorMovedModel->setItem(rowIndex,colIndex,new QStandardItem(qry->value(0).toString().trimmed()));
        actorMovedModel->setItem(rowIndex,++colIndex,new QStandardItem(qry->value(1).toString().trimmed()));
        actorMovedModel->setItem(rowIndex,++colIndex,new QStandardItem(qry->value(2).toString().trimmed()));
        actorMovedModel->setItem(rowIndex,++colIndex,new QStandardItem(qry->value(3).toString().trimmed()));
        actorMovedModel->setItem(rowIndex,++colIndex,new QStandardItem(qry->value(4).toString().trimmed()));
        actorMovedModel->setItem(rowIndex,++colIndex,new QStandardItem(qry->value(5).toString().trimmed()));
        actorMovedModel->setItem(rowIndex,++colIndex,new QStandardItem(qry->value(6).toString().trimmed()));
        actorMovedModel->setItem(rowIndex,++colIndex,new QStandardItem(qry->value(7).toString().trimmed()));
        actorMovedModel->setItem(rowIndex,++colIndex,new QStandardItem(qry->value(8).toString().trimmed()));
        actorMovedModel->setItem(rowIndex,++colIndex,new QStandardItem(qry->value(9).toString().trimmed()));
        actorMovedModel->setItem(rowIndex,++colIndex,new QStandardItem(qry->value(10).toString().trimmed()));

        ++rowIndex;
        colIndex=0;
    }
    emit actorMovedInfo(actorMovedModel);
}

void Database::getVectorPosition(int actor, int dim, int turn, QString scenario)
{
    QString query;

    int i =0;
    QVector<double> x(numStates+1), y(numStates+1);

    query= QString("select * from VectorPosition where Act_i='%1' and Dim_k='%2' and Turn_t<='%3' and  ScenarioId = '%4' ")
            .arg(actor).arg(dim).arg(turn).arg(scenario);

    qry->exec(query);
    while(qry->next())
    {
        x[i]=qry->value(1).toDouble();
        y[i]=qry->value(4).toDouble();// y scales from 0 to 100
        ++i;
    }

    QString actorName;

    QString query2 = QString("select * from ActorDescription where Act_i='%1' and ScenarioId='%2'")
            .arg(actor).arg(scenario);
    qry->exec(query2);
    while(qry->next())
    {
        actorName = qry->value(2).toString();
    }

    emit vectorPosition(x,y,actorName,turn);
}

void Database::readVectorPositionTable(int turn, QString scenario, int dim)
{
    //change in scenario should update STATES ACTORS
    getNumStates();
    if(numStates<turn)
        turn=numStates;

    //Actors
    getNumActors();
    //Affinty
    getAffinityDB();

    sqlmodel = new QStandardItemModel(this);
    QString query;

    query= QString("select * from VectorPosition where Turn_t='%1' and Dim_k='%2' and ScenarioId='%3'")
            .arg(turn).arg(dim).arg(scenario);

    qry->exec(query);

    int rowindex =0;
    while(qry->next())
    {
        QString value = qry->value(0).toString();
        QString value1 = qry->value(1).toString();

        QStandardItem *item = new QStandardItem(value.trimmed());
        QStandardItem *item1 = new QStandardItem(value1.trimmed());

        sqlmodel->setItem(rowindex,0,item);
        sqlmodel->setItem(rowindex,1,item1);

        ++rowindex;
    }
    // load parsed data to model accordingly
    emit dbModel(sqlmodel);

    //To plot graph

    for(int actors=0; actors <= numActors; ++actors)
    {
        getVectorPosition(actors,dim,turn,scenario);//actors, dimension, turn
    }
}

void Database::readVectorPositionTableEdit(QString scenario)
{
    //choose the first scenario as default
    sqlmodelEdit = new QSqlTableModel(this);
    sqlmodelEdit->setTable("VectorPosition");
    sqlmodelEdit->setFilter(QString("Turn_t=0 and ScenarioId='%1'").arg(scenario));
    sqlmodelEdit->select();

    emit dbModelEdit(sqlmodelEdit);
}

void Database::getDatabaseList(bool imp)
{
    postgresDBList = new QStringList;

    QString query= QString("select datname from pg_database" ) ;

    qry->exec(query);

    while(qry->next())
    {
        postgresDBList->append(qry->value(0).toString());
        //        qDebug()<<qry->value(0).toString();
    }
    emit postgresExistingDBList(postgresDBList,imp);
}

void Database::getNumActors()
{
    QString query= QString("select Act_i from ActorDescription where ScenarioId='%1'" )
            .arg(scenarioM);

    qry->exec(query);

    while(qry->next())
    {
        numActors = qry->value(0).toInt();
    }

    emit actorCount(numActors);
}

void Database::getNumStates()
{
    QString query= QString("select DISTINCT Turn_t from VectorPosition where ScenarioId='%1'" )
            .arg(scenarioM);

    qry->exec(query);

    while(qry->next())
    {
        numStates = qry->value(0).toInt();
    }

    emit statesCount(numStates);
}

void Database::getScenarioList(bool run)
{
    scenarioList = new QStringList;
    scenarioIdList = new QStringList;
    scenarioDescList = new QStringList;
    QString query= QString("select Scenario,ScenarioId,'Desc' from ScenarioDesc ");

    qry->exec(query);

    while(qry->next())
    {
        scenarioList->append(qry->value(0).toString());
        scenarioIdList->append(qry->value(1).toString());
        scenarioDescList->append(qry->value(2).toString());
    }

    if(scenarioList->length()>0)
    {
        if(run)
        {
            scenarioM =  scenarioIdList->at(scenarioIdList->length()-1);
            emit scenarios(scenarioList,scenarioIdList,scenarioDescList,scenarioIdList->length()-1);
            //model parameters for current scenario
            getModelParameters();
        }
        else
        {
            scenarioM =  scenarioIdList->at(0);
            emit scenarios(scenarioList,scenarioIdList,scenarioDescList,0);
            //model parameters for current scenario
            getModelParameters();
        }

    }
    else
    {
        Message("Database","there is no Dataset/Scenario's");
    }

}

void Database::getModelParameters()
{
    scenarioModelParam.clear();

    QString query= QString("select * from ScenarioDesc where ScenarioId='%1' ").arg(scenarioM);

    qry->exec(query);

    while(qry->next())
    {
        seedDB = qry->value(3).toString();
        scenarioModelParam.append(qry->value(4).toInt());
        scenarioModelParam.append(qry->value(5).toInt());
        scenarioModelParam.append(qry->value(6).toInt());
        scenarioModelParam.append(qry->value(7).toInt());
        scenarioModelParam.append(qry->value(8).toInt());
        scenarioModelParam.append(qry->value(9).toInt());
        scenarioModelParam.append(qry->value(10).toInt());
        scenarioModelParam.append(qry->value(11).toInt());
        scenarioModelParam.append(qry->value(12).toInt());
    }

    if(scenarioModelParam.length()==9)
    {
        emit scenModelParameters(scenarioModelParam, seedDB);
    }
    else
        Message("Database","there are no/insufficient model parameters");
}
// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
