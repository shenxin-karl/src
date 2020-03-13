#ifndef MYWORD_H
#define MYWORD_H

#include <QObject>
#include <QMainWindow>
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <memory>

class QSpinBox;
class QMdiArea;
class QMenu;
class QAction;
class QComboBox;
class QFontComboBox;
class QMdiSubWindow;
class QSignalMapper;
class MyChild;
class FileManager;

class MyWord : public QMainWindow
{
    Q_OBJECT

public:
    MyWord(QWidget *parent = nullptr);
    ~MyWord() override;

private slots:
    void textAlign(QAction *a);             /* 动作组槽函数 */
    void fileNew();                         /* 新建文件 */
    MyChild *createMyChild();               /* 新建子窗口 */
    void updateMenus();                     /* 更新菜单 */
    MyChild *activeMyChild();               /* 返回活动的子窗口的指针 */
    void updateWindowMenu();                /* 更新窗口菜单 */
    void fileOpen();                        /* 打开文件 */
    void fileSave();                        /* 保存文件 */
    void fileSaveAs();                      /* 另存为文件 */
    void undo();                            /* 撤销 */
    void redo();                            /* 重做 */
    void cut();                             /* 剪切 */
    void copy();                            /* 拷贝 */
    void paste();                           /* 粘贴 */
    void textBold();                        /* 文本加粗 */
    void textItalic();                      /* 文本斜体 */
    void textUnderline();                   /* 文本下划线 */
    void textFamily(QString const &);       /* 文本字体类型 */
    void textSize(QString const &);         /* 文本字体大小 */
    void textColor();                       /* 文字颜色 */
    void textStyle(int styleIndex);         /* 设置文本样式 */
    void filePrint();                       /* 打印 */
    void filePrintPreview();                /* 打印预览 */
    void insertTable();                     /* 在文本编辑器中插入表格 */
    void insertList();                      /* 在文本编辑器中插入列表 */
    void insertImage();                     /* 在文本编辑器中插入一张图片 */

private:
    void createActions();                   /* 创建动作 */
    void createMenus();                     /* 创建菜单 */
    void createToolBars();                  /* 创建工具栏 */
    void createStatusBar();                 /* 创建状态栏 */
    void enabledText();                     /* 将按钮设置成可用 */
    QMdiSubWindow *findMyChild(QString const &);      /* 在已经打开的文档中, 查找子窗口 */
    void updateRecentlyOpenMenu();          /* 更新最近打开菜单 */

protected:
    void closeEvent(QCloseEvent *event) override;     /* 重写关闭事件 */

private:
    std::unique_ptr<FileManager> fileManager;         /* 记录最近打开的文件 */
    QSignalMapper   *windowMapper = nullptr;          /* 信号映射器 */
    QMdiArea        *mdiArea = nullptr;               /* 多文档中心部件 */

    /* 文件 */
    QMenu           *fileMenu = nullptr;              /* 文件菜单 */
    QMenu           *recentlyOpenMenu = nullptr;      /* 最近打开文件子菜单 */
    QAction         *newAct = nullptr;                /* 新建文件动作 */
    QAction         *openAct = nullptr;               /* 打开文件动作 */
    QAction         *saveAct = nullptr;               /* 保存文件动作 */
    QAction         *saveAsAct = nullptr;             /* 另存为文件动作*/
    QAction         *printAct = nullptr;              /* 打印动作 */
    QAction         *printPreviewAct = nullptr;       /* 打印预览动作  */
    QAction         *exitAct = nullptr;               /* 退出动作*/

    /* 编辑 */
    QMenu           *editMenu = nullptr;              /* 编辑菜单 */
    QAction         *undoAct = nullptr;               /* 撤销动作 */
    QAction         *redoAct = nullptr;               /* 反撤销动作 */
    QAction         *cutAct = nullptr;                /* 剪切动作 */
    QAction         *copyAct = nullptr;               /* 拷贝动作 */
    QAction         *pasteAct = nullptr;              /* 粘贴动作 */

    /* 格式 */
    QMenu           *formatMunu = nullptr;            /* 格式菜单 */
    QMenu           *fontMenu = nullptr;              /* 字体子菜单 */
    QMenu           *alignMenu = nullptr;             /* 调整子菜单 */
    QAction         *boldAct = nullptr;               /* 加粗动作 */
    QAction         *italicAct = nullptr;             /* 倾斜动作 */
    QAction         *underlineAct = nullptr;          /* 下划线动作 */
    QAction         *leftAlignAct = nullptr;          /* 左对齐动作 */
    QAction         *centerAct = nullptr;             /* 中间对齐动作 */
    QAction         *rightAlignAct = nullptr;         /* 右对齐动作 */
    QAction         *justifyAct = nullptr;            /* 两端对齐动作 */
    QAction         *colorAct = nullptr;              /* 颜色选择动作  */

    /* 窗口 */
    QMenu           *windowMenu = nullptr;            /* 窗口菜单 */
    QAction         *closeAct = nullptr;              /* 关闭动作 */
    QAction         *closeAllAct = nullptr;           /* 关闭所有动作 */
    QAction         *tileAct = nullptr;               /* 平铺动作 */
    QAction         *cascadeAct = nullptr;            /* 层叠动作 */
    QAction         *nextAct = nullptr;               /* 下一个动作 */
    QAction         *previousAct = nullptr;           /* 前一个动作 */
    QAction         *separatorAct = nullptr;          /* 分隔符 */

    /* 帮助*/
    QMenu           *helpMenu = nullptr;              /* 帮助窗口 */
    QAction         *aboutAct = nullptr;              /* 关于帮助 */
    QAction         *aboutQtAct = nullptr;            /* 关于 Qt */

/*      工具条         */
    QToolBar        *fileToolBar = nullptr;           /* 文件工具条 */
    QToolBar        *editToolBar = nullptr;           /* 编辑工具条 */
    QToolBar        *formatToolBar = nullptr;         /* 格式工具条 */
    QToolBar        *comboToolBar = nullptr;          /* 组合工具条 */
    QComboBox       *comboSytle = nullptr;            /* 标号与编号选择工具条 */
    QFontComboBox   *comboFont = nullptr;             /* 字体选择框 */
    QComboBox       *comboSize = nullptr;             /* 字体大小选择框 */

    /* 插入 */
    QToolBar        *insertActBar = nullptr;          /* 插入动作工具条 */
    QSpinBox        *rowsSpinBox = nullptr;           /* 插入表格的数量 */
    QSpinBox        *columnsSpinBox = nullptr;        /* 插入表格的列数 */
    QAction         *insertTableAct = nullptr;        /* 插入表格 */
    QAction         *insertListAct = nullptr;         /* 插入列表 */
    QAction         *insertImageAct = nullptr;        /* 插入图标 */
};
#endif // MYWORD_H
