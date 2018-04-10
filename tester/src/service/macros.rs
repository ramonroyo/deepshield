#[macro_export]
macro_rules! service_check {
    ($action:expr, $self:expr, $service:expr, $success:expr) => {
        if $success {
            $self.close($service);
            return Err(ServiceError::Control($action.to_string(),
                                             $self.name.to_string(),
                                             io::Error::last_os_error().to_string()))
        } else {
            $self.close($service);
            return Ok($self)
        }
    }
}
