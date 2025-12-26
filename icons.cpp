#include "icons.h"
#include "ui_icons.h"
#include "icongrid.h"
#include "iconmodel.h"

#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

// Generated icon list headers - Bootstrap
#include "library/lib_bootstrap_regular_16.h"
#include "library/lib_bootstrap_fill_16.h"

// Generated icon list headers - Tabler
#include "library/lib_tabler_outline_24.h"
#include "library/lib_tabler_filled_24.h"

// Generated icon list headers - Fluent UI (multiple sizes)
#include "library/lib_fluent_regular_16.h"
#include "library/lib_fluent_filled_16.h"
#include "library/lib_fluent_regular_20.h"
#include "library/lib_fluent_filled_20.h"
#include "library/lib_fluent_regular_24.h"
#include "library/lib_fluent_filled_24.h"
#include "library/lib_fluent_regular_32.h"
#include "library/lib_fluent_filled_32.h"

// Generated icon list headers - Breeze
#include "library/lib_breeze_actions_22.h"
#include "library/lib_breeze_apps_48.h"
#include "library/lib_breeze_places_22.h"
#include "library/lib_breeze_status_22.h"
#include "library/lib_breeze_devices_22.h"
#include "library/lib_breeze_mimetypes_22.h"

// Generated icon list headers - Oxygen (Bitmap)
#include "library/bitmap/lib_oxygen_16.h"
#include "library/bitmap/lib_oxygen_22.h"
#include "library/bitmap/lib_oxygen_32.h"
#include "library/bitmap/lib_oxygen_48.h"
#include "library/bitmap/lib_oxygen_64.h"
#include "library/bitmap/lib_oxygen_128.h"
#include "library/bitmap/lib_oxygen_256.h"

// Generated icon list headers - Oxygen5 (Bitmap)
#include "library/bitmap/lib_oxygen5_16.h"
#include "library/bitmap/lib_oxygen5_22.h"
#include "library/bitmap/lib_oxygen5_32.h"
#include "library/bitmap/lib_oxygen5_48.h"
#include "library/bitmap/lib_oxygen5_64.h"
#include "library/bitmap/lib_oxygen5_128.h"
#include "library/bitmap/lib_oxygen5_256.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, m_ui(new Ui::MainWindow)
{
	m_ui->setupUi(this);

	// Setup icon size action group
	m_iconSizeGroup = new QActionGroup(this);
	m_iconSizeGroup->addAction(m_ui->actionSmallIcons);
	m_iconSizeGroup->addAction(m_ui->actionMediumIcons);
	m_iconSizeGroup->addAction(m_ui->actionLargeIcons);
	m_iconSizeGroup->setExclusive(true);

	// Add permanent icon count label to status bar
	m_iconCountLabel = new QLabel(this);
	statusBar()->addPermanentWidget(m_iconCountLabel);

	setupConnections();
	registerBuiltinCollections();
	loadCollections();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupConnections() {
	// Menu actions
	connect(m_ui->actionCopySvg, &QAction::triggered, this, &MainWindow::onCopySvg);
	connect(m_ui->actionCopyPng, &QAction::triggered, this, &MainWindow::onCopyPng);
	connect(m_ui->actionExport, &QAction::triggered, this, &MainWindow::onExport);
	connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow::onAbout);

	// View actions
	connect(m_ui->actionSmallIcons, &QAction::triggered, this, &MainWindow::setSmallIcons);
	connect(m_ui->actionMediumIcons, &QAction::triggered, this, &MainWindow::setMediumIcons);
	connect(m_ui->actionLargeIcons, &QAction::triggered, this, &MainWindow::setLargeIcons);

	// Icon grid signals
	connect(m_ui->iconGrid, &IconGrid::iconSelected, this, &MainWindow::onIconSelected);

	// Connect model signals for icon count updates
	connect(m_ui->iconGrid->model(), &IconModel::iconListChanged, this, &MainWindow::updateIconCount);
	connect(m_ui->iconGrid->model(), &IconModel::filterChanged, this, &MainWindow::updateIconCount);
}

