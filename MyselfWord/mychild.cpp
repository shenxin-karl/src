#include "mychild.h"
#include <QFileInfo>
#include <QCloseEvent>
#include <QFile>
#include <QTextCodec>
#include <QTextDocument>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QTextDocumentWriter>
#include <QMessageBox>
#include <QTextListFormat>
#include <QTextList>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QTextCursor>

MyChild::MyChild() : isUntitled(true)
{
    setAttribute(Qt::WA_DeleteOnClose);         /* 关闭窗口时, 释放内存 */

    /* 设置默认的初始字体 */
    QFont font(tr("微软雅黑"), 14);
    setFont(font);
    setAcceptDrops(true);

}


/* 新建文件 */
void MyChild::newFile()
{
    static int sequenceNumber = 1;              /* 因为需要一直保存文档的编号, 所以需要使用静态变量 */
    isUntitled = true;

    curFile = QString(tr("文档 %1").arg(sequenceNumber++));
    setWindowTitle(curFile + "[*]" + tr("- Myself Word"));

    connect(document(), &QTextDocument::contentsChanged, this, &MyChild::documentWasModified);
}


/* 提取文件名 */
QString MyChild::userFrinedlyCurrentFile()
{
    return strippedName(curFile);
}


/*加载文件
* 判断文件是否存在, 存在打开文件
* 然后利用 Qt::codeForHtml 将字节数组, 转换成 QTextCodec 类型
* 在将 将 QTextCodec 转换成 万国码 Uniconcode 给 QString
* 最后设置 setHtml
*/
bool MyChild::loadFile(const QString &fileName)
{
    if (fileName.isEmpty())
        return false;
    /* 文件不存在 */
    if (!QFile::exists(fileName))
        return false;

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly))
        return false;

    QByteArray data = file.readAll();
    QTextCodec *codec = Qt::codecForHtml(data);
    QString str = codec->toUnicode(data);

    /* 如果是 富文本 */
    if (Qt::mightBeRichText(str)) {
        this->setHtml(str);

    } else {
        str = QString::fromLocal8Bit(data);     /* 将编码转换成 系统默认的编码 */
        this->setPlainText(str);
    }

    /* 设置当前文件 */
    setCurrentFile(fileName);
    connect(document(), &QTextDocument::contentsChanged, this, &MyChild::documentWasModified);

    return true;
}


/* 设置当前文件 */
void MyChild::setCurrentFile(const QString &fileName)
{
    curFile = QFileInfo(fileName).canonicalFilePath();          /* 将相对路径的文件, 转换成绝对路径 */
    isUntitled = false;
    document()->setModified(false);         /* 设置文档没有被修改过 */
    setWindowModified(false);               /* 设置不显示被更改的标志 */
    setWindowTitle(userFrinedlyCurrentFile() + "[*]");
}


/* 保存文件的通用接口 */
bool MyChild::save()
{
    if (isUntitled)
        return saveAs();
    else
        return saveFile(curFile);
}


/* 文件另存为 */
bool MyChild::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("另存为"), curFile, tr("HTML 文档 (*.html *.htm);;所有文件 (*.*)"));
    if (fileName.isEmpty())
        return false;


    return saveFile(fileName);
}


/* 保存文件 */
bool MyChild::saveFile(QString fileName)
{
    /* 如果后缀不是 .htm 则增加后缀名 */
    if (!(fileName.endsWith(".htm", Qt::CaseInsensitive) || fileName.endsWith(".html", Qt::CaseInsensitive)))
        fileName += ".html";

    QTextDocumentWriter write(fileName);
    bool success = write.write(this->document());
    if (success) {
        setCurrentFile(fileName);
        return success;
    } else {
        QMessageBox::information(this, tr("提示"), tr("文件保存失败!!!"));
        return false;
    }
}


/* 关闭文档时, 如果文档修改了, 则提醒用户保存文件 */
bool MyChild::maybeSave()
{
    /* 文档未修改, 直接返回 */
    if (!document()->isModified())
        return true;

    QMessageBox::StandardButton ret;
    ret = QMessageBox::warning(this, tr("Myself Qt Word"), tr("文档 %1 以被修改是否保存?").arg(userFrinedlyCurrentFile()),
                               QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save)
        return save();
    else if (ret == QMessageBox::Cancel)
        return false;

    return true;
}


