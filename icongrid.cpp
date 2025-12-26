#include "icongrid.h"

#include <QPainter>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QBuffer>
#include <QScrollBar>
#include <QDebug>

// ============================================================================
// IconDelegate
// ============================================================================

IconDelegate::IconDelegate(QObject *parent)
	: QStyledItemDelegate(parent)
{
}

void IconDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
						  const QModelIndex &index) const {
	painter->save();

	// Draw selection/hover background
	if (option.state & QStyle::State_Selected) {
		painter->fillRect(option.rect, option.palette.highlight());
	} else if (option.state & QStyle::State_MouseOver) {
		QColor hoverColor = option.palette.highlight().color();
		hoverColor.setAlpha(50);
		painter->fillRect(option.rect, hoverColor);
	}

	// Get icon pixmap
	QPixmap pixmap = index.data(Qt::DecorationRole).value<QPixmap>();
	QString name = index.data(Qt::DisplayRole).toString();

	// Calculate icon position (centered horizontally)
	int iconX = option.rect.x() + (option.rect.width() - m_iconSize) / 2;
	int iconY = option.rect.y() + m_padding;

	// Draw the icon
	if (!pixmap.isNull()) {
		// Scale if needed
		if (pixmap.width() != m_iconSize || pixmap.height() != m_iconSize) {
			pixmap = pixmap.scaled(m_iconSize, m_iconSize,
								   Qt::KeepAspectRatio, Qt::SmoothTransformation);
		}
		painter->drawPixmap(iconX, iconY, pixmap);
	}

	// Draw the name if enabled
	if (m_showNames && !name.isEmpty()) {
		QRect textRect = option.rect;
		textRect.setTop(iconY + m_iconSize + 4);
		textRect.setHeight(m_nameHeight);

		painter->setPen(option.state & QStyle::State_Selected
						? option.palette.highlightedText().color()
						: option.palette.text().color());

		// Check if text fits on single line
		QFontMetrics fm(painter->font());
		int textWidth = fm.horizontalAdvance(name);
		int alignment = (textWidth <= textRect.width())
						? (Qt::AlignHCenter | Qt::AlignTop)
						: (Qt::AlignLeft | Qt::AlignTop | Qt::TextWrapAnywhere);

		painter->drawText(textRect, alignment, name);
	}

	painter->restore();
}

QSize IconDelegate::sizeHint(const QStyleOptionViewItem &option,
							  const QModelIndex &index) const {
	Q_UNUSED(option)
	Q_UNUSED(index)

	int width = m_iconSize + m_padding * 2;
	int height = m_iconSize + m_padding * 2;

	if (m_showNames) {
		height += m_nameHeight;
	}

	return QSize(width, height);
}

void IconDelegate::setIconSize(int size) {
	m_iconSize = size;
}

int IconDelegate::iconSize() const {
	return m_iconSize;
}

void IconDelegate::setShowNames(bool show) {
	m_showNames = show;
}

bool IconDelegate::showNames() const {
	return m_showNames;
}

// ============================================================================
// SearchBar
// ============================================================================

SearchBar::SearchBar(QWidget *parent)
	: QWidget(parent)
{
	auto *layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	m_lineEdit = new QLineEdit(this);
	m_lineEdit->setPlaceholderText(tr("Search icons..."));
	m_lineEdit->setClearButtonEnabled(true);

	layout->addWidget(m_lineEdit);

	connect(m_lineEdit, &QLineEdit::textChanged, this, &SearchBar::textChanged);
}

QString SearchBar::text() const {
	return m_lineEdit->text();
}

void SearchBar::clear() {
	m_lineEdit->clear();
}

// ============================================================================
// IconToolBar
// ============================================================================

