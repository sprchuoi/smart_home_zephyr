/*
 * Copyright (c) 2025 Sprchuoi
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MENU_MANAGER_HPP
#define MENU_MANAGER_HPP

#include "menu.hpp"

/* Menu Manager - Organizes application menus */
class MenuManager {
public:
    static MenuManager& getInstance();
    
    /* Initialize the menu system */
    int init();
    
    /* Get main menu categories */
    MenuItem* getMainMenu() const { return main_menu_; }
    MenuItem* getAppMenu() const { return app_menu_; }
    MenuItem* getServicesMenu() const { return services_menu_; }
    MenuItem* getModulesMenu() const { return modules_menu_; }
    
    /* Add items dynamically */
    void addAppMenuItem(const char* label, MenuCallback callback);
    void addServiceMenuItem(const char* label, MenuCallback callback);
    void addModuleMenuItem(const char* label, MenuCallback callback);
    
private:
    MenuManager();
    ~MenuManager() = default;
    
    MenuManager(const MenuManager&) = delete;
    MenuManager& operator=(const MenuManager&) = delete;
    
    void createMainMenu();
    void createAppMenu();
    void createServicesMenu();
    void createModulesMenu();
    
    MenuItem* main_menu_;
    MenuItem* app_menu_;
    MenuItem* services_menu_;
    MenuItem* modules_menu_;
};

#endif // MENU_MANAGER_HPP