void MainWindow::registerBuiltinCollections() {
	auto &registry = IconCollectionRegistry::instance();

	// Bootstrap Icons 16 - has Outline + Filled + TwoTone
	registry.registerCollection({
		"bootstrap-16", "Bootstrap 16", 16,
		{
			{IconStyle::Outline, []() { return new BootstrapRegular16IconList(); }},
			{IconStyle::Filled, []() { return new BootstrapFill16IconList(); }}
		}
	});

	// Tabler Icons 24 - has Outline + Filled + TwoTone
	registry.registerCollection({
		"tabler-24", "Tabler 24", 24,
		{
			{IconStyle::Outline, []() { return new TablerOutline24IconList(); }},
			{IconStyle::Filled, []() { return new TablerFilled24IconList(); }}
		}
	});

	// Fluent UI Icons - multiple sizes, each with Regular (Outline) + Filled
	registry.registerCollection({
		"fluent-16", "Fluent UI 16", 16,
		{
			{IconStyle::Outline, []() { return new FluentRegular16IconList(); }},
			{IconStyle::Filled, []() { return new FluentFilled16IconList(); }}
		}
	});
	registry.registerCollection({
		"fluent-20", "Fluent UI 20", 20,
		{
			{IconStyle::Outline, []() { return new FluentRegular20IconList(); }},
			{IconStyle::Filled, []() { return new FluentFilled20IconList(); }}
		}
	});
	registry.registerCollection({
		"fluent-24", "Fluent UI 24", 24,
		{
			{IconStyle::Outline, []() { return new FluentRegular24IconList(); }},
			{IconStyle::Filled, []() { return new FluentFilled24IconList(); }}
		}
	});
	registry.registerCollection({
		"fluent-32", "Fluent UI 32", 32,
		{
			{IconStyle::Outline, []() { return new FluentRegular32IconList(); }},
			{IconStyle::Filled, []() { return new FluentFilled32IconList(); }}
		}
	});

	// Breeze Icons (KDE) - organized by category, single style per category
	registry.registerCollection({
		"breeze-actions", "Breeze Actions", 22,
		{{IconStyle::Outline, []() { return new BreezeActions22IconList(); }}}
	});
	registry.registerCollection({
		"breeze-apps", "Breeze Apps", 48,
		{{IconStyle::Outline, []() { return new BreezeApps48IconList(); }}}
	});
	registry.registerCollection({
		"breeze-places", "Breeze Places", 22,
		{{IconStyle::Outline, []() { return new BreezePlaces22IconList(); }}}
	});
	registry.registerCollection({
		"breeze-status", "Breeze Status", 22,
		{{IconStyle::Outline, []() { return new BreezeStatus22IconList(); }}}
	});
	registry.registerCollection({
		"breeze-devices", "Breeze Devices", 22,
		{{IconStyle::Outline, []() { return new BreezeDevices22IconList(); }}}
	});
	registry.registerCollection({
		"breeze-mimetypes", "Breeze Mimetypes", 22,
		{{IconStyle::Outline, []() { return new BreezeMimetypes22IconList(); }}}
	});

	// Oxygen Icons (Bitmap) - multiple sizes available
	registry.registerBitmapCollection({
		"oxygen", "Oxygen Icons",
		{16, 22, 32, 48, 64, 128, 256},
		[](int size) -> BitmapIconList* {
			switch (size) {
				case 16: return new Oxygen16IconList();
				case 22: return new Oxygen22IconList();
				case 32: return new Oxygen32IconList();
				case 48: return new Oxygen48IconList();
				case 64: return new Oxygen64IconList();
				case 128: return new Oxygen128IconList();
				case 256: return new Oxygen256IconList();
				default: return new Oxygen32IconList();
			}
		}
	});

	// Oxygen5 Icons (Bitmap) - multiple sizes available
	registry.registerBitmapCollection({
		"oxygen5", "Oxygen5 Icons",
		{16, 22, 32, 48, 64, 128, 256},
		[](int size) -> BitmapIconList* {
			switch (size) {
				case 16: return new Oxygen516IconList();
				case 22: return new Oxygen522IconList();
				case 32: return new Oxygen532IconList();
				case 48: return new Oxygen548IconList();
				case 64: return new Oxygen564IconList();
				case 128: return new Oxygen5128IconList();
				case 256: return new Oxygen5256IconList();
				default: return new Oxygen532IconList();
			}
		}
	});
}

void MainWindow::loadCollections() {
	auto &registry = IconCollectionRegistry::instance();
	auto collections = registry.collections();
	auto bitmapCollections = registry.bitmapCollections();

	// Build list of collection names for the toolbar (SVG + Bitmap)
	QStringList names;
	for (const auto &coll : collections) {
		names.append(coll.displayName);
	}
	for (const auto &coll : bitmapCollections) {
		names.append(coll.displayName);
	}

	// Set up the toolbar's collection dropdown
	auto *toolbar = m_ui->iconGrid->findChild<IconToolBar*>();
	if (toolbar) {
		toolbar->setCollections(names);
		connect(toolbar, &IconToolBar::collectionChanged, this, &MainWindow::onCollectionChanged);
		connect(toolbar, &IconToolBar::styleChanged, this, &MainWindow::onStyleChanged);
		connect(toolbar, &IconToolBar::bitmapSizeChanged, this, &MainWindow::onBitmapSizeChanged);
	}

	// Load the first available collection with default style (Outline)
	if (!collections.isEmpty()) {
		m_currentCollectionId = collections.first().id;
		m_currentStyle = IconStyle::Outline;
		m_isBitmapCollection = false;
		loadCurrentCollection();

		// Update available styles in toolbar
		if (toolbar) {
			updateAvailableStyles();
		}
	}

	// Count total icons
	int totalIcons = 0;
	for (const auto &coll : collections) {
		auto *tempList = registry.createIconList(coll.id, IconStyle::Outline);
		if (tempList) {
			totalIcons += tempList->getCount();
			delete tempList;
		}
	}
	for (const auto &coll : bitmapCollections) {
		auto *tempList = registry.createBitmapList(coll.id, coll.defaultSize());
		if (tempList) {
			totalIcons += tempList->getCount();
			delete tempList;
		}
	}

	int totalCollections = collections.size() + bitmapCollections.size();
	statusBar()->showMessage(tr("Loaded %1 collections with %2 icons").arg(totalCollections).arg(totalIcons));
}