IconToolBar::IconToolBar(QWidget *parent)
	: QWidget(parent)
{
	auto *layout = new QHBoxLayout(this);
	layout->setContentsMargins(4, 4, 4, 4);
	layout->setSpacing(8);

	// Collection selector
	auto *collectionLabel = new QLabel(tr("Library:"), this);
	m_collectionCombo = new QComboBox(this);
	m_collectionCombo->setMinimumWidth(150);

	// Style selector
	auto *styleLabel = new QLabel(tr("Style:"), this);
	m_styleCombo = new QComboBox(this);
	m_styleCombo->setMinimumWidth(100);

	// Size selector (for SVG - display size)
	m_sizeLabel = new QLabel(tr("Size:"), this);
	m_sizeCombo = new QComboBox(this);
	m_sizeCombo->addItems({"24", "32", "48", "64", "96", "128"});
	m_sizeCombo->setCurrentIndex(1); // Default to 32

	// Bitmap size selector (for bitmap - icon size from collection)
	m_bitmapSizeLabel = new QLabel(tr("Icon Size:"), this);
	m_bitmapSizeCombo = new QComboBox(this);
	m_bitmapSizeLabel->setVisible(false);
	m_bitmapSizeCombo->setVisible(false);

	// Fill color button
	m_fillColorButton = new QToolButton(this);
	m_fillColorButton->setText(tr("Fill"));
	m_fillColorButton->setToolTip(tr("Set fill color"));
	updateFillColorButton();

	// Background color button
	m_bgColorButton = new QToolButton(this);
	m_bgColorButton->setText(tr("BG"));
	m_bgColorButton->setToolTip(tr("Set background color"));
	updateBgColorButton();

	layout->addWidget(collectionLabel);
	layout->addWidget(m_collectionCombo);
	layout->addSpacing(16);
	layout->addWidget(styleLabel);
	layout->addWidget(m_styleCombo);
	layout->addSpacing(16);
	layout->addWidget(m_sizeLabel);
	layout->addWidget(m_sizeCombo);
	layout->addWidget(m_bitmapSizeLabel);
	layout->addWidget(m_bitmapSizeCombo);
	layout->addStretch();
	layout->addWidget(m_fillColorButton);
	layout->addWidget(m_bgColorButton);

	// Connections
	connect(m_collectionCombo, &QComboBox::currentTextChanged,
			this, &IconToolBar::collectionChanged);
	connect(m_styleCombo, &QComboBox::currentTextChanged,
			this, &IconToolBar::styleChanged);
	connect(m_sizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &IconToolBar::onIconSizeChanged);
	connect(m_bitmapSizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &IconToolBar::onBitmapSizeChanged);
	connect(m_fillColorButton, &QToolButton::clicked,
			this, &IconToolBar::onFillColorClicked);
	connect(m_bgColorButton, &QToolButton::clicked,
			this, &IconToolBar::onBackgroundColorClicked);
}

void IconToolBar::setCollections(const QStringList &names) {
	m_collectionCombo->blockSignals(true);
	m_collectionCombo->clear();
	m_collectionCombo->addItems(names);
	m_collectionCombo->blockSignals(false);
}

QString IconToolBar::currentCollection() const {
	return m_collectionCombo->currentText();
}

void IconToolBar::setStyles(const QStringList &styles) {
	m_styleCombo->blockSignals(true);
	m_styleCombo->clear();
	m_styleCombo->addItems(styles);
	m_styleCombo->blockSignals(false);
}

QString IconToolBar::currentStyle() const {
	return m_styleCombo->currentText();
}

QColor IconToolBar::fillColor() const {
	return m_fillColor;
}

QColor IconToolBar::backgroundColor() const {
	return m_backgroundColor;
}

int IconToolBar::iconSize() const {
	return m_sizeCombo->currentText().toInt();
}

void IconToolBar::setFillColor(const QColor &color) {
	m_fillColor = color;
	updateFillColorButton();
}

void IconToolBar::setBackgroundColor(const QColor &color) {
	m_backgroundColor = color;
	updateBgColorButton();
}

void IconToolBar::onFillColorClicked() {
	QColor color = QColorDialog::getColor(m_fillColor, this, tr("Select Fill Color"));
	if (color.isValid()) {
		m_fillColor = color;
		updateFillColorButton();
		emit fillColorChanged(color);
	}
}

