/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>
#include <QtTest/QSignalSpy>
#include <QtGui/qcursor.h>
#include <QtGui/qstylehints.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquickitem_p.h>
#include "../shared/util.h"
#include "../shared/visualtestutil.h"

#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickoverlay_p.h>
#include <QtQuickTemplates2/private/qquickbutton_p.h>
#include <QtQuickTemplates2/private/qquickmenu_p.h>
#include <QtQuickTemplates2/private/qquickmenuitem_p.h>
#include <QtQuickTemplates2/private/qquickmenuseparator_p.h>

using namespace QQuickVisualTestUtil;

class tst_menu : public QQmlDataTest
{
    Q_OBJECT

public:

private slots:
    void defaults();
    void mouse();
    void contextMenuKeyboard();
    void menuButton();
    void addItem();
    void menuSeparator();
    void repeater();
    void order();
    void popup();
    void actions();
    void removeTakeItem();
    void subMenuMouse_data();
    void subMenuMouse();
    void subMenuKeyboard_data();
    void subMenuKeyboard();
    void subMenuPosition_data();
    void subMenuPosition();
};

void tst_menu::defaults()
{
    QQuickApplicationHelper helper(this, QLatin1String("applicationwindow.qml"));

    QQuickMenu *emptyMenu = helper.appWindow->property("emptyMenu").value<QQuickMenu*>();
    QCOMPARE(emptyMenu->isVisible(), false);
    QCOMPARE(emptyMenu->contentItem()->property("currentIndex"), QVariant(-1));
}

void tst_menu::mouse()
{
    QQuickApplicationHelper helper(this, QLatin1String("applicationwindow.qml"));

    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));
    moveMouseAway(window);

    QQuickMenu *menu = window->property("menu").value<QQuickMenu*>();
    menu->open();
    QVERIFY(menu->isVisible());
    QVERIFY(window->overlay()->childItems().contains(menu->contentItem()->parentItem()));

    QQuickItem *firstItem = menu->itemAt(0);
    QSignalSpy clickedSpy(firstItem, SIGNAL(clicked()));
    QSignalSpy triggeredSpy(firstItem, SIGNAL(triggered()));
    QSignalSpy visibleSpy(menu, SIGNAL(visibleChanged()));

    // Ensure that presses cause the current index to change,
    // so that the highlight acts as a way of illustrating press state.
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(firstItem->width() / 2, firstItem->height() / 2));
    QVERIFY(firstItem->hasActiveFocus());
    QCOMPARE(menu->contentItem()->property("currentIndex"), QVariant(0));
    QVERIFY(menu->isVisible());

    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(firstItem->width() / 2, firstItem->height() / 2));
    QCOMPARE(clickedSpy.count(), 1);
    QCOMPARE(triggeredSpy.count(), 1);
    QCOMPARE(visibleSpy.count(), 1);
    QVERIFY(!menu->isVisible());
    QVERIFY(!window->overlay()->childItems().contains(menu->contentItem()));
    QCOMPARE(menu->contentItem()->property("currentIndex"), QVariant(-1));

    menu->open();
    QCOMPARE(visibleSpy.count(), 2);
    QVERIFY(menu->isVisible());
    QVERIFY(window->overlay()->childItems().contains(menu->contentItem()->parentItem()));

    // Ensure that we have enough space to click outside of the menu.
    QVERIFY(window->width() > menu->contentItem()->width());
    QVERIFY(window->height() > menu->contentItem()->height());
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
        QPoint(menu->contentItem()->width() + 1, menu->contentItem()->height() + 1));
    QCOMPARE(visibleSpy.count(), 3);
    QVERIFY(!menu->isVisible());
    QVERIFY(!window->overlay()->childItems().contains(menu->contentItem()->parentItem()));

    menu->open();
    QCOMPARE(visibleSpy.count(), 4);
    QVERIFY(menu->isVisible());
    QVERIFY(window->overlay()->childItems().contains(menu->contentItem()->parentItem()));

    // Hover-highlight through the menu items one by one
    QQuickItem *prevHoverItem = nullptr;
    QQuickItem *listView = menu->contentItem();
    for (int y = 0; y < listView->height(); ++y) {
        QQuickItem *hoverItem = nullptr;
        QVERIFY(QMetaObject::invokeMethod(listView, "itemAt", Q_RETURN_ARG(QQuickItem *, hoverItem), Q_ARG(qreal, 0), Q_ARG(qreal, listView->property("contentY").toReal() + y)));
        if (!hoverItem || !hoverItem->isVisible() || hoverItem == prevHoverItem)
            continue;
        QTest::mouseMove(window, QPoint(hoverItem->x() + hoverItem->width() / 2, hoverItem->y() + hoverItem->height() / 2));
        QTRY_VERIFY(hoverItem->property("highlighted").toBool());
        if (prevHoverItem)
            QVERIFY(!prevHoverItem->property("highlighted").toBool());
        prevHoverItem = hoverItem;
    }

    // Try pressing within the menu and releasing outside of it; it should close.
    // TODO: won't work until QQuickPopup::releasedOutside() actually gets emitted
