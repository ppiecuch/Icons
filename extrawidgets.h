#ifndef EXTRAWIDGETS_H
#define EXTRAWIDGETS_H

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtGui/qcolor.h>
#include <QtGui/qpainter.h>
#include <QtWidgets/qcombobox.h>
#include <QtWidgets/qformlayout.h>
#include <QtWidgets/qframe.h>
#include <QtWidgets/qgroupbox.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qtoolbutton.h>

class QSize;
class QIcon;
class QPixmap;

namespace QtWidgetsExtraCache {
bool cachePixmap(const QString &key, const QPixmap &pixmap);
QPixmap cachedPixmap(const QString &key);
QPixmap cachedPixmapColor(const QColor &color, const QSize &size);
bool cacheIcon(const QString &key, const QIcon &icon);
QIcon cachedIcon(const QString &key);
QIcon cachedIconColor(const QColor &color, const QSize &size);
} //namespace QtWidgetsExtraCache

// A lightweight button class
//
// The lightweight buttons are implemented from labels that have a "push-down" behaviour. The only
// signal emitted is "clicked" with an integer value that can be assigned to the button.
class ActiveLabel : public QLabel {
	Q_OBJECT

	int m_index;
	bool m_grabbed;

	virtual void mousePressEvent(QMouseEvent *e);
	virtual void mouseReleaseEvent(QMouseEvent *e);

Q_SIGNALS:
	void clicked(int);

public Q_SLOTS:
	void setIndex(int index) { m_index = index; }

public:
	ActiveLabel(QWidget *parent = 0);
	ActiveLabel(int index, const char *title, const char *name = "active_button", QWidget *parent = 0);
	ActiveLabel(int index, const QIcon &icon, const char *tooltip, const char *name = "active_button", QWidget *parent = 0);
};

class CategoryLabel : public ActiveLabel {
	Q_OBJECT

public:
	CategoryLabel(QWidget *parent = 0);

protected:
	void paintEvent(QPaintEvent *e);

Q_SIGNALS:
	void toggle(bool);

public Q_SLOTS:
	void setStatePixmapOff(const QPixmap &pixmap);
	void setStatePixmapOn(const QPixmap &pixmap);

private:
	int m_index;
	bool m_grabbed;
	bool m_state;
	QPixmap m_extrapixmap[2];
};

class AspectRatioPixmapLabel : public QLabel {
	Q_OBJECT
public:
	explicit AspectRatioPixmapLabel(Qt::TransformationMode mode = Qt::SmoothTransformation, QWidget *parent = 0);
	virtual int heightForWidth(int width) const;
	virtual QSize sizeHint() const;
Q_SIGNALS:

public Q_SLOTS:
	void setPixmap(const QPixmap &);
	void resizeEvent(QResizeEvent *);

private:
	QPixmap pix;
	Qt::TransformationMode mod;
};

// NOTE: This class duplicate QColor::NameFormat because this is not a Qt namespace available enum for Qt Designer.

class QColorListModel : public QAbstractListModel {
	Q_OBJECT
	class QColorListModelPrivate *d;
	Q_ENUMS(NameFormat)

public:
	enum NameFormat {
		HexRgb = QColor::HexRgb,
		HexArgb = QColor::HexArgb
	};

	enum CustomRoles {
		HexArgbName = Qt::UserRole
	};

	explicit QColorListModel(QObject *parent = 0);
	explicit QColorListModel(const QStringList &colorListNames, QObject *parent = 0);
	explicit QColorListModel(const QList<QColor> &colorsList, QObject *parent = 0);

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QModelIndex sibling(int row, int column, const QModelIndex &idx) const;

	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

	virtual Qt::ItemFlags flags(const QModelIndex &index) const;

	virtual bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());
	virtual bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());

	virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

	virtual Qt::DropActions supportedDropActions() const;

	QColorListModel::NameFormat nameFormat() const;
	void setNameFormat(QColorListModel::NameFormat nameFormat);

	QStringList colorListNames() const;
	void setColorListNames(const QStringList &colorListNames);

	QList<QColor> colorsList() const;
	void setColorsList(const QList<QColor> &colors);
};

class QColorComboBox : public QComboBox {
	Q_OBJECT
	class QColorComboBoxPrivate *d;

