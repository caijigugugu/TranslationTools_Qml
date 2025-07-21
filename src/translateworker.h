#ifndef TRANSLATEWORKER_H
#define TRANSLATEWORKER_H

#include <QObject>
#include <QQmlEngine>
#include <QFile>
#include <QtXml/QtXml>
#include <QtXlsx/QtXlsx>
#include <QMutex>

#define TranslateWorkerInst TranslateWorker::instance()
class TranslateWorker : public QObject
{
    Q_OBJECT
public:
    enum WorkerTask {
        TaskParseTsToXml,
        TaskParseXmlToTs,
        TaskExtractTsToQm,
        TaskGetLanguageOptions
    };
    Q_ENUM(WorkerTask)
    explicit TranslateWorker(QObject *parent = nullptr);
    ~TranslateWorker();
    static TranslateWorker* instance();
    void registerQmlTypes();
    //多线程调用
    Q_INVOKABLE void executeAsync(WorkerTask task
    , const QString &inputFile
    , const QString &outputFile = ""
    , const QString &targetLang = "");

    //解析ts文件到xml文件
    Q_INVOKABLE void parseTsToXml(const QString &tsFile);
    //解析ts文件目录
    Q_INVOKABLE void parseTsFilesToXml(const QString &tsFilePath);
    //解析ts文件
    Q_INVOKABLE void parseTsFile(const QString &tsFile, const QString &targetLang);
    //提取ts多文件词条到xml文件
    Q_INVOKABLE void extractFilesToXml(const QString &outputPath);
    //解析xml文件
    Q_INVOKABLE void parseXml(const QString &xmlFile
                              , const QString &tsFile
                              , const QString &targetLang);

    //提取xml文件中内容到ts文件中
    Q_INVOKABLE void extractXmlToTs(const QString &tsFile);

    //ts->qm
    Q_INVOKABLE void extractTsToQm(const QString &tsFilePath);

    //解析xml文档已提供哪些语言翻译
    Q_INVOKABLE QStringList getLanguageOptions(const QString &xmlFile);

    //向前端UI发送日志
    Q_INVOKABLE void setLog(const QString &message);

    void flushLogBuffer();

    //提取ts文件名称，如zh_CN.ts->zh_CN
    QString extractLangCode(const QString &fileName);

signals:
    void logReceived(const QString &message);
    void statsReceived(int total, int succeed, int failed);

public slots:
    void bufferLog(const QString &message);

private:
    static TranslateWorker* m_Worker;
    QMutex                  m_Mutex;
    QStringList m_logBuffer;
    QTimer*                 m_logTimer;
    QXmlStreamReader        xml;
    QStringList m_languages; //记录语言顺序
    QMap<QString, QString>  m_TransMapFromTs; //单ts文件词条
    QMap<QString, QString>  m_TransMapFromXml; //xml词条
    QMap<QString, QMap<QString, QString>> m_TsFilesData; //多ts文件词条
};

#endif // TRANSLATEWORKER_H
