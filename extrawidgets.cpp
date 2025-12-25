// Reference:
// ----------
// https://forum.qt.io/topic/64020/settext-and-setpixmap-using/5
// https://github.com/BamBZH/ElidedLabel/blob/master/elidedlabel.cpp

#include "extrawidgets.h"

#include <QtCore/qcache.h>
#include <QtCore/qdebug.h>
#include <QtCore/qvector.h>
#include <QtGui/qevent.h>
#include <QtGui/qfontmetrics.h>
#include <QtGui/qicon.h>
#include <QtGui/qpixmap.h>
#include <QtGui/qpixmapcache.h>
#include <QtWidgets/qaction.h>
#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qlineedit.h>

#include <algorithm>

// --------------------------------------------------------------------
//  ActiveLabel implementation

ActiveLabel::ActiveLabel(QWidget *parent) :
		QLabel(parent), m_index(-1), m_grabbed(false) {
	setAutoFillBackground(true);
	setObjectName(QString::fromLatin1("activelabel"));
	setFrameStyle(QFrame::Panel | QFrame::Raised);
	setBackgroundRole(QPalette::Window);
	setAlignment(Qt::AlignCenter);
}

ActiveLabel::ActiveLabel(int index, const char *title, const char *name, QWidget *parent) :
		QLabel(" " + QString(title) + " ", parent), m_index(index), m_grabbed(false) {
	setAutoFillBackground(true);
	setObjectName(QString::fromLatin1(name));
	setFrameStyle(QFrame::Panel | QFrame::Raised);
	setBackgroundRole(QPalette::Window);
	setAlignment(Qt::AlignCenter);
}

ActiveLabel::ActiveLabel(int index, const QIcon &icon, const char *tooltip, const char *name, QWidget *parent) :
		QLabel("", parent), m_index(index), m_grabbed(false) {
	setPixmap(icon.pixmap(18, 18));
	setAutoFillBackground(true);
	setObjectName(QString::fromLatin1(name));
	setToolTip(QString::fromLatin1(tooltip));
	setFrameStyle(QFrame::Panel | QFrame::Raised);
	setBackgroundRole(QPalette::Window);
	setAlignment(Qt::AlignCenter);
}

void ActiveLabel::mousePressEvent(QMouseEvent *e) {
	if (!m_grabbed && e->button() == Qt::LeftButton) {
		setFrameShadow(QFrame::Sunken);
		m_grabbed = true;
	}
}

void ActiveLabel::mouseReleaseEvent(QMouseEvent *e) {
	if (m_grabbed) {
		setFrameShadow(QFrame::Raised);
		m_grabbed = false;

		if (e->button() == Qt::LeftButton && rect().contains(e->pos())) {
			emit clicked(m_index);
		}
	}
}

// --------------------------------------------------------------------
// CategoryLabel implementation

CategoryLabel::CategoryLabel(QWidget *parent) :
		ActiveLabel(parent), m_state(true) {
	connect(this, &ActiveLabel::clicked, [=](int) {
		emit toggle(m_state = !m_state);
	});
}

void CategoryLabel::paintEvent(QPaintEvent *e) {
	ActiveLabel::paintEvent(e);
	if (!m_extrapixmap[m_state].isNull()) {
		// draw category marker:
		QPainter painter(this);
		QStyleOption opt;
		opt.initFrom(this);
		QRect cr = contentsRect();
		cr.adjust(margin(), margin(), -margin(), -margin());
		style()->drawItemPixmap(&painter, cr, Qt::AlignLeft | Qt::AlignVCenter, m_extrapixmap[m_state]);
	}
}

void CategoryLabel::setStatePixmapOn(const QPixmap &pixmap) {
	m_extrapixmap[1] = pixmap;
	update();
}

void CategoryLabel::setStatePixmapOff(const QPixmap &pixmap) {
	m_extrapixmap[0] = pixmap;
	update();
}

// --------------------------------------------------------------------
//  AspectRatioPixmapLabel

AspectRatioPixmapLabel::AspectRatioPixmapLabel(Qt::TransformationMode mode, QWidget *parent) :
		QLabel(parent), mod(mode) {
	this->setMinimumSize(1, 1);
	this->setMaximumSize(256, 256);
}

void AspectRatioPixmapLabel::setPixmap(const QPixmap &p) {
	pix = p;
	if (!pix.isNull())
		QLabel::setPixmap(pix.scaled(this->size(), Qt::KeepAspectRatio, mod));
	else
		QLabel::setPixmap(pix);
}

