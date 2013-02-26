/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  <copyright holder> <email>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#include "desktopwindow.h"
#include <QWidget>
#include <QDesktopWidget>
#include "./application.h"
#include "mainwindow.h"

#include <QPainter>
#include <QImage>
#include <QPixmap>
#include <QPalette>
#include <QBrush>

using namespace PCManFM;

DesktopWindow::DesktopWindow():
  View(Fm::FolderView::IconMode),
  folder_(NULL),
  model_(NULL),
  proxyModel_(NULL),
  wallpaperMode_(WallpaperNone) {

  QDesktopWidget* desktopWidget = QApplication::desktop();
  setWindowFlags(Qt::Window|Qt::FramelessWindowHint);
  setAttribute(Qt::WA_X11NetWmWindowTypeDesktop);
  setAttribute(Qt::WA_OpaquePaintEvent);

  model_ = new Fm::FolderModel();
  proxyModel_ = new Fm::ProxyFolderModel();
  proxyModel_->setSourceModel(model_);
  folder_ = fm_folder_from_path(fm_path_get_desktop());
  model_->setFolder(folder_);
  setModel(proxyModel_);

  QListView* listView = static_cast<QListView*>(childView());
  listView->setMovement(QListView::Snap);
  listView->setResizeMode(QListView::Adjust);
  listView->setFlow(QListView::TopToBottom);
  // setGridSize();

  // remove frame
  listView->setFrameShape(QFrame::NoFrame);
  
  // inhibit scrollbars FIXME: this should be optional in the future
  listView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  connect(this, SIGNAL(openDirRequested(FmPath*,int)), SLOT(onOpenDirRequested(FmPath*,int)));
}

DesktopWindow::~DesktopWindow() {
  if(proxyModel_)
    delete proxyModel_;
  if(model_)
    delete model_;
  if(folder_)
    g_object_unref(folder_);
}

void DesktopWindow::setBackground(const QColor& color) {
  bgColor_ = color;
}

void DesktopWindow::setForeground(const QColor& color) {
  QListView* listView = static_cast<QListView*>(childView());
  listView->palette().setBrush(QPalette::Text, color);
  fgColor_ = color;
}

void DesktopWindow::setShadow(const QColor& color) {
  // TODO: implement drawing text with shadow
  shadowColor_ = color;
}

void DesktopWindow::onOpenDirRequested(FmPath* path, int target) {
  // open in new window unconditionally.
  Application* app = static_cast<Application*>(qApp);
  MainWindow* newWin = new MainWindow(path);
  // TODO: apply window size from app->settings
  newWin->resize(640, 480);
  newWin->show();
}

void DesktopWindow::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  // resize or wall paper if needed
  if(isVisible() && wallpaperMode_ != WallpaperNone && wallpaperMode_ != WallpaperTile) {
    updateWallpaper();
    update();
  }
}

void DesktopWindow::setWallpaperFile(QString filename) {
  wallpaperFile_ = filename;
}

void DesktopWindow::setWallpaperMode(WallpaperMode mode) {
  wallpaperMode_ = mode;
}

void DesktopWindow::updateWallpaper() {
  // reset the brush
  QListView* listView = static_cast<QListView*>(childView());
  QPalette palette(listView->palette());

  if(wallpaperMode_ == WallpaperNone) { // use background color only
    palette.setBrush(QPalette::Base, bgColor_);
  }
  else { // use wallpaper
    QImage image(wallpaperFile_);
    if(image.isNull()) { // image file cannot be loaded
      palette.setBrush(QPalette::Base, bgColor_);
      QPixmap empty;
      wallpaperPixmap_ = empty; // clear the pixmap
    }
    else { // image file is successfully loaded
      qDebug("Image is loaded!");
      QPixmap pixmap;
      if(wallpaperMode_ == WallpaperTile || image.size() == size()) {
        // if image size == window size, there are no differences among different modes
        pixmap = QPixmap::fromImage(image);
      }
      else if(wallpaperMode_ == WallpaperStretch) {
        QImage scaled = image.scaled(width(), height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        pixmap = QPixmap::fromImage(scaled);
      }
      else {
        QPixmap pixmap(size());
        QPainter painter(&pixmap);
        
        if(wallpaperMode_ == WallpaperFit) {
          QImage scaled = image.scaled(width(), height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
          image = scaled;
        }
        pixmap.fill(bgColor_);
        int x = width() - image.width();
        int y = height() - image.height();
        painter.drawImage(x, y, image);
      }
      wallpaperPixmap_ = pixmap;
      palette.setBrush(QPalette::Base, wallpaperPixmap_);
    } // if(image.isNull())
  }
  listView->setPalette(palette);
}

#include "desktopwindow.moc"
