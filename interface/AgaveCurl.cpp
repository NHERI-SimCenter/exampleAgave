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
#include "AgaveCurl.h"
#include <QApplication>
#include <QProcess>
#include <QDebug>
#include <QTemporaryFile>
#include <QJsonDocument>
#include <QJsonArray>
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

using namespace std;

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}

AgaveCurl::AgaveCurl(QString &_tenant, QString &_storage, QObject *parent)
  :AgaveInterface(parent), tenant(_tenant), storage(_storage), loggedInFlag(false)
{
    //
    // for operation the class needs two temporary files to function
    //  - either use name Qt provides or use a QUid in current dir

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
    // init curl variables
    //

    hnd = curl_easy_init();
    slist1 = NULL;
    tenantURL="https://agave.designsafe-ci.org/";
    appClient = QString("appClient");

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

AgaveCurl::~AgaveCurl()
{
    //
    // clean up, remove temp files, delete QProcess and delete login
    //  - deleted login as never set the widgets parent window

    // if we have logged in .. delete the client app

    curl_slist_free_all(slist1);
    slist1 = NULL;

    if (loggedInFlag == true) {
        QString username(nameLineEdit->text());
        QString password(passwordLineEdit->text());
        QString url = tenantURL + QString("clients/v2/") + appClient + QString("?pretty=true");
        QString user_passwd = username + QString(":") + password;
        qDebug() << "USER:PASSWORD: " << user_passwd;
        qDebug() << url;


        curl_easy_setopt(hnd, CURLOPT_URL, url.toStdString().c_str());
        curl_easy_setopt(hnd, CURLOPT_USERPWD, user_passwd.toStdString().c_str());
        curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "DELETE");
        bool ok = this->invokeCurl();


        /* maybe search for "Client removed successfully" at a later date ***********
        // open results file
        QFile file(uniqueFileName1);
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            emit sendErrorMessage("ERROR: COULD NOT OPEN RESULT");
        }

        // read results file & check for errors
        QString val;
        val=file.readAll();
        file.close();
        qDebug() << val;
        ****************************************************************************/
    }

  QFile file1 (uniqueFileName1);
  file1.remove();
  QFile file2 (uniqueFileName2);
  file2.remove();
  
  delete loginWindow;
  
  delete proc;
  
  curl_easy_cleanup(hnd);

}