int AspectRatioPixmapLabel::heightForWidth(int width) const {
	if (pix.isNull())
		return width;
	else
		return ((qreal)pix.height() * width) / pix.width();
}

QSize AspectRatioPixmapLabel::sizeHint() const {
	int w = this->width();
	return QSize(w, heightForWidth(w));
}

void AspectRatioPixmapLabel::resizeEvent(QResizeEvent *e) {
	if (!pix.isNull())
		QLabel::setPixmap(pix.scaled(this->size(), Qt::KeepAspectRatio, mod));
}

// --------------------------------------------------------------------
//  QColorListModel

class QColorListModelPrivate : public QObject {
	Q_OBJECT

public:
	QColorListModel *object;
	QColorListModel::NameFormat nameFormat;
	QStringList colors;

	QColorListModelPrivate(QColorListModel *objectP) :
			QObject(objectP), object(objectP), nameFormat(QColorListModel::HexRgb) {
		Q_ASSERT(object);
	}
};

static bool ascendingLessThan(const QPair<QString, int> &s1, const QPair<QString, int> &s2) {
	return s1.first < s2.first;
}

static bool decendingLessThan(const QPair<QString, int> &s1, const QPair<QString, int> &s2) {
	return s1.first > s2.first;
}

QColorListModel::QColorListModel(QObject *parent) :
		QAbstractListModel(parent), d(new QColorListModelPrivate(this)) {
}

QColorListModel::QColorListModel(const QStringList &colorListNames, QObject *parent) :
		QAbstractListModel(parent), d(new QColorListModelPrivate(this)) {
	d->colors = colorListNames;
}

QColorListModel::QColorListModel(const QList<QColor> &colorsList, QObject *parent) :
		QAbstractListModel(parent), d(new QColorListModelPrivate(this)) {
	QStringList colors;

	foreach (const QColor &color, colorsList) {
		colors << color.name(QColor::HexArgb);
	}

	d->colors = colors;
}

int QColorListModel::rowCount(const QModelIndex &parent) const {
	if (parent.isValid()) {
		return 0;
	}

	return d->colors.count();
}

QModelIndex QColorListModel::sibling(int row, int column, const QModelIndex &idx) const {
	if (!idx.isValid() || column != 0 || row >= d->colors.count()) {
		return QModelIndex();
	}

	return createIndex(row, 0);
}

QVariant QColorListModel::data(const QModelIndex &index, int role) const {
	if (index.row() < 0 || index.row() >= d->colors.size()) {
		return QVariant();
	}

	if (role == Qt::DecorationRole) {
		return QtWidgetsExtraCache::cachedIconColor(QColor(d->colors.at(index.row())), QSize(64, 64));
	} else if (role == Qt::DisplayRole || role == Qt::EditRole) {
		return QColor(d->colors.at(index.row())).name(QColor::NameFormat(d->nameFormat));
	} else if (role == QColorListModel::HexArgbName) {
		return d->colors.at(index.row());
	}

	return QVariant();
}

