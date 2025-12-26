#include "iconmodel.h"

#include <QDebug>
#include <QSvgRenderer>
#include <QPainter>
#include <QRegularExpression>

// ============================================================================
// IconModel
// ============================================================================

IconModel::IconModel(QObject *parent)
	: QAbstractListModel(parent)
	, m_pixmapCache(500) // Cache up to 500 rendered icons
{
}

IconModel::~IconModel() = default;

int IconModel::rowCount(const QModelIndex &parent) const {
	if (parent.isValid())
		return 0;
	return static_cast<int>(m_filteredIndices.size());
}

QVariant IconModel::data(const QModelIndex &index, int role) const {
	if (!index.isValid() || index.row() >= static_cast<int>(m_filteredIndices.size()))
		return QVariant();

	int actualIndex = m_filteredIndices[index.row()];
	const IconEntry &entry = m_allIcons[actualIndex];

	switch (role) {
		case Qt::DisplayRole:
		case IconNameRole:
			return entry.name;

		case Qt::DecorationRole:
			return renderIcon(actualIndex);

		case IconSvgRole:
			return getIconSvg(actualIndex);

		case IconIndexRole:
			return entry.index;

		case IconLibraryRole:
			return entry.libraryName;

		case Qt::ToolTipRole:
			return entry.name;

		default:
			return QVariant();
	}
}

QHash<int, QByteArray> IconModel::roleNames() const {
	QHash<int, QByteArray> roles = QAbstractListModel::roleNames();
	roles[IconNameRole] = "iconName";
	roles[IconSvgRole] = "iconSvg";
	roles[IconIndexRole] = "iconIndex";
	roles[IconLibraryRole] = "iconLibrary";
	return roles;
}

void IconModel::setIconList(IconList *list) {
	beginResetModel();

	m_iconList = list;
	m_allIcons.clear();
	m_filteredIndices.clear();
	m_pixmapCache.clear();

	if (m_iconList) {
		int count = m_iconList->getCount();
		m_allIcons.reserve(count);

		QString libraryName = m_iconList->getLibraryName();

		for (int i = 0; i < count; ++i) {
			IconEntry entry;
			entry.name = m_iconList->getName(i);
			entry.index = i;
			entry.libraryName = libraryName;
			m_allIcons.push_back(entry);
		}

		rebuildFilteredList();
	}

	endResetModel();
	emit iconListChanged();
}

IconList *IconModel::iconList() const {
	return m_iconList;
}

SVGIconList *IconModel::svgIconList() const {
	if (m_iconList && m_iconList->isSVG())
		return static_cast<SVGIconList*>(m_iconList);
	return nullptr;
}

BitmapIconList *IconModel::bitmapIconList() const {
	if (m_iconList && m_iconList->isBitmap())
		return static_cast<BitmapIconList*>(m_iconList);
	return nullptr;
}

bool IconModel::isBitmapMode() const {
	return m_iconList && m_iconList->isBitmap();
}

void IconModel::setIconSize(int size) {
	if (m_iconSize != size) {
		m_iconSize = size;
		m_pixmapCache.clear();
		emit dataChanged(index(0), index(rowCount() - 1), {Qt::DecorationRole});
	}
}

int IconModel::iconSize() const {
	return m_iconSize;
}

void IconModel::setFillColor(const QColor &color) {
	if (m_fillColor != color) {
		m_fillColor = color;
		if (auto *svg = svgIconList()) {
			svg->setFillColor(color);
		}
		m_pixmapCache.clear();
		emit dataChanged(index(0), index(rowCount() - 1), {Qt::DecorationRole});
	}
}

QColor IconModel::fillColor() const {
	return m_fillColor;
}

void IconModel::setGrayscale(bool enabled) {
	if (m_grayscale != enabled) {
		m_grayscale = enabled;
		if (auto *bitmap = bitmapIconList()) {
			bitmap->setGrayscale(enabled);
		}
		m_pixmapCache.clear();
		emit dataChanged(index(0), index(rowCount() - 1), {Qt::DecorationRole});
	}
}

bool IconModel::isGrayscale() const {
	return m_grayscale;
}

void IconModel::setBackgroundColor(const QColor &color) {
	if (m_backgroundColor != color) {
		m_backgroundColor = color;
		m_pixmapCache.clear();
		emit dataChanged(index(0), index(rowCount() - 1), {Qt::DecorationRole});
	}
}

