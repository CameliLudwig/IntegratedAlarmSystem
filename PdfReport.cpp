#include "PdfReport.h"

#include <QPdfWriter>
#include <QPainter>
#include <QPageSize>
#include <QFontInfo>
#include <QFontMetrics>
#include <QTextOption>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlockFormat>
#include <QFileInfo>
#include <QDir>

/*
 * PdfReport.cpp（优化间距/行距/行高）
 * - 标题与副标题间距更大
 * - 段落改用 QTextDocument + 1.4x 行距，防止中文截字
 * - 表格表头/数据行最小行高提升
 * - 区块间距更松
 */

PdfReport::PdfReport() {}

void PdfReport::setTitle(const QString &t) { m_title = t; }
void PdfReport::setSubtitle(const QString &st) { m_subtitle = st; }
void PdfReport::setFooter(const QString &foot) { m_footer = foot; }
void PdfReport::setThemeColor(const QColor &c) { m_theme = c; }
void PdfReport::setDPI(int dpi)
{
    if (dpi > 0)
        m_dpi = dpi;
}

int PdfReport::px(double mm) const { return qRound(mm * m_dpi / 25.4); }

QFont PdfReport::baseFont(int pt) const
{
#ifdef Q_OS_WIN
    QFont f(QStringLiteral("Microsoft YaHei"), pt);
#else
    QFont f(QStringLiteral("Noto Sans CJK SC"), pt);
    if (QFontInfo(f).family().isEmpty() || QFontInfo(f).family() == "Sans Serif")
        f.setFamily(QStringLiteral("Sans Serif"));
#endif
    f.setPointSize(pt);
    return f;
}

QFont PdfReport::boldFont(int pt) const
{
    QFont f = baseFont(pt);
    f.setBold(true);
    return f;
}

// ---------------- 区块 API ----------------

int PdfReport::addParagraph(const QString &text)
{
    Section s;
    s.type = BlockType::Paragraph;
    s.paragraph = text;
    m_sections.append(s);
    return m_sections.size() - 1;
}

int PdfReport::addKeyValueGrid(const QList<KeyValueRow> &rows)
{
    Section s;
    s.type = BlockType::KeyValueGrid;
    s.kvRows = rows;
    m_sections.append(s);
    return m_sections.size() - 1;
}

int PdfReport::addTable(const QStringList &headers, const QList<QStringList> &rows)
{
    Section s;
    s.type = BlockType::Table;
    s.tableHeaders = headers;
    s.tableRows = rows;
    m_sections.append(s);
    return m_sections.size() - 1;
}

int PdfReport::addImage(const QImage &img, const QSizeF &sizeMM)
{
    Section s;
    s.type = BlockType::Image;
    s.image = img;
    s.imageSizeMM = sizeMM;
    m_sections.append(s);
    return m_sections.size() - 1;
}

int PdfReport::addPageBreak()
{
    Section s;
    s.type = BlockType::PageBreak;
    m_sections.append(s);
    return m_sections.size() - 1;
}

bool PdfReport::replaceSection(int index, const Section &sec)
{
    if (index < 0 || index >= m_sections.size())
        return false;
    m_sections[index] = sec;
    return true;
}

bool PdfReport::removeSection(int index)
{
    if (index < 0 || index >= m_sections.size())
        return false;
    m_sections.removeAt(index);
    return true;
}

void PdfReport::clearSections() { m_sections.clear(); }

// ---------------- 绘制实现 ----------------

