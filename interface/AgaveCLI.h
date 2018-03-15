#ifndef AGAVE_CLI_H
#define AGAVE_CLI_H

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

// Purpose: To interface with Agave through the command line interface routines. This
// requires that these routines be downloaded and installled on user desktop/laptop.

#include "AgaveInterface.h"

class QProcess;

class AgaveCLI : public AgaveInterface
{
public:
    AgaveCLI();
    ~AgaveCLI();

    // methhod to login
    bool login(const QString &login, const QString &password);

    // methods needed for file/dir operations
    bool uploadDirectory(const QString &local, const QString &remote);
    bool removeDirectory(const QString &remote);
    bool downloadFile(const QString &remote, const QString &local);

    // methods needed to start a job
    QString startJob(const QString &jobDescription);
    QString startJob(const QJsonObject &theJob);

    // methods neeed for dealing with remote jobs
    QJsonObject getJobList(const QString &matchingName);
    QJsonObject getJobDetails(const QString &jobID);
    QString getJobStatus(const QString &jobID);
    bool deleteJob(const QString &jobID);

private:
    bool invokeAgaveCLI(const QString & command);

   // int currentInt;
    QString uniqueFileName1;
    QString uniqueFileName2;
    QProcess *proc;
};

#endif // AGAVE_CLI_H
