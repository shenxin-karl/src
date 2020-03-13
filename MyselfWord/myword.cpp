#include "myword.h"
#include "mychild.h"
#include "filemanager.h"
#include <QMdiArea>
#include <QAction>
#include <QMenu>
#include <QIcon>
#include <QMenuBar>
#include <QDebug>
#include <QtWidgets>
#include <QFont>
#include <QActionGroup>
#include <QPixmap>
#include <QToolBar>
#include <QComboBox>
#include <QFontComboBox>
#include <QSignalMapper>
#include <QList>
#include <QDebug>
#include <QCloseEvent>
#include <QFileDialog>
#include <QSpinBox>


MyWord::MyWord(QWidget *parent)
    : QMainWindow(parent), fileManager(new FileManager), mdiArea(new QMdiArea)
{
    /* 创建信号发射器, 进行信号映射 */
    windowMapper = new QSignalMapper(this);

    /* 进行槽连接, 将 pmapped 发送的信号的参数, 作为 QMdiArea::setActiveSubWindow 的参数 */
    void (QSignalMapper::*pMapped)(QWidget*) = &QSignalMapper::mapped;
    connect(windowMapper, pMapped, this, [this](QWidget *active){
        QMdiSubWindow *changeToWindow = qobject_cast<QMdiSubWindow*>(active);
        mdiArea->setActiveSubWindow(changeToWindow);
    });


    /* 初始化 */
    setWindowIcon(QIcon(":/Image/Word.png"));
    mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);       /* 横向滚动 */
    mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);         /* 垂直滚动 */
    setCentralWidget(mdiArea);
    move(200, 150);
    resize(800, 500);
    setWindowTitle(tr("Myself Word"));

    /* 创建动作 和 菜单 */
    createActions();
    createMenus();
    createToolBars();
    createStatusBar();

    /* 如果切换活动文档, 就更新菜单 */
    connect(mdiArea, &QMdiArea::subWindowActivated, this, &MyWord::updateMenus);
    updateMenus();

}

MyWord::~MyWord()
{

}

/* 动作组槽函数 */
void MyWord::textAlign(QAction *act)
{
    if (activeMyChild()) {
        if (act == leftAlignAct)
            activeMyChild()->setAlign(MyChild::Left);

        else if (act == centerAct)
            activeMyChild()->setAlign(MyChild::Center);

        else if (act == rightAlignAct)
            activeMyChild()->setAlign(MyChild::Right);

        else
            activeMyChild()->setAlign(MyChild::Justify);
    }
}


/* 新建文件 */
void MyWord::fileNew()
{
    MyChild *child = createMyChild();
    child->newFile();
    child->show();
    enabledText();
    connect(child, &QTextEdit::selectionChanged, this, &MyWord::updateMenus);
}


/* 新建子窗口 */
MyChild *MyWord::createMyChild()
{
    MyChild *child = new MyChild;
    mdiArea->addSubWindow(child);

    /* 当文本被选择中是, 启动 剪切 和 拷贝按钮 */
    void (MyChild::*pCopyAvailable)(bool) = &MyChild::copyAvailable;
    connect(child, pCopyAvailable, cutAct, &QAction::setEnabled);
    connect(child, pCopyAvailable, copyAct, &QAction::setEnabled);

    return child;
}


/* 更新工具栏 */
void MyWord::updateMenus()
{
    bool hasMyChild = (activeMyChild() != nullptr);
    /* 如果没有活动窗口, 以下功能统统关闭 */
    saveAct->setEnabled(hasMyChild);
    saveAsAct->setEnabled(hasMyChild);
    printAct->setEnabled(hasMyChild);
    printPreviewAct->setEnabled(hasMyChild);
    closeAct->setEnabled(hasMyChild);
    closeAllAct->setEnabled(hasMyChild);
    tileAct->setEnabled(hasMyChild);
    cascadeAct->setEnabled(hasMyChild);
    nextAct->setEnabled(hasMyChild);
    previousAct->setEnabled(hasMyChild);
    /* 分隔条不显示 */
    separatorAct->setVisible(hasMyChild);

    /* 有活动窗口, 并且文本内容被选中 */
    bool hasSelect = (activeMyChild() && activeMyChild()->textCursor().hasSelection());

    /* 启用对应的按钮 */
    cutAct->setEnabled(hasSelect);
    copyAct->setEnabled(hasSelect);

    boldAct->setEnabled(hasSelect);
    italicAct->setEnabled(hasSelect);
    underlineAct->setEnabled(hasSelect);
    leftAlignAct->setEnabled(hasSelect);
    centerAct->setEnabled(hasSelect);
    rightAlignAct->setEnabled(hasSelect);
    justifyAct->setEnabled(hasSelect);
    colorAct->setEnabled(hasSelect);
}


