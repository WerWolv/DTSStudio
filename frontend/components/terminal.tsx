import { useEffect, useMemo, useRef } from "react";
import { useXTerm } from "react-xtermjs";
import { FitAddon } from "@xterm/addon-fit";
import { ResizablePanel } from "@/components/ui/resizable";
import { listen } from '@tauri-apps/api/event';

type TerminalData = {
    terminalId: string;
    data: string;
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

    useEffect(() => {
        if (!instance) return;
        instance.loadAddon(fitAddonRef.current);
        fitAddonRef.current.fit();

        let unlisten = listen<TerminalData>('write-terminal', (event) => {
            if (terminalId == event.payload.terminalId)
                instance.write(event.payload.data);
        })

        return() => {
            unlisten.then((unlistenFn) => unlistenFn());
        }
    }, [instance]);

    return (
        <ResizablePanel className="bg-black" defaultSize={20} minSize={5} onResize={() => fitAddonRef.current.fit()}>
            <div ref={ref} style={{ width: "100%", height: "100%" }} />
        </ResizablePanel>
    );
}
