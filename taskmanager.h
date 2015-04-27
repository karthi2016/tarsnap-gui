#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include "tarsnapclient.h"
#include "persistentmodel/archive.h"
#include "persistentmodel/job.h"

#include <QObject>
#include <QThread>
#include <QUrl>
#include <QUuid>
#include <QMap>
#include <QSharedPointer>
#include <QDateTime>
#include <QThreadPool>
#include <QQueue>


#define CMD_TARSNAP         "tarsnap"
#define CMD_TARSNAPKEYGEN   "tarsnap-keygen"

struct ArchiveRestoreOptions
{
    bool        preservePaths;
    bool        overwriteFiles;
    bool        keepNewerFiles;
    QString     chdir;
};

enum TaskStatus { Queued, Running, Completed, Failed, Paused, Initialized };

class BackupTask: public QObject
{
    Q_OBJECT
public:
    BackupTask():_uuid(QUuid::createUuid()), _status(TaskStatus::Initialized){}

    QUuid uuid() const {return _uuid;}
    void setUuid(const QUuid &uuid) {_uuid = uuid;}

    QList<QUrl> urls() const {return _urls;}
    void setUrls(const QList<QUrl> &urls) {_urls = urls;}

    QString name() const {return _name;}
    void setName(const QString &name) {_name = name;}

    TaskStatus status() const {return _status;}
    void setStatus(const TaskStatus &status) {_status = status; emit statusUpdate();}

    int exitCode() const {return _exitCode;}
    void setExitCode(int exitCode) {_exitCode = exitCode;}

    QString output() const {return _output;}
    void setOutput(const QString &output) {_output = output;}

    ArchivePtr archive() const {return _archive;}
    void setArchive(const ArchivePtr &archive) {_archive = archive;}

    JobPtr job() const {return _job;}
    void setJob(const JobPtr &job) {_job = job;}

signals:
    void statusUpdate();

private:
    QUuid                 _uuid;
    QList<QUrl>           _urls;
    QString               _name;
    TaskStatus            _status;
    int                   _exitCode;
    QString               _output;
    ArchivePtr            _archive;
    JobPtr                _job;
};

typedef BackupTask* BackupTaskPtr;

class TaskManager : public QObject
{
    Q_OBJECT

public:
    explicit TaskManager(QObject *parent = 0);
    ~TaskManager();

signals:
    // Tarsnap task notifications
    void idle(bool status); // signal if we are working on tasks or not
    void registerMachineStatus(TaskStatus status, QString reason);
    void fsckStatus(TaskStatus status, QString reason);
    void nukeStatus(TaskStatus status, QString reason);
    void archiveList(QList<ArchivePtr> archives);
    void archivesDeleted(QList<ArchivePtr> archives);
    void overallStats(qint64 sizeTotal, qint64 sizeCompressed, qint64 sizeUniqueTotal
                      , qint64 sizeUniqueCompressed, qint64 archiveCount, qreal credit
                      , QString accountStatus);
    void restoreArchiveStatus(ArchivePtr archive, TaskStatus status, QString reason);

public slots:
    void loadSettings();

    // Tarsnap tasks
    void registerMachine(QString user, QString password, QString machine
                         ,QString key, QString tarsnapPath, QString cachePath);
    void backupNow(BackupTaskPtr backupTask);
    void getArchiveList();
    void getArchiveStats(ArchivePtr archive);
    void getArchiveContents(ArchivePtr archive);
    void deleteArchives(QList<ArchivePtr> archives);
    void getOverallStats();
    void fsck();
    void nuke();
    void restoreArchive(ArchivePtr archive, ArchiveRestoreOptions options);

private slots:
    // post Tarsnap task processing
    void backupTaskFinished(QUuid uuid, QVariant data, int exitCode, QString output);
    void backupTaskStarted(QUuid uuid);
    void registerMachineFinished(QUuid uuid, QVariant data, int exitCode, QString output);
    void getArchiveListFinished(QUuid uuid, QVariant data, int exitCode, QString output);
    void getArchiveStatsFinished(QUuid uuid, QVariant data, int exitCode, QString output);
    void getArchiveContentsFinished(QUuid uuid, QVariant data, int exitCode, QString output);
    void deleteArchivesFinished(QUuid uuid, QVariant data, int exitCode, QString output);
    void overallStatsFinished(QUuid uuid, QVariant data, int exitCode, QString output);
    void fsckFinished(QUuid uuid, QVariant data, int exitCode, QString output);
    void nukeFinished(QUuid uuid, QVariant data, int exitCode, QString output);
    void restoreArchiveFinished(QUuid uuid, QVariant data, int exitCode, QString output);

    // general task management
    void queueTask(TarsnapClient *cli, bool exclusive = false);
    void startTask(TarsnapClient *cli);
    void dequeueTask(QUuid uuid, QVariant data, int exitCode, QString output);

private:
    void parseArchiveStats(QString tarsnapOutput, bool newArchiveOutput, ArchivePtr archive);
    QString makeTarsnapCommand(QString cmd);

private:
    QString                      _tarsnapDir;
    QString                      _tarsnapCacheDir;
    QString                      _tarsnapKeyFile;
    QThread                      _managerThread; // manager runs on a separate thread
    QMap<QUuid, BackupTaskPtr>   _backupTaskMap; // keeps track of active backup tasks
    QMap<QUuid, ArchivePtr>      _archiveMap; // keeps track of archives encountered
    QMap<QUuid, TarsnapClient*>  _taskMap; // keeps track of currently executing client tasks
    QQueue<TarsnapClient*>       _taskQueue; // keeps track of mutually exclusive client tasks pending execution
    QThreadPool                 *_threadPool;
    bool                         _aggressiveNetworking;
    bool                         _preservePathnames;
};

#endif // TASKMANAGER_H
