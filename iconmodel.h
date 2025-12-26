#ifndef ICONMODEL_H
#define ICONMODEL_H

#include <QAbstractListModel>
#include <QColor>
#include <QPixmap>
#include <QSvgRenderer>
#include <QPainter>
#include <QCache>

#include <memory>
#include <vector>
#include <map>

#include "library/lib_svgiconlist.h"

// Icon style types (matching original Delphi implementation)
enum class IconStyle {
	Outline,
	Filled,
	TwoTone  // Composite of Outline + Filled
};

// Custom roles for icon data
enum IconRoles {
	IconNameRole = Qt::UserRole + 1,
	IconSvgRole,
	IconIndexRole,
	IconLibraryRole
};

// Represents a single icon entry
struct IconEntry {
	QString name;
	int index;
	QString libraryName;
};

// Model for displaying icons in a list/grid view
class IconModel : public QAbstractListModel {
	Q_OBJECT

public:
	explicit IconModel(QObject *parent = nullptr);
	~IconModel() override;

	// QAbstractListModel interface
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QHash<int, QByteArray> roleNames() const override;

	// Icon list management - accepts both SVG and Bitmap lists
	void setIconList(IconList *list);
	IconList *iconList() const;

	// Convenience accessors
	SVGIconList *svgIconList() const;
	BitmapIconList *bitmapIconList() const;
	bool isBitmapMode() const;

	// Display settings
	void setIconSize(int size);
	int iconSize() const;

	void setFillColor(const QColor &color);
	QColor fillColor() const;

	void setToneColor(const QColor &color);
	QColor toneColor() const;
	bool isTwoTone() const;

	void setBackgroundColor(const QColor &color);
	QColor backgroundColor() const;

	// Grayscale mode (for bitmap icons)
	void setGrayscale(bool enabled);
	bool isGrayscale() const;

	// Filtering
	void setFilter(const QString &filter);
	QString filter() const;

	// Get icon at index
	QPixmap getIconPixmap(int index) const;
	QPixmap getIconPixmapAtSize(int index, int size) const;
	QString getIconSvg(int index) const;
	QString getIconName(int index) const;
	QStringList getIconAliases(int index) const;
	QStringList getIconTags(int index) const;
	QString getIconCategory(int index) const;

	// Entity support
	EntityMap getIconEntities(int index) const;
	bool iconHasEntities(int index) const;
	void setIconEntities(int index, const EntityMap &entities);
	EntityMap currentEntities(int index) const;

public slots:
	void refresh();
	void clearCache();

signals:
	void iconListChanged();
	void filterChanged();

private:
	QPixmap renderIcon(int index) const;
	void rebuildFilteredList();

	IconList *m_iconList = nullptr;
	std::vector<IconEntry> m_allIcons;
	std::vector<int> m_filteredIndices;
	QString m_filter;

	int m_iconSize = 32;
	QColor m_fillColor = clNone;
	QColor m_toneColor = QColor(200, 200, 200);
	QColor m_backgroundColor = Qt::transparent;
	bool m_grayscale = false;

	mutable QCache<int, QPixmap> m_pixmapCache;
	mutable QMap<int, EntityMap> m_customEntities;  // Custom entity values per icon
};

// TwoTone icon list - combines outline and filled lists with name-based mapping
class TwoToneIconList : public SVGTwoToneIconList {
	std::unique_ptr<SVGIconList> m_filled;
	std::unique_ptr<SVGIconList> m_outline;
	QColor m_fillColor = Qt::black;
	QColor m_toneColor = QColor(200, 200, 200);

	// Mapping: index in this list -> {outlineIdx, filledIdx}
	struct IconMapping {
		int outlineIdx;
		int filledIdx;  // -1 if no filled version exists
	};
	std::vector<IconMapping> m_mapping;
	QMap<QString, int> m_filledNameToIdx;

	void buildMapping() {
		// Build name->index map for filled icons
		m_filledNameToIdx.clear();
		for (int i = 0; i < m_filled->getCount(); ++i) {
			QString name = m_filled->getName(i);
			// Remove "-fill" suffix if present for matching
			if (name.endsWith("-fill"))
				name = name.left(name.length() - 5);
			m_filledNameToIdx[name.toLower()] = i;
		}

		// Build mapping for outline icons that have filled counterparts
		m_mapping.clear();
		for (int i = 0; i < m_outline->getCount(); ++i) {
			QString outlineName = m_outline->getName(i).toLower();
			auto it = m_filledNameToIdx.find(outlineName);
			if (it != m_filledNameToIdx.end()) {
				m_mapping.push_back({i, it.value()});
			}
		}
	}

public:
	TwoToneIconList(SVGIconList *filled, SVGIconList *outline)
		: m_filled(filled), m_outline(outline) {
		// Set initial colors on sub-lists
		m_outline->setFillColor(m_fillColor);
		m_filled->setFillColor(m_toneColor);
		// Build name-based mapping
		buildMapping();
	}

