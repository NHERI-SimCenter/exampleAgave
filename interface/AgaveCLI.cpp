/* *****************************************************************************
Copyright (c) 2016-2017, The Regents of the University of California (Regents).
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the FreeBSD Project.

REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED HEREUNDER IS 
PROVIDED "AS IS". REGENTS HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, 
UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

*************************************************************************** */

// Written: fmckenna
#include "AgaveCLI.h"
#include <QApplication>
#include <QProcess>
#include <QDebug>
#include <QTemporaryFile>
#include <QJsonDocument>
#include <QMessageBox>
#include <QDir>
#include <QJsonObject>


#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <iterator>
#include <string>

#include <QUuid>

using namespace std;

static void myError(QString errorMessage) {
    qDebug() << errorMessage;
}

AgaveCLI::AgaveCLI()
{
    //
    // for operation the class needs two temporary files to function
    //  - either use name Qt provides or use a QUid in current dir

    /*******************  maybe will use QTemporrary file instead .. save for later day
    QTemporaryFile tmpFile;
    if (tmpFile.open()) {
        uniqueFileName1 = tmpFile.fileName();
        tmpFile.close();
    } else {
        // otherwise file in application dir
        QUuid uniqueName = QUuid::createUuid();
        QString strUnique = uniqueName.toString();
        uniqueFileName1 = QCoreApplication::applicationDirPath() + QDir::separator() + strUnique.mid(1,36);
    }

    QTemporaryFile tmpFile2;
    if (tmpFile2.open()) {
        uniqueFileName2 = tmpFile2.fileName();
        tmpFile2.close();
    } else {
        // otherwise file whereever app is running
        QUuid uniqueName2 = QUuid::createUuid();
        QString strUnique2 = uniqueName2.toString();
        uniqueFileName2 = strUnique2.mid(1,36);
    }
    ******************************************************************************** */

  QUuid uniqueName = QUuid::createUuid();
  QString strUnique = uniqueName.toString();
  uniqueFileName1 = QCoreApplication::applicationDirPath() + QDir::separator() + strUnique.mid(1,36);

  QUuid uniqueName2 = QUuid::createUuid();
  QString strUnique2 = uniqueName2.toString();
  uniqueFileName2 = strUnique2.mid(1,36);


    proc = new QProcess();
}

AgaveCLI::~AgaveCLI()
{
    //
    // remove the temp files
    //

    QFile file1 (uniqueFileName1);
    file1.remove();
    QFile file2 (uniqueFileName2);
    file2.remove();
}

bool 
AgaveCLI::login(const QString &login, const QString &password)
{
    bool result = true;

    //
    // invoke the function by calling the cli externally
    //

    // put up something other than spinning wheel of death
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // call the external application
#ifdef Q_OS_WIN
    //QString command = QString("python ") + pySCRIPT + QString(" ") + tDirectory + QString(" ") + tmpDirectory;
    //  proc->execute("cmd", QStringList() << "/C" << command);
#else
    QString command = QString("source $HOME/.bashrc; auth-tokens-create -S -v -f -u ") + login + QString(" -p ") + password + QString(" > ") + uniqueFileName1;
    //proc->execute("bash ", QStringList() << "-c" <<  command);
    qDebug() << command;
#endif
    this->invokeAgaveCLI(command);
    // wait till done  &then restore application to interactive state
    proc->waitForStarted();
    QApplication::restoreOverrideCursor();

    //
    // process the results
    //

    // open results file
    QFile file(uniqueFileName1);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
      result = false; qDebug() << "ERROR: COULD NOT OPEN RESULT";
      return result;
    }

    // read results file & check for errors
    QString val;
    val=file.readAll();  
    file.close();

    return result;
}

bool
AgaveCLI::uploadDirectory(const QString &local, const QString &remote)
{

    bool result = true;

    //
    // invoke the function by calling the cli externally
    //

    // put up something other than spinning wheel of death
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // call the external application
#ifdef Q_OS_WIN
    //QString command = QString("python ") + pySCRIPT + QString(" ") + tDirectory + QString(" ") + tmpDirectory;
    //  proc->execute("cmd", QStringList() << "/C" << command);
#else
    QString command = QString("source $HOME/.bashrc; files-upload -v -F ") + local + QString("  ") + remote + QString(" > ") + uniqueFileName1;
    proc->execute("bash", QStringList() << "-c" <<  command);
    qDebug() << command;
#endif

     // wait till done  &then restore application to interactive state
    proc->waitForStarted();
    QApplication::restoreOverrideCursor();

    // open results file
    QFile file(uniqueFileName1);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
      result = false; qDebug() << "ERROR: COULD NOT OPEN RESULT";
      return result;
    }

    // read results file & check for errors
    QString val;
    val=file.readAll();
    file.close();

    if (val.contains("Invalid Credentals")) {
       result = false;
       myError("ERROR: NEED TO LOGIN");
       return result;
    }
    if (val.contains("does not exist")) {
       result = false;
       myError("ERROR: remote dir does not exist");
       return result;
    }
    if (val.contains("Invalid authentication credentials")) {
       result = false;
       myError("ERROR: user does not have access to remote dir");
       return result;
    }

    return result;
}


