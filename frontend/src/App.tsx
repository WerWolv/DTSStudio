import { useState } from "react";
import { invoke } from "@tauri-apps/api/core";
import { Button } from "@/components/ui/button";
import { Titlebar } from "@/components/titlebar";
import { Footer } from "@/components/footer";
// Tailwind styles are loaded via index.css in main.tsx

function App() {
  const [greetMsg, setGreetMsg] = useState("");
  const [name, setName] = useState("");

  async function greet() {
    // Learn more about Tauri commands at https://tauri.app/develop/calling-rust/
    setGreetMsg(await invoke("greet", { name }));
  }

  return (
    <>
      <Titlebar />
      <main className="min-h-screen bg-background text-foreground flex items-center justify-center p-6 pt-16">
        <div className="w-full max-w-md rounded-lg border bg-card text-card-foreground shadow-sm p-6 space-y-4">
        <div className="space-y-1">
          <h1 className="text-xl font-semibold tracking-tight">Welcome to DTS Studio</h1>
          <p className="text-sm text-muted-foreground">Tauri + React + Tailwind + shadcn/ui</p>
        </div>

        <form
          className="flex items-center gap-2"
          onSubmit={(e) => {
            e.preventDefault();
            greet();
          }}
        >
          <input
            id="greet-input"
            className="flex-1 rounded-md border bg-background px-3 py-2 text-sm outline-none focus-visible:ring-2 focus-visible:ring-ring"
            onChange={(e) => setName(e.currentTarget.value)}
            placeholder="Enter a name..."
          />
          <Button type="submit">Greet</Button>
        </form>

          {greetMsg && (
            <p className="text-sm text-muted-foreground">{greetMsg}</p>
          )}
        </div>
      </main>
      <Footer />
    </>
  );
}

export default App;
