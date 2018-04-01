#ifndef AGAVE_INTERFACE_H
#define AGAVE_INTERFACE_H

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


/** 
 *  @author  fmckenna
 *  @date    2/2017
 *  @version 1.0 
 *  
 *  @section DESCRIPTION
 *  
 *  This is an abstract interface for dealing with remote servers that offer
 *  a kind of software-as-a-service in which user can upload files, run remote
 *  applications on the server (or whatevr is behind it, e.g. HPC resources),
 *  monitor running applications, and obtain the results back once application 
 *  has run.
 */

#include <QObject>
#include <QString>
#include <QJsonObject>

class AgaveInterface : public QObject
{
        Q_OBJECT
public:

    /** 
     *   @brief Constructor
     */
    AgaveInterface(QObject *);

    /** 
     *   @brief Destructor
     */
    virtual ~AgaveInterface(){};

    // methods to login/logout

    /** 
     *   @brief  login method called to login user to remote service
     *   @return bool indicating login process commenced (login may not actually be finished)
     */  
    virtual bool login() =0;

    /** 
     *   @brief  login method called to logout user from remote service
     */  
    virtual bool logout() =0;

    /** 
     *   @brief  login method called to logout user from remote service
     *   @return bool indicating if user logged in or not
     */  
    virtual bool isLoggedIn() =0;


    // methods needed for file/dir operations .. return success or failure
    //   note: uploadFile() may be needed (uploadDir could compress local, send and uncompress)
    //   note: downloadDir() may be needed (downloadDir could compress remote, download and uncompress)

    /** 
     *   @brief uploadDirectory method to upload local directory to remote file system
     *  
     *   @param  local is string containing local directory path
     *   @param  remote is a string containing remote directory path
     *   @return bool indicating succcess or failure
     */  
    virtual bool uploadDirectory(const QString &local, const QString &remote) =0;


    /** 
     *   @brief removeDirectory method to remove remote directory
     *  
     *   @param  remote is a string containing remote directory path
     *   @return bool indicating succcess or failure
     */  
    virtual bool removeDirectory(const QString &remote) =0;


    /** 
     *   @brief downloadFile method to download a file
     *  
     *   @param  remote is a string containing full path of remote file, i.e. dir and filename
     *   @param  local is a string containing full path of local file
     *   @return bool indicating succcess or failure
     */  
    virtual bool downloadFile(const QString &remote, const QString &local) =0;

    /** 
     *   @brief getHomeDirPath method needed to obtain users remote home dir path
     *  
     *   @return string containing remote path
     */  
    virtual QString getHomeDirPath(void) =0;

    // methods needed to start a job .. returns jobID in a string

    /** 
     *   @brief startJob method to start a remote job
     *   @param jobDescriptionFilename string containing full ath to a file cobtaining the JSON job description
     *   @return string containing the unique ID of job that was started
     */  
    virtual QString startJob(const QString &jobDescription) =0;

    /** 
     *   @brief startJob method to start a remote job
     *   @param jobDescription a JSON object containing job description information
     *   @return string containing the unique ID of job that was started
     */  
    virtual QString startJob(const QJsonObject &theJob) =0;

    // methods neeed for dealing with remote jobs .. return JsonObject direct from Agave or obvious

    /** 
     *   @brief getJobList method to obtain list of running jobs whose name containg certian string, 
     *   all jobs if string empty 
     *   @param matchingName string used in search job name
     *   @return JSON object containing array of all jobs whose name contains the matching name
     */  
    virtual QJsonObject getJobList(const QString &matchingName) =0;

    /** 
     *   @brief getJobDetails method to return specific info about 1 particular job
     *   @param jobID string containing jobID of the job whose info is being requested
     *   @return JSON object containing jobs details
     */  
    virtual QJsonObject getJobDetails(const QString &jobID) =0;

    /** 
     *   @brief getJobStaus method to return specific info as t state of Job
     *   @param jobID string containing jobID of the job 
     *   @return string containing jobs states, e.g. FINISHED, RUNNING,
     */  
    virtual QString getJobStatus(const QString &jobID) =0;

    /** 
     *   @brief deleteJob method to remove a job from remote service. This will delete
     *   job and the files associated with it on the rmote server.
     *   @param jobID string containing jobID of the job 
     *   @return bool indicating success or failure of request
     */  
    virtual bool deleteJob(const QString &jobID) =0;

 signals:

    /**
     *   @brief sendFatalMessage signal to be emitted when object needs to shut program down
     *   @param message to be returned
     *   @return void
     */
    void sendFatalMessage(QString message);

    /**
     *   @brief sendErrorMessage signal to be emitted when object needs to communicate error with user
     *   @param message to be returned
     *   @return void
     */
    void sendErrorMessage(QString message);

private:

};


#endif // AGAVE_INTERFACE_H