//    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(firstItem->width() / 2, firstItem->height() / 2));
//    QVERIFY(firstItem->hasActiveFocus());
//    QCOMPARE(menu->contentItem()->property("currentIndex"), QVariant(0));
//    QVERIFY(menu->isVisible());
//    QCOMPARE(triggeredSpy.count(), 1);

//    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(menu->contentItem()->width() + 1, firstItem->height() / 2));
//    QCOMPARE(clickedSpy.count(), 1);
//    QCOMPARE(triggeredSpy.count(), 1);
//    QCOMPARE(visibleSpy.count(), 5);
//    QVERIFY(!menu->isVisible());
//    QVERIFY(!window->overlay()->childItems().contains(menu->contentItem()));
//    QCOMPARE(menu->contentItem()->property("currentIndex"), QVariant(-1));
}

void tst_menu::contextMenuKeyboard()
{
    if (QGuiApplication::styleHints()->tabFocusBehavior() != Qt::TabFocusAllControls)
        QSKIP("This platform only allows tab focus for text controls");

    QQuickApplicationHelper helper(this, QLatin1String("applicationwindow.qml"));

    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QVERIFY(QGuiApplication::focusWindow() == window);
    moveMouseAway(window);

    QQuickMenu *menu = window->property("menu").value<QQuickMenu*>();
    QCOMPARE(menu->contentItem()->property("currentIndex"), QVariant(-1));

    QQuickItem *firstItem = menu->itemAt(0);
    QSignalSpy visibleSpy(menu, SIGNAL(visibleChanged()));

    menu->setFocus(true);
    menu->open();
    QCOMPARE(visibleSpy.count(), 1);
    QVERIFY(menu->isVisible());
    QVERIFY(window->overlay()->childItems().contains(menu->contentItem()->parentItem()));
    QVERIFY(!firstItem->hasActiveFocus());
    QVERIFY(!firstItem->property("highlighted").toBool());
    QCOMPARE(menu->contentItem()->property("currentIndex"), QVariant(-1));

    QTest::keyClick(window, Qt::Key_Tab);
    QVERIFY(firstItem->hasActiveFocus());
    QVERIFY(firstItem->property("highlighted").toBool());
    QCOMPARE(menu->contentItem()->property("currentIndex"), QVariant(0));

    QQuickItem *secondItem = menu->itemAt(1);
    QTest::keyClick(window, Qt::Key_Tab);
    QVERIFY(!firstItem->hasActiveFocus());
    QVERIFY(!firstItem->property("highlighted").toBool());
    QVERIFY(secondItem->hasActiveFocus());
    QVERIFY(secondItem->property("highlighted").toBool());
    QCOMPARE(menu->contentItem()->property("currentIndex"), QVariant(1));

    QSignalSpy secondTriggeredSpy(secondItem, SIGNAL(triggered()));
    QTest::keyClick(window, Qt::Key_Space);
    QCOMPARE(secondTriggeredSpy.count(), 1);
    QCOMPARE(visibleSpy.count(), 2);
    QVERIFY(!menu->isVisible());
    QVERIFY(!window->overlay()->childItems().contains(menu->contentItem()));
    QVERIFY(!firstItem->hasActiveFocus());
    QVERIFY(!firstItem->property("highlighted").toBool());
    QVERIFY(!secondItem->hasActiveFocus());
    QVERIFY(!secondItem->property("highlighted").toBool());
    QCOMPARE(menu->contentItem()->property("currentIndex"), QVariant(-1));

    menu->open();
    QCOMPARE(visibleSpy.count(), 3);
    QVERIFY(menu->isVisible());
    QVERIFY(window->overlay()->childItems().contains(menu->contentItem()->parentItem()));
    QVERIFY(!firstItem->hasActiveFocus());
    QVERIFY(!firstItem->property("highlighted").toBool());
    QVERIFY(!secondItem->hasActiveFocus());
    QVERIFY(!secondItem->property("highlighted").toBool());
    QCOMPARE(menu->contentItem()->property("currentIndex"), QVariant(-1));

    QTest::keyClick(window, Qt::Key_Down);
    QVERIFY(firstItem->hasActiveFocus());
    QVERIFY(firstItem->property("highlighted").toBool());

    QTest::keyClick(window, Qt::Key_Down);
    QVERIFY(secondItem->hasActiveFocus());
    QVERIFY(secondItem->property("highlighted").toBool());

    QTest::keyClick(window, Qt::Key_Down);
    QQuickItem *thirdItem = menu->itemAt(2);
    QVERIFY(!firstItem->hasActiveFocus());
    QVERIFY(!firstItem->property("highlighted").toBool());
    QVERIFY(!secondItem->hasActiveFocus());
    QVERIFY(!secondItem->property("highlighted").toBool());
    QVERIFY(thirdItem->hasActiveFocus());
    QVERIFY(thirdItem->property("highlighted").toBool());

    // Key navigation shouldn't wrap by default.
    QTest::keyClick(window, Qt::Key_Down);
    QVERIFY(!firstItem->hasActiveFocus());
    QVERIFY(!firstItem->property("highlighted").toBool());
    QVERIFY(!secondItem->hasActiveFocus());
    QVERIFY(!secondItem->property("highlighted").toBool());
    QVERIFY(thirdItem->hasActiveFocus());
    QVERIFY(thirdItem->property("highlighted").toBool());

    QTest::keyClick(window, Qt::Key_Escape);
    QCOMPARE(visibleSpy.count(), 4);
    QVERIFY(!menu->isVisible());
}

