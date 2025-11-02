import { Titlebar } from "@/components/titlebar";
import { Footer } from "@/components/footer";
import { Terminal } from "@/components/terminal";
import {
    ResizablePanelGroup,
    ResizablePanel,
    ResizableHandle,
} from "@/components/ui/resizable";


function App() {
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
                          B
                      </ResizablePanel>
                      <ResizableHandle />

                      <ResizablePanel defaultSize={20} minSize={5}>
                          C
                      </ResizablePanel>
                  </ResizablePanelGroup>
              </ResizablePanel>
              <ResizableHandle />
              <Terminal terminalId={"linux"} />
          </ResizablePanelGroup>
      </main>
      <Footer />
    </>
  );
}

export default App;
