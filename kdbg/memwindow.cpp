// $Id$

// Copyright by Johannes Sixt
// This file is under GPL, the GNU General Public Licence

#include "memwindow.h"
#include <qheader.h>
#if QT_VERSION >= 200
#include <klocale.h>
#else
#include <kapp.h>
#endif
#include <ksimpleconfig.h>
#include "debugger.h"
#include "dbgdriver.h"			/* memory dump formats */

MemoryWindow::MemoryWindow(QWidget* parent, const char* name) :
	QWidget(parent, name),
	m_debugger(0),
	m_expression(true, this, "expression"),
	m_memory(this, "memory"),
	m_layout(this, 0, 2),
	m_format(MDTword | MDThex)
{
    QSize minSize = m_expression.sizeHint();
    m_expression.setMinimumSize(minSize);
    m_expression.setInsertionPolicy(QComboBox::NoInsertion);
    m_expression.setMaxCount(15);

    m_memory.addColumn(i18n("Address"), 80);
    m_memory.addColumn(i18n("Contents"), 200);
    m_memory.setSorting(-1);		/* don't sort */
    m_memory.setAllColumnsShowFocus(true);
    m_memory.header()->setClickEnabled(false);

    // create layout
    m_layout.addWidget(&m_expression, 0);
    m_layout.addWidget(&m_memory, 10);
    m_layout.activate();

#if QT_VERSION < 200
    connect(&m_expression, SIGNAL(activated(const char*)),
	    this, SLOT(slotNewExpression(const char*)));
#else
    connect(&m_expression, SIGNAL(activated(const QString&)),
	    this, SLOT(slotNewExpression(const QString&)));
#endif

    // the popup menu
    m_popup.insertItem("&Bytes", MDTbyte);
    m_popup.insertItem("&Halfwords (2 Bytes)", MDThalfword);
    m_popup.insertItem("&Words (4 Bytes)", MDTword);
    m_popup.insertItem("&Giantwords (8 Bytes)", MDTgiantword);
    m_popup.insertSeparator();
    m_popup.insertItem("He&xadecimal", MDThex);
    m_popup.insertItem("Signed &decimal", MDTsigned);
    m_popup.insertItem("&Unsigned decimal", MDTunsigned);
    m_popup.insertItem("&Octal", MDToctal);
    m_popup.insertItem("Bi&nary", MDTbinary);
    m_popup.insertItem("&Addresses", MDTaddress);
    m_popup.insertItem("&Character", MDTchar);
    m_popup.insertItem("&Floatingpoint", MDTfloat);
    m_popup.insertItem("&Strings", MDTstring);
    m_popup.insertItem("&Instructions", MDTinsn);
    connect(&m_popup, SIGNAL(activated(int)), this, SLOT(slotTypeChange(int)));

    /*
     * Handle right mouse button. Signal righteButtonClicked cannot be
     * used, because it works only over items, not over the blank window.
     */
    m_memory.viewport()->installEventFilter(this);

    m_formatCache.setAutoDelete(true);
}

MemoryWindow::~MemoryWindow()
{
}

#if QT_VERSION < 200
# define MOUSEPRESS Event_MouseButtonPress
#else
# define MOUSEPRESS QEvent::MouseButtonPress
#endif

bool MemoryWindow::eventFilter(QObject* o, QEvent* ev)
{
    if (ev->type() == MOUSEPRESS) {
	handlePopup(static_cast<QMouseEvent*>(ev));
	return true;
    }
    return false;
}

void MemoryWindow::handlePopup(QMouseEvent* ev)
{
    if (ev->button() == RightButton)
    {
	// show popup menu
	if (m_popup.isVisible())
	{
	    m_popup.hide();
	}
	else
	{
	    m_popup.popup(mapToGlobal(ev->pos()));
	}
	return;
    }
}

// this slot is only needed for Qt 1.44
void MemoryWindow::slotNewExpression(const char* newText)
{
    slotNewExpression(QString(newText));
}

void MemoryWindow::slotNewExpression(const QString& newText)
{
    QString text = newText.simplifyWhiteSpace();

    // see if the string is in the list
    // (note: must count downwards because of removeItem!)
    for (int i = m_expression.count()-1; i >= 0; i--)
    {
	if (m_expression.text(i) == text) {
	    // yes it is!
	    // look up the format that was used last time for this expr
	    unsigned* pFormat = m_formatCache[text];
	    if (pFormat != 0) {
		m_format = *pFormat;
		m_debugger->setMemoryFormat(m_format);
	    }
	    // remove this text, will be inserted at the top
	    m_expression.removeItem(i);
	}
    }
    m_expression.insertItem(text, 0);

    if (text.isEmpty()) {
	// if format was not in the cache, insert it
	if (m_formatCache[text] == 0) {
	    m_formatCache.insert(text, new unsigned(m_format));
	    
	}
    }

    displayNewExpression(text);
}

