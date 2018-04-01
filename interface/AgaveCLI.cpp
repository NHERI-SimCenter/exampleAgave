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

#include <QWindow>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStringList>

#include <QUuid>

//using namespace std;

AgaveCLI::AgaveCLI(QString &_tenant, QString &_storage, QObject *parent)
    :AgaveInterface(parent), tenant(_tenant), storage(_storage), loggedInFlag(false)
{
    //
    // for operation the class needs two temporary files to function
    //  - either use name Qt provides or use a QUid in current dir

    /*******************  maybe will use QTemporrary file instead .. save for rainy day
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

    //
    // start the QProcess, that process that will make call to the terminal cmd
    //

    proc = new QProcess();

    //
    // finally create a login widget to allow users to login
    //

    loginWindow = new QWidget();
    QGridLayout *loginLayout = new QGridLayout();
    QLabel *nameLabel = new QLabel();
    nameLabel->setText("username:");
    QLabel *passwordLabel = new QLabel();
    passwordLabel->setText("password:");
    nameLineEdit = new QLineEdit();
    passwordLineEdit = new QLineEdit();
    passwordLineEdit->setEchoMode(QLineEdit::Password);
    QPushButton *submitButton = new QPushButton();
    submitButton->setText("Login");
    loginLayout->addWidget(nameLabel,0,0);
    loginLayout->addWidget(nameLineEdit,0,1);
    loginLayout->addWidget(passwordLabel,1,0);
    loginLayout->addWidget(passwordLineEdit,1,1);
    loginLayout->addWidget(submitButton,2,2);
    loginWindow->setLayout(loginLayout);

    // connect the login submit button clicked signal with the code to actually login
    connect(submitButton,SIGNAL(clicked(bool)),this,SLOT(loginSubmitButtonClicked()));
}

AgaveCLI::~AgaveCLI()
{
    //
    // clean up, remove temp files, delete QProcess and delete login
    //  - deleted login as never set the widgets parent window

    // if we have logged in .. delete the client app
    if (loggedInFlag == true) {
        QString command = QString("clients-delete  -S -v -N SimCenter_uqFEM -u ") +
                nameLineEdit->text() + QString(" -p ") + passwordLineEdit->text() + QString(" > ") + uniqueFileName1;
        this->invokeAgaveCLI(command);
    }

    QFile file1 (uniqueFileName1);
    file1.remove();
    QFile file2 (uniqueFileName2);
    file2.remove();

    delete loginWindow;

    delete proc;

}

void
AgaveCLI::loginSubmitButtonClicked(void) {

    //
    // method called when push button on this classes login widget is clicked
    //  - gathers login and password from the widget
    //  - tests correctness by trying to login with said info to agave
    //  - returns if correct login (setting loggedInFlag) or max tries exceeded

    int maxNumTries = 3;

    if (loggedInFlag == false && numTries < maxNumTries) {
        // obtain login info
        QString login = nameLineEdit->text();
        QString password = passwordLineEdit->text();
        if (login.size() == 0) {
            login = "no_username";
        }
        if (password.size() == 0)
            password = "no_password";

        // try loogin in with username and password
        loggedInFlag = this->login(login, password);
        numTries++;
    }

    // return if logged in or maxtries exceeded
    if (loggedInFlag == true || numTries >= maxNumTries)
        loginWindow->hide();
}

bool 
AgaveCLI::login()
{
    // login method called by external class (MainWindow) - just resets login tries
    // and then shows the login window

    numTries = 0;
    loginWindow->show();
    return true;
}

bool
AgaveCLI::logout()
{
    bool result = false;

    QString command = QString("auth-tokens-revoke > ")  + uniqueFileName1;
    this->invokeAgaveCLI(command);

    // set loggedInFlag to false
    loggedInFlag = false;
    return true;
}

bool 
AgaveCLI::isLoggedIn()
{
    return loggedInFlag;
}


bool 
AgaveCLI::login(const QString &login, const QString &password)
{
    bool result = false;

    // make sure tenants set correctly
    QString command = QString("tenants-¬init -t ") + tenant + 
             QString(" > ") + uniqueFileName1;
    this->invokeAgaveCLI(command);

    // create a client app for the user ..
    //  NOTE: will not create duplicate but ok (cost of checking to see if exists more expensive
    command = QString("clients-create  -S -v -N SimCenter_uqFEM -u ") +
            login + QString(" -p ") + password + QString(" > ") + uniqueFileName1;
    this->invokeAgaveCLI(command);

    // now create the Oauth bearer token .. good for 4 hours
    command = QString("auth-tokens-create -S -v -f -u ") +
            login + QString(" -p ") + password + QString(" > ") + uniqueFileName1;
    this->invokeAgaveCLI(command);

    // open results file
    QFile file(uniqueFileName1);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        result = false;
        qDebug() << "ERROR: COULD NOT OPEN RESULT";
        return result;
    }

    // read results file & check for errors
    QString val;
    val=file.readAll();
    file.close();

    if (val.contains("Invalid Credentals") || val.isEmpty()) {
        result = false;
        emit sendErrorMessage(QString("BAD username and/or password"));
    } else if (val.contains("successfully refreshed")) {
        result = true;
        emit sendErrorMessage("");
    }

    return result;
}


QString
AgaveCLI::getHomeDirPath(void){

   // QString result = QString("agave://designsafe.storage.default/") + nameLineEdit->text();
    QString result = storage + nameLineEdit->text();
    return result;
}


bool
AgaveCLI::uploadDirectory(const QString &local, const QString &remote)
{
    bool result = true;

    // for upload we need to remove he agave storage URL
    QString storage("agave://designsafe.storage.default/");
    QString remoteCleaned = remote;
    remoteCleaned.remove(storage);

    QString command = QString("files-upload -v -F ") + local + QString("  ") +
            remoteCleaned + QString(" > ") + uniqueFileName1;
    this->invokeAgaveCLI(command);

    // open results file
    QFile file(uniqueFileName1);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        result = false;
        emit sendErrorMessage("ERROR: COULD NOT RUN AGAVE-CLI COMMAND");
        return result;
    }

    // read results file & check for errors
    QString val;
    val=file.readAll();
    file.close();

    if (val.contains("Invalid Credentals")) {
        result = false;
        emit sendErrorMessage("ERROR: YOU NEED TO LOGIN");
    } else if (val.contains("does not exist")) {
        result = false;
        emit sendErrorMessage("ERROR: remote dir does not exist");
    } else if (val.contains("Invalid authentication credentials")) {
        result = false;
        emit sendErrorMessage("ERROR: user does not have access to remote dir");
    }

    return result;
}


bool
AgaveCLI::removeDirectory(const QString &remote)
{
    bool result = true;
    emit sendErrorMessage("Removing Directory from DesignSafe");

    QString command = QString("files-delete -v ")  + remote + QString(" > ") + uniqueFileName1;
    this->invokeAgaveCLI(command);

    //
    // process the results
    //

    // open results file
    QFile file(uniqueFileName1);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        result = false;
        emit sendErrorMessage("ERROR: COULD NOT OPEN RESULT");
        return result;
    }

    // read results file & check for errors
    QString val;
    val=file.readAll();
    file.close();

    if (val.contains("Invalid Credentals")) {
        result = false;
        emit sendErrorMessage("ERROR: NEED TO LOGIN");
        return result;
    }
    if (val.contains("does not exist")) {
        result = false;
        emit sendErrorMessage("ERROR: remote dir does not exist");
        return result;
    }
    if (val.contains("Invalid authentication credentials")) {
        result = false;
        emit sendErrorMessage("ERROR: user does not have access to remote dir");
        return result;
    }

    emit sendErrorMessage("");
    return result;
}


bool
AgaveCLI::downloadFile(const QString &remote, const QString &local)
{
    bool result = true;

    QString command = QString("files-get -v -N ")  + local + QString(" ") + remote + QString(" > ") + uniqueFileName1;
    this->invokeAgaveCLI(command);

    //
    // process the results
    //

    // open results file
    QFile file(uniqueFileName1);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        result = false;
        emit sendErrorMessage("ERROR: COULD NOT OPEN RESULT");
        return result;
    }

    // read results file & check for errors
    QString val;
    val=file.readAll();
    file.close();

    if (val.contains("Invalid Credentals")) {
        result = false;
        emit sendErrorMessage("ERROR: NEED TO LOGIN");
        return result;
    }
    if (val.contains("does not exist")) {
        result = false;
        emit sendErrorMessage("ERROR: remote dir does not exist");
        return result;
    }
    if (val.contains("Invalid authentication credentials")) {
        result = false;
        emit sendErrorMessage("ERROR: user does not have access to remote dir");
        return result;
    }
    if (val.contains("Failed to create")) {
        result = false;
        emit sendErrorMessage("ERROR: User Gave wrong local name");
        return result;
    }

    return result;
}


QString
AgaveCLI::startJob(const QString &jobDescriptionFile)
{
    QString result = "FAILURE";

    QString command = QString("jobs-submit -v -F ") + jobDescriptionFile + QString(" > ") + uniqueFileName1;
    this->invokeAgaveCLI(command);

    //
    // process the results
    //

    // open results file
    QFile file(uniqueFileName1);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        result = false;
        emit sendErrorMessage("ERROR: COULD NOT OPEN RESULT");
        return result;
    }

    // read results file & check for errors
    QString val;
    val=file.readAll();
    file.close();

    if (val.contains("Invalid Credentals")) {
        result = false;
        emit sendErrorMessage("ERROR: NEED TO LOGIN");
        return result;
    }
    if (val.contains("does not exist")) {
        result = false;
        emit sendErrorMessage("ERROR: remote dir does not exist");
        return result;
    }
    if (val.contains("Invalid authentication credentials")) {
        result = false;
        emit sendErrorMessage("ERROR: user does not have access to remote dir");
        return result;
    }

    // sucessfull submission, go get jobID
    QJsonDocument doc = QJsonDocument::fromJson(val.toUtf8());
    QJsonObject jsonObj = doc.object();
    if (jsonObj.contains("id"))
        result = jsonObj["id"].toString();
    else {
        emit sendErrorMessage("ARROR: AGAVE FOLKS CHANGED API .. contact SimCenter");
        result = "ERROR: Agave FOLKS CHANGED API";
        return result;
    }

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
        emit sendErrorMessage("ERROR: COULD NOT OPEN TEMP FILE TO WRITE JSON");
        return result;
    }

    QJsonDocument doc2(theJob);
    file2.write(doc2.toJson());
    file2.close();

    // invoke previos method & return
    return this->startJob(uniqueFileName2);
}

QJsonObject
AgaveCLI::getJobList(const QString &matchingName)
{
    emit sendErrorMessage("Getting List of Jobs from DesignSafe");
    QJsonObject result;

    QString command = QString("jobs-list -v ") + QString(" > ") + uniqueFileName1;
    this->invokeAgaveCLI(command);

    //
    // process the results
    //

    // open results file
    QFile file(uniqueFileName1);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        emit sendErrorMessage("ERROR: COULD NOT OPEN RESULT");
        return result;
    }

    // read results file & check for errors
    QString val;
    val=file.readAll();
    file.close();

    if (val.contains("Invalid Credentals")) {
        emit sendErrorMessage("ERROR: NEED TO LOGIN");
        return result;
    }

    QString jsonString = QString("{ \"jobs\":") + val + QString("}");

    // sucessfull submission, go get jobID
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
    result = doc.object();

    return result;
}

QJsonObject
AgaveCLI::getJobDetails(const QString &jobID)
{
    emit sendErrorMessage("Getting Job Details from DesignSafe");
    QJsonObject result;

    QString command = QString("jobs-list -v ") + jobID + QString(" > ") + uniqueFileName1;
    this->invokeAgaveCLI(command);

    //
    // process the results
    //

    // open results file
    QFile file(uniqueFileName1);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        emit sendErrorMessage("ERROR: COULD NOT OPEN RESULT");
        return result;
    }

    // read results file & check for errors
    QString val;
    val=file.readAll();
    file.close();

    if (val.contains("Invalid Credentals")) {
        emit sendErrorMessage("ERROR: NEED TO LOGIN");
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

    QString command = QString("jobs-status ") + jobID + QString(" > ") + uniqueFileName1;
    if (this->invokeAgaveCLI(command) == false) {
        return result;
    }

    //
    // process the results
    //

    QFile file(uniqueFileName1);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        emit sendErrorMessage("ERROR: COULD NOT OPEN RESULT");
        return result;
    }

    // read results file & check for errors
    QString val;
    val=file.readAll();
    file.close();
    if (val.contains("Invalid Credentals")) {
        emit sendErrorMessage("ERROR: NEED TO LOGIN");
        return result;
    }
    if (val.contains("Not job found")) {
        emit sendErrorMessage("ERROR: NEED TO LOGIN");
        return result;
    }

    result = val;

    return result;
}

bool
AgaveCLI::deleteJob(const QString &jobID)
{
    bool result = false;

    QString command = QString("jobs-delete ") + jobID + QString(" > ") + uniqueFileName1;
    if (this->invokeAgaveCLI(command) == false) {
        return false; // invokeAGICLI to call error handler
    }

    //
    // process the results
    //

    // open results file
    QFile file(uniqueFileName1);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        emit sendErrorMessage("ERROR: COULD NOT OPEN RESULT");
        return result;
    }

    // read results file & check for errors
    QString val;
    val=file.readAll();
    file.close();
    if (val.contains("Invalid Credentals")) {
        emit sendErrorMessage("ERROR: NEED TO LOGIN");
        return result;
    }
    if (val.contains("No job found")) {
        emit sendErrorMessage("ERROR: No job found!");
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
    QString newCommand = QString("bash -c \" source $HOME/.bashrc; ") + command + QString("\"");
    qDebug() << newCommand;
#endif

    proc->start(newCommand);
    proc->waitForFinished(-1); //v-1 = wait indef

    QByteArray processOutput = proc->readAllStandardError();

    // return full control to UI
    QApplication::restoreOverrideCursor();

    // not really testing if worked!
    return true;
}