void tst_menu::menuButton()
{
    if (QGuiApplication::styleHints()->tabFocusBehavior() != Qt::TabFocusAllControls)
        QSKIP("This platform only allows tab focus for text controls");

    QQuickApplicationHelper helper(this, QLatin1String("applicationwindow.qml"));

    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QVERIFY(QGuiApplication::focusWindow() == window);

    QQuickMenu *menu = window->property("menu").value<QQuickMenu*>();
    QQuickButton *menuButton = window->property("menuButton").value<QQuickButton*>();
    QSignalSpy visibleSpy(menu, SIGNAL(visibleChanged()));

    menuButton->setVisible(true);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
        menuButton->mapToScene(QPointF(menuButton->width() / 2, menuButton->height() / 2)).toPoint());
    QCOMPARE(visibleSpy.count(), 1);
    QVERIFY(menu->isVisible());

    QTest::keyClick(window, Qt::Key_Tab);
    QQuickItem *firstItem = menu->itemAt(0);
    QVERIFY(firstItem->hasActiveFocus());
}

void tst_menu::addItem()
{
    QQuickApplicationHelper helper(this, QLatin1String("addItem.qml"));
    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickMenu *menu = window->property("menu").value<QQuickMenu*>();
    QVERIFY(menu);
    menu->open();
    QVERIFY(menu->isVisible());

    QQuickItem *menuItem = menu->itemAt(0);
    QVERIFY(menuItem);
    QTRY_VERIFY(!QQuickItemPrivate::get(menuItem)->culled); // QTBUG-53262

    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
        menuItem->mapToScene(QPointF(menuItem->width() / 2, menuItem->height() / 2)).toPoint());
    QTRY_VERIFY(!menu->isVisible());
}

void tst_menu::menuSeparator()
{
    QQuickApplicationHelper helper(this, QLatin1String("menuSeparator.qml"));
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickMenu *menu = window->property("menu").value<QQuickMenu*>();
    QVERIFY(menu);
    menu->open();
    QVERIFY(menu->isVisible());

    QQuickMenuItem *newMenuItem = qobject_cast<QQuickMenuItem*>(menu->itemAt(0));
    QVERIFY(newMenuItem);
    QCOMPARE(newMenuItem->text(), QStringLiteral("New"));

    QQuickMenuSeparator *menuSeparator = qobject_cast<QQuickMenuSeparator*>(menu->itemAt(1));
    QVERIFY(menuSeparator);

    QQuickMenuItem *saveMenuItem = qobject_cast<QQuickMenuItem*>(menu->itemAt(2));
    QVERIFY(saveMenuItem);
    QCOMPARE(saveMenuItem->text(), QStringLiteral("Save"));
    QTRY_VERIFY(!QQuickItemPrivate::get(saveMenuItem)->culled); // QTBUG-53262

    // Clicking on items should still close the menu.
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
        newMenuItem->mapToScene(QPointF(newMenuItem->width() / 2, newMenuItem->height() / 2)).toPoint());
    QTRY_VERIFY(!menu->isVisible());

    menu->open();
    QVERIFY(menu->isVisible());

    // Clicking on a separator shouldn't close the menu.
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
        menuSeparator->mapToScene(QPointF(menuSeparator->width() / 2, menuSeparator->height() / 2)).toPoint());
    QVERIFY(menu->isVisible());

    // Clicking on items should still close the menu.
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
        saveMenuItem->mapToScene(QPointF(saveMenuItem->width() / 2, saveMenuItem->height() / 2)).toPoint());
    QTRY_VERIFY(!menu->isVisible());
}

void tst_menu::repeater()
{
    QQuickApplicationHelper helper(this, QLatin1String("repeater.qml"));
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickMenu *menu = window->property("menu").value<QQuickMenu*>();
    QVERIFY(menu);
    menu->open();
    QVERIFY(menu->isVisible());

    QObject *repeater = window->property("repeater").value<QObject*>();
    QVERIFY(repeater);

    int count = repeater->property("count").toInt();
    QCOMPARE(count, 5);

    for (int i = 0; i < count; ++i) {
        QQuickItem *item = menu->itemAt(i);
        QVERIFY(item);
        QCOMPARE(item->property("idx").toInt(), i);

        QQuickItem *repeaterItem = nullptr;
        QVERIFY(QMetaObject::invokeMethod(repeater, "itemAt", Q_RETURN_ARG(QQuickItem*, repeaterItem), Q_ARG(int, i)));
        QCOMPARE(item, repeaterItem);
    }

    repeater->setProperty("model", 3);

    count = repeater->property("count").toInt();
    QCOMPARE(count, 3);

    for (int i = 0; i < count; ++i) {
        QQuickItem *item = menu->itemAt(i);
        QVERIFY(item);
        QCOMPARE(item->property("idx").toInt(), i);

        QQuickItem *repeaterItem = nullptr;
        QVERIFY(QMetaObject::invokeMethod(repeater, "itemAt", Q_RETURN_ARG(QQuickItem*, repeaterItem), Q_ARG(int, i)));
        QCOMPARE(item, repeaterItem);
    }
}