QColor IconModel::backgroundColor() const {
	return m_backgroundColor;
}

void IconModel::setFilter(const QString &filter) {
	if (m_filter != filter) {
		beginResetModel();
		m_filter = filter;
		rebuildFilteredList();
		endResetModel();
		emit filterChanged();
	}
}

QString IconModel::filter() const {
	return m_filter;
}

QPixmap IconModel::getIconPixmap(int index) const {
	return renderIcon(index);
}

QString IconModel::getIconSvg(int index) const {
	if (!m_iconList || index < 0 || index >= static_cast<int>(m_allIcons.size()))
		return QString();
	if (auto *svg = svgIconList()) {
		QString source = svg->getSource(index);
		// Resolve entities if present
		EntityMap entities = currentEntities(index);
		if (!entities.isEmpty()) {
			source = SVGIconList::resolveEntities(source, entities);
		}
		return source;
	}
	return QString(); // No SVG for bitmap icons
}

QString IconModel::getIconName(int index) const {
	if (index < 0 || index >= static_cast<int>(m_allIcons.size()))
		return QString();
	return m_allIcons[index].name;
}

QStringList IconModel::getIconAliases(int index) const {
	if (!m_iconList || index < 0 || index >= static_cast<int>(m_allIcons.size()))
		return QStringList();
	if (auto *bitmap = bitmapIconList())
		return bitmap->getAliases(index);
	return QStringList(); // No aliases for SVG icons
}

EntityMap IconModel::getIconEntities(int index) const {
	if (!m_iconList || index < 0 || index >= static_cast<int>(m_allIcons.size()))
		return EntityMap();
	if (auto *svg = svgIconList())
		return svg->getEntities(index);
	return EntityMap();
}

bool IconModel::iconHasEntities(int index) const {
	return !getIconEntities(index).isEmpty();
}

void IconModel::setIconEntities(int index, const EntityMap &entities) {
	if (index < 0 || index >= static_cast<int>(m_allIcons.size()))
		return;
	m_customEntities[index] = entities;
	m_pixmapCache.remove(index);  // Force re-render
}

EntityMap IconModel::currentEntities(int index) const {
	if (m_customEntities.contains(index))
		return m_customEntities[index];
	return getIconEntities(index);
}

void IconModel::refresh() {
	m_pixmapCache.clear();
	emit dataChanged(index(0), index(rowCount() - 1));
}

void IconModel::clearCache() {
	m_pixmapCache.clear();
}

QPixmap IconModel::renderIcon(int index) const {
	if (!m_iconList || index < 0 || index >= static_cast<int>(m_allIcons.size()))
		return QPixmap();

	// Check cache first
	if (QPixmap *cached = m_pixmapCache.object(index))
		return *cached;

	int actualIndex = m_allIcons[index].index;
	QPixmap pixmap;

	if (auto *bitmap = bitmapIconList()) {
		// Bitmap icon - get directly from resource
		pixmap = bitmap->getPixmap(actualIndex);
		if (!pixmap.isNull() && (pixmap.width() != m_iconSize || pixmap.height() != m_iconSize)) {
			pixmap = pixmap.scaled(m_iconSize, m_iconSize,
								   Qt::KeepAspectRatio, Qt::SmoothTransformation);
		}
	} else if (auto *svg = svgIconList()) {
		// SVG icon - render from source
		QString svgSource = svg->getSource(actualIndex);
		if (svgSource.isEmpty())
			return QPixmap();

		// Resolve entities if present
		EntityMap entities = currentEntities(index);
		if (!entities.isEmpty()) {
			svgSource = SVGIconList::resolveEntities(svgSource, entities);
		}

		QSvgRenderer renderer(svgSource.toUtf8());
		if (!renderer.isValid())
			return QPixmap();

		pixmap = QPixmap(m_iconSize, m_iconSize);
		pixmap.fill(m_backgroundColor);

		QPainter painter(&pixmap);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setRenderHint(QPainter::SmoothPixmapTransform);
		renderer.render(&painter);
		painter.end();
	}

	if (pixmap.isNull())
		return pixmap;

	// Apply background for bitmap icons if needed
	if (isBitmapMode() && m_backgroundColor.alpha() > 0) {
		QPixmap withBg(pixmap.size());
		withBg.fill(m_backgroundColor);
		QPainter painter(&withBg);
		painter.drawPixmap(0, 0, pixmap);
		painter.end();
		pixmap = withBg;
	}

	// Cache the result
	m_pixmapCache.insert(index, new QPixmap(pixmap));

	return pixmap;
}