void
AgaveCurl::loginSubmitButtonClicked(void) {

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
AgaveCurl::login()
{
    // login method called by external class (MainWindow) - just resets login tries
    // and then shows the login window

    numTries = 0;
    loginWindow->show();
    return true;
}

bool
AgaveCurl::logout()
{
    bool result = false;

    QString command = QString("auth-tokens-revoke > ")  + uniqueFileName1;
    //this->invokeCurl();

    // set loggedInFlag to false
    loggedInFlag = false;
    return true;
}

bool 
AgaveCurl::isLoggedIn()
{
    return loggedInFlag;
}


bool 
AgaveCurl::login(const QString &login, const QString &password)
{
    bool result = true;
    passwordLineEdit->setText(password);
    nameLineEdit->setText(login);

    QString consumerSecret;
    QString consumerKey;

    //
    // first try creating a client with username & password
    // with purpose of getting the two keys: consumerSecret and consumerKey
    //

    // invoke curl
    QString user_passwd = login + QString(":") + password;
    QString url = tenantURL + QString("clients/v2/?pretty=true");

    curl_easy_setopt(hnd, CURLOPT_URL, url.toStdString().c_str());
    curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(hnd, CURLOPT_USERPWD, user_passwd.toStdString().c_str());

    QString postField = QString("clientName=") + appClient + QString("&tier=Unlimited&description=client for app development&callbackUrl=");
    int postFieldLength = postField.length() ; // strlen(postFieldChar);
    char *pField = new char[postFieldLength+1]; // have to do new char as seems to miss ending string char when pass directcly
    strncpy(pField, postField.toStdString().c_str(),postFieldLength+1);
    
    curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, pField);
    curl_easy_setopt(hnd, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)postFieldLength);
    curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "POST");
    
    if (this->invokeCurl() == false)
        return false;

    // process curl results .. open results file
    QFile file(uniqueFileName1);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        emit sendErrorMessage("ERROR: COULD NOT OPEN RESULT");
        return false;
    }

    // read results file & check for errors
    QString val;
    val=file.readAll();
    file.close();
    
    // process results into json object and get the results object whhich contains the two keys

    if (val.contains("Invalid Credentals") || val.isEmpty()) {
        emit sendErrorMessage(QString("BAD username and/or password"));
        return false;
    } else if (val.contains("success")) {
        QJsonDocument doc = QJsonDocument::fromJson(val.toUtf8());
        QJsonObject jsonObj = doc.object();
        if (jsonObj.contains("result")) {
            QJsonObject resultObj =  jsonObj["result"].toObject();
            if (resultObj.contains("consumerKey"))
                consumerKey = resultObj["consumerKey"].toString();
            if (resultObj.contains("consumerSecret"))
                consumerSecret = resultObj["consumerSecret"].toString();
        }

        // some processing to remove some crap from frnt and end of strings
        consumerKey.remove("\"");
        consumerSecret.remove("\"");
    }

    //
    // given keys we now want to create the n Oauth bearer token .. good for 4 hours
    //

    // invoke curl

    QString combined = consumerKey + QString(":") + consumerSecret;
    url = tenantURL + QString("token");
    curl_easy_setopt(hnd, CURLOPT_URL, url.toStdString().c_str());
    curl_easy_setopt(hnd, CURLOPT_USERPWD, combined.toStdString().c_str());
    QString postField2 = QString("username=") + login + QString("&password=") + password +
            QString("&grant_type=password&scope=PRODUCTION");
    int postFieldLength2 = postField2.length() ; // strlen(postFieldChar);
    char *pField2 = new char[postFieldLength2+1]; // have to do new char as seems to miss ending string char when pass directcly
    strncpy(pField2, postField2.toStdString().c_str(),postFieldLength2+1);
    
    curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, pField2);
    curl_easy_setopt(hnd, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)postFieldLength2);
    curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "POST");
    if (this->invokeCurl() == false)
        return false;
    

    // process result .. we want that access token
    // when we have it we add to slist1 variable which is used in all the subsequent curl calls
    // we also mark loggedInFlag as true for subsequent calls to logn

    QFile file2(uniqueFileName1);
    if (!file2.open(QFile::ReadOnly | QFile::Text)) {
        result = false;
        emit sendErrorMessage("ERROR: COULD NOT OPEN RESULT of OAuth");
        return result;
    }
    
    // read results file & check for errors
    val=file2.readAll();
    file2.close();

    QString accessToken;
    
    if (val.contains("Invalid Credentals") || val.isEmpty()) {
        emit sendErrorMessage("ERROR: Invalid Credentials in OAuth!");
        return false;
    } else {
        QJsonDocument doc = QJsonDocument::fromJson(val.toUtf8());
        QJsonObject jsonObj = doc.object();
        if (jsonObj.contains("access_token")) {
            accessToken = jsonObj["access_token"].toString();
            qDebug() << "accessToken" << accessToken;
            QString bearer = QString("Authorization: Bearer ") + accessToken;
            slist1 = curl_slist_append(slist1, bearer.toStdString().c_str());
            loggedInFlag = true;
        }
    }

    return result;
}


QString
AgaveCurl::getHomeDirPath(void) {

    // QString result = QString("agave://designsafe.storage.default/") + nameLineEdit->text();
    QString result = storage + nameLineEdit->text();
    return result;
}


