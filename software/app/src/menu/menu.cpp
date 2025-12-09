/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#include "menu.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(menu_system, CONFIG_APP_LOG_LEVEL);

/* MenuItem implementation */
MenuItem::MenuItem(const char* label, MenuItemType type)
    : label_(label)
    , type_(type)
    , next_(nullptr)
    , prev_(nullptr)
    , parent_(nullptr)
    , submenu_(nullptr)
    , callback_(nullptr)
    , value_getter_(nullptr)
{
}

MenuItem::~MenuItem()
{
    // Destructor - resources cleaned up by MenuSystem
}

/* MenuSystem implementation */
MenuSystem::MenuSystem()
    : root_menu_(nullptr)
    , current_menu_(nullptr)
    , current_item_(nullptr)
    , display_callback_(nullptr)
{
    k_mutex_init(&mutex_);
}

MenuSystem& MenuSystem::getInstance()
{
    static MenuSystem instance;
    return instance;
}

MenuItem* MenuSystem::createMenuItem(const char* label, MenuItemType type)
{
    MenuItem* item = new MenuItem(label, type);
    if (!item) {
        LOG_ERR("Failed to allocate menu item");
        return nullptr;
    }
    return item;
}

void MenuSystem::addMenuItem(MenuItem* parent, MenuItem* item)
{
    if (!parent || !item) {
        LOG_ERR("Invalid parent or item");
        return;
    }
    
    k_mutex_lock(&mutex_, K_FOREVER);
    
    // Set parent relationship
    item->setParent(parent);
    
    // Get the submenu list from parent
    MenuItem* submenu = parent->getSubmenu();
    
    if (!submenu) {
        // First item in submenu
        parent->setSubmenu(item);
        item->setPrev(nullptr);
        item->setNext(nullptr);
    } else {
        // Find the last item in the list
        MenuItem* last = submenu;
        while (last->getNext()) {
            last = last->getNext();
        }
        
        // Append to the end
        last->setNext(item);
        item->setPrev(last);
        item->setNext(nullptr);
    }
    
    k_mutex_unlock(&mutex_);
    
    LOG_DBG("Added menu item '%s' to parent '%s'", item->getLabel(), parent->getLabel());
}

void MenuSystem::deleteMenu(MenuItem* menu)
{
    if (!menu) {
        return;
    }
    
    k_mutex_lock(&mutex_, K_FOREVER);
    
    // Recursively delete submenu items
    MenuItem* submenu = menu->getSubmenu();
    while (submenu) {
        MenuItem* next = submenu->getNext();
        deleteMenu(submenu);
        submenu = next;
    }
    
    // Delete this item
    delete menu;
    
    k_mutex_unlock(&mutex_);
}

void MenuSystem::navigate(MenuItem* item)
{
    if (!item) {
        return;
    }
    
    k_mutex_lock(&mutex_, K_FOREVER);
    current_item_ = item;
    k_mutex_unlock(&mutex_);
    
    updateDisplay();
}

void MenuSystem::navigateUp()
{
    if (!current_item_) {
        return;
    }
    
    k_mutex_lock(&mutex_, K_FOREVER);
    
    MenuItem* prev = current_item_->getPrev();
    if (prev) {
        current_item_ = prev;
    } else {
        // Wrap to last item
        MenuItem* menu = current_menu_;
        if (menu) {
            MenuItem* submenu = menu->getSubmenu();
            if (submenu) {
                // Find last item
                while (submenu->getNext()) {
                    submenu = submenu->getNext();
                }
                current_item_ = submenu;
            }
        }
    }
    
    k_mutex_unlock(&mutex_);
    
    updateDisplay();
}

void MenuSystem::navigateDown()
{
    if (!current_item_) {
        return;
    }
    
    k_mutex_lock(&mutex_, K_FOREVER);
    
    MenuItem* next = current_item_->getNext();
    if (next) {
        current_item_ = next;
    } else {
        // Wrap to first item
        MenuItem* menu = current_menu_;
        if (menu) {
            MenuItem* submenu = menu->getSubmenu();
            if (submenu) {
                current_item_ = submenu;
            }
        }
    }
    
    k_mutex_unlock(&mutex_);
    
    updateDisplay();
}

void MenuSystem::navigateBack()
{
    if (!current_menu_) {
        return;
    }
    
    k_mutex_lock(&mutex_, K_FOREVER);
    
    MenuItem* parent = current_menu_->getParent();
    if (parent) {
        current_menu_ = parent->getParent() ? parent->getParent() : root_menu_;
        current_item_ = parent;
    }
    
    k_mutex_unlock(&mutex_);
    
    updateDisplay();
}

void MenuSystem::select()
{
    if (!current_item_) {
        return;
    }
    
    k_mutex_lock(&mutex_, K_FOREVER);
    
    MenuItemType type = current_item_->getType();
    
    switch (type) {
        case MenuItemType::SUBMENU: {
            // Navigate into submenu
            MenuItem* submenu = current_item_->getSubmenu();
            if (submenu) {
                current_menu_ = current_item_;
                current_item_ = submenu;
            }
            break;
        }
        
        case MenuItemType::ACTION: {
            // Execute action callback
            current_item_->executeCallback();
            break;
        }
        
        case MenuItemType::VALUE: {
            // Value editing would be implemented here
            // For now, just execute callback if available
            current_item_->executeCallback();
            break;
        }
        
        case MenuItemType::BACK: {
            // Navigate back to parent menu
            navigateBack();
            break;
        }
    }
    
    k_mutex_unlock(&mutex_);
    
    updateDisplay();
}

void MenuSystem::updateDisplay()
{
    if (display_callback_) {
        display_callback_(current_menu_, current_item_);
    }
}
