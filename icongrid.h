#ifndef ICONGRID_H
#define ICONGRID_H

#include <QWidget>
#include <QListView>
#include <QStyledItemDelegate>
#include <QLineEdit>
#include <QComboBox>
#include <QToolButton>
#include <QCheckBox>
#include <QLabel>
#include <QListWidget>
#include <QTableWidget>
#include <QStackedWidget>
#include <QButtonGroup>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QColorDialog>
#include <QMenu>

#include "iconmodel.h"
#include "extrawidgets.h"

// Info for icon export
struct ExportIconInfo {
	QString name;
	QPixmap pixmap;
	QString svg;
	QString style;
	int size;
};

// Custom delegate for rendering icons in the grid
class IconDelegate : public QStyledItemDelegate {
	Q_OBJECT

public:
	explicit IconDelegate(QObject *parent = nullptr);

	void paint(QPainter *painter, const QStyleOptionViewItem &option,
			   const QModelIndex &index) const override;

	QSize sizeHint(const QStyleOptionViewItem &option,
				   const QModelIndex &index) const override;

	void setIconSize(int size);
	int iconSize() const;

	void setShowNames(bool show);
	bool showNames() const;

private:
	int m_iconSize = 32;
	bool m_showNames = true;
	int m_padding = 8;
	int m_nameHeight = 32;  // Height for up to 2 lines of text
};

// Search bar widget
class SearchBar : public QWidget {
	Q_OBJECT

public:
	explicit SearchBar(QWidget *parent = nullptr);

	QString text() const;
	void clear();

signals:
	void textChanged(const QString &text);

private:
	QLineEdit *m_lineEdit;
	QToolButton *m_clearButton;
};

// Toolbar for icon controls
class IconToolBar : public QWidget {
	Q_OBJECT

public:
	explicit IconToolBar(QWidget *parent = nullptr);

	void setCollections(const QStringList &names);
	QString currentCollection() const;

	void setStyles(const QStringList &styles);
	QString currentStyle() const;

	QColor fillColor() const;
	QColor toneColor() const;
	QColor backgroundColor() const;

	int iconSize() const;

	void setTwoToneMode(bool enabled);

	void setBitmapSizes(const QList<int> &sizes);
	int currentBitmapSize() const;

	void setBitmapMode(bool isBitmap);

signals:
	void collectionChanged(const QString &name);
	void fillColorChanged(const QColor &color);
	void toneColorChanged(const QColor &color);
	void backgroundColorChanged(const QColor &color);
	void iconSizeChanged(int size);
	void styleChanged(const QString &style);
	void bitmapSizeChanged(int size);

public slots:
	void setFillColor(const QColor &color);
	void setToneColor(const QColor &color);
	void setBackgroundColor(const QColor &color);

private slots:
	void onFillColorClicked();
	void onToneColorClicked();
	void onBackgroundColorClicked();
	void onIconSizeChanged(int index);
	void onBitmapSizeChanged(int index);

private:
	void updateFillColorButton();
	void updateToneColorButton();
	void updateBgColorButton();

	QComboBox *m_collectionCombo;
	QComboBox *m_styleCombo;
	QComboBox *m_sizeCombo;
	QLabel *m_sizeLabel;
	QComboBox *m_bitmapSizeCombo;
	QLabel *m_bitmapSizeLabel;
	QToolButton *m_fillColorButton;
	QToolButton *m_toneColorButton;
	QToolButton *m_bgColorButton;
	QColor m_fillColor = Qt::black;
	QColor m_toneColor = QColor(200, 200, 200);
	QColor m_backgroundColor = Qt::transparent;
};

// Preview panel for selected icon
class IconPreview : public QWidget {
	Q_OBJECT

public:
	explicit IconPreview(QWidget *parent = nullptr);

	void setIcon(const QPixmap &pixmap, const QString &name, const QString &svg,
				 const QString &style, int size, const QStringList &aliases = QStringList(),
				 const QStringList &tags = QStringList(), const QString &category = QString());
	void clear();

	void addToExportList(const ExportIconInfo &info);
	void clearExportList();
	QList<ExportIconInfo> exportList() const;

	void setBitmapMode(bool isBitmap);

	// Entity support
	void setEntities(const EntityMap &entities);
	EntityMap currentEntities() const;
	bool hasEntities() const;

signals:
	void copySvgRequested();
	void copyPngRequested();
	void exportRequested(bool merge);
	void entitiesChanged(const EntityMap &entities);

private slots:
	void onButtonClicked(int index);
	void onEntityValueChanged(int row, int column);

private:
	void updateEntitiesTable();

	QLabel *m_iconLabel;
	QLabel *m_nameLabel;
	QLabel *m_aliasesLabel;
	ActiveLabel *m_copySvgButton;
	ActiveLabel *m_copyPngButton;
	ActiveLabel *m_copyInfoButton;
	ActiveLabel *m_exportButton;
	QString m_currentSvg;
	QString m_currentName;
	QString m_currentStyle;
	int m_currentSize = 0;
	QStringList m_currentAliases;
	QStringList m_currentTags;
	QString m_currentCategory;
	QPixmap m_currentPixmap;

	// Export content
	QWidget *m_exportWidget;
	QListWidget *m_exportListWidget;
	ActiveLabel *m_clearExportButton;
	QList<ExportIconInfo> m_exportList;
	QLabel *m_exportOptionsLabel;
	QCheckBox *m_exportMergedCheckbox;

	// View switcher (Export/Entities buttons + stacked widget)
	QWidget *m_viewSwitcher;
	QToolButton *m_exportViewButton;
	QToolButton *m_entitiesViewButton;
	QStackedWidget *m_stackedWidget;
	QWidget *m_entitiesWidget;
	QTableWidget *m_entitiesTable;
	EntityMap m_entities;
};

// Main icon grid widget combining all components
class IconGrid : public QWidget {
	Q_OBJECT

public:
	explicit IconGrid(QWidget *parent = nullptr);
	~IconGrid() override;

	void setIconList(IconList *list);
	IconModel *model() const;
	IconPreview *preview() const;
	IconToolBar *toolBar() const;

	void setIconSize(int size);
	int iconSize() const;

signals:
	void iconSelected(int index, const QString &name, const QStringList &tags, const QString &category);

public slots:
	void setFilter(const QString &filter);
	void setFillColor(const QColor &color);
	void setToneColor(const QColor &color);
	void setBackgroundColor(const QColor &color);

private slots:
	void onSelectionChanged(const QModelIndex &current, const QModelIndex &previous);
	void onDoubleClicked(const QModelIndex &index);
	void onContextMenu(const QPoint &pos);
	void onAddToExport();

private:
	void addCurrentToExportList();

	QListView *m_listView;
	IconModel *m_model;
	IconDelegate *m_delegate;
	SearchBar *m_searchBar;
	IconToolBar *m_toolBar;
	IconPreview *m_preview;
	QMenu *m_contextMenu;
	QAction *m_addToExportAction;
};

#endif // ICONGRID_H