bool
AgaveCurl::uploadDirectory(const QString &local, const QString &remote)
{
    bool result = false;

    //
    // check local exists
    //

    QDir originDirectory(local);
    if (! originDirectory.exists())
    {
        return false;
    }
    QString dirName = originDirectory.dirName();

    //
    // create remote dir
    //


    QString remoteCleaned = remote;
    remoteCleaned.remove(storage);
    if (this->mkdir(dirName, remoteCleaned) != true)
        return false;
    remoteCleaned = remoteCleaned + QString("/") + dirName;

    //
    // now we go through each file in local dir & upload to new remote directory
    //   - if dir we recursivily call the method
    //


   // originDirectory.mkpath(destinationDir);

    foreach (QString directoryName, originDirectory.entryList(QDir::Dirs | \
                                                              QDir::NoDotAndDotDot))
    {
        QString nextRemote = remoteCleaned + "/" + directoryName;
        QString nextLocal = local + QDir::separator() + directoryName;

        if (this->uploadDirectory(nextLocal, remoteCleaned) != true) {
            this->removeDirectory(remoteCleaned); // remove any directory we created if failure
            return false;
        }

    }

    foreach (QString fileName, originDirectory.entryList(QDir::Files))
    {
        QString localFile = local + QDir::separator() + fileName;
        QString remoteFile = remoteCleaned + "/" + fileName;
        if (this->uploadFile(localFile, remoteFile) != true) {
            this->removeDirectory(remoteCleaned);
            return false;
        }
    }

    return true;
}


bool
AgaveCurl::removeDirectory(const QString &remote)
{
    bool result = false;

    // invoke curl to remove the file or directory
    QString url = tenantURL + QString("files/v2/media/") + remote;
    curl_easy_setopt(hnd, CURLOPT_URL, url.toStdString().c_str());
    curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "DELETE");
    this->invokeCurl();

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

    // read into json object
   QJsonDocument doc = QJsonDocument::fromJson(val.toUtf8());
   QJsonObject theObj = doc.object();

   // parse json object for status
   //   if error emit error message & return NOT FOUND, if no status it's an error
   //   if success then get result & return the job sttaus

   QString status;
   if (theObj.contains("status")) {
       status = theObj["status"].toString();
       if (status == "error") {

           // if error get errormessage and return error
           QString message("Job Not Found");
           if (theObj.contains("message"))
               message = theObj["message"].toString();

           qDebug() << "ERROR MESSAGE: " << message;
           emit this->sendErrorMessage(message);
           return false;

       } else if (status == "success") {
           return true;
       }

   } else if (theObj.contains("fault")) {
       QJsonObject theFault = theObj["fault"].toObject();
       if (theFault.contains("message")) {
           QString message = theFault["message"].toString();
           emit this->sendErrorMessage(message);
       }
   }

    return result;
}

bool
AgaveCurl::mkdir(const QString &remoteName, const QString &remotePath) {
    qDebug() << "creating remote dir: " << remoteName << "  at path " << remotePath;

     bool result = false;

      QString url = tenantURL + QString("files/v2/media/") + remotePath;

      QString postField = QString("action=mkdir&path=") + remoteName;
      int postFieldLength = postField.length() ; // strlen(postFieldChar);
      char *pField = new char[postFieldLength+1]; // have to do new char as seems to miss ending string char when pass directcly
      strncpy(pField, postField.toStdString().c_str(),postFieldLength+1);

      curl_easy_setopt(hnd, CURLOPT_URL, url.toStdString().c_str());
      curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, pField);
      curl_easy_setopt(hnd, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)postFieldLength);
      curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "PUT");

      if (this->invokeCurl() == false)
          return result;

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

      // read into json object
     QJsonDocument doc = QJsonDocument::fromJson(val.toUtf8());
     QJsonObject theObj = doc.object();

     // parse json object for status
     //   if error emit error message & return NOT FOUND, if no status it's an error
     //   if success then get result & return the job sttaus

     QString status;
     if (theObj.contains("status")) {
         status = theObj["status"].toString();
         if (status == "error") {
             QString message("Job Not Found");
             if (theObj.contains("message"))
                 message = theObj["message"].toString();
             qDebug() << "ERROR MESSAGE: " << message;
             emit this->sendErrorMessage(message);
             return result;
         } else if (status == "success") {
             return true;
         }

     }
     return result;
}


