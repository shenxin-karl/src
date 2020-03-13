#ifndef MYCHILD_H
#define MYCHILD_H

#include <QWidget>
#include <QTextEdit>

class MyChild : public QTextEdit
{
    Q_OBJECT
public:
    /* 对齐方式 */
    enum Align {
      Left,
      Right,
      Center,
      Justify
    };

    /* 文本样式 */
    enum Style {
        Normal,                 /* 正常 */
        SolidCircle,            /* 实心圆 */
        HollowCircle,           /* 空心圆 */
        SolidSquare,            /* 实心正方形 */
        Number,                 /* 数字 */
        LowLetter,              /* 小写字母 */
        UppLetter,              /* 大写字母 */
        LowGreekNum,            /* 小写希腊数字 */
        UppGreekNum             /* 大写希腊数字 */
    };

public:
    MyChild();
    void newFile();                                         /* 新建文件 */
    QString userFrinedlyCurrentFile();                      /* 提取文件名 */
    QString currentFile() { return curFile; }               /* 返回当前路径 */
    bool loadFile(QString const &fileName);                 /* 加载文件 */
    void setCurrentFile(QString const &fileName);           /* 设置当前文件名 */
    bool save();                                            /* 保存文件通用接口 */
    bool saveAs();                                          /* 另存为 */
    bool saveFile(QString fileName);                        /* 保存 */
    bool maybeSave();                                       /* 关闭文档时, 如果文档改动, 则提醒用户保存 */
    void margeFormatOnWordSelection(QTextCharFormat const &); /* 调整选择的文本格式 */
    void setAlign(enum Align a);                             /* 设置对齐方式 */
    void setStyle(int style);                               /* 设置文本标号 */
    void insertTable(int rows, int columns);                /* 插入表格 */
    void insertList();                                      /* 插入列表 */
    void insertImage();                                     /* 插入图片 */

protected:
    void closeEvent(QCloseEvent *event) override;           /* 关闭窗口事件 */


private slots:
    void documentWasModified();                             /* 问答被更改时, 显示状态 */

private:
    QString strippedName(const QString &fullFileName);      /* 获取较短的绝对路径 */

private:
    QString     curFile;        /* 保存当前文件路径 */
    bool        isUntitled;     /* 文件是否被保存过的标识 */
};

#endif // MYCHILD_H
