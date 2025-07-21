#include "translateworker.h"
#include <QThread>
#include <QtConcurrent>
#include <QDebug>

TranslateWorker* TranslateWorker::m_Worker = nullptr;

TranslateWorker::TranslateWorker(QObject *parent) : QObject(parent)
{
    m_logTimer = new QTimer(this);
    m_logTimer->setInterval(200); // 200毫秒刷新一次
    connect(m_logTimer, &QTimer::timeout, this, &TranslateWorker::flushLogBuffer);
}

TranslateWorker::~TranslateWorker()
{
    if(m_Worker) {
        delete m_Worker;
        m_Worker = nullptr;
    }
}

TranslateWorker *TranslateWorker::instance()
{
    if(!m_Worker)
    {
        m_Worker = new TranslateWorker();
    }
    return m_Worker;
}

void TranslateWorker::registerQmlTypes()
{
    qmlRegisterSingletonType<TranslateWorker>(
            "MyTranslate", 1, 0, "MyTranslate",
            [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject * {
                Q_UNUSED(engine)
                Q_UNUSED(scriptEngine)
                return TranslateWorker::instance();
            }
        );

}

void TranslateWorker::executeAsync(WorkerTask task
    , const QString &inputFile
    , const QString &outputFile
    , const QString &targetLang)
{
    QtConcurrent::run([=]() {
        switch(task) {
        case TaskParseTsToXml:
            parseTsFilesToXml(inputFile);
            break;
        case TaskParseXmlToTs:
            parseXml(inputFile, outputFile, targetLang);
            break;
        case TaskExtractTsToQm:
            extractTsToQm(inputFile);
            break;
        case TaskGetLanguageOptions:
            getLanguageOptions(inputFile);
            break;
        default:
            break;
        }
    });
}

void TranslateWorker::parseTsToXml(const QString &tsFile)
{
    m_TransMapFromTs.clear();
    setLog(tsFile);
    //解析Ts文件
    QFile file(tsFile);
    if (!file.open(QIODevice::ReadOnly)) {
        setLog(QString("%1文件打开失败").arg(tsFile));
        return;
    }
    QString sourceText;
    QString translation;
    xml.setDevice(&file);

    while (!xml.atEnd() && !xml.hasError()) {
            QXmlStreamReader::TokenType token = xml.readNext();

            if (token == QXmlStreamReader::StartElement) {
                if (xml.name() == "source") {
                    sourceText = xml.readElementText();
                } else if (xml.name() == "translation") {
                    translation = xml.readElementText();

                    // 无条件存储，即使翻译为空
                    if (!sourceText.isEmpty()) {
                        m_TransMapFromTs[sourceText] = translation;
                        setLog(QString("读取ts,源文:%1->翻译:%2").arg(sourceText).arg(translation));
                    }
                    sourceText.clear();
                }
            }
        }

    if (xml.hasError()) {
        qWarning() << "解析错误:" << xml.errorString();
    }
    file.close();
    //写入Xml文件
    int iSuccessCount = 0;
    int iFailureCount = 0;
    int iTotalCount = 0;
    QXlsx::Document xlsx;
    xlsx.addSheet(tr("多语言"));
    // 设置表头（A1=源文，B1=language）
    xlsx.write(1, 1, "源文");
    xlsx.write(1, 2, "中文");  //目前先写死第二列为中文

    // 写入数据（A列=源文，B列=译文）
    int row = 2;  // 从第 2 行开始
    for (auto it = m_TransMapFromTs.begin(); it != m_TransMapFromTs.end(); ++it, ++row)
    {
        bool writeKey = xlsx.write(row, 1, it.key());    // A列：源文
        bool writeValue = xlsx.write(row, 2, it.value());  // B列：译文
        if (writeKey && writeValue)
        {
            iSuccessCount++;
        } else {
            iFailureCount++;
        }
        iTotalCount++;
    }    
    // 保存 Excel 文件
    QFileInfo fileInfo(tsFile);
    QString basePath = QDir(fileInfo.absolutePath()).filePath("translations");
    QString outputPath = basePath + ".xlsx";

    // 检查文件是否存在，如果存在则添加后缀
    int suffix = 1;
    QFile checkFile(outputPath);
    while (checkFile.exists()) {
        outputPath = QString("%1_%2.xlsx").arg(basePath).arg(suffix);
        checkFile.setFileName(outputPath);
        suffix++;
    }

    if (xlsx.saveAs(outputPath)) {
        setLog(QString("Excel 文件已保存到:%1").arg(outputPath));
        emit statsReceived(iTotalCount,iSuccessCount,iFailureCount);
    } else {
        setLog(QString("保存失败:%1").arg(outputPath));
        emit statsReceived(iTotalCount,iFailureCount,iSuccessCount);
    }
    return;
}

void TranslateWorker::parseTsFilesToXml(const QString &tsFilePath)
{
    m_TsFilesData.clear();
    m_languages.clear();
    setLog(QString("当前目录%1").arg(tsFilePath));
    //遍历目录，查找所有.ts文件
    QDir dir(tsFilePath);
    if (!dir.exists()) {
        setLog(QString("目录不存在:%1").arg(tsFilePath));
        return;
    }
    QStringList tsFiles = dir.entryList(QStringList() << "*.ts", QDir::Files);
    if (tsFiles.isEmpty()) {
        setLog(QString("目录中没有找到.ts文件:%1").arg(tsFilePath));
        return;
    }
    // 先处理中文ts文件，zn.ts文件作为基准
    bool bFoundZn = false;
    foreach (const QString &file, tsFiles) {
        if (file.contains("zn.ts")) {
            parseTsFile(dir.filePath(file), "zn");
            m_languages.append("zn");
            bFoundZn = true;
            break;
        }
    }
    if (!bFoundZn) {
        //不存在则结束
        setLog(QString("%1目录下不存在 zn.ts 文件").arg(tsFilePath));
        return;
    }
    // 处理其他语言文件
    foreach (const QString &file, tsFiles) {
        if (file == "zn.ts") continue;

        QString langCode = extractLangCode(file);
        parseTsFile(dir.filePath(file), langCode);
        m_languages.append(langCode);
    }
    extractFilesToXml(dir.filePath("translations.xlsx"));
    return;
}

void TranslateWorker::parseTsFile(const QString &tsFile, const QString &targetLang)
{
    setLog(QString("正在解析文件: %1").arg(tsFile));
    QFile file(tsFile);
    if (!file.open(QIODevice::ReadOnly)) {
        setLog(QString("%1文件打开失败").arg(tsFile));
        return;
    }

    QXmlStreamReader xml(&file);
    QString sourceText;

    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement) {
            if (xml.name() == "source") {
                sourceText = xml.readElementText();
            } else if (xml.name() == "translation") {
                QString translation = xml.readElementText();
                if (!sourceText.isEmpty()) {
                    // 如果是中文（zn.ts），直接存储
                    if (targetLang == "zn")
                    {
                        m_TsFilesData[sourceText]["zn"] = translation;
                    } else if (m_TsFilesData.contains(sourceText))
                    {
                        // 如果是其他语言，仅存储匹配zn.ts源文的翻译
                        m_TsFilesData[sourceText][targetLang] = translation;
                    }
                    setLog(QString("读取ts,语言:%1,源文:%2->翻译:%3")
                         .arg(targetLang).arg(sourceText).arg(translation));
                }
                sourceText.clear();
            }
        }
    }

    if (xml.hasError()) {
        qWarning() << "解析错误:" << xml.errorString();
    }
    file.close();
    return;
}

