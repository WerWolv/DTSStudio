use std::thread;
use std::thread::{JoinHandle, Thread};
use serde::Serialize;
use tauri::{AppHandle, Emitter};
use emulator;

static mut EMULATOR_THREADS: Vec<JoinHandle<()>> = Vec::new();


#[derive(Clone, Serialize)]
#[serde(rename_all = "camelCase")]
struct TerminalData<'a> {
    terminal_id: &'a str,
    data: &'a str,
}

#[tauri::command]
fn write_terminal_data(app: AppHandle, terminal_id: &str, text: &str) {
    app.emit("write-terminal", TerminalData {
        terminal_id,
        data: text
    }).expect("???");
}

pub fn run() {

    tauri::Builder::default()
        .plugin(tauri_plugin_opener::init())
        .invoke_handler(tauri::generate_handler![])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