	Q_PROPERTY(QColorListModel::NameFormat nameFormat READ nameFormat WRITE setNameFormat)
	Q_PROPERTY(QStringList colorListNames READ colorListNames WRITE setColorListNames)
	Q_PROPERTY(QString currentColorName READ currentColorName WRITE setCurrentColorName USER true)

public:
	explicit QColorComboBox(QWidget *parent = 0);
	explicit QColorComboBox(const QStringList &colorListNames, QWidget *parent = 0);
	explicit QColorComboBox(const QList<QColor> &colorsList, QWidget *parent = 0);

	QColorListModel::NameFormat nameFormat() const;
	void setNameFormat(QColorListModel::NameFormat nameFormat);

	QStringList colorListNames() const;
	void setColorListNames(const QStringList &colorListNames);

	QList<QColor> colorsList() const;
	void setColorsList(const QList<QColor> &colorsList);

	QString currentColorName() const;
	QString currentTextColorName() const;

	QColor currentColor() const;
	QColor currentTextColor() const;

public Q_SLOTS:
	void setCurrentColorName(const QString &colorName);
	void setCurrentTextColorName(const QString &colorName);
	void setEditTextColorName(const QString &colorName);

	void setCurrentColor(const QColor &color);
	void setCurrentTextColor(const QColor &color);
	void setEditTextColor(const QColor &color);

protected:
	virtual void paintEvent(QPaintEvent *event);

Q_SIGNALS:
	void activated(const QColor &color);
	void currentIndexChanged(const QColor &color);
	void currentTextChanged(const QColor &color);
	void editTextChanged(const QColor &color);
	void highlighted(const QColor &color);
};

class QColorButton : public QToolButton {
	Q_OBJECT
public:
	QColorButton(QWidget *parent = 0) :
			QToolButton(parent) {}
	void paintEvent(QPaintEvent *e) {
		QToolButton::paintEvent(e);
		QPainter p(this);
		QColor col = palette().color(QPalette::Button);
		p.setPen(col.darker());
		p.setBrush(col);
		QRect rc = rect() - QMargins(6, 6, 6, 6);
		p.drawRect(rc);
	}
Q_SIGNALS:
	void colorChanged();
};

// A specialized QLabel that displays an ellipsis when its text is larger than the label width.
//
// If the text string displayed in the label is longer than the width of the
// label, an ellipsis is drawn based on the elide mode returned by
// getElideMode().  To allow the label to resize smaller than the text string,
// a minimum size must be set (e.g. QWidget::setMimimumWidth()).
//
// With an ElidedLabel, when determining whether the text should be elided, the QLabel
// word wrap property is ignored.
class ElidedLabel : public QLabel {
	Q_OBJECT

	ElidedLabel(const ElidedLabel &rhs);
	ElidedLabel &operator=(const ElidedLabel &rhs);
	Qt::TextElideMode mElideMode;
	QString mPreviousText;
	QRect mPreviousRect;
	QString mElidedText;

protected:
	//  Draws the label and its text.
	//
	// The default implementation draws an ellipsis based on the elide mode
	// returned by getElideMode() if the text string is longer than the label
	// width.  Otherwise, the QLabel base class implementation is called.
	virtual void paintEvent(QPaintEvent *pEvent);

public:
	virtual ~ElidedLabel();

	// Sets the position in the text of the ellipsis that is drawn when the
	// text is longer the label width.
	void setElideMode(Qt::TextElideMode mode);

	// Returns the position in the text of the ellipsis that is drawn when the
	// text is longer the label width.
	Qt::TextElideMode getElideMode() const;

	// Creates a new elided label with a default elide mode of Qt::ElideLeft.
	ElidedLabel(QWidget *pParent = NULL);
	ElidedLabel(const QString &text, QWidget *pParent = nullptr);
};

// QPushButton with QActon
namespace Qt {
enum ActionPushButtonStyle {
	ActionPushButtonIconOnly,
	ActionPushButtonTextOnly,
	ActionPushButtonTextBesideIcon
};
}

class QActionPushButton : public QPushButton {
	Q_OBJECT

	Qt::ActionPushButtonStyle m_style;
	QAction *m_action;

private Q_SLOTS:
	void updateStyle();

public:
	QActionPushButton(QWidget *parent = 0);
	void setDefaultAction(QAction *action);
	void setActionPushButtonStyle(Qt::ActionPushButtonStyle style);
};

#endif // EXTRAWIDGETS_H
