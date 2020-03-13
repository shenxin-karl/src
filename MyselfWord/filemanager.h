#ifndef FILEMANAGER_H
#define FILEMANAGER_H


#include <QObject>
#include <cstring>
#include <QList>

constexpr int RECORDSIZE = 10;

struct FileInfo {
    char fileName[64];
    char filePath[128];
public:
    FileInfo() = default;

    FileInfo(QString name, QString path)
    {
        strncpy(fileName, name.toUtf8().data(), sizeof(fileName));
        strncpy(filePath, path.toUtf8().data(), sizeof(filePath));
    }

    FileInfo &operator= (FileInfo const &other)
    {
        if (this == &other)
            return *this;

        strncpy(fileName, other.fileName, sizeof(fileName));
        strncpy(filePath, other.filePath, sizeof(filePath));

        return *this;
    }
};


class FileManager
{
public:
    FileManager();
    ~FileManager();

    void writeFile();                           /* 写入文件中 */
    void addItem(QString const &path);          /* 添加文件到数组中 */
    QList<QString> getAllFileName();            /* 获得所有的名字 */
    QString getFilePath(QString const &fileName);     /* 获得文件的路径 */

private:
    int index;
    int size;
    bool isSave;

    static FileInfo fileData[RECORDSIZE];
};

#endif // FILEMANAGER_H
