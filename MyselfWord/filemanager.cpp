#include "filemanager.h"
#include <QFile>
#include <QFileInfo>
#include <QDebug>


char const * const filePath = "./fileData";
FileInfo FileManager::fileData[10];


FileManager::FileManager() : index(), size()
{
    /* 将文件的内容读取 数组中 */
    QFile file(filePath);
    if (file.open(QFile::ReadOnly)) {
        file.read(reinterpret_cast<char*>(&size), sizeof(int));
        index = (size == 0 ? 0 : size - 1);
        for (int i = 0; i < size; ++i)
            file.read(reinterpret_cast<char*>(&fileData[i]), sizeof(FileInfo));     
    }
}

FileManager::~FileManager()
{
    if (isSave)
        return;

    writeFile();
}


/* 将数组信息保存到文件中 */
void FileManager::writeFile()
{
    if (size == 0)
        return ;

    QFile file(filePath);
    if (!file.open(QFile::WriteOnly))
        return;

    int ix = index;
    file.write(reinterpret_cast<char*>(&size), sizeof(int));
    for (int i = 0; i < size; ++i) {
        if (ix < 0)
           ix = RECORDSIZE - 1;

        file.write(reinterpret_cast<char*>(&fileData[ix]), sizeof(FileInfo));
        isSave = true;
    }
}


/* 保存文件信息 */
void FileManager::addItem(const QString &path)
{
    QFileInfo info(path);

    int ix = index;
    /* 如果文件中存在重复名的记录, 则更新路径 */
    for (int i = 0; i < RECORDSIZE; ++i, --ix) {
        if (ix < 0)
            ix = RECORDSIZE - 1;

        if (fileData[ix].fileName == info.fileName()) {
            qDebug() << __LINE__ << "找到重复记录";
            strncpy(fileData->filePath, info.absoluteFilePath().toUtf8().data(), sizeof(fileData->filePath));
            return;
        }
    }

    /* 添加到文件信息数组中 */
    FileInfo data(info.fileName(), info.absoluteFilePath());
    fileData[++index] = data;
    isSave = false;

    size = size < 10 ? size + 1 : 10;
}


/* 获取所有的文件名 */
QList<QString> FileManager::getAllFileName()
{
   QList<QString> nameList;
   int ix = index;
   for (int i = 0; i < size; ++i, --ix) {
       if (ix < 0)
           ix = RECORDSIZE - 1;

       QString name(fileData[ix].fileName);
       nameList << name;
   }

   return nameList;
}


/* 根据文件名查找对应的文件路径 */
QString FileManager::getFilePath(QString const &fileName)
{
    int ix = index;
    for (int i = 0; i < size; ++i, --ix) {
        if (ix < 0)
            ix = RECORDSIZE - 1;

        if (fileName == fileData[ix].fileName) {
            qDebug() << __FILE__ << __LINE__ << fileData[ix].filePath;
            return fileData[ix].filePath;
        }
    }

    return QString();
}




