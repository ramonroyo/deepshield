// Copyright © ByteHeed.  All rights reserved.

// external namespaces
use std::time::Duration;
use std::thread;

use std::ptr::null_mut;
use std::io;
use std::mem::{zeroed, size_of_val};

use super::ffi;
use super::winapi;
use super::winapi::um::winsvc;
use super::winapi::um::winsvc::{SC_HANDLE};

use super::error::ServiceError;

// custom namespaces

use super::ffi::traits::EncodeUtf16;
use super::structs::{SERVICE_STATUS_PROCESS, ServiceInfo};
use super::consts::{SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL};


#[derive(Debug, PartialEq)]
pub enum ServiceStatusError {
    DeletePending,
    ServiceDisabled,
    ServiceAlreadyExists,
    ServiceCannotAcceptCtrl,
    ServiceAlreadyRunning,
    ServiceDoesNotExist,
    AccessViolation,
    ServiceNotActive,
    InvalidHandle,
    UnknownError(i32),
}

#[derive(Debug, Clone)]
pub struct WindowsServiceControlManager {
    handle: SC_HANDLE
}

impl WindowsServiceControlManager {
    fn open() -> Result<SC_HANDLE, String> {
        let handle = unsafe {
            winsvc::OpenSCManagerW(
                null_mut(),
                null_mut(),
                winapi::um::winsvc::SC_MANAGER_ALL_ACCESS,
            )
        };

        if handle.is_null() {
            return Err(io::Error::last_os_error().to_string());
        }

        Ok(handle)
    }

    pub fn new() -> WindowsServiceControlManager {
        let handle = WindowsServiceControlManager::open().expect("Unable to open Service Control Manager");

        WindowsServiceControlManager {
            handle: handle
        }
    }
}

impl Drop for WindowsServiceControlManager {
    fn drop(&mut self) {
        WindowsService::close_service_handle(self.handle);
    }
}

#[derive(Debug, Clone)]
pub struct WindowsService {
    name: String,
    path: String,
    manager: WindowsServiceControlManager,
    retries: Duration
}

impl WindowsService {
    pub fn new(name: &str, path: &str) -> WindowsService {
        let manager = WindowsServiceControlManager::new();

        WindowsService {
            name: name.to_string(),
            path: path.to_string(),
            manager: manager,
            retries: Duration::from_secs(60)
        }
    }

    pub fn remove(&mut self) -> Result<&Self, ServiceError> {
        self.delete()?;

        Ok(self)
    }

    pub fn install(&mut self) -> &Self {
        let wait = Duration::from_secs(1);

        if let Err(err) = self.create() {
            match WindowsService::service_error() {
                ServiceStatusError::ServiceAlreadyExists => {
                    // println!("Failed to install {:?}: Service already exists.", self.name);
                },
                ServiceStatusError::DeletePending => {
                    let service_name = self.name.clone();

                    self.retries = self.retries.checked_sub(wait).ok_or_else(move|| {
                        panic!("{}: reached timeout while delete is pending, exiting.", service_name);
                    }).unwrap();

                    println!("delete is pending, waiting {} seconds", self.retries.as_secs());
                    thread::sleep(wait);

                    self.install();
                }
                _ => println!("Failed to install {:?}: unknown error {:?}", self.name, err),
            }
        }
        // else {
        //     println!("Service {:?} has been successfully installed", self.name);
        // }

        self.retries = Duration::from_secs(60);

        self
    }

    pub fn name(&self) -> String {
        self.name.clone()
    }

    pub fn stop(&self) -> Result<&Self, ServiceError> {
        let service = self.open().expect("Unable to open service");

        let mut status: winsvc::SERVICE_STATUS = unsafe {zeroed()};

        let success = unsafe {
            winsvc::ControlService(
            service,
            winsvc::SERVICE_CONTROL_STOP,
            &mut status) == 0
        };

        service_check!("stop", self, service, success);

        // self.close(service)
    }