void IconToolBar::onBackgroundColorClicked() {
	QColor color = QColorDialog::getColor(m_backgroundColor, this, tr("Select Background Color"),
										  QColorDialog::ShowAlphaChannel);
	if (color.isValid()) {
		m_backgroundColor = color;
		updateBgColorButton();
		emit backgroundColorChanged(color);
	}
}

void IconToolBar::onIconSizeChanged(int index) {
	Q_UNUSED(index)
	emit iconSizeChanged(iconSize());
}

void IconToolBar::setBitmapSizes(const QList<int> &sizes) {
	m_bitmapSizeCombo->blockSignals(true);
	m_bitmapSizeCombo->clear();
	for (int size : sizes) {
		m_bitmapSizeCombo->addItem(QString::number(size), size);
	}
	m_bitmapSizeCombo->blockSignals(false);

	bool isBitmap = !sizes.isEmpty();
	// Show bitmap size selector for bitmap collections
	m_bitmapSizeLabel->setVisible(isBitmap);
	m_bitmapSizeCombo->setVisible(isBitmap);
	// Hide regular size selector for bitmap collections
	m_sizeLabel->setVisible(!isBitmap);
	m_sizeCombo->setVisible(!isBitmap);
}

int IconToolBar::currentBitmapSize() const {
	return m_bitmapSizeCombo->currentData().toInt();
}

void IconToolBar::onBitmapSizeChanged(int index) {
	Q_UNUSED(index)
	emit bitmapSizeChanged(currentBitmapSize());
}

void IconToolBar::setBitmapMode(bool isBitmap) {
	// Hide fill/background color buttons for bitmap collections
	m_fillColorButton->setVisible(!isBitmap);
	m_bgColorButton->setVisible(!isBitmap);
}

void IconToolBar::updateFillColorButton() {
	QPixmap pixmap(16, 16);
	pixmap.fill(m_fillColor);
	m_fillColorButton->setIcon(QIcon(pixmap));
}

void IconToolBar::updateBgColorButton() {
	QPixmap pixmap(16, 16);
	if (m_backgroundColor.alpha() == 0) {
		// Checkerboard pattern for transparent
		QPainter painter(&pixmap);
		painter.fillRect(0, 0, 8, 8, Qt::lightGray);
		painter.fillRect(8, 8, 8, 8, Qt::lightGray);
		painter.fillRect(8, 0, 8, 8, Qt::white);
		painter.fillRect(0, 8, 8, 8, Qt::white);
	} else {
		pixmap.fill(m_backgroundColor);
	}
	m_bgColorButton->setIcon(QIcon(pixmap));
}

// ============================================================================
// IconPreview
// ============================================================================