/* 返回活动的子窗口指针*/
MyChild *MyWord::activeMyChild()
{
    QMdiSubWindow *activeSubWindow = nullptr;
    if ((activeSubWindow = mdiArea->activeSubWindow()) != nullptr) {
        return qobject_cast<MyChild *>(activeSubWindow->widget());
    }

    return nullptr;
}


/* 更新窗口菜单 */
void MyWord::updateWindowMenu()
{
    /* 首先清空菜单, 然后重新添加动作 */
    windowMenu->clear();
    windowMenu->addAction(closeAct);
    windowMenu->addAction(closeAllAct);
    windowMenu->addSeparator();
    windowMenu->addAction(tileAct);
    windowMenu->addAction(cascadeAct);
    windowMenu->addSeparator();
    windowMenu->addAction(nextAct);
    windowMenu->addAction(previousAct);
    windowMenu->addAction(separatorAct);

    /* 获得所有的窗口 */
    QList<QMdiSubWindow*> windows = mdiArea->subWindowList();
    if (!windows.isEmpty())
        windowMenu->addSeparator();

    /* 将所有的窗口添加到 窗口列表中 */
    for (int i = 0; i < windows.size(); ++i) {
        MyChild *child = qobject_cast<MyChild*>(windows.at(i)->widget());
        QString text;

        /* 前 9 个文档添加伙伴键 */
        if (i < 9) {
            text = tr("&%1 %2").arg(i + 1).arg(child->userFrinedlyCurrentFile());
        } else {
            text = tr("%1 %2").arg(i + 1).arg(child->userFrinedlyCurrentFile());
        }

        /* 添加到窗口列表中, 并且设置可以选择 */
        QAction *action = windowMenu->addAction(text);
        action->setCheckable(true);

        /* 将当前窗口设置为选中状态 */
        action->setChecked(child == activeMyChild());

        /* 关联动作的信号 到 信号映射器的 map 槽, 这个槽会发送 mapped 信号 */
        void (QSignalMapper::*pMap)() = &QSignalMapper::map;
        connect(action, &QAction::triggered, windowMapper, pMap);

        /* 将动作与相应的窗口部件进行映射, 在发送 mapped 时, 就会以这个 action 作为参数 */
        windowMapper->setMapping(action, windows.at(i));
    }
    enabledText();
}


/* 加载一个已经存在的文件 */
void MyWord::fileOpen()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("选择打开文件"), QString(), tr("HTML 文档 (*.htm *.html *.txt);;所有文件 (*.*)"));
    if (fileName.isEmpty())
        return;

    /* 如果文件已经打开了, 就设置文件为活动窗口 */
    QMdiSubWindow *existing = findMyChild(fileName);
    if (existing) {
        mdiArea->setActiveSubWindow(existing);
        return;
    }

    MyChild *child = createMyChild();
    /* 打开文件成功 */
    if (child->loadFile(fileName)) {
        statusBar()->showMessage(tr("文件已经载入"));
        fileManager->addItem(fileName);
        child->show();
        connect(child, &QTextEdit::selectionChanged, this, &MyWord::updateMenus);
    } else
        child->close();
}


/* 保存文件 */
void MyWord::fileSave()
{
    if (activeMyChild()) {
       QString fileName = activeMyChild()->currentFile();
       activeMyChild()->save();
       statusBar()->showMessage(tr("保存成功! ^_^"), 2000);
       fileManager->addItem(fileName);
       updateRecentlyOpenMenu();
    }
}


/* 另存为文件 */
void MyWord::fileSaveAs()
{
    if (activeMyChild()) {
        QString fileName = activeMyChild()->currentFile();
        activeMyChild()->saveAs();
        statusBar()->showMessage(tr("保存成功! ^_^"), 2000);
        fileManager->addItem(fileName);
        updateRecentlyOpenMenu();
    }
}


/* 文本撤销 */
void MyWord::undo()
{
    if (activeMyChild())
        activeMyChild()->undo();
}


