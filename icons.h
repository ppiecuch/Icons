#ifndef ICONS_H
#define ICONS_H

#include <QMainWindow>
#include <QActionGroup>
#include <QScopedPointer>

#include <memory>
#include <vector>

#include "library/lib_svgiconlist.h"
#include "iconmodel.h"

namespace Ui {
class MainWindow;
}

class IconGrid;

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow() override;

private slots:
	void onCollectionChanged(const QString &name);
	void onStyleChanged(const QString &styleName);
	void onBitmapSizeChanged(int size);
	void onIconSelected(int index, const QString &name);
	void onIconDoubleClicked(int index, const QString &name);

	void onCopySvg();
	void onCopyPng();
	void onExport();
	void onAbout();

	void setSmallIcons();
	void setMediumIcons();
	void setLargeIcons();

private:
	void setupConnections();
	void loadCollections();
	void registerBuiltinCollections();
	void loadCurrentCollection();
	void updateAvailableStyles();

	QScopedPointer<Ui::MainWindow> m_ui;
	QActionGroup *m_iconSizeGroup;

	std::vector<std::unique_ptr<SVGIconList>> m_iconLists;
	std::vector<std::unique_ptr<BitmapIconList>> m_bitmapLists;
	IconList *m_currentList = nullptr;

	QString m_currentCollectionId;
	IconStyle m_currentStyle = IconStyle::Outline;
	bool m_isBitmapCollection = false;
	int m_currentBitmapSize = 32;

	int m_selectedIndex = -1;
	QString m_selectedName;
};

#endif // ICONS_H