bool
AgaveCLI::removeDirectory(const QString &remote)
{
    bool result = true;

    //
    // invoke the function by calling the cli externally
    //

    // put up something other than spinning wheel of death
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // call the external application
#ifdef Q_OS_WIN
    //QString command = QString("python ") + pySCRIPT + QString(" ") + tDirectory + QString(" ") + tmpDirectory;
    //  proc->execute("cmd", QStringList() << "/C" << command);
#else
    QString command = QString("source $HOME/.bashrc; files-delete -v ")  + remote + QString(" > ") + uniqueFileName1;
    proc->execute("bash", QStringList() << "-c" <<  command);
    qDebug() << command;
#endif

     // wait till done  &then restore application to interactive state
    proc->waitForStarted();
    QApplication::restoreOverrideCursor();

    //
    // process the results
    //

    // open results file
    QFile file(uniqueFileName1);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
      result = false;
      myError("ERROR: COULD NOT OPEN RESULT");
      return result;
    }

    // read results file & check for errors
    QString val;
    val=file.readAll();
    file.close();

    if (val.contains("Invalid Credentals")) {
       result = false;
       myError("ERROR: NEED TO LOGIN");
       return result;
    }
    if (val.contains("does not exist")) {
       result = false;
       myError("ERROR: remote dir does not exist");
       return result;
    }
    if (val.contains("Invalid authentication credentials")) {
       result = false;
       myError("ERROR: user does not have access to remote dir");
       return result;
    }

    return result;
}


bool
AgaveCLI::downloadFile(const QString &remote, const QString &local)
{
    bool result = true;

    //
    // invoke the function by calling the cli externally
    //

    // put up something other than spinning wheel of death
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // call the external application
#ifdef Q_OS_WIN
    //QString command = QString("python ") + pySCRIPT + QString(" ") + tDirectory + QString(" ") + tmpDirectory;
    //  proc->execute("cmd", QStringList() << "/C" << command);
#else
    QString command = QString("source $HOME/.bashrc; files-get -v -N ")  + local + QString(" ") + remote + QString(" > ") + uniqueFileName1;
    proc->execute("bash", QStringList() << "-c" <<  command);
    qDebug() << command;
#endif

     // wait till done  &then restore application to interactive state
    proc->waitForFinished(-1);
    QApplication::restoreOverrideCursor();

    //
    // process the results
    //

    // open results file
    QFile file(uniqueFileName1);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
      result = false;
      myError("ERROR: COULD NOT OPEN RESULT");
      return result;
    }

    // read results file & check for errors
    QString val;
    val=file.readAll();
    file.close();

    if (val.contains("Invalid Credentals")) {
       result = false;
       myError("ERROR: NEED TO LOGIN");
       return result;
    }
    if (val.contains("does not exist")) {
       result = false;
       myError("ERROR: remote dir does not exist");
       return result;
    }
    if (val.contains("Invalid authentication credentials")) {
       result = false;
       myError("ERROR: user does not have access to remote dir");
       return result;
    }
    if (val.contains("Failed to create")) {
       result = false;
       myError("ERROR: User Gave wrong local name");
       return result;
    }

    return result;
}


QString
AgaveCLI::startJob(const QString &jobDescriptionFile)
{
   QString result = "FAILURE";

   //
   // invoke the function by calling the cli externally
   //

   // put up something other than spinning wheel of death
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

   // call the external application
#ifdef Q_OS_WIN
    //  QString command = QString("python ") + pySCRIPT + QString(" ") + tDirectory + QString(" ") + tmpDirectory;
    //  proc->execute("cmd", QStringList() << "/C" << command);
#else
    QString command = QString("source $HOME/.bashrc; jobs-submit -v -F ") + jobDescriptionFile + QString(" > ") + uniqueFileName1;
    proc->execute("bash", QStringList() << "-c" <<  command);
    qDebug() << command;
#endif

    // wait till done  &then restore application to interactive state
   proc->waitForStarted();
   QApplication::restoreOverrideCursor();

   //
   // process the results
   //

    // open results file
    QFile file(uniqueFileName1);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
      result = false;
      myError("ERROR: COULD NOT OPEN RESULT");
      return result;
    }

    // read results file & check for errors
    QString val;
    val=file.readAll();
    file.close();

    if (val.contains("Invalid Credentals")) {
       result = false;
       myError("ERROR: NEED TO LOGIN");
       return result;
    }
    if (val.contains("does not exist")) {
       result = false;
       myError("ERROR: remote dir does not exist");
       return result;
    }
    if (val.contains("Invalid authentication credentials")) {
       result = false;
       myError("ERROR: user does not have access to remote dir");
       return result;
    }

    // sucessfull submission, go get jobID
    QJsonDocument doc = QJsonDocument::fromJson(val.toUtf8());
        QJsonObject jsonObj = doc.object();
        if (jsonObj.contains("id"))
            result = jsonObj["id"].toString();
        else
            result = "ERROR: Agave FOLKS CHANGED API";


    return result;
}

