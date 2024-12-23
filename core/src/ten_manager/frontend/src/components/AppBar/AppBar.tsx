//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import React, { useState, useEffect, useRef } from "react";
import { useTranslation } from "react-i18next";
import FileMenu from "./FileMenu";
import EditMenu from "./EditMenu";
import HelpMenu from "./HelpMenu";
import "./AppBar.scss";

interface AppBarProps {
  // The current version of tman.
  version: string;

  // An error message to be displayed if any issue occurs.
  error: string;

  onOpenSettings: () => void;
  onAutoLayout: () => void;
}

type MenuType = "file" | "edit" | "help" | null;

const AppBar: React.FC<AppBarProps> = ({
  version,
  error,
  onOpenSettings,
  onAutoLayout,
}) => {
  const { t } = useTranslation("common");
  const [openMenu, setOpenMenu] = useState<MenuType>(null);
  const appBarRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    const handleMouseDown = (event: MouseEvent) => {
      const targetElement = event.target as HTMLElement;

      // Check if the click is inside the AppBar.
      if (appBarRef.current && appBarRef.current.contains(targetElement)) {
        // Check if the click is inside any menu-container.
        const menuContainer = targetElement.closest(".menu-container");
        if (!menuContainer) {
          // Clicked inside AppBar but not on any menu-container, close any open
          // dropdown.
          setOpenMenu(null);
        }
        // Else, let the menu component handle its own logic.
      } else {
        // Clicked outside AppBar, close any open dropdown.
        setOpenMenu(null);
      }
    };

    // Add the mousedown listener in the capturing phase.
    //
    // Note: It is necessary to intercept the `mousedown` event during the
    // **capturing phase**, as third-party components like ReactFlow may
    // intercept the event and prevent it from propagating further. Therefore,
    // intercepting the event in the **bubbling phase** might not work as
    // expected.
    document.addEventListener("mousedown", handleMouseDown, true);
    return () => {
      document.removeEventListener("mousedown", handleMouseDown, true);
    };
  }, []);

  // Function to open a specific menu or toggle it if it's already open.
  const handleOpenMenu = (menu: MenuType) => {
    setOpenMenu((prevMenu) => (prevMenu === menu ? null : menu));
  };

  // Function to switch menus on hover only if a menu is already open.
  const handleSwitchMenu = (menu: MenuType) => {
    if (openMenu !== null && menu !== openMenu) {
      setOpenMenu(menu);
    }
  };

  // Function to close any open menu.
  const closeMenu = () => {
    setOpenMenu(null);
  };

  return (
    <div className="app-bar" ref={appBarRef}>
      {/* Left part is the menu itself. */}
      <div className="app-bar-left">
        <FileMenu
          isOpen={openMenu === "file"}
          onClick={() => handleOpenMenu("file")}
          onHover={() => handleSwitchMenu("file")}
          closeMenu={closeMenu}
        />
        <EditMenu
          isOpen={openMenu === "edit"}
          onClick={() => handleOpenMenu("edit")}
          onHover={() => handleSwitchMenu("edit")}
          onOpenSettings={onOpenSettings}
          closeMenu={closeMenu}
          onAutoLayout={onAutoLayout}
        />
        <HelpMenu
          isOpen={openMenu === "help"}
          onClick={() => handleOpenMenu("help")}
          onHover={() => handleSwitchMenu("help")}
          closeMenu={closeMenu}
        />
      </div>

      {/* Right part is the logo. */}
      <div className="app-bar-right">
        {error ? (
          <span style={{ color: "red" }}>{t("error_fetching")}</span>
        ) : (
          <span>Powered by TEN Framework {version}</span>
        )}
      </div>
    </div>
  );
};

export default AppBar;