void tst_menu::order()
{
    QQuickApplicationHelper helper(this, QLatin1String("order.qml"));
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickMenu *menu = window->property("menu").value<QQuickMenu*>();
    QVERIFY(menu);
    menu->open();
    QVERIFY(menu->isVisible());

    const QStringList texts = {"dynamic_0", "static_1", "repeated_2", "repeated_3", "static_4", "dynamic_5", "dynamic_6"};

    for (int i = 0; i < texts.count(); ++i) {
        QQuickItem *item = menu->itemAt(i);
        QVERIFY(item);
        QCOMPARE(item->property("text").toString(), texts.at(i));
    }
}

void tst_menu::popup()
{
    QQuickApplicationHelper helper(this, QLatin1String("popup.qml"));
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickMenu *menu = window->property("menu").value<QQuickMenu *>();
    QVERIFY(menu);

    QQuickMenuItem *menuItem1 = window->property("menuItem1").value<QQuickMenuItem *>();
    QVERIFY(menuItem1);

    QQuickMenuItem *menuItem2 = window->property("menuItem2").value<QQuickMenuItem *>();
    QVERIFY(menuItem2);

    QQuickMenuItem *menuItem3 = window->property("menuItem3").value<QQuickMenuItem *>();
    QVERIFY(menuItem3);

#if QT_CONFIG(cursor)
    QPoint oldCursorPos = QCursor::pos();
    QPoint cursorPos = window->mapToGlobal(QPoint(11, 22));
    QCursor::setPos(cursorPos);
    QTRY_COMPARE(QCursor::pos(), cursorPos);

    QVERIFY(QMetaObject::invokeMethod(window, "popupAtCursor"));
    QCOMPARE(menu->contentItem()->property("currentIndex").toInt(), -1);
    QTRY_COMPARE(menu->property("x").toInt(), 11);
    QTRY_COMPARE(menu->property("y").toInt(), 22);
    menu->close();

    QVERIFY(QMetaObject::invokeMethod(window, "popupAtPos", Q_ARG(QVariant, QPointF(33, 44))));
    QCOMPARE(menu->contentItem()->property("currentIndex").toInt(), -1);
    QTRY_COMPARE(menu->property("x").toInt(), 33);
    QTRY_COMPARE(menu->property("y").toInt(), 44);
    menu->close();

    QVERIFY(QMetaObject::invokeMethod(window, "popupAtCoord", Q_ARG(QVariant, 55), Q_ARG(QVariant, 66)));
    QCOMPARE(menu->contentItem()->property("currentIndex").toInt(), -1);
    QTRY_COMPARE(menu->property("x").toInt(), 55);
    QTRY_COMPARE(menu->property("y").toInt(), 66);
    menu->close();

    cursorPos = window->mapToGlobal(QPoint(12, window->height() / 2));
    QCursor::setPos(cursorPos);
    QTRY_COMPARE(QCursor::pos(), cursorPos);

    const QList<QQuickMenuItem *> menuItems = QList<QQuickMenuItem *>() << menuItem1 << menuItem2 << menuItem3;
    for (QQuickMenuItem *menuItem : menuItems) {
        QVERIFY(QMetaObject::invokeMethod(window, "popupItemAtCursor", Q_ARG(QVariant, QVariant::fromValue(menuItem))));
        QCOMPARE(menu->contentItem()->property("currentIndex").toInt(), menuItems.indexOf(menuItem));
        QTRY_COMPARE(menu->property("x").toInt(), 12);
        QTRY_COMPARE(menu->property("y").toInt(), window->height() / 2 + menu->topPadding() - menuItem->y());
        menu->close();

        QVERIFY(QMetaObject::invokeMethod(window, "popupItemAtPos", Q_ARG(QVariant, QPointF(33, window->height() / 3)), Q_ARG(QVariant, QVariant::fromValue(menuItem))));
        QCOMPARE(menu->contentItem()->property("currentIndex").toInt(), menuItems.indexOf(menuItem));
        QTRY_COMPARE(menu->property("x").toInt(), 33);
        QTRY_COMPARE(menu->property("y").toInt(), window->height() / 3 + menu->topPadding() - menuItem->y());
        menu->close();

        QVERIFY(QMetaObject::invokeMethod(window, "popupItemAtCoord", Q_ARG(QVariant, 55), Q_ARG(QVariant, window->height() / 3 * 2), Q_ARG(QVariant, QVariant::fromValue(menuItem))));
        QCOMPARE(menu->contentItem()->property("currentIndex").toInt(), menuItems.indexOf(menuItem));
        QTRY_COMPARE(menu->property("x").toInt(), 55);
        QTRY_COMPARE(menu->property("y").toInt(), window->height() / 3 * 2 + menu->topPadding() - menuItem->y());
        menu->close();
    }

    QCursor::setPos(oldCursorPos);
    QTRY_COMPARE(QCursor::pos(), oldCursorPos);
#endif
}

