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
	QColor m_backgroundColor = Qt::transparent;
	bool m_grayscale = false;

	mutable QCache<int, QPixmap> m_pixmapCache;
	mutable QMap<int, EntityMap> m_customEntities;  // Custom entity values per icon
};

// TwoTone icon list - combines outline and filled lists
class TwoToneIconList : public SVGIconList {
	std::unique_ptr<SVGIconList> m_filled;
	std::unique_ptr<SVGIconList> m_outline;
	QColor m_fillColor = clNone;
	QColor m_secondaryColor = QColor(200, 200, 200);

public:
	TwoToneIconList(SVGIconList *filled, SVGIconList *outline)
		: m_filled(filled), m_outline(outline) {}

	int getCount() const override { return m_filled->getCount(); }
	QString getName(int index) const override { return m_filled->getName(index); }

	QString getBody(int index) const override {
		// Combine filled (background) and outline (foreground)
		QString body = m_filled->getBody(index);
		body += m_outline->getBody(index);
		return body;
	}

	QString getSource(int index) const override {
		// TwoTone: filled part uses secondary color, outline uses primary
		QString filledBody = m_filled->getBody(index);
		QString outlineBody = m_outline->getBody(index);

		// Apply secondary color to filled, primary to outline
		if (m_secondaryColor.isValid() && m_secondaryColor != clNone)
			filledBody.replace(" fill=\"#212121\"", QString(" fill=\"%1\"").arg(m_secondaryColor.name()));
		if (m_fillColor.isValid() && m_fillColor != clNone)
			outlineBody.replace(" fill=\"#212121\"", QString(" fill=\"%1\"").arg(m_fillColor.name()));

		return QString("<svg viewBox=\"0 0 %1 %1\" xmlns=\"http://www.w3.org/2000/svg\">")
			.arg(m_filled->getBaseSize()) + filledBody + outlineBody + "</svg>";
	}

	QColor getFillColor() const override { return m_fillColor; }
	void setFillColor(QColor value) override { m_fillColor = value; }

	QColor getSecondaryColor() const { return m_secondaryColor; }
	void setSecondaryColor(QColor value) { m_secondaryColor = value; }

	QString getLibraryName() const override { return m_filled->getLibraryName() + " TwoTone"; }
	int getBaseSize() const override { return m_filled->getBaseSize(); }
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