    pub fn start(&self) -> Result<&Self, ServiceError> {
        let service = self.open().expect("Unable to open service");

        let success = unsafe {
            ffi::StartServiceW(
                service,
                0,
                null_mut(),
            ) == 0
        };

        service_check!("start", self, service, success);
    }


    fn query_service_status(
        service: SC_HANDLE
    ) -> Result<SERVICE_STATUS_PROCESS, String> {

        let mut process: SERVICE_STATUS_PROCESS = unsafe { zeroed() };
        let mut size: u32 = size_of_val(&process) as u32;

        let result = unsafe {
            winsvc::QueryServiceStatusEx(
                service,
                winsvc::SC_STATUS_PROCESS_INFO,
                &mut process as *mut SERVICE_STATUS_PROCESS as *mut u8,
                size,
                &mut size,
            )
        };

        if result == 0 {
            return Err(io::Error::last_os_error().to_string())
        }

        Ok(process)
    }


    pub fn query(&self) -> ServiceInfo {
        let service = self.open().expect("Unable to open service");

        let info = WindowsService::query_service_status( service ).expect("Can't query service");

        self.close(service);

        ServiceInfo::from(info)
    }

    pub fn close(&self, handle: SC_HANDLE) -> &Self {
        WindowsService::close_service_handle(handle);
        self
    }

    pub fn open(&self) -> Result<SC_HANDLE, ServiceStatusError> {
        let handle = unsafe {
            winsvc::OpenServiceW(
                self.manager.handle,
                self.name.encode_utf16_null().as_ptr(),
                winsvc::SERVICE_ALL_ACCESS,
            )
        };

        if handle.is_null() {
            return Err(WindowsService::service_error())
        }

        Ok(handle)
    }

    fn service_error() -> ServiceStatusError {
        match io::Error::last_os_error().raw_os_error() {
            Some(1056) => ServiceStatusError::ServiceAlreadyRunning,
            Some(1058) => ServiceStatusError::ServiceDisabled,
            Some(1060) => ServiceStatusError::ServiceDoesNotExist,
            Some(1061) => ServiceStatusError::ServiceCannotAcceptCtrl,
            Some(1062) => ServiceStatusError::ServiceNotActive,
            Some(1072) => ServiceStatusError::DeletePending,
            Some(1073) => ServiceStatusError::ServiceAlreadyExists,
            Some(5)    => ServiceStatusError::AccessViolation,
            Some(6)    => ServiceStatusError::InvalidHandle,
            Some(code) => ServiceStatusError::UnknownError(code),
            _          => panic!("Can't retrieve OS Error, panicking!")
        }
    }

    pub fn exists(&self) -> bool {
        match self.open() {
            Err(ServiceStatusError::AccessViolation) => {
                println!("INFO: Access violation while opening service.");
                false
            },
            Err(_) => false,
            Ok(handle) => {
                self.close(handle);
                true
            }
        }
    }

    pub fn delete(&self) -> Result<&Self, ServiceError> {
        let handle = self.open().expect("Can't open service");
        let success = unsafe { winsvc::DeleteService(handle) == 0 };

        service_check!("delete", self, handle, success);
    }

    pub fn create(&self) -> Result<&Self, ServiceError> {
        let handle = unsafe {
            winsvc::CreateServiceW(
                self.manager.handle,                    // handle
                self.name.encode_utf16_null().as_ptr(),              // service name
                self.name.encode_utf16_null().as_ptr(),              // display name
                winsvc::SERVICE_ALL_ACCESS, // desired access
                SERVICE_KERNEL_DRIVER,      // service type
                SERVICE_DEMAND_START,       // start type
                SERVICE_ERROR_NORMAL,       // error control
                self.path.encode_utf16_null().as_ptr(),              // binary path
                null_mut(),                 // load order
                null_mut(),                 // tag id
                null_mut(),                 // dependencies
                null_mut(),                 // start name
                null_mut(),                 // password
            )
        };

        service_check!("create", self, handle, handle == null_mut());
    }

    fn close_service_handle(handle: SC_HANDLE) {
        unsafe {
            winsvc::CloseServiceHandle(handle);
        }
    }
}