IconPreview::IconPreview(QWidget *parent)
	: QWidget(parent)
{
	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(4, 4, 4, 4);
	layout->setAlignment(Qt::AlignTop);

	// Icon display
	m_iconLabel = new QLabel(this);
	m_iconLabel->setFixedSize(128, 128);
	m_iconLabel->setAlignment(Qt::AlignCenter);
	m_iconLabel->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);

	// Name label
	m_nameLabel = new QLabel(this);
	m_nameLabel->setAlignment(Qt::AlignCenter);
	m_nameLabel->setWordWrap(true);

	// Aliases label
	m_aliasesLabel = new QLabel(this);
	m_aliasesLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	m_aliasesLabel->setWordWrap(true);
	m_aliasesLabel->setStyleSheet("QLabel { color: gray; font-size: 10px; }");
	m_aliasesLabel->setVisible(false);

	// Buttons using ActiveLabel
	auto *buttonLayout = new QVBoxLayout();
	buttonLayout->setSpacing(4);

	m_copySvgButton = new ActiveLabel(0, "Copy SVG", "copy_svg_button", this);
	m_copySvgButton->setToolTip(tr("Copy SVG source to clipboard"));

	m_copyPngButton = new ActiveLabel(1, "Copy PNG", "copy_png_button", this);
	m_copyPngButton->setToolTip(tr("Copy as PNG image to clipboard"));

	m_copyInfoButton = new ActiveLabel(2, "Copy Info", "copy_info_button", this);
	m_copyInfoButton->setToolTip(tr("Copy icon information to clipboard"));

	m_exportButton = new ActiveLabel(3, "Export ...", "export_button", this);
	m_exportButton->setToolTip(tr("Export icons from list to files"));

	buttonLayout->addWidget(m_copySvgButton);
	buttonLayout->addWidget(m_copyPngButton);
	buttonLayout->addWidget(m_copyInfoButton);
	buttonLayout->addWidget(m_exportButton);

	// === View switcher (buttons + stacked widget) ===
	m_viewSwitcher = new QWidget(this);
	m_viewSwitcher->setVisible(false);  // Hidden initially (shown when entities exist)
	auto *switcherLayout = new QVBoxLayout(m_viewSwitcher);
	switcherLayout->setContentsMargins(0, 0, 0, 0);
	switcherLayout->setSpacing(2);

	// Buttons for switching views
	auto *buttonBar = new QWidget(m_viewSwitcher);
	auto *buttonBarLayout = new QHBoxLayout(buttonBar);
	buttonBarLayout->setContentsMargins(0, 0, 0, 0);
	buttonBarLayout->setSpacing(0);

	m_exportViewButton = new QToolButton(buttonBar);
	m_exportViewButton->setText(tr("Export"));
	m_exportViewButton->setCheckable(true);
	m_exportViewButton->setChecked(true);
	m_exportViewButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	m_entitiesViewButton = new QToolButton(buttonBar);
	m_entitiesViewButton->setText(tr("Entities"));
	m_entitiesViewButton->setCheckable(true);
	m_entitiesViewButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	buttonBarLayout->addWidget(m_exportViewButton);
	buttonBarLayout->addWidget(m_entitiesViewButton);

	// Stacked widget for content
	m_stackedWidget = new QStackedWidget(m_viewSwitcher);

	switcherLayout->addWidget(buttonBar);
	switcherLayout->addWidget(m_stackedWidget, 1);

	// === Export widget ===
	m_exportWidget = new QWidget(this);
	auto *exportLayout = new QVBoxLayout(m_exportWidget);
	exportLayout->setContentsMargins(0, 4, 0, 0);

	m_exportOptionsLabel = new QLabel(tr("Export Options:"), m_exportWidget);
	m_exportMergedCheckbox = new QCheckBox(tr("Export merged png"), m_exportWidget);
	m_exportMergedCheckbox->setToolTip(tr("Merge all icons into a single PNG image"));

	m_exportListWidget = new QListWidget(m_exportWidget);

	m_clearExportButton = new ActiveLabel(4, "Clear", "clear_export_button", m_exportWidget);
	m_clearExportButton->setToolTip(tr("Clear export list"));

	exportLayout->addWidget(m_exportOptionsLabel);
	exportLayout->addWidget(m_exportMergedCheckbox);
	exportLayout->addWidget(m_exportListWidget, 1);
	exportLayout->addWidget(m_clearExportButton);

	// === Entities widget ===
	m_entitiesWidget = new QWidget();
	auto *entitiesLayout = new QVBoxLayout(m_entitiesWidget);
	entitiesLayout->setContentsMargins(0, 4, 0, 0);

	m_entitiesTable = new QTableWidget(m_entitiesWidget);
	m_entitiesTable->setAttribute(Qt::WA_MacSmallSize);
	m_entitiesTable->setColumnCount(2);
	m_entitiesTable->setHorizontalHeaderLabels({tr("Name"), tr("Value")});
	m_entitiesTable->horizontalHeader()->setStretchLastSection(true);
	m_entitiesTable->verticalHeader()->setVisible(false);
	m_entitiesTable->verticalHeader()->setDefaultSectionSize(20);
	m_entitiesTable->setSelectionBehavior(QAbstractItemView::SelectRows);

	entitiesLayout->addWidget(m_entitiesTable);

	// Add widgets to stacked widget
	m_stackedWidget->addWidget(m_exportWidget);
	m_stackedWidget->addWidget(m_entitiesWidget);

	layout->addWidget(m_iconLabel, 0, Qt::AlignHCenter);
	layout->addWidget(m_nameLabel);
	layout->addWidget(m_aliasesLabel);
	layout->addLayout(buttonLayout);
	layout->addWidget(m_exportWidget, 1);   // Export widget shown directly initially
	layout->addWidget(m_viewSwitcher, 1);   // View switcher hidden initially

	// Connections
	connect(m_copySvgButton, &ActiveLabel::clicked, this, &IconPreview::onButtonClicked);
	connect(m_copyPngButton, &ActiveLabel::clicked, this, &IconPreview::onButtonClicked);
	connect(m_copyInfoButton, &ActiveLabel::clicked, this, &IconPreview::onButtonClicked);
	connect(m_exportButton, &ActiveLabel::clicked, this, &IconPreview::onButtonClicked);
	connect(m_clearExportButton, &ActiveLabel::clicked, this, &IconPreview::onButtonClicked);
	connect(m_entitiesTable, &QTableWidget::cellChanged, this, &IconPreview::onEntityValueChanged);

	// View switcher button connections
	connect(m_exportViewButton, &QToolButton::clicked, this, [this]() {
		m_exportViewButton->setChecked(true);
		m_entitiesViewButton->setChecked(false);
		m_stackedWidget->setCurrentIndex(0);
	});
	connect(m_entitiesViewButton, &QToolButton::clicked, this, [this]() {
		m_exportViewButton->setChecked(false);
		m_entitiesViewButton->setChecked(true);
		m_stackedWidget->setCurrentIndex(1);
	});

	clear();
}