void TranslateWorker::extractFilesToXml(const QString &outputPath)
{
    int iSuccessCount = 0;
    int iFailureCount = 0;
    int iTotalCount = m_TsFilesData.size();;
    QXlsx::Document xlsx;
    xlsx.addSheet(tr("多语言"));

    //写入表头
    xlsx.write(1, 1, "源文");
    for (int col = 0; col < m_languages.size(); ++col) {
        xlsx.write(1, col + 2, m_languages[col]);
    }

    // 写入数据
    int row = 2; //从第2行开始
    for (auto it = m_TsFilesData.begin(); it != m_TsFilesData.end(); ++it, ++row) {
        bool writeSuccess = true;

        // 写入源文
        writeSuccess &= xlsx.write(row, 1, it.key());

        // 写入各语言翻译
        for (int col = 0; col < m_languages.size(); ++col) {
            const QString &lang = m_languages[col];
            QString translation = it.value().value(lang, ""); // 找不到翻译则为空
            writeSuccess &= xlsx.write(row, col + 2, translation);
        }

        if (writeSuccess) {
            iSuccessCount++;
        } else {
            iFailureCount++;
        }
    }

    // 处理文件名冲突
    QFileInfo fileInfo(outputPath);
    QString basePath = QDir(fileInfo.absolutePath()).filePath("translations");
    QString finalPath = basePath + ".xlsx";

    // 检查文件是否存在，如果存在则添加后缀
    int suffix = 1;
    QFile checkFile(finalPath);
    while (checkFile.exists()) {
        finalPath = QString("%1_%2.xlsx").arg(basePath).arg(suffix);
        checkFile.setFileName(finalPath);
        suffix++;
    }
    if (xlsx.saveAs(finalPath)) {
        setLog(QString("Excel 文件已保存到: %1").arg(finalPath));
        emit statsReceived(iTotalCount, iSuccessCount, iFailureCount);
    } else {
        setLog(QString("保存失败: %1").arg(finalPath));
        emit statsReceived(iTotalCount, iFailureCount, iSuccessCount);
    }
    return;
}