void MainWindow::loadCurrentCollection() {
	auto &registry = IconCollectionRegistry::instance();

	if (m_isBitmapCollection) {
		auto *list = registry.createBitmapList(m_currentCollectionId, m_currentBitmapSize);
		if (list) {
			m_bitmapLists.push_back(std::unique_ptr<BitmapIconList>(list));
			m_currentList = list;
			m_ui->iconGrid->setIconList(list);
		}
	} else {
		auto *list = registry.createIconList(m_currentCollectionId, m_currentStyle);
		if (list) {
			m_iconLists.push_back(std::unique_ptr<SVGIconList>(list));
			m_currentList = list;
			m_ui->iconGrid->setIconList(list);
		}
	}
}

void MainWindow::updateAvailableStyles() {
	auto *toolbar = m_ui->iconGrid->toolBar();
	if (!toolbar)
		return;

	auto &registry = IconCollectionRegistry::instance();

	// Update bitmap mode for toolbar and preview
	toolbar->setBitmapMode(m_isBitmapCollection);
	m_ui->iconGrid->preview()->setBitmapMode(m_isBitmapCollection);

	if (m_isBitmapCollection) {
		// Bitmap collections have Color/Grayscale styles
		toolbar->setStyles({"Color", "Grayscale"});

		// Also update available sizes
		const BitmapCollection *coll = registry.findBitmapCollection(m_currentCollectionId);
		if (coll) {
			toolbar->setBitmapSizes(coll->availableSizes);
		}
	} else {
		// SVG collections have Outline/Filled/TwoTone styles
		const IconCollection *coll = registry.findCollection(m_currentCollectionId);
		if (!coll)
			return;

		QStringList styles;
		for (IconStyle style : coll->availableStyles()) {
			styles.append(iconStyleToString(style));
		}
		// Add TwoTone if both Outline and Filled are available
		if (coll->hasStyle(IconStyle::Outline) && coll->hasStyle(IconStyle::Filled)) {
			if (!styles.contains("TwoTone"))
				styles.append("TwoTone");
		}
		toolbar->setStyles(styles);
		toolbar->setBitmapSizes({}); // Hide size selector for SVG
	}
}

void MainWindow::onCollectionChanged(const QString &name) {
	auto &registry = IconCollectionRegistry::instance();

	// Check if it's a bitmap collection first
	for (const auto &coll : registry.bitmapCollections()) {
		if (coll.displayName == name) {
			m_currentCollectionId = coll.id;
			m_isBitmapCollection = true;
			m_currentBitmapSize = coll.defaultSize();
			loadCurrentCollection();
			updateAvailableStyles();
			return;
		}
	}

	// Otherwise it's an SVG collection
	auto collections = registry.collections();

	for (const auto &coll : collections) {
		if (coll.displayName == name) {
			m_currentCollectionId = coll.id;
			m_isBitmapCollection = false;
			// Reset to Outline style when changing collection
			m_currentStyle = IconStyle::Outline;
			loadCurrentCollection();
			updateAvailableStyles();
			break;
		}
	}
}

void MainWindow::onStyleChanged(const QString &styleName) {
	if (m_isBitmapCollection) {
		// Handle bitmap styles (Color/Grayscale)
		bool grayscale = (styleName == "Grayscale");
		m_ui->iconGrid->model()->setGrayscale(grayscale);
	} else {
		// Handle SVG styles
		m_currentStyle = stringToIconStyle(styleName);
		loadCurrentCollection();
	}
}

void MainWindow::onBitmapSizeChanged(int size) {
	if (m_isBitmapCollection) {
		m_currentBitmapSize = size;
		loadCurrentCollection();
	}
}

void MainWindow::onIconSelected(int index, const QString &name) {
	m_selectedIndex = index;
	m_selectedName = name;
	statusBar()->showMessage(tr("Selected: %1").arg(name));
}