void tst_menu::actions()
{
    QQuickApplicationHelper helper(this, QLatin1String("actions.qml"));
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickMenu *menu = window->property("menu").value<QQuickMenu *>();
    QVERIFY(menu);

    QQuickMenuItem *menuItem1 = qobject_cast<QQuickMenuItem *>(menu->itemAt(0));
    QVERIFY(menuItem1);
    QVERIFY(menuItem1->action());
    QCOMPARE(menuItem1->text(), "action1");

    QQuickMenuItem *menuItem2 = qobject_cast<QQuickMenuItem *>(menu->itemAt(1));
    QVERIFY(menuItem2);
    QVERIFY(!menuItem2->action());
    QCOMPARE(menuItem2->text(), "menuitem2");

    QQuickMenuItem *menuItem3 = qobject_cast<QQuickMenuItem *>(menu->itemAt(2));
    QVERIFY(menuItem3);
    QVERIFY(menuItem3->action());
    QCOMPARE(menuItem3->text(), "action3");

    QQuickMenuItem *menuItem4 = qobject_cast<QQuickMenuItem *>(menu->itemAt(3));
    QVERIFY(menuItem4);
    QVERIFY(!menuItem4->action());
    QCOMPARE(menuItem4->text(), "menuitem4");
}

void tst_menu::removeTakeItem()
{
    QQuickApplicationHelper helper(this, QLatin1String("removeTakeItem.qml"));
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickMenu *menu = window->property("menu").value<QQuickMenu *>();
    QVERIFY(menu);

    QPointer<QQuickMenuItem> menuItem1 = window->property("menuItem1").value<QQuickMenuItem *>();
    QVERIFY(!menuItem1.isNull());
    QCOMPARE(menuItem1->menu(), menu);

    QPointer<QQuickMenuItem> menuItem2 = window->property("menuItem2").value<QQuickMenuItem *>();
    QVERIFY(!menuItem2.isNull());
    QCOMPARE(menuItem2->menu(), menu);

    QPointer<QQuickMenuItem> menuItem3 = window->property("menuItem3").value<QQuickMenuItem *>();
    QVERIFY(!menuItem3.isNull());
    QCOMPARE(menuItem3->menu(), menu);

    // takeItem(int) does not destroy
    QVariant ret;
    QVERIFY(QMetaObject::invokeMethod(window, "takeSecondItem", Q_RETURN_ARG(QVariant, ret)));
    QCOMPARE(ret.value<QQuickMenuItem *>(), menuItem2);
    QVERIFY(!menuItem2->menu());
    QCoreApplication::sendPostedEvents(menuItem2, QEvent::DeferredDelete);
    QVERIFY(!menuItem2.isNull());

    // removeItem(Item) destroys
    QVERIFY(QMetaObject::invokeMethod(window, "removeFirstItem"));
    QVERIFY(!menuItem1->menu());
    QCoreApplication::sendPostedEvents(menuItem1, QEvent::DeferredDelete);
    QVERIFY(menuItem1.isNull());

    // removeItem(null) must not call removeItem(0)
    QVERIFY(QMetaObject::invokeMethod(window, "removeNullItem"));
    QCOMPARE(menuItem3->menu(), menu);
    QCoreApplication::sendPostedEvents(menuItem3, QEvent::DeferredDelete);
    QVERIFY(!menuItem3.isNull());

    // deprecated removeItem(int) does not destroy
    QVERIFY(QMetaObject::invokeMethod(window, "removeFirstIndex"));
    QVERIFY(!menuItem3->menu());
    QCoreApplication::sendPostedEvents(menuItem3, QEvent::DeferredDelete);
    QVERIFY(!menuItem3.isNull());
}

void tst_menu::subMenuMouse_data()
{
    QTest::addColumn<bool>("cascade");

    QTest::newRow("cascading") << true;
    QTest::newRow("non-cascading") << false;
}

