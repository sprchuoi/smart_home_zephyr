/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MENU_HPP
#define MENU_HPP

#include <zephyr/kernel.h>
#include <stdint.h>

/* Forward declarations */
class MenuItem;

/* Menu item callback type */
using MenuCallback = void (*)(void);

/* Menu item types */
enum class MenuItemType {
    SUBMENU,      // Opens a submenu
    ACTION,       // Executes an action
    VALUE,        // Shows/edits a value
    BACK          // Return to parent menu
};

/* Menu item class - node in linked list */
class MenuItem {
public:
    MenuItem(const char* label, MenuItemType type);
    ~MenuItem();
    
    /* Linked list operations */
    void setNext(MenuItem* next) { next_ = next; }
    void setPrev(MenuItem* prev) { prev_ = prev; }
    MenuItem* getNext() const { return next_; }
    MenuItem* getPrev() const { return prev_; }
    
    /* Menu hierarchy */
    void setParent(MenuItem* parent) { parent_ = parent; }
    void setSubmenu(MenuItem* submenu) { submenu_ = submenu; }
    MenuItem* getParent() const { return parent_; }
    MenuItem* getSubmenu() const { return submenu_; }
    
    /* Properties */
    const char* getLabel() const { return label_; }
    MenuItemType getType() const { return type_; }
    
    /* Action callback */
    void setCallback(MenuCallback callback) { callback_ = callback; }
    void executeCallback() const { if (callback_) callback_(); }
    
    /* Value display (for VALUE type) */
    void setValueGetter(const char* (*getter)()) { value_getter_ = getter; }
    const char* getValue() const { return value_getter_ ? value_getter_() : nullptr; }
    
private:
    const char* label_;
    MenuItemType type_;
    
    /* Linked list pointers */
    MenuItem* next_;
    MenuItem* prev_;
    
    /* Menu hierarchy */
    MenuItem* parent_;
    MenuItem* submenu_;
    
    /* Action callback */
    MenuCallback callback_;
    
    /* Value getter for VALUE type */
    const char* (*value_getter_)();
};

/* Menu system manager */
class MenuSystem {
public:
    static MenuSystem& getInstance();
    
    /* Menu structure management */
    MenuItem* createMenuItem(const char* label, MenuItemType type);
    void addMenuItem(MenuItem* parent, MenuItem* item);
    void deleteMenu(MenuItem* menu);
    
    /* Navigation */
    void navigate(MenuItem* item);
    void navigateUp();
    void navigateDown();
    void navigateBack();
    void select();
    
    /* Current state */
    MenuItem* getCurrentMenu() const { return current_menu_; }
    MenuItem* getCurrentItem() const { return current_item_; }
    
    /* Display update callback */
    void setDisplayCallback(void (*callback)(MenuItem* menu, MenuItem* selected)) {
        display_callback_ = callback;
    }
    
    /* Initialize with root menu */
    void setRootMenu(MenuItem* root) { 
        root_menu_ = root;
        current_menu_ = root;
        current_item_ = root;
    }
    
private:
    MenuSystem();
    ~MenuSystem() = default;
    
    MenuSystem(const MenuSystem&) = delete;
    MenuSystem& operator=(const MenuSystem&) = delete;
    
    void updateDisplay();
    
    MenuItem* root_menu_;
    MenuItem* current_menu_;
    MenuItem* current_item_;
    
    void (*display_callback_)(MenuItem* menu, MenuItem* selected);
    
    struct k_mutex mutex_;
};

#endif // MENU_HPP
