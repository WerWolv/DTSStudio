import { Titlebar } from "@/components/titlebar";
import { Footer } from "@/components/footer";
import { Terminal } from "@/components/terminal";
import {
    ResizablePanelGroup,
    ResizablePanel,
    ResizableHandle,
} from "@/components/ui/resizable";
import {Button} from "@/components/ui/button.tsx";
import {invoke} from "@tauri-apps/api/core";
import {PlayIcon, SquareIcon} from "lucide-react";
import {Editor, useMonaco} from "@monaco-editor/react";
import {dtsLanguage} from "@/components/editor/device-tree"
import {useEffect} from "react";

function startEmulator() {
    invoke("start_emulation");
}

function stopEmulator() {
    invoke("stop_emulation");
}

function App() {
  const monaco = useMonaco();

  useEffect(() => {
      if (!monaco) return;

      monaco.languages.register({ id: "dts" });
      monaco.languages.setMonarchTokensProvider("dts", dtsLanguage);
  })

  return (
    <>
      <Titlebar />
      <main className="bg-background text-foreground flex flex-col min-h-screen" style={{ paddingBottom: "4.3rem"}}>
          <ResizablePanelGroup direction="vertical" className="flex-1 flex">
              <ResizablePanel defaultSize={20} minSize={5}>
                  <ResizablePanelGroup direction="horizontal" className="flex-1 flex">
                      <ResizablePanel defaultSize={20} minSize={5}>
                          A
                      </ResizablePanel>
                      <ResizableHandle />

                      <ResizablePanel defaultSize={100} minSize={5}>
                          <Editor defaultLanguage="dts" theme="vs-dark"/>
                      </ResizablePanel>
                      <ResizableHandle />

                      <ResizablePanel defaultSize={20} minSize={5} className="p-2">
                          <Button variant="outline" size="icon" onClick={startEmulator}>
                              <PlayIcon color="forestGreen" />
                          </Button>
                          <Button variant="outline" size="icon" onClick={stopEmulator}>
                              <SquareIcon color="indianRed" />
                          </Button>
                      </ResizablePanel>
                  </ResizablePanelGroup>
              </ResizablePanel>
              <ResizableHandle />
              <Terminal terminalId={"linux-terminal"} />
          </ResizablePanelGroup>
      </main>
      <Footer />
    </>
  );
}

export default App;