void PdfReport::drawHeader(QPainter &p, int contentW, int &cursorY) const
{
    const int barH = px(14);
    const int gapTop = px(8);
    const int titleY = barH + gapTop;

    // 顶部主题色带
    p.save();
    p.setPen(Qt::NoPen);
    p.setBrush(m_theme);
    p.drawRect(0, 0, contentW, barH);
    p.restore();

    // 标题
    p.save();
    p.setPen(QPen(QColor(10, 25, 50)));
    QFont ft = boldFont(19);
    p.setFont(ft);
    QFontMetrics fm(ft);
    QRect tr(0, titleY, contentW, fm.height() * 2);
    p.drawText(tr, Qt::AlignLeft | Qt::AlignVCenter,
               m_title.isEmpty() ? QStringLiteral("检测报告") : m_title);

    // 副标题（加大与标题间距）
    if (!m_subtitle.isEmpty())
    {
        QFont fs = baseFont(12);
        p.setFont(fs);
        QFontMetrics fms(fs);
        const int subGap = px(10);
        QRect sr(0, tr.bottom() + subGap, contentW, fms.height() * 2);
        p.setPen(QPen(QColor(60, 90, 130)));
        p.drawText(sr, Qt::AlignLeft | Qt::AlignVCenter, m_subtitle);
        cursorY = sr.bottom() + px(10);
    }
    else
    {
        cursorY = tr.bottom() + px(10);
    }
    p.restore();

    // 分隔线 + 额外留白
    p.setPen(QPen(QColor(200, 210, 230)));
    p.drawLine(0, cursorY, contentW, cursorY);
    cursorY += px(10);
}

void PdfReport::drawFooter(QPainter &p, int contentW, int pageH, int pageIndex, int pageCount) const
{
    const int y = pageH - px(m_marginB) + px(8);
    p.save();
    QFont ff = baseFont(9);
    p.setFont(ff);
    p.setPen(QPen(QColor(80, 90, 100)));

    p.drawText(QRect(0, y, contentW / 2, px(10)),
               Qt::AlignLeft | Qt::AlignVCenter, m_footer);

    p.drawText(QRect(contentW / 2, y, contentW / 2, px(10)),
               Qt::AlignRight | Qt::AlignVCenter,
               QStringLiteral("第 %1 / %2 页").arg(pageIndex).arg(pageCount));
    p.restore();
}

int PdfReport::drawParagraph(QPainter &p, int x, int y, int w, const QString &text) const
{
    const qreal lineFactor = 1.4; // 1.4x 行距
    const int padTop = px(2);
    const int padBottom = px(8);

    QTextDocument doc;
    QFont f = baseFont(12);
    doc.setDefaultFont(f);

    QTextOption opt;
    opt.setWrapMode(QTextOption::WordWrap);
    doc.setDefaultTextOption(opt);

    QTextCursor cur(&doc);
    cur.insertText(text);
    cur.select(QTextCursor::Document);
    QTextBlockFormat bf;
    bf.setLineHeight(int(lineFactor * 100), QTextBlockFormat::ProportionalHeight);
    cur.mergeBlockFormat(bf);

    doc.setTextWidth(w);

    p.save();
    const int drawY = y + padTop;
    doc.drawContents(&p, QRectF(x, drawY, w, doc.size().height()));
    p.restore();

    return int(doc.size().height()) + padTop + padBottom;
}

int PdfReport::drawKeyValue(QPainter &p, int x, int y, int w, const QList<KeyValueRow> &rows) const
{
    if (rows.isEmpty())
        return 0;

    const int gapKV = px(4);
    const int padV = px(4);
    const int minRowH = px(22);
    const double keyRatio = 0.26;

    const int keyW = int(w * keyRatio);
    const int valW = w - keyW - gapKV;

    QFont kf = boldFont(12);
    QFont vf = baseFont(12);

    int curY = y;
    for (const auto &r : rows)
    {
        QFontMetrics kfm(kf), vfm(vf);
        int kh = kfm.boundingRect(0, 0, keyW, 100000, Qt::TextWordWrap, r.key).height();
        int vh = vfm.boundingRect(0, 0, valW, 100000, Qt::TextWordWrap, r.value).height();
        int rowH = qMax(kh, vh) + padV * 2;
        rowH = qMax(rowH, minRowH);

        p.setFont(kf);
        p.setPen(QPen(QColor(30, 60, 120)));
        p.drawText(QRect(x, curY, keyW, rowH),
                   Qt::TextWordWrap | Qt::AlignVCenter | Qt::AlignLeft, r.key);

        p.setFont(vf);
        p.setPen(QPen(Qt::black));
        p.drawText(QRect(x + keyW + gapKV, curY, valW, rowH),
                   Qt::TextWordWrap | Qt::AlignVCenter | Qt::AlignLeft, r.value);

        p.setPen(QPen(QColor(220, 228, 240)));
        p.drawLine(x, curY + rowH, x + w, curY + rowH);

        curY += rowH;
    }
    return (curY - y) + px(8);
}