void MainWindow::onIconDoubleClicked(int index, const QString &name) {
	Q_UNUSED(index)
	if (!m_currentList || index < 0)
		return;

	if (m_isBitmapCollection) {
		// Copy PNG on double-click for bitmap collections
		QPixmap pixmap = m_ui->iconGrid->model()->getIconPixmap(index);
		if (!pixmap.isNull()) {
			QApplication::clipboard()->setPixmap(pixmap);
			statusBar()->showMessage(tr("Copied PNG: %1").arg(name), 3000);
		}
	} else {
		// Copy SVG on double-click for SVG collections
		auto *svg = static_cast<SVGIconList*>(m_currentList);
		QString source = svg->getSource(index);
		QApplication::clipboard()->setText(source);
		statusBar()->showMessage(tr("Copied SVG: %1").arg(name), 3000);
	}
}

void MainWindow::onCopySvg() {
	if (!m_currentList || m_selectedIndex < 0)
		return;

	if (m_isBitmapCollection) {
		statusBar()->showMessage(tr("SVG not available for bitmap icons"), 3000);
		return;
	}

	auto *svg = static_cast<SVGIconList*>(m_currentList);
	QString source = svg->getSource(m_selectedIndex);
	QApplication::clipboard()->setText(source);
	statusBar()->showMessage(tr("Copied SVG to clipboard"), 3000);
}

void MainWindow::onCopyPng() {
	if (m_selectedIndex >= 0) {
		QPixmap pixmap = m_ui->iconGrid->model()->getIconPixmap(m_selectedIndex);
		if (!pixmap.isNull()) {
			QApplication::clipboard()->setPixmap(pixmap);
			statusBar()->showMessage(tr("Copied PNG to clipboard"), 3000);
		}
	}
}

void MainWindow::onExport() {
	if (m_currentList && m_selectedIndex >= 0) {
		QString defaultExt = m_isBitmapCollection ? ".png" : ".svg";
		QString defaultName = m_selectedName + defaultExt;
		QString filter = m_isBitmapCollection
			? tr("PNG Files (*.png);;All Files (*)")
			: tr("SVG Files (*.svg);;PNG Files (*.png);;All Files (*)");

		QString filename = QFileDialog::getSaveFileName(
			this,
			tr("Export Icon"),
			defaultName,
			filter
		);

		if (!filename.isEmpty()) {
			if (filename.endsWith(".svg", Qt::CaseInsensitive) && !m_isBitmapCollection) {
				auto *svgList = static_cast<SVGIconList*>(m_currentList);
				QString svg = svgList->getSource(m_selectedIndex);
				QFile file(filename);
				if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
					file.write(svg.toUtf8());
					file.close();
					statusBar()->showMessage(tr("Exported to %1").arg(filename), 3000);
				}
			} else if (filename.endsWith(".png", Qt::CaseInsensitive)) {
				QPixmap pixmap = m_ui->iconGrid->model()->getIconPixmap(m_selectedIndex);
				if (!pixmap.isNull()) {
					pixmap.save(filename, "PNG");
					statusBar()->showMessage(tr("Exported to %1").arg(filename), 3000);
				}
			}
		}
	}
}

void MainWindow::onAbout() {
	QMessageBox::about(this, tr("About Icon Viewer"),
		tr("<h3>Icon Viewer</h3>"
		   "<p>Version 1.0</p>"
		   "<p>A viewer for icon collections including:</p>"
		   "<ul>"
		   "<li>Bootstrap Icons (SVG)</li>"
		   "<li>Tabler Icons (SVG)</li>"
		   "<li>Microsoft Fluent UI Icons (SVG)</li>"
		   "<li>KDE Breeze Icons (SVG)</li>"
		   "<li>KDE Oxygen Icons (Bitmap)</li>"
		   "</ul>"
		   "<p>Based on <a href='https://github.com/skamradt/SVGIconViewer'>SVGIconViewer</a></p>"));
}

void MainWindow::setSmallIcons() {
	m_ui->iconGrid->setIconSize(24);
}

void MainWindow::setMediumIcons() {
	m_ui->iconGrid->setIconSize(32);
}

void MainWindow::setLargeIcons() {
	m_ui->iconGrid->setIconSize(48);
}

void MainWindow::updateIconCount() {
	auto *model = m_ui->iconGrid->model();
	int total = model->iconList() ? model->iconList()->getCount() : 0;
	int visible = model->rowCount();
	QString filter = model->filter();

	QString text;
	if (filter.isEmpty()) {
		text = tr("%1 icons").arg(total);
	} else {
		text = tr("%1 of %2 icons").arg(visible).arg(total);
	}
	m_iconCountLabel->setText(text);
}

int main(int argc, char *argv[]) {
	QApplication app(argc, argv);
	app.setApplicationName("SVG Icon Viewer");
	app.setOrganizationName("KomSoft");
	app.setApplicationVersion("1.0");

	MainWindow w;
	w.show();

	return app.exec();
}