/* 文本重做 */
void MyWord::redo()
{
    if (activeMyChild())
        activeMyChild()->redo();
}


/* 文本剪切 */
void MyWord::cut()
{
    if (activeMyChild())
        activeMyChild()->cut();
}


/* 文本复制 */
void MyWord::copy()
{
    if (activeMyChild())
        activeMyChild()->copy();
}


/* 文本粘贴 */
void MyWord::paste()
{
    if (activeMyChild())
        activeMyChild()->paste();
}


/* 文本加粗 */
void MyWord::textBold()
{
    QTextCharFormat fmt;
    fmt.setFontWeight(boldAct->isChecked() ? QFont::Bold : QFont::Normal);
    if (activeMyChild())
        activeMyChild()->mergeCurrentCharFormat(fmt);
}


/* 文本斜体 */
void MyWord::textItalic()
{
    QTextCharFormat fmt;
    fmt.setFontItalic(italicAct->isChecked());
    if (activeMyChild())
        activeMyChild()->margeFormatOnWordSelection(fmt);
}


/* 下划线 */
void MyWord::textUnderline()
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(underlineAct->isChecked());
    if (activeMyChild())
        activeMyChild()->margeFormatOnWordSelection(fmt);
}


/* 设置字体类型 */
void MyWord::textFamily(QString const &famliy)
{
    QTextCharFormat fmt;
    fmt.setFontFamily(famliy);
    if (activeMyChild())
        activeMyChild()->mergeCurrentCharFormat(fmt);
}


/* 设置字体大小 */
void MyWord::textSize(QString const &size)
{
    qreal pointSize = size.toDouble();
    if (pointSize > 0) {
        QTextCharFormat fmt;
        fmt.setFontPointSize(pointSize);
        if (activeMyChild())
            activeMyChild()->mergeCurrentCharFormat(fmt);
    }
}


/* 设置文字颜色 */
void MyWord::textColor()
{
    if (!activeMyChild())
        return;

    QColor col = QColorDialog::getColor(activeMyChild()->textColor(), this);
    if (col.isValid()) {
        QTextCharFormat fmt;
        fmt.setForeground(col);
        activeMyChild()->mergeCurrentCharFormat(fmt);
    }
}


/* 文本样式 */
void MyWord::textStyle(int styleIndex)
{
    if (activeMyChild())
        activeMyChild()->setStyle(styleIndex);
}


/* 打印文件 */
void MyWord::filePrint()
{
    QPrinter printer(QPrinter::HighResolution);                         /* 打印机的分辨率 */
    QPrintDialog *dlg = new QPrintDialog(&printer, this);
    if (activeMyChild())
        dlg->setOption(QAbstractPrintDialog::PrintSelection, true);     /* 设置打印的内容 */

    dlg->setWindowTitle(tr("打印文档"));

    if (dlg->exec() == QDialog::Accepted)
        activeMyChild()->print(&printer);

    delete dlg;
}


/* 打印文件预览 */
void MyWord::filePrintPreview()
{
    QPrinter printer(QPrinter::HighResolution);
    QPrintPreviewDialog preview(&printer, this);
    connect(&preview, &QPrintPreviewDialog::paintRequested, this, [this](QPrinter *printer) {
        if (activeMyChild())
            activeMyChild()->print(printer);
    });

    preview.exec();
}


/* 插入表格 */
void MyWord::insertTable()
{
    if (activeMyChild()) {
        int rows = rowsSpinBox->value();
        int columns = columnsSpinBox->value();
        activeMyChild()->insertTable(rows, columns);
    }
}


/* 插入列表 */
void MyWord::insertList()
{
    if (activeMyChild())
        activeMyChild()->insertList();
}


/* 插入图片 */
void MyWord::insertImage()
{
    if (activeMyChild())
        activeMyChild()->insertImage();
}