int PdfReport::drawTable(QPainter &p, int x, int y, int w,
                         const QStringList &headers, const QList<QStringList> &rows) const
{
    const int cols = qMax(headers.size(), rows.isEmpty() ? 0 : rows.first().size());
    if (cols <= 0)
        return 0;

    const int padCell = px(4);
    const int minHeaderH = px(26);
    const int minRowH = px(26);
    const int blockGapBtm = px(10);

    // 均分列宽
    QVector<int> colW(cols, w / cols);
    int remainder = w - colW[0] * cols;
    for (int i = 0; i < remainder; ++i)
        colW[i]++;

    QFont hf = boldFont(12);
    QFont bf = baseFont(11);

    int curY = y;

    // 表头
    p.save();
    p.setFont(hf);
    QFontMetrics hfm(hf);
    int headerH = 0;
    for (int c = 0, xx = x; c < cols; ++c)
    {
        const QString txt = c < headers.size() ? headers[c] : QString();
        QRect tr(xx, curY, colW[c], 100000);
        int th = hfm.boundingRect(tr, Qt::TextWordWrap, txt).height() + padCell * 2;
        headerH = qMax(headerH, th);
        xx += colW[c];
    }
    headerH = qMax(headerH, minHeaderH);

    // 表头底色
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(m_theme.red(), m_theme.green(), m_theme.blue(), 36));
    p.drawRect(x, curY, w, headerH);

    // 表头文字
    p.setPen(QPen(QColor(20, 50, 100)));
    int xx = x;
    for (int c = 0; c < cols; ++c)
    {
        const QString txt = c < headers.size() ? headers[c] : QString();
        p.drawText(QRect(xx + px(2), curY, colW[c] - px(4), headerH),
                   Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, txt);
        xx += colW[c];
    }
    // 表头下边线
    p.setPen(QPen(QColor(200, 210, 225)));
    p.drawLine(x, curY + headerH, x + w, curY + headerH);
    curY += headerH;

    // 数据行
    p.setFont(bf);
    QFontMetrics bfm(bf);
    for (const auto &r : rows)
    {
        int rowH = 0, xx2 = x;
        for (int c = 0; c < cols; ++c)
        {
            const QString cell = c < r.size() ? r[c] : QString();
            int ch = bfm.boundingRect(QRect(xx2, curY, colW[c], 100000),
                                      Qt::TextWordWrap, cell)
                         .height() +
                     padCell * 2;
            rowH = qMax(rowH, ch);
            xx2 += colW[c];
        }
        rowH = qMax(rowH, minRowH);

        p.setPen(QPen(QColor(230, 234, 242)));
        p.setBrush(Qt::NoBrush);
        xx2 = x;
        for (int c = 0; c < cols; ++c)
        {
            p.drawRect(QRect(xx2, curY, colW[c], rowH));
            const QString cell = c < r.size() ? r[c] : QString();
            p.setPen(Qt::black);
            p.drawText(QRect(xx2 + px(3), curY + px(2),
                             colW[c] - px(6), rowH - px(4)),
                       Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, cell);
            xx2 += colW[c];
        }
        curY += rowH;
    }
    p.restore();
    return (curY - y) + blockGapBtm;
}

int PdfReport::drawImage(QPainter &p, int x, int y, int w, const QImage &img, const QSizeF &sizeMM) const
{
    if (img.isNull())
        return 0;
    int drawW = w, drawH = 0;
    if (sizeMM.width() > 0.1)
        drawW = px(sizeMM.width());
    if (sizeMM.height() > 0.1)
        drawH = px(sizeMM.height());

    const double ar = img.width() / double(img.height());
    if (drawH <= 0 && drawW > 0)
        drawH = int(drawW / ar);
    if (drawW <= 0 && drawH > 0)
        drawW = int(drawH * ar);

    drawW = qMin(drawW, w);
    if (drawH <= 0)
        drawH = int(drawW / ar);

    p.drawImage(QRect(x, y, drawW, drawH), img);
    return drawH + px(12);
}