void IconPreview::onButtonClicked(int index) {
	switch (index) {
		case 0: // Copy SVG
			if (!m_currentSvg.isEmpty()) {
				QApplication::clipboard()->setText(m_currentSvg);
			}
			break;
		case 1: // Copy PNG
			if (!m_currentPixmap.isNull()) {
				QApplication::clipboard()->setPixmap(m_currentPixmap);
			}
			break;
		case 2: // Copy Info
			{
				QString info;
				info += QString("Name: %1\n").arg(m_currentName);
				info += QString("Style: %1\n").arg(m_currentStyle);
				info += QString("Size: %1\n").arg(m_currentSize);
				if (!m_currentAliases.isEmpty()) {
					info += QString("Aliases: %1\n").arg(m_currentAliases.join(", "));
				}
				QApplication::clipboard()->setText(info.trimmed());
			}
			break;
		case 3: // Export
			emit exportRequested(m_exportMergedCheckbox->isChecked());
			break;
		case 4: // Clear export list
			clearExportList();
			break;
	}
}

void IconPreview::setIcon(const QPixmap &pixmap, const QString &name, const QString &svg,
						  const QString &style, int size, const QStringList &aliases) {
	m_currentPixmap = pixmap;
	m_currentSvg = svg;
	m_currentName = name;
	m_currentStyle = style;
	m_currentSize = size;
	m_currentAliases = aliases;

	if (pixmap.isNull()) {
		m_iconLabel->clear();
	} else {
		QPixmap scaled = pixmap.scaled(120, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		m_iconLabel->setPixmap(scaled);
	}

	m_nameLabel->setText(name);

	// Display aliases if any
	if (aliases.isEmpty()) {
		m_aliasesLabel->setVisible(false);
	} else {
		QString aliasText = tr("Aliases:\n") + aliases.join("\n");
		m_aliasesLabel->setText(aliasText);
		m_aliasesLabel->setVisible(true);
	}

	m_copySvgButton->setEnabled(!svg.isEmpty());
	m_copyPngButton->setEnabled(!pixmap.isNull());
	m_copyInfoButton->setEnabled(!name.isEmpty());
	m_exportButton->setEnabled(!m_exportList.isEmpty());
}

void IconPreview::addToExportList(const ExportIconInfo &info) {
	// Check if already in list by name
	for (const auto &existing : m_exportList) {
		if (existing.name == info.name && existing.style == info.style && existing.size == info.size)
			return;
	}

	m_exportList.append(info);

	QListWidgetItem *item = new QListWidgetItem(m_exportListWidget);
	QString displayText = QString("%1 (%2, %3)").arg(info.name).arg(info.style).arg(info.size);
	item->setText(displayText);
	if (!info.pixmap.isNull()) {
		item->setIcon(QIcon(info.pixmap.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
	}

	m_exportButton->setEnabled(true);
	// Switch to Export view if view switcher is visible
	if (m_viewSwitcher->isVisible()) {
		m_exportViewButton->setChecked(true);
		m_entitiesViewButton->setChecked(false);
		m_stackedWidget->setCurrentIndex(0);
	}
}

void IconPreview::clearExportList() {
	m_exportList.clear();
	m_exportListWidget->clear();
	m_exportButton->setEnabled(false);
}

QList<ExportIconInfo> IconPreview::exportList() const {
	return m_exportList;
}

void IconPreview::clear() {
	m_iconLabel->clear();
	m_nameLabel->clear();
	m_aliasesLabel->clear();
	m_aliasesLabel->setVisible(false);
	m_currentSvg.clear();
	m_currentName.clear();
	m_currentStyle.clear();
	m_currentSize = 0;
	m_currentAliases.clear();
	m_currentPixmap = QPixmap();

	m_copySvgButton->setEnabled(false);
	m_copyPngButton->setEnabled(false);
	m_copyInfoButton->setEnabled(false);
	m_exportButton->setEnabled(!m_exportList.isEmpty());

	// Clear entities and show direct export view
	m_entities.clear();
	m_entitiesTable->setRowCount(0);
	m_viewSwitcher->setVisible(false);
	m_exportWidget->setParent(this);
	// Re-insert export widget into main layout
	QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(layout());
	if (mainLayout && mainLayout->indexOf(m_exportWidget) < 0) {
		int switcherIndex = mainLayout->indexOf(m_viewSwitcher);
		if (switcherIndex >= 0) {
			mainLayout->insertWidget(switcherIndex, m_exportWidget, 1);
		}
	}
	m_exportWidget->setVisible(true);
}

void IconPreview::setBitmapMode(bool isBitmap) {
	// Hide Copy SVG button for bitmap collections
	m_copySvgButton->setVisible(!isBitmap);
	// In bitmap mode, entities are not supported, so always show direct export view
	if (isBitmap) {
		m_viewSwitcher->setVisible(false);
		m_exportWidget->setParent(this);
		QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(layout());
		if (mainLayout && mainLayout->indexOf(m_exportWidget) < 0) {
			int switcherIndex = mainLayout->indexOf(m_viewSwitcher);
			if (switcherIndex >= 0) {
				mainLayout->insertWidget(switcherIndex, m_exportWidget, 1);
			}
		}
		m_exportWidget->setVisible(true);
	}
}

void IconPreview::setEntities(const EntityMap &entities) {
	m_entities = entities;
	updateEntitiesTable();

	bool hasEntities = !entities.isEmpty();

	if (hasEntities) {
		// Show view switcher with Export and Entities buttons
		// Move export widget into stacked widget
		m_exportWidget->setParent(m_stackedWidget);
		m_stackedWidget->insertWidget(0, m_exportWidget);
		m_exportWidget->setVisible(true);

		// Select Export view by default
		m_exportViewButton->setChecked(true);
		m_entitiesViewButton->setChecked(false);
		m_stackedWidget->setCurrentIndex(0);
		m_viewSwitcher->setVisible(true);
	} else {
		// Show direct export view (no view switcher)
		m_viewSwitcher->setVisible(false);

		// Reparent export widget back to main widget
		m_exportWidget->setParent(this);
		QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(layout());
		if (mainLayout && mainLayout->indexOf(m_exportWidget) < 0) {
			int switcherIndex = mainLayout->indexOf(m_viewSwitcher);
			if (switcherIndex >= 0) {
				mainLayout->insertWidget(switcherIndex, m_exportWidget, 1);
			}
		}
		m_exportWidget->setVisible(true);
	}
}

EntityMap IconPreview::currentEntities() const {
	return m_entities;
}

bool IconPreview::hasEntities() const {
	return !m_entities.isEmpty();
}

void IconPreview::updateEntitiesTable() {
	m_entitiesTable->blockSignals(true);
	m_entitiesTable->setRowCount(m_entities.size());

	int row = 0;
	for (auto it = m_entities.begin(); it != m_entities.end(); ++it, ++row) {
		// Name column (read-only)
		auto *nameItem = new QTableWidgetItem(it.key());
		nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
		m_entitiesTable->setItem(row, 0, nameItem);

		// Value column (editable)
		auto *valueItem = new QTableWidgetItem(it.value());
		m_entitiesTable->setItem(row, 1, valueItem);
	}

	m_entitiesTable->resizeColumnsToContents();
	m_entitiesTable->blockSignals(false);
}

void IconPreview::onEntityValueChanged(int row, int column) {
	if (column != 1)  // Only value column is editable
		return;

	QString name = m_entitiesTable->item(row, 0)->text();
	QString value = m_entitiesTable->item(row, 1)->text();

	if (m_entities.contains(name) && m_entities[name] != value) {
		m_entities[name] = value;
		emit entitiesChanged(m_entities);
	}
}

// ============================================================================
// IconGrid
// ============================================================================

IconGrid::IconGrid(QWidget *parent)
	: QWidget(parent)
{
	auto *mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);

	// Toolbar
	m_toolBar = new IconToolBar(this);

	// Search bar
	m_searchBar = new SearchBar(this);

	// Content area with grid and preview
	auto *contentWidget = new QWidget(this);
	auto *contentLayout = new QHBoxLayout(contentWidget);
	contentLayout->setContentsMargins(4, 4, 4, 4);

	// List view for icons
	m_listView = new QListView(this);
	m_listView->setViewMode(QListView::IconMode);
	m_listView->setFlow(QListView::LeftToRight);
	m_listView->setWrapping(true);
	m_listView->setResizeMode(QListView::Adjust);
	m_listView->setUniformItemSizes(true);
	m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
	m_listView->setSpacing(4);

	// Model and delegate
	m_model = new IconModel(this);
	m_delegate = new IconDelegate(this);

	m_listView->setModel(m_model);
	m_listView->setItemDelegate(m_delegate);

	// Preview panel
	m_preview = new IconPreview(this);
	m_preview->setFixedWidth(180);

	contentLayout->addWidget(m_listView, 1);
	contentLayout->addWidget(m_preview);

	mainLayout->addWidget(m_toolBar);
	mainLayout->addWidget(m_searchBar);
	mainLayout->addWidget(contentWidget, 1);

	// Context menu
	m_contextMenu = new QMenu(this);
	m_addToExportAction = m_contextMenu->addAction(tr("Add to List"));
	connect(m_addToExportAction, &QAction::triggered, this, &IconGrid::onAddToExport);

	m_listView->setContextMenuPolicy(Qt::CustomContextMenu);

	// Connections
	connect(m_searchBar, &SearchBar::textChanged, this, &IconGrid::setFilter);

	connect(m_toolBar, &IconToolBar::fillColorChanged, this, &IconGrid::setFillColor);
	connect(m_toolBar, &IconToolBar::backgroundColorChanged, this, &IconGrid::setBackgroundColor);
	connect(m_toolBar, &IconToolBar::iconSizeChanged, this, &IconGrid::setIconSize);

	connect(m_listView->selectionModel(), &QItemSelectionModel::currentChanged,
			this, &IconGrid::onSelectionChanged);
	connect(m_listView, &QListView::doubleClicked, this, &IconGrid::onDoubleClicked);
	connect(m_listView, &QListView::customContextMenuRequested, this, &IconGrid::onContextMenu);

	// Entity editing
	connect(m_preview, &IconPreview::entitiesChanged, this, [this](const EntityMap &entities) {
		QModelIndex current = m_listView->currentIndex();
		if (current.isValid()) {
			int actualIndex = current.data(IconIndexRole).toInt();
			m_model->setIconEntities(actualIndex, entities);
			// Refresh the preview with new pixmap
			QPixmap largePixmap = m_model->getIconPixmap(actualIndex);
			QString name = current.data(IconNameRole).toString();
			QString svg = m_model->getIconSvg(actualIndex);
			m_preview->setIcon(largePixmap, name, svg, m_toolBar->currentStyle(),
							   m_model->iconSize(), m_model->getIconAliases(actualIndex));
		}
	});

}

IconGrid::~IconGrid() = default;

void IconGrid::setIconList(IconList *list) {
	m_model->setIconList(list);
	m_preview->clear();
}

IconModel *IconGrid::model() const {
	return m_model;
}

IconPreview *IconGrid::preview() const {
	return m_preview;
}

IconToolBar *IconGrid::toolBar() const {
	return m_toolBar;
}

void IconGrid::setIconSize(int size) {
	m_model->setIconSize(size);
	m_delegate->setIconSize(size);
	m_listView->doItemsLayout(); // Force relayout
}

int IconGrid::iconSize() const {
	return m_model->iconSize();
}

void IconGrid::setFilter(const QString &filter) {
	m_model->setFilter(filter);
}

void IconGrid::setFillColor(const QColor &color) {
	m_model->setFillColor(color);
}

void IconGrid::setBackgroundColor(const QColor &color) {
	m_model->setBackgroundColor(color);
}

void IconGrid::onSelectionChanged(const QModelIndex &current, const QModelIndex &previous) {
	Q_UNUSED(previous)

	if (!current.isValid()) {
		m_preview->clear();
		return;
	}

	QString name = current.data(IconNameRole).toString();
	QString svg = current.data(IconSvgRole).toString();
	QPixmap pixmap = current.data(Qt::DecorationRole).value<QPixmap>();

	// Render at larger size for preview
	int actualIndex = current.data(IconIndexRole).toInt();
	QPixmap largePixmap = m_model->getIconPixmap(actualIndex);

	// Get aliases for bitmap icons
	QStringList aliases = m_model->getIconAliases(actualIndex);

	QString style = m_toolBar->currentStyle();
	int size = m_model->iconSize();
	m_preview->setIcon(largePixmap, name, svg, style, size, aliases);

	// Set entities if any
	EntityMap entities = m_model->getIconEntities(actualIndex);
	m_preview->setEntities(entities);

	emit iconSelected(actualIndex, name);
}

void IconGrid::onDoubleClicked(const QModelIndex &index) {
	if (!index.isValid())
		return;

	// Double-click adds to export list
	addCurrentToExportList();
}

void IconGrid::onContextMenu(const QPoint &pos) {
	QModelIndex index = m_listView->indexAt(pos);
	if (!index.isValid())
		return;

	m_contextMenu->exec(m_listView->viewport()->mapToGlobal(pos));
}

void IconGrid::onAddToExport() {
	addCurrentToExportList();
}

void IconGrid::addCurrentToExportList() {
	QModelIndex current = m_listView->currentIndex();
	if (!current.isValid())
		return;

	ExportIconInfo info;
	info.name = current.data(IconNameRole).toString();
	info.svg = current.data(IconSvgRole).toString();

	int actualIndex = current.data(IconIndexRole).toInt();
	info.pixmap = m_model->getIconPixmap(actualIndex);
	info.style = m_toolBar->currentStyle();
	info.size = m_model->iconSize();

	m_preview->addToExportList(info);
}

