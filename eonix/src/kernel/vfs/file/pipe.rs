use super::{File, FileType, PollEvent};
use crate::{
    io::{Buffer, Stream},
    kernel::{
        constants::{EINTR, EPIPE},
        task::Thread,
    },
    prelude::KResult,
    sync::CondVar,
};
use alloc::{collections::vec_deque::VecDeque, sync::Arc};
use eonix_sync::Mutex;
use posix_types::{open::OpenFlags, signal::Signal};

struct PipeInner {
    buffer: VecDeque<u8>,
    read_closed: bool,
    write_closed: bool,
}

pub struct Pipe {
    inner: Mutex<PipeInner>,
    cv_read: CondVar,
    cv_write: CondVar,
}

pub struct PipeReadEnd {
    pipe: Arc<Pipe>,
}

pub struct PipeWriteEnd {
    pipe: Arc<Pipe>,
}

fn send_sigpipe_to_current() {
    let current = Thread::current();
    current.raise(Signal::SIGPIPE);
}

impl Pipe {
    const PIPE_SIZE: usize = 4096;

    /// # Return
    /// `(read_end, write_end)`
    pub fn new(flags: OpenFlags) -> (File, File) {
        let pipe = Arc::new(Self {
            inner: Mutex::new(PipeInner {
                buffer: VecDeque::with_capacity(Self::PIPE_SIZE),
                read_closed: false,
                write_closed: false,
            }),
            cv_read: CondVar::new(),
            cv_write: CondVar::new(),
        });

        let read_flags = flags.difference(OpenFlags::O_WRONLY | OpenFlags::O_RDWR);
        let mut write_flags = read_flags;
        write_flags.insert(OpenFlags::O_WRONLY);

        let read_pipe = pipe.clone();
        let write_pipe = pipe;

        (
            File::new(
                read_flags,
                FileType::PipeRead(PipeReadEnd { pipe: read_pipe }),
            ),
            File::new(
                write_flags,
                FileType::PipeWrite(PipeWriteEnd { pipe: write_pipe }),
            ),
        )
    }

    fn read_ready(inner: &PipeInner, event: PollEvent) -> PollEvent {
        let mut retval = PollEvent::empty();
        if event.contains(PollEvent::Readable) && (!inner.buffer.is_empty() || inner.write_closed) {
            retval |= PollEvent::Readable;
        }
        retval
    }

    fn write_ready(inner: &PipeInner, event: PollEvent) -> PollEvent {
        let mut retval = PollEvent::empty();
        if event.contains(PollEvent::Writable)
            && (inner.buffer.len() < Self::PIPE_SIZE || inner.read_closed)
        {
            retval |= PollEvent::Writable;
        }
        retval
    }

    pub async fn poll_read(&self, event: PollEvent) -> KResult<PollEvent> {
        let mut inner = self.inner.lock().await;
        loop {
            let ready = Self::read_ready(&inner, event);
            if !ready.is_empty() {
                return Ok(ready);
            }

            inner = self.cv_read.wait(inner).await;
            if Thread::current().signal_list.has_pending_signal() {
                return Err(EINTR);
            }
        }
    }

    pub async fn poll_write(&self, event: PollEvent) -> KResult<PollEvent> {
        let mut inner = self.inner.lock().await;
        loop {
            let ready = Self::write_ready(&inner, event);
            if !ready.is_empty() {
                return Ok(ready);
            }

            inner = self.cv_write.wait(inner).await;
            if Thread::current().signal_list.has_pending_signal() {
                return Err(EINTR);
            }
        }
    }

    pub fn poll_read_ready(&self, event: PollEvent) -> KResult<PollEvent> {
        let Some(inner) = self.inner.try_lock() else {
            return Ok(PollEvent::empty());
        };

        Ok(Self::read_ready(&inner, event))
    }

    pub fn poll_write_ready(&self, event: PollEvent) -> KResult<PollEvent> {
        let Some(inner) = self.inner.try_lock() else {
            return Ok(PollEvent::empty());
        };

        Ok(Self::write_ready(&inner, event))
    }

    pub async fn read(&self, buffer: &mut dyn Buffer) -> KResult<usize> {
        let mut inner = self.inner.lock().await;

        while !inner.write_closed && inner.buffer.is_empty() {
            inner = self.cv_read.wait(inner).await;
            if Thread::current().signal_list.has_pending_signal() {
                return Err(EINTR);
            }
        }

        let (data1, data2) = inner.buffer.as_slices();
        let nread = buffer.fill(data1)?.allow_partial() + buffer.fill(data2)?.allow_partial();
        inner.buffer.drain(..nread);

        self.cv_write.notify_all();
        Ok(nread)
    }

    async fn write_atomic(&self, data: &[u8]) -> KResult<usize> {
        let mut inner = self.inner.lock().await;

        if inner.read_closed {
            send_sigpipe_to_current();
            return Err(EPIPE);
        }

        while inner.buffer.len() + data.len() > Self::PIPE_SIZE {
            inner = self.cv_write.wait(inner).await;
            if Thread::current().signal_list.has_pending_signal() {
                return Err(EINTR);
            }

            if inner.read_closed {
                send_sigpipe_to_current();
                return Err(EPIPE);
            }
        }

        inner.buffer.extend(data);

        self.cv_read.notify_all();
        return Ok(data.len());
    }

    pub async fn write(&self, stream: &mut dyn Stream) -> KResult<usize> {
        let mut buffer = [0; Self::PIPE_SIZE];
        let mut total = 0;
        while let Some(data) = stream.poll_data(&mut buffer)? {
            let nwrote = self.write_atomic(data).await?;
            total += nwrote;
            if nwrote != data.len() {
                break;
            }
        }
        Ok(total)
    }
}

impl PipeReadEnd {
    pub async fn read(&self, buffer: &mut dyn Buffer) -> KResult<usize> {
        self.pipe.read(buffer).await
    }

    pub async fn poll(&self, event: PollEvent) -> KResult<PollEvent> {
        self.pipe.poll_read(event).await
    }

    pub fn poll_ready(&self, event: PollEvent) -> KResult<PollEvent> {
        self.pipe.poll_read_ready(event)
    }

    pub async fn close(&self) {
        let mut inner = self.pipe.inner.lock().await;
        if inner.read_closed {
            return;
        }

        inner.read_closed = true;
        self.pipe.cv_write.notify_all();
    }
}

impl PipeWriteEnd {
    pub async fn write(&self, stream: &mut dyn Stream) -> KResult<usize> {
        self.pipe.write(stream).await
    }

    pub async fn poll(&self, event: PollEvent) -> KResult<PollEvent> {
        self.pipe.poll_write(event).await
    }

    pub fn poll_ready(&self, event: PollEvent) -> KResult<PollEvent> {
        self.pipe.poll_write_ready(event)
    }

    pub async fn close(&self) {
        let mut inner = self.pipe.inner.lock().await;
        if inner.write_closed {
            return;
        }

        inner.write_closed = true;
        self.pipe.cv_read.notify_all();
    }
}

impl Drop for Pipe {
    fn drop(&mut self) {
        debug_assert!(
            self.inner.get_mut().read_closed,
            "Pipe read end should be closed before dropping (check File::close())."
        );

        debug_assert!(
            self.inner.get_mut().write_closed,
            "Pipe write end should be closed before dropping (check File::close())."
        );
    }
}