Qt::ItemFlags QColorListModel::flags(const QModelIndex &index) const {
	if (!index.isValid()) {
		return QAbstractListModel::flags(index) | Qt::ItemIsDropEnabled;
	}

	return QAbstractListModel::flags(index) | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

bool QColorListModel::setData(const QModelIndex &index, const QVariant &value, int role) {
	if (index.row() >= 0 && index.row() < d->colors.size() && (role == Qt::EditRole || role == Qt::DisplayRole)) {
		d->colors.replace(index.row(), value.value<QColor>().name(QColor::HexArgb));
		emit dataChanged(index, index, QVector<int>() << role);
		return true;
	}

	return false;
}

bool QColorListModel::insertRows(int row, int count, const QModelIndex &parent) {
	if (count < 1 || row < 0 || row > rowCount(parent)) {
		return false;
	}

	beginInsertRows(QModelIndex(), row, row + count - 1);

	for (int r = 0; r < count; ++r) {
		d->colors.insert(row, QString());
	}

	endInsertRows();

	return true;
}

bool QColorListModel::removeRows(int row, int count, const QModelIndex &parent) {
	if (count <= 0 || row < 0 || (row + count) > rowCount(parent)) {
		return false;
	}

	beginRemoveRows(QModelIndex(), row, row + count - 1);

	for (int r = 0; r < count; ++r) {
		d->colors.removeAt(row);
	}

	endRemoveRows();

	return true;
}

void QColorListModel::sort(int, Qt::SortOrder order) {
	emit layoutAboutToBeChanged(QList<QPersistentModelIndex>(), VerticalSortHint);

	QList<QPair<QString, int>> list;
	for (int i = 0; i < d->colors.count(); ++i) {
		list.append(QPair<QString, int>(d->colors.at(i), i));
	}

	if (order == Qt::AscendingOrder) {
		std::sort(list.begin(), list.end(), ascendingLessThan);
	} else {
		std::sort(list.begin(), list.end(), decendingLessThan);
	}

	d->colors.clear();
	QVector<int> forwarding(list.count());
	for (int i = 0; i < list.count(); ++i) {
		d->colors.append(list.at(i).first);
		forwarding[list.at(i).second] = i;
	}

	QModelIndexList oldList = persistentIndexList();
	QModelIndexList newList;
	for (int i = 0; i < oldList.count(); ++i) {
		newList.append(index(forwarding.at(oldList.at(i).row()), 0));
	}
	changePersistentIndexList(oldList, newList);

	emit layoutChanged(QList<QPersistentModelIndex>(), VerticalSortHint);
}

Qt::DropActions QColorListModel::supportedDropActions() const {
	return QAbstractItemModel::supportedDropActions() | Qt::MoveAction;
}

QColorListModel::NameFormat QColorListModel::nameFormat() const {
	return d->nameFormat;
}

void QColorListModel::setNameFormat(QColorListModel::NameFormat nameFormat) {
	d->nameFormat = nameFormat;

	if (rowCount() > 0) {
		emit dataChanged(index(0, 0), index(rowCount() - 1, 0), QVector<int>() << Qt::DisplayRole);
	}
}

QStringList QColorListModel::colorListNames() const {
	return d->colors;
}

void QColorListModel::setColorListNames(const QStringList &colorListNames) {
	QStringList colors;

	foreach (const QString &colorName, colorListNames) {
		colors << QColor(colorName).name(QColor::HexArgb);
	}

	emit beginResetModel();
	d->colors = colors;
	emit endResetModel();
}

QList<QColor> QColorListModel::colorsList() const {
	QList<QColor> colors;

	foreach (const QString &colorName, d->colors) {
		colors << QColor(colorName);
	}

	return colors;
}

void QColorListModel::setColorsList(const QList<QColor> &colorsList) {
	QStringList colors;

	foreach (const QColor &color, colorsList) {
		colors << color.name(QColor::HexArgb);
	}

	emit beginResetModel();
	d->colors = colors;
	emit endResetModel();
}

// --------------------------------------------------------------------
//  QColorComboBox

class QColorComboBoxPrivate : public QObject {
	Q_OBJECT

public Q_SLOTS:
	void activated(const QString &text) {
		emit widget->activated(QColor(text));
	}

	void currentIndexChanged(const QString &text) {
		emit widget->currentIndexChanged(QColor(text));
	}

	void currentTextChanged(const QString &text) {
		emit widget->currentTextChanged(QColor(text));
	}

	void editTextChanged(const QString &text) {
		emit widget->editTextChanged(QColor(text));
	}

	void highlighted(const QString &text) {
		emit widget->highlighted(QColor(text));
	}

public:
	QColorComboBox *widget;
	QColorListModel *model;

	QColorComboBoxPrivate(QColorComboBox *widgetP) :
			QObject(widgetP), widget(widgetP), model(new QColorListModel(this)) {
		Q_ASSERT(widget);

		widget->setModel(model);
		updateValidator();

		connect(widget, SIGNAL(activated(QString)), this, SLOT(activated(QString)));
		connect(widget, SIGNAL(currentIndexChanged(QString)), this, SLOT(currentIndexChanged(QString)));
		connect(widget, SIGNAL(currentTextChanged(QString)), this, SLOT(currentTextChanged(QString)));
		connect(widget, SIGNAL(editTextChanged(QString)), this, SLOT(editTextChanged(QString)));
		connect(widget, SIGNAL(highlighted(QString)), this, SLOT(highlighted(QString)));
	}

	void updateValidator() {
		if (widget->lineEdit()) {
			QString mask = QLatin1String("\\#HHHhhh");

			if (model->nameFormat() == QColorListModel::HexArgb) {
				mask = QLatin1String("\\#HHHhhhhh");
			}

			if (widget->lineEdit()->inputMask() != mask) {
				widget->lineEdit()->setInputMask(mask);
			}
		}
	}

	QString internalColorName(const QColor &color) const {
		return color.name(QColor::HexArgb);
	}
};

QColorComboBox::QColorComboBox(QWidget *parent) :
		QComboBox(parent), d(new QColorComboBoxPrivate(this)) {
}

QColorComboBox::QColorComboBox(const QStringList &colorListNames, QWidget *parent) :
		QComboBox(parent), d(new QColorComboBoxPrivate(this)) {
	d->model->setColorListNames(colorListNames);
}

QColorComboBox::QColorComboBox(const QList<QColor> &colors, QWidget *parent) :
		QComboBox(parent), d(new QColorComboBoxPrivate(this)) {
	d->model->setColorsList(colors);
}

QColorListModel::NameFormat QColorComboBox::nameFormat() const {
	return d->model->nameFormat();
}

void QColorComboBox::setNameFormat(QColorListModel::NameFormat nameFormat) {
	d->model->setNameFormat(nameFormat);
	d->updateValidator();
}

QStringList QColorComboBox::colorListNames() const {
	return d->model->colorListNames();
}

void QColorComboBox::setColorListNames(const QStringList &colorNames) {
	d->model->setColorListNames(colorNames);
}

QList<QColor> QColorComboBox::colorsList() const {
	return d->model->colorsList();
}

void QColorComboBox::setColorsList(const QList<QColor> &colors) {
	d->model->setColorsList(colors);
}

QString QColorComboBox::currentColorName() const {
	return currentData(QColorListModel::HexArgbName).toString();
}

void QColorComboBox::setCurrentColorName(const QString &colorName) {
	setCurrentIndex(findData(d->internalColorName(QColor(colorName)), QColorListModel::HexArgbName));
}

QString QColorComboBox::currentTextColorName() const {
	return d->internalColorName(QColor(currentText()));
}

void QColorComboBox::setCurrentTextColorName(const QString &colorName) {
	setCurrentText(d->internalColorName(QColor(colorName)));
}

void QColorComboBox::setEditTextColorName(const QString &colorName) {
	setEditText(d->internalColorName(QColor(colorName)));
}

QColor QColorComboBox::currentColor() const {
	return QColor(currentColorName());
}

void QColorComboBox::setCurrentColor(const QColor &color) {
	setCurrentColorName(d->internalColorName(color));
}

QColor QColorComboBox::currentTextColor() const {
	return QColor(currentTextColorName());
}

void QColorComboBox::setCurrentTextColor(const QColor &color) {
	setCurrentTextColorName(d->internalColorName(color));
}

void QColorComboBox::setEditTextColor(const QColor &color) {
	setEditTextColorName(d->internalColorName(color));
}

void QColorComboBox::paintEvent(QPaintEvent *event) {
	QComboBox::paintEvent(event);

	// there is no real way to be notified about "editable" property change so let check it here...
	d->updateValidator();
}

// --------------------------------------------------------------------
//  Caching utilities

namespace QtWidgetsExtraCache {
QCache<qint64, QIcon> cachedIcons;
}

bool QtWidgetsExtraCache::cachePixmap(const QString &key, const QPixmap &pixmap) {
	if (!QPixmapCache::insert(key, pixmap)) {
		qWarning("%s: Can not cache pixmap with key: %s", Q_FUNC_INFO, qPrintable(key));
		return false;
	}

	return true;
}

QPixmap QtWidgetsExtraCache::cachedPixmap(const QString &key) {
	QPixmap pixmap;
	QPixmapCache::find(key, &pixmap);
	return pixmap;
}

QPixmap QtWidgetsExtraCache::cachedPixmapColor(const QColor &color, const QSize &size) {
	const QString key = QString(QLatin1String("%1-%2x%3-pixmap"))
								.arg(color.name(QColor::HexArgb))
								.arg(size.width())
								.arg(size.height());
	QPixmap pixmap = QtWidgetsExtraCache::cachedPixmap(key);

	if (pixmap.isNull()) {
		pixmap = QPixmap(size);
		pixmap.fill(color);
		QtWidgetsExtraCache::cachePixmap(key, pixmap);
	}

	return pixmap;
}

bool QtWidgetsExtraCache::cacheIcon(const QString &key, const QIcon &icon) {
	if (icon.isNull()) {
		qWarning("%s: Can not cache null icon with key: %s", Q_FUNC_INFO, qPrintable(key));
		return false;
	}

	QIcon *ptr = new QIcon(icon);

	if (!cachedIcons.insert(qHash(key), ptr)) {
		qWarning("%s: Can not cache icon with key: %s", Q_FUNC_INFO, qPrintable(key));
		delete ptr;
		return false;
	}

	return true;
}

QIcon QtWidgetsExtraCache::cachedIcon(const QString &key) {
	QIcon *icon = cachedIcons.object(qHash(key));
	return icon ? *icon : QIcon();
}

QIcon QtWidgetsExtraCache::cachedIconColor(const QColor &color, const QSize &size) {
	const QString key = QString(QLatin1String("%1-%2x%3-icon"))
								.arg(color.name(QColor::HexArgb))
								.arg(size.width())
								.arg(size.height());
	QIcon icon = QtWidgetsExtraCache::cachedIcon(key);

	if (icon.isNull()) {
		QPixmap pixmap = QtWidgetsExtraCache::cachedPixmapColor(color, size);

		if (!pixmap.isNull()) {
			icon = QIcon(pixmap);
			QtWidgetsExtraCache::cacheIcon(key, icon);
		}
	}

	return icon;
}

// --------------------------------------------------------------------
//  Elided label

ElidedLabel::ElidedLabel(QWidget *pParent) :
		QLabel(pParent),
		mElideMode(Qt::ElideLeft) {}

ElidedLabel::ElidedLabel(const QString &text, QWidget *pParent) :
		QLabel(text, pParent),
		mElideMode(Qt::ElideLeft) {}

ElidedLabel::~ElidedLabel() {}

void ElidedLabel::setElideMode(Qt::TextElideMode mode) {
	if (mode != mElideMode) {
		// Update the elide mode
		mElideMode = mode;

		// Update the elided text
		QString currentText = text();
		if (currentText.isEmpty() == false) {
			int textMargin = margin();
			QRect textRect = contentsRect();
			textRect.adjust(textMargin, textMargin, textMargin, textMargin);
			mElidedText = fontMetrics().elidedText(currentText, mElideMode, textRect.width());
		}

		// Redraw the button with the updated text
		repaint();
	}
}

Qt::TextElideMode ElidedLabel::getElideMode() const {
	return mElideMode;
}

void ElidedLabel::paintEvent(QPaintEvent *pEvent) {
	QString currentText = text();
	if (currentText.isEmpty() == false) // Drawing text
	{
		bool updateElidedText = false;
		int textMargin = margin();

		QRect textRect = contentsRect();
		textRect.adjust(textMargin, textMargin, textMargin, textMargin);
		if (textRect != mPreviousRect) {
			mPreviousRect = textRect;
			updateElidedText = true;
		}

		if (currentText != mPreviousText) {
			mPreviousText = currentText;
			updateElidedText = true;
		}

		if (updateElidedText == true) {
			mElidedText = fontMetrics().elidedText(currentText, mElideMode, textRect.width());
		}

		// Draw elided text
		if (currentText != mElidedText) {
			QPainter p(this);
			p.drawText(textRect, mElidedText);
			return;
		}
	}

	// Draw a picture, pixmap, movie, or non-elided text, which may have an associated alignment
	QLabel::paintEvent(pEvent);
}

// --------------------------------------------------------------------
//  QActionPushButton button

QActionPushButton::QActionPushButton(QWidget *parent) :
		QPushButton(parent), m_style(Qt::ActionPushButtonTextBesideIcon), m_action(0) {
}

void QActionPushButton::setDefaultAction(QAction *action) {
	if (m_action != action) {
		if (m_action) {
			disconnect(this, SIGNAL(clicked()), m_action, SLOT(trigger()));
			disconnect(m_action, SIGNAL(changed()), this, SLOT(updateStyle()));
		}
		m_action = action;
		connect(this, SIGNAL(clicked()), m_action, SLOT(trigger()));
		connect(m_action, SIGNAL(changed()), this, SLOT(updateStyle()));
		updateStyle();
	}
}

void QActionPushButton::setActionPushButtonStyle(Qt::ActionPushButtonStyle style) {
	if (m_style != style) {
		m_style = style;
		updateStyle();
	}
}

void QActionPushButton::updateStyle() {
	if (!m_action)
		return;
	switch (m_style) {
		case Qt::ActionPushButtonIconOnly: {
			setIcon(m_action->icon());
			setText("");
		}; break;
		case Qt::ActionPushButtonTextOnly: {
			setIcon(QIcon());
			setText(m_action->text());
		}; break;
		case Qt::ActionPushButtonTextBesideIcon: {
			setIcon(m_action->icon());
			setText(m_action->text());
		}; break;
	}
	setEnabled(m_action->isEnabled());
}

#include "extrawidgets.moc"