/* 调整选中的文本的格式 */
void MyChild::margeFormatOnWordSelection(QTextCharFormat const &format)
{
    QTextCursor cursor = this->textCursor();        /* 获取 QTextEdit 默认构造的 QTextCurSor 光标类 */

    /* 如果没有选择文本, 则自动选中光标下的单词 */
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);

    /* 将格式应用到选中的文本上 */
    cursor.mergeCharFormat(format);
    this->mergeCurrentCharFormat(format);
}


/* 设置对齐方式 */
void MyChild::setAlign(enum Align a)
{
    switch (a) {
    case Left:
        this->setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);
        break;
    case Center:
        this->setAlignment(Qt::AlignCenter);
        break;
    case Right:
        this->setAlignment(Qt::AlignRight | Qt::AlignAbsolute);
        break;
    case Justify:
        this->setAlignment(Qt::AlignJustify);
        break;
    }
}


/* 设置文本格式 */
void MyChild::setStyle(int style)
{
    MyChild::Style s = static_cast<enum Style>(style);
    QTextCursor cursor = textCursor();

    if (s != Normal) {
        QTextListFormat::Style stylename = QTextListFormat::ListDisc;
        switch (s) {
        default:
        case SolidCircle:
            stylename = QTextListFormat::ListDisc;          /* 实心圆 */
            break;
        case HollowCircle:
            stylename = QTextListFormat::ListCircle;        /* 空心圆 */
            break;
        case SolidSquare:
            stylename = QTextListFormat::ListSquare;        /* 正方形 */
            break;
        case Number:
            stylename = QTextListFormat::ListDecimal;       /* 数字 */
            break;
        case LowLetter:
            stylename = QTextListFormat::ListLowerAlpha;    /* 小写字母 */
            break;
        case UppLetter:
            stylename = QTextListFormat::ListUpperAlpha;    /* 大写字母 */
            break;
        case LowGreekNum:
            stylename = QTextListFormat::ListLowerRoman;    /* 小写罗马数字 */
            break;
        case UppGreekNum:
            stylename = QTextListFormat::ListUpperRoman;    /* 大写罗马数字 */
            break;
        }

        /* 为了支持撤销操作 */
        cursor.beginEditBlock();

        QTextBlockFormat blockFmt = cursor.blockFormat();
        QTextListFormat listFmt;

        if (cursor.currentList())
            listFmt = cursor.currentList()->format();
        else {
            listFmt.setIndent(blockFmt.indent() + 1);
            blockFmt.setIndent(0);
            cursor.setBlockFormat(blockFmt);
        }

        /* 设置样式 */
        listFmt.setStyle(stylename);
        cursor.createList(listFmt);
        cursor.endEditBlock();

    } else {
        QTextBlockFormat bfmt;
        bfmt.setObjectIndex(-1);
        cursor.mergeBlockFormat(bfmt);
    }
}




/* 关闭窗口事件 */
void MyChild::closeEvent(QCloseEvent *event)
{
    /* 提醒用户保存 */
    if (maybeSave())
        event->accept();        /* 关闭窗口 */
    else
        event->ignore();        /* 忽略事件 */
}


/* 更改状态 */
void MyChild::documentWasModified()
{
    setWindowModified(document()->isModified());
}


/* 插入表格 */
void MyChild::insertTable(int rows, int columns)
{
    QTextCursor cursor = textCursor();
    QTextTableFormat format;
    format.setCellPadding(10);
    format.setCellSpacing(2);
    cursor.insertTable(rows, columns, format);
}


/* 插入列表 */
void MyChild::insertList()
{
    QTextListFormat format;
    format.setStyle(QTextListFormat::ListDecimal);
    textCursor().insertList(format);
}


/* 插入图片 */
void MyChild::insertImage()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("插入的图片"), QString(), tr("图片 (*.png *.jpg *.bmp)"));

    if (!fileName.isEmpty()) {
        QTextImageFormat format;
        format.setName(fileName);

        textCursor().insertImage(format);
        append("\r\n");
    }

}


/* 获取较短的绝对路径 */
QString MyChild::strippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}