QString
AgaveCLI::startJob(const QJsonObject &theJob)
{
    QString result = "FAILURE";

    //
    // write job data to file
    //

    QFile file2(uniqueFileName2);
    if (!file2.open(QFile::WriteOnly | QFile::Text)) {
      myError("ERROR: COULD NOT OPEN TEMP FILE TO WRATE JSON");
      return result;
    }

    QJsonDocument doc2(theJob);
    file2.write(doc2.toJson());
    file2.close();

    // invoke previos method & return
    return this->startJob(uniqueFileName2);

 #ifdef Q_OS_WIN
     //  QString command = QString("python ") + pySCRIPT + QString(" ") + tDirectory + QString(" ") + tmpDirectory;
     //  proc->execute("cmd", QStringList() << "/C" << command);
 #else
    QString command = QString("source $HOME/.bashrc; jobs-submit -v -F ") + uniqueFileName2 + QString(" > ") + uniqueFileName1;
    proc->execute("bash", QStringList() << "-c" <<  command);
     qDebug() << command;
 #endif

     // wait till done  &then restore application to interactive state
    proc->waitForFinished(-1);
    QApplication::restoreOverrideCursor();

     // open results file
     QFile file(uniqueFileName1);
     if (!file.open(QFile::ReadOnly | QFile::Text)) {
       result = false;
       myError("ERROR: COULD NOT OPEN RESULT");
       return result;
     }

     // read results file & check for errors
     QString val;
     val=file.readAll();
     file.close();

     if (val.contains("Invalid Credentals")) {
        result = false;
        myError("ERROR: NEED TO LOGIN");
        return result;
     }
     if (val.contains("does not exist")) {
        result = false;
        myError("ERROR: remote dir does not exist");
        return result;
     }
     if (val.contains("Invalid authentication credentials")) {
        result = false;
        myError("ERROR: user does not have access to remote dir");
        return result;
     }

     // sucessfull submission, go get jobID
     QJsonDocument doc = QJsonDocument::fromJson(val.toUtf8());
         QJsonObject jsonObj = doc.object();
         if (jsonObj.contains("id"))
             result = jsonObj["id"].toString();
         else
             result = "ERROR: Either Agave FOLKS CHANGED API or YOU Failed to create auth token";

     return result;
     // open results file
}

QJsonObject
AgaveCLI::getJobList(const QString &matchingName)
{
    QJsonObject result;

    //
    // invoke the function by calling the cli externally
    //

    // put up something other than spinning wheel of death
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // call the external application
#ifdef Q_OS_WIN
    //  QString command = QString("python ") + pySCRIPT + QString(" ") + tDirectory + QString(" ") + tmpDirectory;
    //  proc->execute("cmd", QStringList() << "/C" << command);
#else
   QString command = QString("source $HOME/.bashrc; jobs-list -v ") + QString(" > ") + uniqueFileName1;
    proc->execute("bash", QStringList() << "-c" <<  command);
    qDebug() << command;
#endif

    // wait till done  &then restore application to interactive state
   proc->waitForFinished(-1);
   QApplication::restoreOverrideCursor();

    //this->invokeAgaveCLI(command);

   //
   // process the results
   //

    // open results file
    QFile file(uniqueFileName1);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
      myError("ERROR: COULD NOT OPEN RESULT");
      return result;
    }

    // read results file & check for errors
    QString val;
    val=file.readAll();
    file.close();

    if (val.contains("Invalid Credentals")) {
       myError("ERROR: NEED TO LOGIN");
       return result;
    }

    //qDebug() << "VAL:" << val;
    QString jsonString = QString("{ \"jobs\":") + val + QString("}");
    // sucessfull submission, go get jobID
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
    //QJsonObject jsonObj = doc.object();
    result = doc.object();

    return result;
}