void MemoryWindow::displayNewExpression(const QString& expr)
{
    m_debugger->setMemoryExpression(expr);
    m_expression.setEditText(expr);

    // clear memory dump if no dump wanted
    if (expr.isEmpty()) {
	m_memory.clear();
    }
}

void MemoryWindow::slotTypeChange(int id)
{
    // compute new type
    if (id & MDTsizemask)
	m_format = (m_format & ~MDTsizemask) | id;
    if (id & MDTformatmask)
	m_format = (m_format & ~MDTformatmask) | id;
    m_debugger->setMemoryFormat(m_format);

    // change the format in the cache
    QString expr = m_expression.currentText();
    expr = expr.simplifyWhiteSpace();
    unsigned* pFormat = m_formatCache[expr];
    if (pFormat != 0)
	*pFormat = m_format;

    // force redisplay
    displayNewExpression(expr);
}

void MemoryWindow::slotNewMemoryDump(const QString& msg, QList<MemoryDump>& memdump)
{
    m_memory.clear();
    if (!msg.isEmpty()) {
	new QListViewItem(&m_memory, QString(), msg);
	return;
    }

    QListViewItem* after = 0;
    MemoryDump* md = memdump.first();
    for (; md != 0; md = memdump.next()) {
	QString addr = md->address.asString() + " " + md->address.fnoffs;
	after = new QListViewItem(&m_memory, after, addr, md->dump);
    }
}

static const char MemoryGroup[] = "Memory";
static const char NumExprs[] = "NumExprs";
static const char ExpressionFmt[] = "Expression%d";
static const char FormatFmt[] = "Format%d";
static const char ColumnWidths[] = "ColumnWidths";

void MemoryWindow::saveProgramSpecific(KSimpleConfig* config)
{
    KConfigGroupSaver s(config, MemoryGroup);

    int numEntries = m_expression.count();
    config->writeEntry(NumExprs, numEntries);
    QString exprEntry;
    QString fmtEntry;
    for (int i = 0; i < numEntries;) {
	QString text = m_expression.text(i);
	i++;				/* entries are counted 1-based */
	exprEntry.sprintf(ExpressionFmt, i);
	fmtEntry.sprintf(FormatFmt, i);
	config->writeEntry(exprEntry, text);
	unsigned* pFormat = m_formatCache[text];
	unsigned fmt = pFormat != 0  ?  *pFormat  :  MDTword | MDThex;
	config->writeEntry(fmtEntry, fmt);
    }

    // column widths
    QStrList widths;
    QString wStr;
    for (int i = 0; i < 2; i++) {
	int w = m_memory.columnWidth(i);
	wStr.setNum(w);
	widths.append(wStr);
    }
    config->writeEntry(ColumnWidths, widths);
}

void MemoryWindow::restoreProgramSpecific(KSimpleConfig* config)
{
    KConfigGroupSaver s(config, MemoryGroup);

    int numEntries = config->readNumEntry(NumExprs, 0);
    m_expression.clear();

    QString exprEntry;
    QString fmtEntry;
    // entries are counted 1-based
    for (int i = 1; i <= numEntries; i++) {
	exprEntry.sprintf(ExpressionFmt, i);
	fmtEntry.sprintf(FormatFmt, i);
	QString expr = config->readEntry(exprEntry);
	unsigned fmt = config->readNumEntry(fmtEntry, MDTword | MDThex);
	m_expression.insertItem(expr);
	m_formatCache.replace(expr, new unsigned(fmt & (MDTsizemask | MDTformatmask)));
    }

    // initialize with top expression
    if (numEntries > 0) {
	m_expression.setCurrentItem(0);
	QString expr = m_expression.text(0);
	m_format = *m_formatCache[expr];
	m_debugger->setMemoryFormat(m_format);
	displayNewExpression(expr);
    }

    // column widths
    QStrList widths;
    int n = config->readListEntry(ColumnWidths, widths);
    if (n > 2)
	n = 2;
    for (int i = 0; i < n; i++) {
	QString wStr = widths.at(i);
	bool ok;
	int w = wStr.toInt(&ok);
	if (ok)
	    m_memory.setColumnWidth(i, w);
    }
}


#include "memwindow.moc"
