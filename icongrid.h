#ifndef ICONGRID_H
#define ICONGRID_H

#include <QWidget>
#include <QListView>
#include <QStyledItemDelegate>
#include <QLineEdit>
#include <QComboBox>
#include <QToolButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QColorDialog>

#include "iconmodel.h"

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
	int m_nameHeight = 20;
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
	QColor backgroundColor() const;

	int iconSize() const;

	void setBitmapSizes(const QList<int> &sizes);
	int currentBitmapSize() const;

signals:
	void collectionChanged(const QString &name);
	void fillColorChanged(const QColor &color);
	void backgroundColorChanged(const QColor &color);
	void iconSizeChanged(int size);
	void styleChanged(const QString &style);
	void bitmapSizeChanged(int size);

public slots:
	void setFillColor(const QColor &color);
	void setBackgroundColor(const QColor &color);

private slots:
	void onFillColorClicked();
	void onBackgroundColorClicked();
	void onIconSizeChanged(int index);
	void onBitmapSizeChanged(int index);

private:
	void updateFillColorButton();
	void updateBgColorButton();

	QComboBox *m_collectionCombo;
	QComboBox *m_styleCombo;
	QComboBox *m_sizeCombo;
	QLabel *m_sizeLabel;
	QComboBox *m_bitmapSizeCombo;
	QLabel *m_bitmapSizeLabel;
	QToolButton *m_fillColorButton;
	QToolButton *m_bgColorButton;
	QColor m_fillColor = Qt::black;
	QColor m_backgroundColor = Qt::transparent;
};

// Preview panel for selected icon
class IconPreview : public QWidget {
	Q_OBJECT

public:
	explicit IconPreview(QWidget *parent = nullptr);

	void setIcon(const QPixmap &pixmap, const QString &name, const QString &svg);
	void clear();

signals:
	void copySvgRequested();
	void copyPngRequested();
	void exportRequested();

private:
	QLabel *m_iconLabel;
	QLabel *m_nameLabel;
	QToolButton *m_copySvgButton;
	QToolButton *m_copyPngButton;
	QToolButton *m_exportButton;
	QString m_currentSvg;
	QPixmap m_currentPixmap;
};

// Main icon grid widget combining all components
class IconGrid : public QWidget {
	Q_OBJECT

public:
	explicit IconGrid(QWidget *parent = nullptr);
	~IconGrid() override;

	void setIconList(IconList *list);
	IconModel *model() const;

	void setIconSize(int size);
	int iconSize() const;

signals:
	void iconSelected(int index, const QString &name);
	void iconDoubleClicked(int index, const QString &name);

public slots:
	void setFilter(const QString &filter);
	void setFillColor(const QColor &color);
	void setBackgroundColor(const QColor &color);

private slots:
	void onSelectionChanged(const QModelIndex &current, const QModelIndex &previous);
	void onDoubleClicked(const QModelIndex &index);
	void updateStatusBar();

private:
	QListView *m_listView;
	IconModel *m_model;
	IconDelegate *m_delegate;
	SearchBar *m_searchBar;
	IconToolBar *m_toolBar;
	IconPreview *m_preview;
	QLabel *m_statusLabel;
};

#endif // ICONGRID_H