	int getCount() const override { return static_cast<int>(m_mapping.size()); }

	QString getName(int index) const override {
		if (index < 0 || index >= static_cast<int>(m_mapping.size()))
			return QString();
		return m_outline->getName(m_mapping[index].outlineIdx);
	}

	QString getBody(int index) const override {
		if (index < 0 || index >= static_cast<int>(m_mapping.size()))
			return QString();
		const auto &map = m_mapping[index];
		QString body = m_filled->getBody(map.filledIdx);
		body += m_outline->getBody(map.outlineIdx);
		return body;
	}

	QString getSource(int index) const override {
		if (index < 0 || index >= static_cast<int>(m_mapping.size()))
			return QString();

		const auto &map = m_mapping[index];

		// Get body content (paths) from each list
		QString filledBody = m_filled->getBody(map.filledIdx);
		QString outlineBody = m_outline->getBody(map.outlineIdx);

		// Build fill color strings
		QString toneColorStr = (m_toneColor.isValid() && m_toneColor.alpha() > 0)
			? m_toneColor.name() : "#c8c8c8";
		QString fillColorStr = (m_fillColor.isValid() && m_fillColor.alpha() > 0)
			? m_fillColor.name() : "#000000";

		// Wrap each layer in a group with the appropriate fill color
		QString filledLayer = QString("<g fill=\"%1\">%2</g>").arg(toneColorStr, filledBody);
		QString outlineLayer = QString("<g fill=\"%1\">%2</g>").arg(fillColorStr, outlineBody);

		// Combine with SVG wrapper
		return QString("<svg viewBox=\"0 0 %1 %1\" xmlns=\"http://www.w3.org/2000/svg\">%2%3</svg>")
			.arg(m_outline->getBaseSize())
			.arg(filledLayer)
			.arg(outlineLayer);
	}

	QColor getFillColor() const override { return m_fillColor; }
	void setFillColor(QColor value) override {
		m_fillColor = value;
		m_outline->setFillColor(value);  // Primary color goes to outline layer
	}

	QColor getToneColor() const override { return m_toneColor; }
	void setToneColor(QColor value) override {
		m_toneColor = value;
		m_filled->setFillColor(value);  // Tone color goes to filled layer
	}

	QString getLibraryName() const override { return m_outline->getLibraryName() + " TwoTone"; }
	int getBaseSize() const override { return m_outline->getBaseSize(); }
};

// Bitmap style types
enum class BitmapStyle {
	Color,
	Grayscale
};

// Represents an SVG icon collection with multiple styles
struct IconCollection {
	QString id;
	QString displayName;
	int baseSize;
	std::map<IconStyle, std::function<SVGIconList*()>> styles;

	bool hasStyle(IconStyle style) const { return styles.count(style) > 0; }
	QList<IconStyle> availableStyles() const {
		QList<IconStyle> result;
		for (const auto &pair : styles)
			result.append(pair.first);
		return result;
	}
};

// Represents a bitmap icon collection with multiple sizes
struct BitmapCollection {
	QString id;
	QString displayName;
	QList<int> availableSizes;
	std::function<BitmapIconList*(int size)> factory;

	int defaultSize() const { return availableSizes.isEmpty() ? 32 : availableSizes.first(); }
};

// Registry of available icon collections
class IconCollectionRegistry : public QObject {
	Q_OBJECT

public:
	static IconCollectionRegistry &instance();

	// SVG collections
	void registerCollection(const IconCollection &collection);
	QList<IconCollection> collections() const;
	const IconCollection *findCollection(const QString &id) const;
	SVGIconList *createIconList(const QString &collectionId, IconStyle style) const;

	// Bitmap collections
	void registerBitmapCollection(const BitmapCollection &collection);
	QList<BitmapCollection> bitmapCollections() const;
	const BitmapCollection *findBitmapCollection(const QString &id) const;
	BitmapIconList *createBitmapList(const QString &collectionId, int size) const;

	// Combined list for UI
	QStringList allCollectionNames() const;
	bool isBitmapCollection(const QString &displayName) const;

signals:
	void collectionsChanged();

private:
	IconCollectionRegistry() = default;
	QList<IconCollection> m_collections;
	QList<BitmapCollection> m_bitmapCollections;
};

// Helper to convert IconStyle to/from string
inline QString iconStyleToString(IconStyle style) {
	switch (style) {
		case IconStyle::Outline: return "Outline";
		case IconStyle::Filled: return "Filled";
		case IconStyle::TwoTone: return "TwoTone";
	}
	return "Outline";
}

inline IconStyle stringToIconStyle(const QString &str) {
	if (str == "Filled") return IconStyle::Filled;
	if (str == "TwoTone") return IconStyle::TwoTone;
	return IconStyle::Outline;
}

#endif // ICONMODEL_H