void tst_menu::subMenuMouse()
{
    QFETCH(bool, cascade);

    QQuickApplicationHelper helper(this, QLatin1String("subMenus.qml"));
    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));
    moveMouseAway(window);

    QQuickMenu *mainMenu = window->property("mainMenu").value<QQuickMenu *>();
    QVERIFY(mainMenu);
    mainMenu->setCascade(cascade);
    QCOMPARE(mainMenu->cascade(), cascade);

    QQuickMenu *subMenu1 = window->property("subMenu1").value<QQuickMenu *>();
    QVERIFY(subMenu1);

    QQuickMenu *subMenu2 = window->property("subMenu2").value<QQuickMenu *>();
    QVERIFY(subMenu2);

    QQuickMenu *subSubMenu1 = window->property("subSubMenu1").value<QQuickMenu *>();
    QVERIFY(subSubMenu1);

    mainMenu->open();
    QVERIFY(mainMenu->isVisible());
    QVERIFY(!subMenu1->isVisible());
    QVERIFY(!subMenu2->isVisible());
    QVERIFY(!subSubMenu1->isVisible());

    // open the sub-menu with mouse click
    QQuickMenuItem *subMenu1Item = qobject_cast<QQuickMenuItem *>(mainMenu->itemAt(1));
    QVERIFY(subMenu1Item);
    QCOMPARE(subMenu1Item->subMenu(), subMenu1);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, subMenu1Item->mapToScene(QPoint(1, 1)).toPoint());
    QCOMPARE(mainMenu->isVisible(), cascade);
    QVERIFY(subMenu1->isVisible());
    QVERIFY(!subMenu2->isVisible());
    QVERIFY(!subSubMenu1->isVisible());

    // open the cascading sub-sub-menu with mouse hover
    QQuickMenuItem *subSubMenu1Item = qobject_cast<QQuickMenuItem *>(subMenu1->itemAt(2));
    QVERIFY(subSubMenu1Item);
    QCOMPARE(subSubMenu1Item->subMenu(), subSubMenu1);
    QTest::mouseMove(window, subSubMenu1Item->mapToScene(QPoint(1, 1)).toPoint());
    QCOMPARE(mainMenu->isVisible(), cascade);
    QVERIFY(subMenu1->isVisible());
    QVERIFY(!subMenu2->isVisible());
    QCOMPARE(subSubMenu1->isVisible(), cascade);

    // close the sub-sub-menu with mouse hover over another parent menu item
    QQuickMenuItem *subMenuItem1 = qobject_cast<QQuickMenuItem *>(subMenu1->itemAt(0));
    QVERIFY(subMenuItem1);
    QVERIFY(!subMenuItem1->subMenu());
    QTest::mouseMove(window, subMenuItem1->mapToScene(QPoint(1, 1)).toPoint());
    QCOMPARE(mainMenu->isVisible(), cascade);
    QVERIFY(subMenu1->isVisible());
    QVERIFY(!subMenu2->isVisible());
    QVERIFY(!subSubMenu1->isVisible());

    // re-open the sub-sub-menu with mouse hover
    QTest::mouseMove(window, subSubMenu1Item->mapToScene(QPoint(1, 1)).toPoint());
    QCOMPARE(mainMenu->isVisible(), cascade);
    QVERIFY(subMenu1->isVisible());
    QVERIFY(!subMenu2->isVisible());
    QCOMPARE(subSubMenu1->isVisible(), cascade);

    // close sub-menu and sub-sub-menu with mouse hover in the main menu
    QQuickMenuItem *mainMenuItem1 = qobject_cast<QQuickMenuItem *>(mainMenu->itemAt(0));
    QVERIFY(mainMenuItem1);
    QTest::mouseMove(window, mainMenuItem1->mapToScene(QPoint(1, 1)).toPoint());
    QCOMPARE(mainMenu->isVisible(), cascade);
    QCOMPARE(subMenu1->isVisible(), !cascade);
    QVERIFY(!subMenu2->isVisible());
    QVERIFY(!subSubMenu1->isVisible());

    // close all menus by click triggering an item
    QQuickMenuItem *subSubMenuItem1 = qobject_cast<QQuickMenuItem *>(subSubMenu1->itemAt(0));
    QVERIFY(subSubMenuItem1);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, subSubMenuItem1->mapToScene(QPoint(1, 1)).toPoint());
    QVERIFY(!mainMenu->isVisible());
    QVERIFY(!subMenu1->isVisible());
    QVERIFY(!subMenu2->isVisible());
    QVERIFY(!subSubMenu1->isVisible());
}

void tst_menu::subMenuKeyboard_data()
{
    QTest::addColumn<bool>("cascade");
    QTest::addColumn<bool>("mirrored");

    QTest::newRow("cascading") << true << false;
    QTest::newRow("cascading,mirrored") << true << true;
    QTest::newRow("non-cascading") << false << false;
    QTest::newRow("non-cascading,mirrored") << false << true;
}