void TranslateWorker::parseXml(const QString &xmlFile
                               , const QString &tsFile
                               , const QString &targetLang)
{
    m_TransMapFromXml.clear();
    //从Excel读取翻译数据
    QXlsx::Document xlsx(xmlFile);
    if (xlsx.sheetNames().size() == 0) {
        setLog(QString("无法打开Excel文件:%1").arg(xmlFile));
        return;
    }
    xlsx.selectSheet(xlsx.sheetNames().first());
    // 查找目标语言列
    int targetCol = -1;
    int lastCol = xlsx.dimension().lastColumn();
    int lastRow = xlsx.dimension().lastRow();

    for (int col = 1; col <= lastCol; ++col) {
        if (xlsx.read(1, col).toString() == targetLang) {
            targetCol = col;
            break;
        }
    }

    if (targetCol == -1) {
        setLog(QString("未找到语言:%1").arg(targetLang));
        return;
    }

    //读取翻译数据
    for (int row = 2; row <= lastRow; ++row) {
        QString sourceText = xlsx.read(row, 1).toString();
        QString translation = xlsx.read(row, targetCol).toString();
        if (!sourceText.isEmpty()) {
            m_TransMapFromXml[sourceText] = translation;
        }
    }
    extractXmlToTs(tsFile);
    return;
}

void TranslateWorker::extractXmlToTs(const QString &tsFile)
{
    QFile file(tsFile);
    if (!file.open(QFile::ReadOnly)) {
        qDebug() << "Failed to open TS file:" << tsFile;
        setLog(QString("Failed to open TS file:%1").arg(tsFile));
        return;
    }
    for (auto it = m_TransMapFromXml.constBegin();
         it != m_TransMapFromXml.constEnd(); ++it)
    {
        setLog(QString("解析Xml,源文:%1 -> 翻译:%2").arg(it.key()).arg(it.value()));
    }
    setLog(QString("总共处理:%1").arg(m_TransMapFromXml.size()));
    QDomDocument doc;
    if (!doc.setContent(&file)) {
        setLog("Invalid XML content in TS file");
        file.close();
        return;
    }
    file.close();

    //更新翻译内容
    int iSuccessCount = 0;
    int iFailureCount = 0;
    int iTotalCount = 0;
    QDomElement root = doc.documentElement();
    QDomNodeList contextList = root.elementsByTagName("context");

    for (int i = 0; i < contextList.size(); ++i) {
        QDomNode contextNode = contextList.at(i);
        QDomNodeList messageList = contextNode.toElement().elementsByTagName("message");

        for (int j = 0; j < messageList.size(); ++j) {
            QDomNode messageNode = messageList.at(j);
            QDomNode sourceNode = messageNode.toElement().elementsByTagName("source").at(0);
            QDomNode translationNode = messageNode.toElement().elementsByTagName("translation").at(0);

            if (!sourceNode.isNull() && !translationNode.isNull()) {
                QString sourceText = sourceNode.toElement().text();

                if (m_TransMapFromXml.contains(sourceText)) {
                    QString newTranslation = m_TransMapFromXml.value(sourceText);
                    QDomElement translationElem = translationNode.toElement();

                    // 移除unfinished属性（如果存在）
                    if (translationElem.hasAttribute("type") &&
                        translationElem.attribute("type") == "unfinished") {
                        translationElem.removeAttribute("type");
                    }

                    // 移除所有子节点（包括文本节点）
                    while (translationElem.hasChildNodes()) {
                        translationElem.removeChild(translationElem.firstChild());
                    }

                    // 添加新的翻译文本
                    translationElem.appendChild(doc.createTextNode(newTranslation));
                    iSuccessCount++;
                } else{
                    iFailureCount++;
                }
                iTotalCount++;
            }
        }
    }

    //写回文件
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        QFileInfo fileInfo(file);
        setLog(QString("Failed to write TS file:%1.Error:%2,Permissions:%3")
               .arg(tsFile)
               .arg(file.errorString())
               .arg(fileInfo.isWritable()));

        qDebug() << "Is writable:" << fileInfo.isWritable();
        qDebug() << "Permissions:" << fileInfo.permissions();
        emit statsReceived(iTotalCount,iFailureCount,iSuccessCount);
        return;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    doc.save(out, 4); // 保持4空格缩进
    file.close();
    emit statsReceived(iTotalCount,iSuccessCount,iFailureCount);
    setLog(QString("TS file updated successfully. Updated%1").arg(m_TransMapFromXml.size()));
}