void IconModel::rebuildFilteredList() {
	m_filteredIndices.clear();

	if (m_filter.isEmpty()) {
		// No filter - show all icons
		m_filteredIndices.reserve(m_allIcons.size());
		for (size_t i = 0; i < m_allIcons.size(); ++i) {
			m_filteredIndices.push_back(static_cast<int>(i));
		}
	} else {
		// Filter by name (case-insensitive)
		QRegularExpression regex(
			QRegularExpression::escape(m_filter),
			QRegularExpression::CaseInsensitiveOption
		);

		for (size_t i = 0; i < m_allIcons.size(); ++i) {
			if (m_allIcons[i].name.contains(regex)) {
				m_filteredIndices.push_back(static_cast<int>(i));
			}
		}
	}
}

// ============================================================================
// IconCollectionRegistry
// ============================================================================

IconCollectionRegistry &IconCollectionRegistry::instance() {
	static IconCollectionRegistry registry;
	return registry;
}

void IconCollectionRegistry::registerCollection(const IconCollection &collection) {
	m_collections.append(collection);
	emit collectionsChanged();
}

QList<IconCollection> IconCollectionRegistry::collections() const {
	return m_collections;
}

const IconCollection *IconCollectionRegistry::findCollection(const QString &id) const {
	for (const auto &coll : m_collections) {
		if (coll.id == id)
			return &coll;
	}
	return nullptr;
}

SVGIconList *IconCollectionRegistry::createIconList(const QString &collectionId, IconStyle style) const {
	const IconCollection *coll = findCollection(collectionId);
	if (!coll)
		return nullptr;

	// Handle TwoTone - requires both Outline and Filled
	if (style == IconStyle::TwoTone) {
		if (coll->hasStyle(IconStyle::Filled) && coll->hasStyle(IconStyle::Outline)) {
			auto filledFactory = coll->styles.at(IconStyle::Filled);
			auto outlineFactory = coll->styles.at(IconStyle::Outline);
			return new TwoToneIconList(filledFactory(), outlineFactory());
		}
		// Fallback to Outline if TwoTone not possible
		style = IconStyle::Outline;
	}

	auto it = coll->styles.find(style);
	if (it != coll->styles.end())
		return it->second();

	// Try fallback styles
	if (coll->hasStyle(IconStyle::Outline))
		return coll->styles.at(IconStyle::Outline)();
	if (coll->hasStyle(IconStyle::Filled))
		return coll->styles.at(IconStyle::Filled)();

	return nullptr;
}

void IconCollectionRegistry::registerBitmapCollection(const BitmapCollection &collection) {
	m_bitmapCollections.append(collection);
	emit collectionsChanged();
}

QList<BitmapCollection> IconCollectionRegistry::bitmapCollections() const {
	return m_bitmapCollections;
}

const BitmapCollection *IconCollectionRegistry::findBitmapCollection(const QString &id) const {
	for (const auto &coll : m_bitmapCollections) {
		if (coll.id == id)
			return &coll;
	}
	return nullptr;
}

BitmapIconList *IconCollectionRegistry::createBitmapList(const QString &collectionId, int size) const {
	const BitmapCollection *coll = findBitmapCollection(collectionId);
	if (!coll || !coll->factory)
		return nullptr;

	// Use requested size or default
	int actualSize = coll->availableSizes.contains(size) ? size : coll->defaultSize();
	return coll->factory(actualSize);
}

QStringList IconCollectionRegistry::allCollectionNames() const {
	QStringList names;
	for (const auto &coll : m_collections)
		names.append(coll.displayName);
	for (const auto &coll : m_bitmapCollections)
		names.append(coll.displayName);
	return names;
}

bool IconCollectionRegistry::isBitmapCollection(const QString &displayName) const {
	for (const auto &coll : m_bitmapCollections) {
		if (coll.displayName == displayName)
			return true;
	}
	return false;
}