void tst_menu::subMenuKeyboard()
{
    QFETCH(bool, cascade);
    QFETCH(bool, mirrored);

    QQuickApplicationHelper helper(this, QLatin1String("subMenus.qml"));
    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));
    moveMouseAway(window);

    if (mirrored)
        window->setLocale(QLocale("ar_EG"));

    QQuickMenu *mainMenu = window->property("mainMenu").value<QQuickMenu *>();
    QVERIFY(mainMenu);
    mainMenu->setCascade(cascade);
    QCOMPARE(mainMenu->cascade(), cascade);

    QQuickMenu *subMenu1 = window->property("subMenu1").value<QQuickMenu *>();
    QVERIFY(subMenu1);

    QQuickMenu *subMenu2 = window->property("subMenu2").value<QQuickMenu *>();
    QVERIFY(subMenu2);

    QQuickMenu *subSubMenu1 = window->property("subSubMenu1").value<QQuickMenu *>();
    QVERIFY(subSubMenu1);

    mainMenu->open();
    QVERIFY(mainMenu->isVisible());
    QVERIFY(!subMenu1->isVisible());
    QVERIFY(!subMenu2->isVisible());
    QVERIFY(!subSubMenu1->isVisible());

    // navigate to the sub-menu item and trigger it
    QQuickMenuItem *subMenu1Item = qobject_cast<QQuickMenuItem *>(mainMenu->itemAt(1));
    QVERIFY(subMenu1Item);
    QVERIFY(!subMenu1Item->isHighlighted());
    QCOMPARE(subMenu1Item->subMenu(), subMenu1);
    QTest::keyClick(window, Qt::Key_Down);
    QTest::keyClick(window, Qt::Key_Down);
    QVERIFY(subMenu1Item->isHighlighted());
    QTest::keyClick(window, Qt::Key_Space);
    QCOMPARE(mainMenu->isVisible(), cascade);
    QVERIFY(subMenu1->isVisible());
    QVERIFY(!subMenu2->isVisible());
    QVERIFY(!subSubMenu1->isVisible());

    // navigate to the sub-sub-menu item and open it with the arrow key
    QQuickMenuItem *subSubMenu1Item = qobject_cast<QQuickMenuItem *>(subMenu1->itemAt(2));
    QVERIFY(subSubMenu1Item);
    QVERIFY(!subSubMenu1Item->isHighlighted());
    QCOMPARE(subSubMenu1Item->subMenu(), subSubMenu1);
    QTest::keyClick(window, Qt::Key_Down);
    QTest::keyClick(window, Qt::Key_Down);
    QTest::keyClick(window, Qt::Key_Down);
    QVERIFY(subSubMenu1Item->isHighlighted());
    QCOMPARE(mainMenu->isVisible(), cascade);
    QVERIFY(subMenu1->isVisible());
    QVERIFY(!subMenu2->isVisible());
    QVERIFY(!subSubMenu1->isVisible());
    QTest::keyClick(window, mirrored ? Qt::Key_Left : Qt::Key_Right);
    QCOMPARE(mainMenu->isVisible(), cascade);
    QCOMPARE(subMenu1->isVisible(), cascade);
    QVERIFY(!subMenu2->isVisible());
    QVERIFY(subSubMenu1->isVisible());

    // navigate within the sub-sub-menu
    QQuickMenuItem *subSubMenuItem1 = qobject_cast<QQuickMenuItem *>(subSubMenu1->itemAt(0));
    QVERIFY(subSubMenuItem1);
    QQuickMenuItem *subSubMenuItem2 = qobject_cast<QQuickMenuItem *>(subSubMenu1->itemAt(1));
    QVERIFY(subSubMenuItem2);
    QVERIFY(subSubMenuItem1->isHighlighted());
    QVERIFY(!subSubMenuItem2->isHighlighted());
    QTest::keyClick(window, Qt::Key_Down);
    QVERIFY(!subSubMenuItem1->isHighlighted());
    QVERIFY(subSubMenuItem2->isHighlighted());

    // navigate to the parent menu with the arrow key
    QTest::keyClick(window, mirrored ? Qt::Key_Right : Qt::Key_Left);
    QVERIFY(subSubMenu1Item->isHighlighted());
    QCOMPARE(mainMenu->isVisible(), cascade);
    QVERIFY(subMenu1->isVisible());
    QVERIFY(!subMenu2->isVisible());
    QVERIFY(!subSubMenu1->isVisible());

    // navigate within the sub-menu
    QQuickMenuItem *subMenuItem1 = qobject_cast<QQuickMenuItem *>(subMenu1->itemAt(0));
    QVERIFY(subMenuItem1);
    QQuickMenuItem *subMenuItem2 = qobject_cast<QQuickMenuItem *>(subMenu1->itemAt(1));
    QVERIFY(subMenuItem2);
    QVERIFY(!subMenuItem1->isHighlighted());
    QVERIFY(!subMenuItem2->isHighlighted());
    QVERIFY(subSubMenu1Item->isHighlighted());
    QTest::keyClick(window, Qt::Key_Up);
    QVERIFY(!subMenuItem1->isHighlighted());
    QVERIFY(subMenuItem2->isHighlighted());
    QVERIFY(!subSubMenu1Item->isHighlighted());

    // close the menus with esc
    QTest::keyClick(window, Qt::Key_Escape);
    QCOMPARE(mainMenu->isVisible(), cascade);
    QVERIFY(!subMenu1->isVisible());
    QVERIFY(!subMenu2->isVisible());
    QVERIFY(!subSubMenu1->isVisible());
    QTest::keyClick(window, Qt::Key_Escape);
    QVERIFY(!mainMenu->isVisible());
    QVERIFY(!subMenu1->isVisible());
    QVERIFY(!subMenu2->isVisible());
    QVERIFY(!subSubMenu1->isVisible());
}

void tst_menu::subMenuPosition_data()
{
    QTest::addColumn<bool>("cascade");
    QTest::addColumn<bool>("mirrored");
    QTest::addColumn<qreal>("overlap");

    QTest::newRow("cascading") << true << false << 0.0;
    QTest::newRow("cascading,overlap") << true << false << 10.0;
    QTest::newRow("cascading,mirrored") << true << true << 0.0;
    QTest::newRow("cascading,mirrored,overlap") << true << true << 10.0;
    QTest::newRow("non-cascading") << false << false << 0.0;
}