void TranslateWorker::extractTsToQm(const QString &tsFilePath)
{
    //获取lrelease工具路径
    QString lreleasePath = QLibraryInfo::location(QLibraryInfo::BinariesPath) + "/lrelease";
    #ifdef Q_OS_WIN
        lreleasePath += ".exe";
    #endif

    // 检查lrelease是否存在
    if (!QFile::exists(lreleasePath)) {
        setLog(QString("lrelease工具未找到，路径:%1").arg(lreleasePath));
        return;
    }

    //遍历目录，查找所有.ts文件
    QDir dir(tsFilePath);
    if (!dir.exists()) {
        setLog(QString("目录不存在:%1").arg(tsFilePath));
        return;
    }

    QStringList tsFiles = dir.entryList(QStringList() << "*.ts", QDir::Files);
    if (tsFiles.isEmpty()) {
        setLog(QString("目录中没有找到.ts文件:%1").arg(tsFilePath));
        return;
    }
    int iSuccessCount = 0;
    int iFailureCount = 0;
    //逐个转换文件
    foreach (const QString &tsFileName, tsFiles) {
        QString tsFilePath = dir.absoluteFilePath(tsFileName);
        QString qmFilePath = tsFilePath;
        qmFilePath.replace(".ts", ".qm");

        QProcess process;
        process.setProgram(lreleasePath);
        QStringList arguments;
        arguments << tsFilePath << "-qm" << qmFilePath;
        process.setArguments(arguments);
        setLog(QString("正在转换..."));

        process.start();
        if (!process.waitForFinished(30000)) { // 30秒超时
            iFailureCount++;
            qDebug() << "转换超时:" << tsFilePath;
            continue;
        }

        if (process.exitCode() != 0) {
            iFailureCount++;
            setLog(QString("转换失败:%1,错误:%2")
                   .arg(tsFilePath)
                   .arg(QString::fromUtf8(process.readAllStandardError())));
            continue;
        }
        iSuccessCount++;
        setLog(QString("转换成功...%1").arg(qmFilePath));
    }
    emit statsReceived(tsFiles.length(),iSuccessCount,iFailureCount);
    return;
}

QStringList TranslateWorker::getLanguageOptions(const QString &xmlFile)
{
    QStringList languages;

    // 打开Excel文件
    QXlsx::Document xlsx(xmlFile);
    if (xlsx.sheetNames().size() == 0) {
        qDebug() << "无法打开Excel文件:" << xmlFile;
        return languages;
    }

    xlsx.selectSheet(xlsx.sheetNames().first());

    // 获取第一行的所有列（从第二列开始）
    int lastCol = xlsx.dimension().lastColumn();
    // 第一列是"源文"，所以从第二列开始读取语言
    for (int col = 2; col <= lastCol; ++col) {
        QString language = xlsx.read(1, col).toString();
        qDebug() << "language:" << language;
        if (!language.isEmpty()) {
            languages.append(language);
        }
    }
    qDebug() << "Available languages:" << languages;
    return languages;
}

void TranslateWorker::setLog(const QString &message)
{
    //后面可做功能扩增
    QMutexLocker locker(&m_Mutex);
    QMetaObject::invokeMethod(this, "bufferLog",
                            Qt::QueuedConnection,
                            Q_ARG(QString, message));
}

void TranslateWorker::bufferLog(const QString &message)
{
    m_logBuffer.append(message);

    if (!m_logTimer->isActive()) {
        m_logTimer->start();
    }

    if (m_logBuffer.size() >= 200) {
        flushLogBuffer();
    }
}

void TranslateWorker::flushLogBuffer()
{
    QMutexLocker locker(&m_Mutex);
    if (m_logBuffer.isEmpty()) {
            m_logTimer->stop();
            return;
        }
        emit logReceived(m_logBuffer.join("\n"));
        m_logBuffer.clear();
}

QString TranslateWorker::extractLangCode(const QString &fileName)
{
    if (fileName.endsWith(".ts", Qt::CaseInsensitive))
    {
        return fileName.left(fileName.length() - 3);
    }
    return fileName;
}
