import { useCallback, useEffect, useState } from "react";
import { getCurrentWindow } from "@tauri-apps/api/window";
import { Button } from "@/components/ui/button";
import { MainMenuBar } from "@/components/main_menu_bar";
import { VscChromeClose, VscChromeMaximize, VscChromeMinimize, VscChromeRestore } from "react-icons/vsc"
import { platform } from '@tauri-apps/plugin-os';

const appWindow = getCurrentWindow();

function MaximizeRestoreButton() {
  const [isMax, setIsMax] = useState(false);

  const refreshMaxState = useCallback(async () => {
    console.log("Refreshing app window " + await appWindow.isMaximized());
    setIsMax(await appWindow.isMaximized());
  }, []);

  useEffect( () => {
    // Listen to window resize to keep maximize state in sync
    const bind = async () =>
        appWindow.onResized(() => {
          void refreshMaxState();
        });
    const unlistenPromise = bind();
    return () => {
      void unlistenPromise.then((un) => un());
    };
  }, [refreshMaxState]);

  return (
      <Button
          size="icon"
          variant="ghost"
          className="h-10 w-12 rounded-none hover:bg-accent/80 active:bg-accent/100"
          onClick={async () => {
            if (await appWindow.isMaximized()) {
              await appWindow.unmaximize();
            } else {
              await appWindow.maximize();
            }
            await refreshMaxState();
          }}
          aria-label={isMax ? "Restore" : "Maximize"}
      >
          { isMax ? <VscChromeRestore className="h-4 w-4"/> : <VscChromeMaximize className="h-4 w-4"/> }
      </Button>
  )
}

function Title() {
    if (platform() === 'macos') {
        return <div className="pl-20 text-xs text-muted-foreground font-bold">DTS Studio</div>;
    } else {
        return <div className="text-xs text-muted-foreground font-bold">DTS Studio</div>;
    }
}

function TitleBarButtons() {
    if (platform() === 'macos') {
        return <div> </div>;
    } else {
        return (
            <div className="ml-auto flex items-center">
                <Button
                    size="icon"
                    variant="ghost"
                    className="h-10 w-12 rounded-none hover:bg-accent/80 active:bg-accent/100"
                    onClick={() => appWindow.minimize()}
                    aria-label="Minimize"
                >
                    <VscChromeMinimize className="h-4 w-4" />
                </Button>
                <MaximizeRestoreButton/>
                <Button
                    size="icon"
                    variant="ghost"
                    className="h-10 w-12 rounded-none hover:bg-destructive/80 active:bg-destructive/100"
                    onClick={() => appWindow.close()}
                    aria-label="Close"
                >
                    <VscChromeClose className="h-4 w-4" />
                </Button>
            </div>
        );
    }
}

export function Titlebar() {
  return (
    <div
      className="inset-x-0 top-0 h-10 select-none border-b bg-background/80 backdrop-blur supports-[backdrop-filter]:bg-background/60"
    >
      <div className="flex h-full items-center justify-between pl-3" data-tauri-drag-region>
        <Title />
        <MainMenuBar/>
        <TitleBarButtons />
      </div>
    </div>
  );
}