void tst_menu::subMenuPosition()
{
    QFETCH(bool, cascade);
    QFETCH(bool, mirrored);
    QFETCH(qreal, overlap);

    QQuickApplicationHelper helper(this, QLatin1String("subMenus.qml"));
    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));
    moveMouseAway(window);

    if (mirrored)
        window->setLocale(QLocale("ar_EG"));

    QQuickMenu *mainMenu = window->property("mainMenu").value<QQuickMenu *>();
    QVERIFY(mainMenu);
    mainMenu->setCascade(cascade);
    QCOMPARE(mainMenu->cascade(), cascade);
    mainMenu->setOverlap(overlap);
    QCOMPARE(mainMenu->overlap(), overlap);

    QQuickMenu *subMenu1 = window->property("subMenu1").value<QQuickMenu *>();
    QVERIFY(subMenu1);
    subMenu1->setCascade(cascade);
    QCOMPARE(subMenu1->cascade(), cascade);
    subMenu1->setOverlap(overlap);
    QCOMPARE(subMenu1->overlap(), overlap);

    QQuickMenu *subMenu2 = window->property("subMenu2").value<QQuickMenu *>();
    QVERIFY(subMenu2);
    subMenu2->setCascade(cascade);
    QCOMPARE(subMenu2->cascade(), cascade);
    subMenu2->setOverlap(overlap);
    QCOMPARE(subMenu2->overlap(), overlap);

    QQuickMenu *subSubMenu1 = window->property("subSubMenu1").value<QQuickMenu *>();
    QVERIFY(subSubMenu1);
    subSubMenu1->setCascade(cascade);
    QCOMPARE(subSubMenu1->cascade(), cascade);
    subSubMenu1->setOverlap(overlap);
    QCOMPARE(subSubMenu1->overlap(), overlap);

    if (mirrored)
        mainMenu->setPosition(QPointF(290, 10));
    else
        mainMenu->setPosition(QPointF(10, 10));

    mainMenu->open();
    QVERIFY(mainMenu->isVisible());
    QVERIFY(!subMenu1->isVisible());
    QVERIFY(!subMenu2->isVisible());
    QVERIFY(!subSubMenu1->isVisible());

    // open the sub-menu
    QQuickMenuItem *subMenu1Item = qobject_cast<QQuickMenuItem *>(mainMenu->itemAt(1));
    QVERIFY(subMenu1Item);
    QCOMPARE(subMenu1Item->subMenu(), subMenu1);
    emit subMenu1Item->triggered();
    QCOMPARE(mainMenu->isVisible(), cascade);
    QVERIFY(subMenu1->isVisible());
    QVERIFY(!subMenu2->isVisible());
    QVERIFY(!subSubMenu1->isVisible());

    if (cascade) {
        QCOMPARE(subMenu1->parentItem(), subMenu1Item);
        // vertically aligned to the parent menu item
        QCOMPARE(subMenu1->popupItem()->y(), mainMenu->popupItem()->y() + subMenu1Item->y() - subMenu1->topPadding());
        if (mirrored)
            QCOMPARE(subMenu1->popupItem()->x(), mainMenu->popupItem()->x() - subMenu1->width() + overlap); // on the left of the parent menu
        else
            QCOMPARE(subMenu1->popupItem()->x(), mainMenu->popupItem()->x() + mainMenu->width() - overlap); // on the right of the parent menu
    } else {
        QCOMPARE(subMenu1->parentItem(), mainMenu->parentItem());
        // centered over the parent menu
        QCOMPARE(subMenu1->popupItem()->x(), mainMenu->popupItem()->x() + (mainMenu->width() - subMenu1->width()) / 2);
        QCOMPARE(subMenu1->popupItem()->y(), mainMenu->popupItem()->y() + (mainMenu->height() - subMenu1->height()) / 2);
    }

    // open the sub-sub-menu
    QQuickMenuItem *subSubMenu1Item = qobject_cast<QQuickMenuItem *>(subMenu1->itemAt(2));
    QVERIFY(subSubMenu1Item);
    QCOMPARE(subSubMenu1Item->subMenu(), subSubMenu1);
    emit subSubMenu1Item->triggered();
    QCOMPARE(mainMenu->isVisible(), cascade);
    QCOMPARE(subMenu1->isVisible(), cascade);
    QVERIFY(!subMenu2->isVisible());
    QVERIFY(subSubMenu1->isVisible());

    if (cascade) {
        QCOMPARE(subSubMenu1->parentItem(), subSubMenu1Item);
        // vertically aligned to the parent menu item
        QCOMPARE(subSubMenu1->popupItem()->y(), subMenu1->popupItem()->y() + subSubMenu1Item->y() - subSubMenu1->topPadding());
        if (mirrored)
            QCOMPARE(subSubMenu1->popupItem()->x(), subMenu1->popupItem()->x() - subSubMenu1->width() + overlap); // on the left of the parent menu
        else
            QCOMPARE(subSubMenu1->popupItem()->x(), subMenu1->popupItem()->x() + subMenu1->width() - overlap); // on the right of the parent menu
    } else {
        QCOMPARE(subSubMenu1->parentItem(), subMenu1->parentItem());
        // centered over the parent menu
        QCOMPARE(subSubMenu1->popupItem()->x(), subMenu1->popupItem()->x() + (subMenu1->width() - subSubMenu1->width()) / 2);
        QCOMPARE(subSubMenu1->popupItem()->y(), subMenu1->popupItem()->y() + (subMenu1->height() - subSubMenu1->height()) / 2);
    }
}

QTEST_MAIN(tst_menu)

#include "tst_menu.moc"
