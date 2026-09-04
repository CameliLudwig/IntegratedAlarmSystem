#ifndef PDFREPORT_H
#define PDFREPORT_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QImage>
#include <QSizeF>
#include <QColor>

// 仅做前向声明即可（避免头文件强行依赖 gui 里的大头）
class QPainter;
class QPdfWriter;

/**
 * PdfReport
 * - 纯代码生成 PDF（QPdfWriter + QPainter）
 * - 提供区块式 API：段落 / 键值对 / 表格 / 图片 / 分页
 * - 自动分页 + 页眉/页脚
 */
class PdfReport
{
public:
    // 键值对一行
    struct KeyValueRow {
        QString key;
        QString value;
    };

    // 区块类型
    enum class BlockType { Paragraph, KeyValueGrid, Table, Image, PageBreak };

    // 区块实体
    struct Section {
        BlockType type = BlockType::Paragraph;
        // 段落
        QString paragraph;
        // 键值对
        QList<KeyValueRow> kvRows;
        // 表格
        QStringList tableHeaders;
        QList<QStringList> tableRows;
        // 图片
        QImage image;
        QSizeF imageSizeMM;
    };

public:
    PdfReport();

    // 基本信息
    void setTitle(const QString& t);
    void setSubtitle(const QString& st);
    void setFooter(const QString& foot);
    void setThemeColor(const QColor& c);
    void setDPI(int dpi);

    // 追加区块
    int  addParagraph(const QString& text);
    int  addKeyValueGrid(const QList<KeyValueRow>& rows);
    int  addTable(const QStringList& headers, const QList<QStringList>& rows);
    int  addImage(const QImage& img, const QSizeF& sizeMM = QSizeF());
    int  addPageBreak();
    bool replaceSection(int index, const Section& sec);
    bool removeSection(int index);
    void clearSections();

    // 导出
    bool exportPdf(const QString& filePath, QString* error = nullptr) const;

private:
    // 工具
    int   px(double mm) const;
    QFont baseFont(int pt) const;
    QFont boldFont(int pt) const;

    // 绘制
    void drawHeader(QPainter& p, int contentW, int& cursorY) const;
    void drawFooter(QPainter& p, int contentW, int pageH, int pageIndex, int pageCount) const;
    int  drawParagraph(QPainter& p, int x, int y, int w, const QString& text) const;
    int  drawKeyValue(QPainter& p, int x, int y, int w, const QList<KeyValueRow>& rows) const;
    int  drawTable(QPainter& p, int x, int y, int w, const QStringList& headers, const QList<QStringList>& rows) const;
    int  drawImage(QPainter& p, int x, int y, int w, const QImage& img, const QSizeF& sizeMM) const;
    bool ensureSpaceOrNewPage(QPdfWriter& writer, QPainter& p, int neededH,
                              int contentW, int& cursorY, int& pageIndex) const;

private:
    // 版式与主题
    QString m_title;
    QString m_subtitle;
    QString m_footer;
    QColor  m_theme = QColor("#1E88E5");
    // 页边距（毫米）
    double  m_marginL = 16.0;
    double  m_marginR = 16.0;
    double  m_marginT = 16.0;
    double  m_marginB = 16.0;
    // 分辨率
    int     m_dpi     = 300;

    // 区块集合
    QList<Section> m_sections;
};

#endif // PDFREPORT_H
