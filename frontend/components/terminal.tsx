import { useEffect, useMemo, useRef } from "react";
import { useXTerm } from "react-xtermjs";
import { FitAddon } from "@xterm/addon-fit";
import { ResizablePanel } from "@/components/ui/resizable";
import { listen } from '@tauri-apps/api/event';

type WriteCommandData = {
    terminalId: string;
    data: string;
};

type ClearCommandData = {
    terminalId: string;
};

interface TerminalProps {
    terminalId: string;
}

export function Terminal({ terminalId } : TerminalProps) {
    const TERMINAL_CONFIG = useMemo(() => ({
        theme: {},
        fontFamily: "'Fira Code', Menlo, Monaco, 'Courier New', monospace",
        fontSize: 13,
        allowProposedApi: true,
        scrollback: 1000,
        cursorBlink: true,
        smoothScrollDuration: 100,
        macOptionIsMeta: true,
        macOptionClickForcesSelection: true,
        convertEol: true,
        linuxMode: false,
        rendererType: "canvas",
        unicode: { activeVersion: "11" }
    }), []);

    const { instance, ref } = useXTerm({ options: TERMINAL_CONFIG });

    const fitAddonRef = useRef<FitAddon>(new FitAddon());

    function fitTerminal() {
        fitAddonRef.current.fit()
    }

    useEffect(() => {
        if (!instance) return;
        instance.loadAddon(fitAddonRef.current);
        fitAddonRef.current.fit();

        let writeUnlisten = listen<WriteCommandData>('write-terminal', (event) => {
            if (terminalId == event.payload.terminalId)
                instance.write(event.payload.data);
        });
        let clearUnlisten = listen<ClearCommandData>('clear-terminal', (event) => {
            if (terminalId == event.payload.terminalId)
                instance.clear();
        })

        document.addEventListener("resize", fitTerminal)

        return() => {
            writeUnlisten.then((unlistenFn) => unlistenFn());
            clearUnlisten.then((unlistenFn) => unlistenFn());
            window.removeEventListener('resize', fitTerminal)
        }
    }, [instance]);

    return (
        <ResizablePanel className="bg-black" defaultSize={20} minSize={5} onResize={fitTerminal}>
            <div ref={ref} style={{ width: "100%", height: "100%" }} />
        </ResizablePanel>
    );
}