bool PdfReport::ensureSpaceOrNewPage(QPdfWriter &writer, QPainter &p, int neededH,
                                     int contentW, int &cursorY, int &pageIndex) const
{
    const int contentBottom = writer.height() - px(m_marginB);
    if (cursorY + neededH <= contentBottom)
        return false;

    writer.newPage();
    pageIndex++;

    p.restore();
    p.save();
    p.translate(px(m_marginL), px(m_marginT));
    cursorY = 0;
    drawHeader(p, contentW, cursorY);
    return true;
}

// ---------------- 导出 ----------------

bool PdfReport::exportPdf(const QString &filePath, QString *error) const
{
    if (filePath.trimmed().isEmpty())
    {
        if (error)
            *error = QStringLiteral("文件路径为空");
        return false;
    }

    // 确保目录存在
    QFileInfo fi(filePath);
    if (!fi.absolutePath().isEmpty())
        QDir().mkpath(fi.absolutePath());

    QPdfWriter writer(filePath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(m_dpi);

    QPainter p(&writer);
    if (!p.isActive())
    {
        if (error)
            *error = QStringLiteral("无法打开 PDF 进行绘制（请检查写入权限/路径是否存在）");
        return false;
    }

    const int pageW = writer.width();
    const int pageH = writer.height();
    const int contentW = pageW - px(m_marginL + m_marginR);

    p.save();
    p.translate(px(m_marginL), px(m_marginT));

    int y = 0;
    int currentPage = 1;

    // 页眉
    drawHeader(p, contentW, y);

    // 区块绘制
    for (int i = 0; i < m_sections.size(); ++i)
    {
        const Section &s = m_sections[i];
        switch (s.type)
        {
        case BlockType::Paragraph:
        {
            int preH = px(36); // 文本采用 QTextDocument，估计稍大
            ensureSpaceOrNewPage(writer, p, preH, contentW, y, currentPage);
            y += drawParagraph(p, 0, y, contentW, s.paragraph);
            break;
        }
        case BlockType::KeyValueGrid:
        {
            int preH = qMax(1, s.kvRows.size()) * px(26) + px(12);
            ensureSpaceOrNewPage(writer, p, preH, contentW, y, currentPage);
            y += drawKeyValue(p, 0, y, contentW, s.kvRows);
            break;
        }
        case BlockType::Table:
        {
            int preH = px(28) + qMax(1, s.tableRows.size()) * px(26) + px(12);
            ensureSpaceOrNewPage(writer, p, preH, contentW, y, currentPage);
            y += drawTable(p, 0, y, contentW, s.tableHeaders, s.tableRows);
            break;
        }
        case BlockType::Image:
        {
            int iw = (s.imageSizeMM.width() > 0.1) ? px(s.imageSizeMM.width()) : contentW;
            int ih = (s.imageSizeMM.height() > 0.1) ? px(s.imageSizeMM.height())
                                                    : (iw > 0 && !s.image.isNull()
                                                           ? int(iw * (s.image.height() / double(s.image.width())))
                                                           : px(60));
            ensureSpaceOrNewPage(writer, p, ih + px(16), contentW, y, currentPage);
            y += drawImage(p, 0, y, contentW, s.image, s.imageSizeMM);
            break;
        }
        case BlockType::PageBreak:
        {
            writer.newPage();
            currentPage++;
            p.restore();
            p.save();
            p.translate(px(m_marginL), px(m_marginT));
            y = 0;
            drawHeader(p, contentW, y);
            break;
        }
        }
    }

    // （简化版）页脚：此示例只在最后一页绘制一次
    drawFooter(p, contentW, pageH, /*pageIndex*/ 1, /*pageCount*/ 1);

    p.restore();
    p.end();
    return true;
}