/* 创建动作 */
void MyWord::createActions()
{
    /* 主菜单动作初始化 */

    /* 新建文件动作初始化 */
    newAct = new QAction(QIcon(":/Image/create_new_file.png"), tr("新建(&N)"), this);
    newAct->setShortcuts(QKeySequence::New);            /* ctrl + n */
    newAct->setToolTip(tr("新建"));
    newAct->setStatusTip(tr("创建一个新文档"));
    connect(newAct, &QAction::triggered, this, &MyWord::fileNew);

    /* 打开文件动作初始化 */
    openAct = new QAction(QIcon(":/Image/open_file.png"), tr("打开文件(&O)"), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setToolTip(tr("打开"));
    openAct->setStatusTip(tr("打开已存在文档"));
    connect(openAct, &QAction::triggered, this, &MyWord::fileOpen);

    /* 保存文件动作初始化 */
    saveAct = new QAction(QIcon(":/Image/save.png"), tr("保存(&S)"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setToolTip(tr("保存"));
    saveAct->setStatusTip(tr("当前文档保存"));
    connect(saveAct, &QAction::triggered, this, &MyWord::fileSave);

    /* 另存为动作初始化 */
    saveAsAct = new QAction(tr("另存为(&A)"), this);
    saveAsAct->setShortcuts(QKeySequence::SaveAs);
    saveAsAct->setToolTip(tr("另存为"));
    saveAsAct->setStatusTip(tr("当前文件另存为"));
    connect(saveAsAct, &QAction::triggered, this, &MyWord::fileSaveAs);

    /* 打印文档动作初始化 */
    printAct = new QAction(QIcon(":/Image/printer.png"), tr("打印(&P)"), this);
    printAct->setShortcuts(QKeySequence::Print);
    printAct->setToolTip(tr("打印"));
    printAct->setStatusTip(tr("打印文档"));
    connect(printAct, &QAction::triggered, this, &MyWord::filePrint);

    /* 打印预览动作初始化 */
    printPreviewAct = new QAction(tr("打印预览"), this);
    printPreviewAct->setStatusTip(tr("预览打印效果"));
    connect(printPreviewAct, &QAction::triggered, this, &MyWord::filePrintPreview);

    /* 退出动作初始化 */
    exitAct = new QAction(tr("退出(&X)"), this);
    exitAct->setStatusTip(tr("退出应用程序"));
//    connect(exitAct, &QAction::triggered, this, &closeAllWindows);



    /* 编辑菜单动作初始化 */

    /* 撤销动作初始化 */
    undoAct = new QAction(QIcon(":/Image/undo.png"), tr("撤销(&U)"), this);
    undoAct->setShortcuts(QKeySequence::Undo);
    undoAct->setToolTip(tr("撤销"));
    undoAct->setStatusTip(tr("撤销当前动作"));
    connect(undoAct, &QAction::triggered, this, &MyWord::undo);

    /* 重做动作初始化 */
    redoAct = new QAction(QIcon(":/Image/redo.png"), tr("重做(&R)"), this);
    redoAct->setShortcuts(QKeySequence::Redo);
    redoAct->setToolTip(tr("重做"));
    redoAct->setStatusTip(tr("恢复之气的操作"));
    connect(redoAct, &QAction::triggered, this, &MyWord::redo);

    /* 剪切动作初始化 */
    cutAct = new QAction(QIcon(":/Image/cut.png"), tr("剪切(&T)"), this);
    cutAct->setShortcuts(QKeySequence::Cut);
    cutAct->setToolTip(tr("剪切"));
    cutAct->setStatusTip(tr("从文档中剪切选中的内容, 放入剪切板"));
    connect(cutAct, &QAction::triggered, this, &MyWord::cut);

    /* 复制动作初始化 */
    copyAct = new QAction(QIcon(":/Image/copy.png"), tr("复制(&C)"), this);
    copyAct->setShortcuts(QKeySequence::Copy);
    copyAct->setToolTip(tr("复制"));
    copyAct->setStatusTip(tr("复制选中的内容, 并放入剪切板"));
    connect(copyAct, &QAction::triggered, this, &MyWord::copy);

    /* 粘贴动作初始化 */
    pasteAct = new QAction(QIcon(":/Image/paste.png"), tr("粘贴(&P)"), this);
    pasteAct->setShortcuts(QKeySequence::Paste);
    pasteAct->setToolTip(tr("粘贴"));
    pasteAct->setStatusTip(tr("将剪切板的内容粘贴到文档"));
    connect(pasteAct, &QAction::triggered, this, &MyWord::paste);


    /* 格式主菜单 */

    /* 加粗 */
    boldAct = new QAction(QIcon(":/Image/bold.png"), tr("加粗(&B)"), this);
    boldAct->setCheckable(true);
    boldAct->setShortcut(Qt::CTRL + Qt::Key_B);
    boldAct->setToolTip(tr("加粗"));
    boldAct->setStatusTip(tr("选中的字体加粗"));
    QFont bold;
    bold.setBold(true);
    boldAct->setFont(bold);
    connect(boldAct, &QAction::triggered, this, &MyWord::textBold);

    /* 倾斜 */
    italicAct = new QAction(QIcon(":/Image/italic.png"), tr("倾斜(&I)"), this);
    italicAct->setShortcut(Qt::CTRL + Qt::Key_I);
    italicAct->setCheckable(true);
    italicAct->setToolTip(tr("加粗"));
    italicAct->setStatusTip(tr("选择的字体倾斜"));
    QFont italic;
    italic.setItalic(true);
    italicAct->setFont(italic);
    connect(italicAct, &QAction::triggered, this, &MyWord::textItalic);

    /* 下划线 */
    underlineAct = new QAction(QIcon(":/Image/underline.png"), tr("下划线(&U)"), this);
    underlineAct->setShortcut(Qt::CTRL + Qt::Key_U);
    underlineAct->setCheckable(true);
    underlineAct->setToolTip(tr("下划线"));
    underlineAct->setStatusTip(tr("选中的字体下划线"));
    QFont underline;
    underline.setUnderline(true);
    underlineAct->setFont(underline);
    connect(underlineAct, &QAction::triggered, this, &MyWord::textUnderline);

    /* 格式->段落 子菜单下的各项为一个菜单组, 只能选择期中一项 */
    QActionGroup *grp = new QActionGroup(this);
    connect(grp, &QActionGroup::triggered, this, &MyWord::textAlign);

    /* 如果布局方向是左到右 */
    if (QApplication::isLeftToRight()) {
        leftAlignAct = new QAction(QIcon(":/Image/leftAlign.png"), tr("左对齐(&L)"), grp);
        centerAct = new QAction(QIcon(":/Image/centerAlign.png"), tr("居中(&E)"), grp);
        rightAlignAct = new QAction(QIcon(":/Image/rightAlign.png"), tr("右对齐(&R)"), grp);

    /* 如果对齐方向是右到左 */
    } else {
        rightAlignAct = new QAction(QIcon(":/Image/rightAlign.png"), tr("右对齐(&R)"), grp);
        centerAct = new QAction(QIcon(":/Image/centerAlign.png"), tr("居中(&E)"), grp);
        leftAlignAct = new QAction(QIcon(":/Image/leftAlign.png"), tr("左对齐(&L)"), grp);
    }

    justifyAct = new QAction(QIcon(":/Image/justifyAlign.png"), tr("两端对齐(&J)"), grp);

    /* 左对齐初始化 */
    leftAlignAct->setShortcut(Qt::CTRL + Qt::Key_L);
    leftAlignAct->setCheckable(true);
    leftAlignAct->setToolTip(tr("左对齐"));
    leftAlignAct->setStatusTip(tr("文字左对齐"));

    /* 中间对齐初始化 */
    centerAct->setShortcut(Qt::CTRL + Qt::Key_E);
    centerAct->setCheckable(true);
    centerAct->setToolTip(tr("居中"));
    centerAct->setStatusTip(tr("文字居中"));

    /* 右对齐 */
    rightAlignAct->setShortcut(Qt::CTRL + Qt::Key_R);
    rightAlignAct->setCheckable(true);
    rightAlignAct->setToolTip(tr("右对齐"));
    rightAlignAct->setStatusTip(tr("文字右对齐"));

    /* 两端对齐 */
    justifyAct->setShortcut(Qt::CTRL + Qt::Key_J);
    justifyAct->setCheckable(true);
    justifyAct->setToolTip(tr("两端对齐"));
    justifyAct->setStatusTip(tr("将文字左右两端同时对齐, 根据需要增加字距"));

    /* 选择字体颜色 */
    colorAct = new QAction(QIcon(":/Image/colorFont.png"), tr("颜色(&C)"), this);
    colorAct->setToolTip(tr("颜色"));
    colorAct->setStatusTip(tr("选择字体颜色"));
    connect(colorAct, &QAction::triggered, this, &MyWord::textColor);


    /* 文档窗口动作初始化 */

    /* 关闭当前窗口 */
    closeAct = new QAction(tr("关闭(&O)"), this);
    closeAct->setStatusTip(tr("关闭活动文档子窗口"));
    connect(closeAct, &QAction::triggered, mdiArea, &QMdiArea::closeActiveSubWindow);
    /* 关闭所有窗口 */
    closeAllAct = new QAction(tr("关闭所有(&A)"), this);
    closeAllAct->setStatusTip(tr("关闭所有活动文档子窗口"));
    connect(closeAllAct, &QAction::triggered, mdiArea, &QMdiArea::closeAllSubWindows);
    /* 层叠 */
    cascadeAct = new QAction(tr("层叠(&C)"), this);
    cascadeAct->setStatusTip(tr("层叠子窗口"));
    connect(cascadeAct, &QAction::triggered, mdiArea, &QMdiArea::cascadeSubWindows);
    /* 平铺 */
    tileAct = new QAction(tr("平铺(&T)"), this);
    tileAct->setStatusTip(tr("平铺子窗口"));
    connect(tileAct, &QAction::triggered, mdiArea, &QMdiArea::tileSubWindows);
    /* 下一个窗口 */
    nextAct = new QAction(tr("下一个(&X)"), this);
    nextAct->setStatusTip(tr("焦点移动到下一个子窗口"));
    nextAct->setShortcuts(QKeySequence::NextChild);
    connect(nextAct, &QAction::triggered, mdiArea, &QMdiArea::activateNextSubWindow);
    /* 前一个窗口 */
    previousAct = new QAction(tr("前一个(&V)"), this);
    previousAct->setStatusTip(tr("焦点移动到前一个子窗口"));
    previousAct->setShortcuts(QKeySequence::PreviousChild);
    connect(previousAct, &QAction::triggered, mdiArea, &QMdiArea::activatePreviousSubWindow);
    /* 分离 */
    separatorAct = new QAction(this);
    separatorAct->setSeparator(true);


    /* 关于软件 */
    aboutAct = new QAction(QIcon(":/Image/about.png"), tr("关于(&A)"), this);
    aboutAct->setStatusTip(tr("关于 Myself Word"));
    connect(aboutAct, &QAction::triggered, this, [=] {
       QMessageBox::about(this, tr("关于软件"), tr("一个简单的记事本软件 \n\t\t-断赋千歌"));
    });
    /* 关于 Qt */
    aboutQtAct = new QAction(QIcon(":/Image/qt.png"), tr("关于Qt(&Q)"), this);
    aboutQtAct->setStatusTip(tr("关于Qt库"));
    connect(aboutQtAct, &QAction::triggered, this, [=]{
        QMessageBox::aboutQt(this, "关于 Qt 库");
    });


    /* 表格行数计数 */
    rowsSpinBox = new QSpinBox(this);
    rowsSpinBox->setValue(0);

    /* 表格列数计数器 */
    columnsSpinBox = new QSpinBox(this);
    columnsSpinBox->setValue(0);

    /* 插入表格 */
    insertTableAct = new QAction(QIcon(":/Image/insertTable.png"), tr("插入表格"), this);
    insertTableAct->setStatusTip("插入表格");
    insertTableAct->setToolTip("插入一个的表格");
    connect(insertTableAct, &QAction::triggered, this, &MyWord::insertTable);
    /* 插入列表 */
    insertListAct = new QAction(QIcon(":/Image/insertList.png"), tr("插入列表"), this);
    insertListAct->setToolTip("插入一个列表");
    insertListAct->setStatusTip("插入列表");
    connect(insertListAct, &QAction::triggered, this, &MyWord::insertList);
    /* 插入图片 */
    insertImageAct = new QAction(QIcon(":/Image/insertImage.png"), tr("插入图片"), this);
    insertImageAct->setToolTip(tr("在文本编辑器中插入一张图片"));
    insertImageAct->setStatusTip("插入图片");
    connect(insertImageAct, &QAction::triggered, this, &MyWord::insertImage);
}


/* 创建菜单 */
void MyWord::createMenus()
{
    /* 文件菜单 */

    /* 新建文件 和 打开文件 */
    fileMenu = menuBar()->addMenu(tr("文件(&F)"));
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addSeparator();

    /* 最近打开 */
    recentlyOpenMenu = fileMenu->addMenu(tr("最近打开(&R)"));
    updateRecentlyOpenMenu();

    /* 保存文件 和 另存为 */
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
    fileMenu->addSeparator();
    /* 打印 和 打印预览 */
    fileMenu->addAction(printAct);
    fileMenu->addAction(printPreviewAct);
    fileMenu->addSeparator();
    /* 退出 */
    fileMenu->addAction(exitAct);
    menuBar()->addMenu(fileMenu);


    /* 编辑菜单 */
    editMenu = menuBar()->addMenu(tr("编辑(&E)"));
    editMenu->addAction(undoAct);
    editMenu->addAction(redoAct);
    editMenu->addSeparator();
    editMenu->addAction(cutAct);
    editMenu->addAction(copyAct);
    editMenu->addAction(pasteAct);


    /* 格式编辑栏 */
    formatMunu = menuBar()->addMenu(tr("格式(&O)"));
    /* 字体子菜单 */
    fontMenu = formatMunu->addMenu(QIcon(":/Image/font.png"), tr("字体(&D)"));
    fontMenu->addAction(boldAct);
    fontMenu->addAction(italicAct);
    fontMenu->addAction(underlineAct);
    /* 段落子菜单 */
    alignMenu = formatMunu->addMenu(QIcon(":/Image/align.png"), tr("段落"));
    alignMenu->addAction(leftAlignAct);
    alignMenu->addAction(centerAct);
    alignMenu->addAction(rightAlignAct);
    alignMenu->addAction(justifyAct);
    /* 字体颜色*/
    formatMunu->addAction(colorAct);

    /* 窗口菜单 */
    windowMenu = menuBar()->addMenu(tr("窗口(&W)"));
    updateMenus();
    connect(windowMenu, &QMenu::aboutToShow, this, &MyWord::updateWindowMenu);      /* 每次打开 窗口菜单, 将更新窗口菜单 */
    windowMenu->addAction(closeAct);
    windowMenu->addAction(closeAllAct);
    windowMenu->addAction(cascadeAct);
    windowMenu->addAction(tileAct);
    windowMenu->addAction(nextAct);
    windowMenu->addAction(previousAct);
    windowMenu->addAction(separatorAct);

    /* 帮助菜单 */
    helpMenu = menuBar()->addMenu(tr("帮助(&H)"));
    helpMenu->addAction(aboutAct);
    helpMenu->addSeparator();
    helpMenu->addAction(aboutQtAct);
}


/* 创建工具条 */
void MyWord::createToolBars()
{
    /* 文件工具条 */
    fileToolBar = addToolBar(tr("文件"));
    fileToolBar->addAction(newAct);
    fileToolBar->addAction(openAct);
    fileToolBar->addAction(saveAct);
    fileToolBar->addSeparator();
    fileToolBar->addAction(printAct);
    fileToolBar->setMovable(false);

    /* 编辑工具条 */
    editToolBar = addToolBar(tr("编辑"));
    editToolBar->addAction(undoAct);
    editToolBar->addAction(redoAct);
    editToolBar->addSeparator();
    editToolBar->addAction(cutAct);
    editToolBar->addAction(copyAct);
    editToolBar->addAction(pasteAct);
    editToolBar->setMovable(false);

    /* 格式工具条 */
    formatToolBar = addToolBar(tr("格式"));
    formatToolBar->addAction(boldAct);
    formatToolBar->addAction(italicAct);
    formatToolBar->addAction(underlineAct);
    formatToolBar->addSeparator();
    formatToolBar->addAction(leftAlignAct);
    formatToolBar->addAction(centerAct);
    formatToolBar->addAction(rightAlignAct);
    formatToolBar->addAction(justifyAct);
    formatToolBar->addSeparator();
    formatToolBar->addAction(colorAct);
    formatToolBar->addSeparator();
    formatToolBar->addAction(tileAct);
    formatToolBar->setMovable(false);

    /* 组合选择栏 */
    addToolBarBreak(Qt::TopToolBarArea);
    /* 段落 */
    comboToolBar = addToolBar(tr("组合选择"));
    comboToolBar->setMovable(false);
    comboSytle = new QComboBox;
    comboToolBar->addWidget(comboSytle);
    comboSytle->addItem(tr("标准"));
    comboSytle->addItem(tr("项目符号 (●)"));
    comboSytle->addItem(tr("项目符号 (○)"));
    comboSytle->addItem(tr("项目符号 (■)"));
    comboSytle->addItem(tr("项目符号 (1.2.3.)"));
    comboSytle->addItem(tr("项目符号 (a.b.c.)"));
    comboSytle->addItem(tr("项目符号 (A.B.C.)"));
    comboSytle->addItem(tr("项目符号 (ⅰ.ⅱ.ⅲ."));
    comboSytle->addItem(tr("项目符号 (Ⅰ.Ⅱ.Ⅲ."));
    comboSytle->setStatusTip(tr("段落加编号或者标号"));
    void (QComboBox::*pComboStyle)(int) = &QComboBox::activated;
    connect(comboSytle, pComboStyle, this, &MyWord::textStyle);

    /* 字体类型 */
    comboFont = new QFontComboBox;
    comboToolBar->addWidget(comboFont);
    comboFont->setStatusTip(tr("更改字体"));
    void (QComboBox::*pFontFamily)(QString const&) = &QComboBox::activated;
    connect(comboFont, pFontFamily, this, &MyWord::textFamily);

    /* 字体大小 */
    comboSize = new QComboBox;
    comboToolBar->addWidget(comboSize);
    comboSize->setEditable(true);
    comboSize->setStatusTip(tr("更改字号"));
    /* 手动添加字号 */
    QFontDatabase db;                   /* 系统底层可用的字体信息 */
    for (int size : db.standardSizes())
        comboSize->addItem(QString::number(size));

    void (QComboBox::*pComboBoxSize)(QString const&) = &QComboBox::activated;
    connect(comboSize, pComboBoxSize, this, &MyWord::textSize);
//    comboSize->setCurrentIndex(comboSize->findText(QString::number(QApplication::font().pointSize())));       /* 将当前字号设置为系统默认字号 */

    /* 默认的文本设置 */
    comboSize->setCurrentText("16");
    QFont font(tr("微软雅黑"), 16);
    comboFont->setCurrentFont(font);

    /* 插入 工具条 */
    insertActBar = addToolBar(tr("插入"));
    insertActBar->addWidget(new QLabel(tr("表格行:"), this));
    insertActBar->addWidget(rowsSpinBox);
    insertActBar->addWidget(new QLabel(tr("表格列:"), this));
    insertActBar->addWidget(columnsSpinBox);
    insertActBar->addAction(insertTableAct);
    insertActBar->addSeparator();
    insertActBar->addAction(insertListAct);
    insertActBar->addAction(insertImageAct);
}


/* 创建状态栏 */
void MyWord::createStatusBar()
{
    statusBar()->showMessage(tr("就绪"));
    statusBar()->setSizeGripEnabled(true);
}


/* 将工具按钮设置成 可用状态 */
void MyWord::enabledText()
{
    boldAct->setEnabled(true);
    italicAct->setEnabled(true);
    underlineAct->setEnabled(true);
    leftAlignAct->setEnabled(true);
    centerAct->setEnabled(true);
    rightAlignAct->setEnabled(true);
    justifyAct->setEnabled(true);
    colorAct->setEnabled(true);
}


/* 在已经打开的 文档中 查找文件 */
QMdiSubWindow *MyWord::findMyChild(const QString &fileName)
{
    QString canonicaFilePath = QFileInfo(fileName).canonicalFilePath();

    for (QMdiSubWindow *window : mdiArea->subWindowList()) {

        MyChild *myChild = reinterpret_cast<MyChild*>(window->widget());
        if (myChild && myChild->currentFile() == canonicaFilePath)
            return window;
    }

    return nullptr;
}


/* 更新最近打开菜单 */
void MyWord::updateRecentlyOpenMenu()
{
    QList<QString> fileNameList;
    fileNameList = fileManager->getAllFileName();

    recentlyOpenMenu->clear();

    /* 将所有的文件名, 添加到最近打开中 */
    for (QString const &fileName : fileNameList) {
        QAction *act = new QAction(fileName);
        recentlyOpenMenu->addAction(act);

        connect(act, &QAction::triggered, this, [=]() {

            QMdiSubWindow *existing = findMyChild(act->text());
            if (existing) {
                     mdiArea->setActiveSubWindow(existing);

            /* 不存在时, 打开文件 */
            } else {
                QString path = fileManager->getFilePath(act->text());
                /* 如果文件名出错 */
                if (path.isEmpty()) {
                    QMessageBox::critical(this, tr("错误"), tr("文件没找到"));
                    return;
                }

                MyChild *child = createMyChild();
                child->loadFile(path);
                child->show();
            }

        });
    }
}


/* 关闭事件 */
void MyWord::closeEvent(QCloseEvent *event)
{
    mdiArea->closeAllSubWindows();
    if (mdiArea->currentSubWindow())
        event->ignore();
    else
        event->accept();
}