bool
AgaveCurl::uploadFile(const QString &local, const QString &remote) {

    qDebug() << "uploadingfile: " << local << " to: " << remote;
    bool result = false;

    //
    // obtain filename and remote path from the remote string
    // note: for upload we need to remove the agave storage URL if there
    //

    QString remoteCleaned = remote;
    remoteCleaned.remove(storage);
    QFileInfo   fileInfo(remoteCleaned);
    QString remoteName = fileInfo.fileName();
    QString remotePath = fileInfo.path();

    // invoke curl to upload the file or directory
    struct curl_httppost *post1;
    struct curl_httppost *postend;

    post1 = NULL;
    postend = NULL;
    curl_formadd(&post1, &postend,
                 CURLFORM_COPYNAME, "fileToUpload",
                 CURLFORM_FILE, local.toStdString().c_str(),
                 CURLFORM_CONTENTTYPE, "text/plain",
                 CURLFORM_END);
    curl_formadd(&post1, &postend,
                 CURLFORM_COPYNAME, "fileName",
                 CURLFORM_COPYCONTENTS, remoteName.toStdString().c_str(),
                 CURLFORM_END);

    QString url = tenantURL + QString("files/v2/media/") + remotePath;
    curl_easy_setopt(hnd, CURLOPT_URL, url.toStdString().c_str());
    curl_easy_setopt(hnd, CURLOPT_HTTPPOST, post1);
    curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "POST");

    if (this->invokeCurl() == false)
        return result;

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

    // read into json object
   QJsonDocument doc = QJsonDocument::fromJson(val.toUtf8());
   QJsonObject theObj = doc.object();

   // parse json object for status
   //   if error emit error message & return NOT FOUND, if no status it's an error
   //   if success then get result & return the job sttaus

   QString status;
   if (theObj.contains("status")) {
       status = theObj["status"].toString();
       if (status == "error") {
           QString message("Job Not Found");
           if (theObj.contains("message"))
               message = theObj["message"].toString();
           qDebug() << "ERROR MESSAGE: " << message;
           emit this->sendErrorMessage(message);
           return result;
       } else if (status == "success") {
           return true;
       }

   } else if (theObj.contains("fault")) {
       QJsonObject theFault = theObj["fault"].toObject();
       if (theFault.contains("message")) {
           QString message = theFault["message"].toString();
           emit this->sendErrorMessage(message);
       }
   }

    return result;
}