QJsonObject
AgaveCLI::getJobDetails(const QString &jobID)
{
    QJsonObject result;

    //
    // invoke the function by calling the cli externally
    //

    // put up something other than spinning wheel of death
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // call the external application
#ifdef Q_OS_WIN
    //  QString command = QString("python ") + pySCRIPT + QString(" ") + tDirectory + QString(" ") + tmpDirectory;
    //  proc->execute("cmd", QStringList() << "/C" << command);
#else
   QString command = QString("source $HOME/.bashrc; jobs-list -v ") + jobID + QString(" > ") + uniqueFileName1;
   proc->execute("bash", QStringList() << "-c" <<  command);
    qDebug() << command;
#endif

    // wait till done  &then restore application to interactive state
   proc->waitForFinished(-1);
   QApplication::restoreOverrideCursor();

   //
   // process the results
   //

    // open results file
    QFile file(uniqueFileName1);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
      myError("ERROR: COULD NOT OPEN RESULT");
      return result;
    }

    // read results file & check for errors
    QString val;
    val=file.readAll();
    file.close();

    if (val.contains("Invalid Credentals")) {
       myError("ERROR: NEED TO LOGIN");
       return result;
    }

  //  qDebug() << "VAL:" << val;
   // QString jsonString = QString("{ \"jobs\":") + val + QString("}");
    // sucessfull submission, go get jobID
    QJsonDocument doc = QJsonDocument::fromJson(val.toUtf8());
    //QJsonObject jsonObj = doc.object();
    result = doc.object();

    return result;
}

QString
AgaveCLI::getJobStatus(const QString &jobID){
    QString result("OOPS!");

    //
    // invoke the function by calling the cli externally
    //

    // put up something other than spinning wheel of death
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // call the external application
#ifdef Q_OS_WIN
    //  QString command = QString("python ") + pySCRIPT + QString(" ") + tDirectory + QString(" ") + tmpDirectory;
    //  proc->execute("cmd", QStringList() << "/C" << command);
#else
   QString command = QString("source $HOME/.bashrc; jobs-status ") + jobID + QString(" > ") + uniqueFileName1;
   proc->execute("bash", QStringList() << "-c" <<  command);
    qDebug() << command;
#endif

   // wait till done  &then restore application to interactive state
   proc->waitForFinished(-1);
   QApplication::restoreOverrideCursor();

   //
   // process the results
   //

    QFile file(uniqueFileName1);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
      myError("ERROR: COULD NOT OPEN RESULT");
      return result;
    }

    // read results file & check for errors
    QString val;
    val=file.readAll();
    file.close();
    if (val.contains("Invalid Credentals")) {
       myError("ERROR: NEED TO LOGIN");
       return result;
    }
    if (val.contains("Not job found")) {
       myError("ERROR: NEED TO LOGIN");
       return result;
    }

    result = val;
    qDebug() << result;

    return result;
}

bool
AgaveCLI::deleteJob(const QString &jobID)
{
   bool result = false;

   //
   // invoke the function by calling the cli externally
   //

   // put up something other than spinning wheel of death
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

   //
   // set up command & call the external application
   //
#ifdef Q_OS_WIN
    //  QString command = QString("python ") + pySCRIPT + QString(" ") + tDirectory + QString(" ") + tmpDirectory;
    //  proc->execute("cmd", QStringList() << "/C" << command);
#else
   QString command = QString("source $HOME/.bashrc; jobs-delete ") + jobID + QString(" > ") + uniqueFileName1;
   proc->execute("bash", QStringList() << "-c" <<  command);
  // qDebug() << command;
#endif

   // wait till done  &then restore application to interactive state
   proc->waitForFinished(-1);
   QApplication::restoreOverrideCursor();

   // this->invokeAgaveCLI(command);

   //
   // process the results
   //

    // open results file
    QFile file(uniqueFileName1);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
      myError("ERROR: COULD NOT OPEN RESULT");
      return result;
    }

    // read results file & check for errors
    QString val;
    val=file.readAll();
    file.close();
    if (val.contains("Invalid Credentals")) {
       myError("ERROR: NEED TO LOGIN");
       return result;
    }
    if (val.contains("No job found")) {
       myError("ERROR: No job found!");
       return result;
    }

    // if get here it worked .. result true
    result = true;

    return result;
}


bool
AgaveCLI::invokeAgaveCLI(const QString &command)
{
    //
    // invoke the function by calling the cli externally
    //

    // put up something other than spinning wheel of death
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // call the external application
#ifdef Q_OS_WIN
    proc->execute("cmd", QStringList() << "/C" << command);
#else
    //int ok = proc->execute("bash ", QStringList() << "-c" <<  command);
   QString newCommand = QString("bash -c \" ") + command + QString("\"");
#endif

    proc->start(newCommand);
    proc->waitForFinished(-1);

    QByteArray processOutput = proc->readAllStandardError();
    //qDebug() << processOutput;
    //qDebug() << proc->error();

    // return full control to UI
    QApplication::restoreOverrideCursor();

    // open results file
    QFile file(uniqueFileName1);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
      myError("ERROR: COULD NOT OPEN RESULT");
      return false;
    }
    return true;
}
