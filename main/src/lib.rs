#![feature(c_size_t)]

use std::ffi::{c_char, c_size_t};

extern "C" {
    fn start_emulation();
    fn stop_emulation();
    fn is_emulation_running() -> bool;
    fn set_device_tree_source(source: *mut c_char, length: c_size_t);
}

mod interface {

    use std::sync::OnceLock;
    use serde::Serialize;
    use tauri::{AppHandle, Emitter};

    pub static APP_HANDLE: OnceLock<AppHandle> = OnceLock::new();

    #[derive(Clone, Serialize)]
    #[serde(rename_all = "camelCase")]
    struct TerminalData<'a> {
        terminal_id: &'a str,
        data: &'a str,
    }

    fn _send_terminal_data(terminal_id: &str, text: &str) {
        APP_HANDLE.get().unwrap().emit(
            "write-terminal",
            TerminalData {
                terminal_id,
                data: text,
            },
        )
            .expect("???");
    }

    #[no_mangle]
    pub extern "C" fn send_terminal_data(terminal_id: *const std::os::raw::c_char, text: *const std::os::raw::c_char) {
        use std::ffi::CStr;

        unsafe {
            let terminal_id = CStr::from_ptr(terminal_id).to_string_lossy();
            let text = CStr::from_ptr(text).to_string_lossy();
            _send_terminal_data(&terminal_id, &text);
        }
    }

    #[tauri::command]
    pub fn start_emulation() {
        unsafe {
            crate::start_emulation();
        }
    }

    #[tauri::command]
    pub fn stop_emulation() {
        unsafe {
            crate::stop_emulation();
        }
    }

}

pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_os::init())
        .plugin(tauri_plugin_opener::init())
        .setup(|app| {
            // store the app handle globally
            interface::APP_HANDLE.set(app.handle().clone()).unwrap();
            Ok(())
        })
        .invoke_handler(tauri::generate_handler![interface::start_emulation, interface::stop_emulation])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
