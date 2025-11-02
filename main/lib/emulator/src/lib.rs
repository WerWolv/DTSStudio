use std::ffi::c_void;

unsafe extern "C" {
    fn create() -> *mut c_void;
    fn destroy(emulator: *mut c_void);
    fn step(emulator: *mut c_void);
}

pub struct Emulator {
    emulator: *mut c_void,
}

impl Emulator {
    pub fn new() -> Self {
        unsafe {
            let emu = create();
            Emulator { emulator: emu }
        }
    }

    pub fn step(&mut self) {
        unsafe {
            step(self.emulator);
        }
    }
}

impl Drop for Emulator {
    fn drop(&mut self) {
        unsafe {
            destroy(self.emulator);
        }
    }
}