bool
AgaveCurl::downloadFile(const QString &remoteFile, const QString &localFile)
{
    // this method does not invoke the invokeCurl() as want to write to local file directtly

    bool result = true;
    CURLcode ret;

    // set up the call
    QString url = tenantURL + QString("files/v2/media/") + remoteFile;

    curl_easy_setopt(hnd, CURLOPT_URL, url.toStdString().c_str());
    curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(hnd, CURLOPT_USERAGENT, "curl/7.54.0");
    curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, slist1);
    curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(hnd, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
    curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(hnd, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(hnd, CURLOPT_FAILONERROR, true);

    // openup localfile and set the writedata pointer to it
    FILE *pagefile = fopen(localFile.toStdString().c_str(), "wb");
    if(pagefile) {
        curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(hnd, CURLOPT_WRITEDATA, pagefile);
    }

    // make the call
    ret = curl_easy_perform(hnd);
    fclose(pagefile);

    // reset the handle for next method as per normal
    curl_easy_reset(hnd);

    // if no error, return
    if (ret == CURLE_OK)
        return true;

    // if failure, go get message, emit signal and return false;

    QString message = QString("Failed to Download File: ") + remoteFile; // more descriptive message
    // const char *str = curl_easy_strerror(ret);
    //QString errorString(str);
    qDebug() << "ERROR: " << message;
    emit sendErrorMessage(message);

    return false;
}


QString
AgaveCurl::startJob(const QString &jobDescriptionFile)
{
    QString result = "FAILURE";

    // invoke curl to upload the file or directory
    struct curl_httppost *post1;
    struct curl_httppost *postend;

    post1 = NULL;
    postend = NULL;
    curl_formadd(&post1, &postend,
                 CURLFORM_COPYNAME, "fileToUpload",
                 CURLFORM_FILE, jobDescriptionFile.toStdString().c_str(),
                 CURLFORM_CONTENTTYPE, "application/octet-stream",
                 CURLFORM_END);

    QString url = tenantURL + QString("jobs/v2/?pretty=true");
    curl_easy_setopt(hnd, CURLOPT_URL, url.toStdString().c_str());
    curl_easy_setopt(hnd, CURLOPT_HTTPPOST, post1);
    curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "POST");

    if (this->invokeCurl() == false)
        return result;

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
qDebug() << val;
    // read into json object
   QJsonDocument doc = QJsonDocument::fromJson(val.toUtf8());
   QJsonObject theObj = doc.object();

   // parse json object for status
   //   if error emit error message & return NOT FOUND, if no status it's an error
   //   if success then get result & return the job sttaus

   QString status;
   if (theObj.contains("status")) {
       status = theObj["status"].toString();
       if (status == "error") {
           QString message("Job Not Found");
           if (theObj.contains("message"))
               message = theObj["message"].toString();
           qDebug() << "ERROR MESSAGE: " << message;
           emit this->sendErrorMessage(message);
           return result;
       } else if (status == "success") {
           if (theObj.contains("result")) {
               QJsonObject resObj = theObj["result"].toObject();
               if (resObj.contains("id"))
                   return resObj["id"].toString();
           }
           return result;
       }

   } else if (theObj.contains("fault")) {
       QJsonObject theFault = theObj["fault"].toObject();
       if (theFault.contains("message")) {
           QString message = theFault["message"].toString();
           emit this->sendErrorMessage(message);
       }
   }

    return result;


    // sucessfull submission, go get jobID
/*
    if (jsonObj.contains("id"))
        result = jsonObj["id"].toString();
    else {
        emit sendErrorMessage("ARROR: AGAVE FOLKS CHANGED API .. contact SimCenter");
        result = "ERROR: Agave FOLKS CHANGED API";
        return result;
    }
*/
    return result;
}

QString
AgaveCurl::startJob(const QJsonObject &theJob)
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
AgaveCurl::getJobList(const QString &matchingName)
{
  QJsonObject result;

  QString url = tenantURL + QString("jobs/v2");
  curl_easy_setopt(hnd, CURLOPT_URL, url.toStdString().c_str());
  this->invokeCurl();

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
  
  if ((val.contains("Missing Credentals")) || (val.contains("Invalid Credentals"))){
    emit sendErrorMessage("ERROR: Trouble LOGGING IN .. try Logout and Login Again");
    return result;
  }

  QJsonDocument doc = QJsonDocument::fromJson(val.toUtf8());
  QJsonObject theObj = doc.object();
  if (theObj.contains("result")){
      QJsonArray jobs = theObj["result"].toArray();
      result["jobs"] = jobs;
  }

  return result;
}

QJsonObject
AgaveCurl::getJobDetails(const QString &jobID)
{
  QJsonObject result;


  QString url = tenantURL + QString("jobs/v2/") + jobID;
  curl_easy_setopt(hnd, CURLOPT_URL, url.toStdString().c_str());
  if (this->invokeCurl() == false)
      return result;

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

  QJsonDocument doc = QJsonDocument::fromJson(val.toUtf8());
  QJsonObject theObj = doc.object();
  if (theObj.contains("result")){
      result = theObj["result"].toObject();
  }

  return result;
}

