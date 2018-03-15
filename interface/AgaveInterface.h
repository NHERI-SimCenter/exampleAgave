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

// Written: fmckenna

// Purpose: The abstract interface with Agave. concrete alternatives to be provided.

#include <QString>
#include <QJsonObject>

class AgaveInterface
{
public:
    AgaveInterface(){};
    virtual ~AgaveInterface(){};

    // methhod to login
    virtual bool login(const QString &login, const QString &password) =0;

    // methods needed for file/dir operations .. return success or failure
    //   note: uploadFile() may be needed (uploadDir could compress local, send and uncompress)
    //   note: downloadDir() may be needed (downloadDir could compress remote, download and uncompress)
    virtual bool uploadDirectory(const QString &local, const QString &remote) =0;
    virtual bool removeDirectory(const QString &remote) =0;
    virtual bool downloadFile(const QString &remote, const QString &local) =0;

    // methods needed to start a job .. returns jobID in a string
    virtual QString startJob(const QString &jobDescription) =0;
    virtual QString startJob(const QJsonObject &theJob) =0;

    // methods neeed for dealing with remote jobs .. return JsonObject direct from Agave or obvious
    virtual QJsonObject getJobList(const QString &matchingName) =0;
    virtual QJsonObject getJobDetails(const QString &jobID) =0;
    virtual QString getJobStatus(const QString &jobID) =0;
    virtual bool deleteJob(const QString &jobID) =0;

private:

};

#endif // AGAVE_INTERFACE_H