QString
AgaveCurl::getJobStatus(const QString &jobID){

    QString result("NOT FOUND");

    //
    // invoke curl
    //

    QString url = tenantURL + QString("jobs/v2/") + jobID + QString("/status");
    curl_easy_setopt(hnd, CURLOPT_URL, url.toStdString().c_str());
    if (this->invokeCurl() == false) {
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

    // read results file
    QString val;
    val=file.readAll();
    file.close();

     // read into json object
    QJsonDocument doc = QJsonDocument::fromJson(val.toUtf8());
    QJsonObject theObj = doc.object();

    // parse json object for status
    //   if error emit error message & return NOT FOUND
    //   if sucess then get result & return the job sttaus

    QString status;
    if (theObj.contains("status")) {
        status = theObj["status"].toString();
        if (status == "error") {
            QString message("Job Not Found");
            if (theObj.contains("message"))
                message = theObj["message"].toString();
            qDebug() << "ERROR MESSAGE: " << message;
            emit this->sendErrorMessage(message);
            return result;
        } else if (status == "success")

            if (theObj.contains("result")) {
                QJsonObject resultObj = theObj["result"].toObject();
                if (resultObj.contains("status"))
                    result = resultObj["status"].toString();
            }
    }

    return result;
}

bool
AgaveCurl::deleteJob(const QString &jobID)
{
    bool result = false;

    //
    // invoke curl
    //

    QString url = tenantURL + QString("jobs/v2/") + jobID;

    curl_easy_setopt(hnd, CURLOPT_URL, url.toStdString().c_str());
    curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "DELETE");

    if (this->invokeCurl() == false) {
        return result;
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

    // read into json object
    QString val;
    val=file.readAll();
    file.close();
    QJsonDocument doc = QJsonDocument::fromJson(val.toUtf8());
    QJsonObject theObj = doc.object();

    // parse json object for status
    //   if error emit error message & return NOT FOUND
    //   if sucess then get result & return the job sttaus

    QString status;
    if (theObj.contains("status")) {
        status = theObj["status"].toString();
        if (status == "error") {
            QString message("Unknown ERROR with Curl Call - deleteJob");
            if (theObj.contains("message"))
                message = theObj["message"].toString();
            qDebug() << "ERROR MESSAGE: " << message;
            emit this->sendErrorMessage(message);
            return false;
        } else if (status == "success")
            return true;
    }

    return result;
}



bool 
AgaveCurl::invokeCurl(void) {

  CURLcode ret;

  //
  // the methods set many of the options, this private method just sets the
  // default ones, invokes curl and then reset the handler for the next call
  // if an error occurs it gets the error messag and emits a signal before returning false
  //

  // set default options
  curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(hnd, CURLOPT_USERAGENT, "curl/7.54.0");
  if (slist1 != NULL) 
    curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, slist1);
  curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
  curl_easy_setopt(hnd, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
  curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYHOST, 0L);
  curl_easy_setopt(hnd, CURLOPT_TCP_KEEPALIVE, 1L);

  // we send the result of curl request to a file > uniqueFileName1
  FILE *pagefile = fopen(uniqueFileName1.toStdString().c_str(), "wb");
  if(pagefile) {
    curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(hnd, CURLOPT_WRITEDATA, pagefile);
  }
  
  // perform the curl operation that has been set up
  ret = curl_easy_perform(hnd);
  fclose(pagefile);

  // reset the handle so methods can fill in the different options before next call
  curl_easy_reset(hnd);

  // check for success
  if (ret == CURLE_OK)
    return true;

  // if failure, go get message, emit signal and return false;
  const char *str = curl_easy_strerror(ret);
  QString errorString(str);
  qDebug() << errorString;
  emit sendErrorMessage(errorString);

  return false;
}
